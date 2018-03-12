#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <assert.h>

// input
#include <libinput.h>
#include <libudev.h>
#include <fcntl.h>
#include <errno.h>
#include <fnmatch.h>
#include <linux/input.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aike_platform.h"

#define GL_FUNC_VAR(fname) PFNGL ## fname fname;
GL_FUNC_VAR(glGenVertexArrays);
GL_FUNC_VAR(glDeleteVertexArrays);
GL_FUNC_VAR(glGenBuffers);
GL_FUNC_VAR(glDeleteBuffers);
GL_FUNC_VAR(glBindBuffer);
GL_FUNC_VAR(glBufferData);
GL_FUNC_VAR(glBufferSubData);
GL_FUNC_VAR(glEnableVertexAttribArray);
GL_FUNC_VAR(glCreateShader);
GL_FUNC_VAR(glDeleteShader);
GL_FUNC_VAR(glShaderSource);
GL_FUNC_VAR(glCompileShader);
GL_FUNC_VAR(glGetShaderiv);
GL_FUNC_VAR(glGetShaderInfoLog);
GL_FUNC_VAR(glAttachShader);
GL_FUNC_VAR(glGetProgramiv);
GL_FUNC_VAR(glGetProgramInfoLog);
GL_FUNC_VAR(glDetachShader);
GL_FUNC_VAR(glCreateProgram);
GL_FUNC_VAR(glDeleteProgram);
GL_FUNC_VAR(glLinkProgram);
GL_FUNC_VAR(glVertexAttribPointer);
GL_FUNC_VAR(glVertexAttribIPointer);
GL_FUNC_VAR(glUseProgram);
GL_FUNC_VAR(glBindVertexArray);
GL_FUNC_VAR(glUniform1i);
GL_FUNC_VAR(glGetUniformLocation);
GL_FUNC_VAR(glUniformMatrix3fv);
GL_FUNC_VAR(glUniformMatrix4fv);
GL_FUNC_VAR(glGetStringi);
GL_FUNC_VAR(glDebugMessageCallback);


struct X11WindowState
{
    Window window;
    XVisualInfo *visualInfo;
    GLXFBConfig *fbConfig;
    Atom wmDeleteWindow;
};

struct X11State
{
    Display* display;
    int screen;
    GLXContext glxContext;

    Cursor horcursor;
    Cursor vercursor;
};

struct LinuxPlatformState
{
    X11State x11;
    struct libinput *libInput;
    // CLOCK_MONOTONIC at application startup
    struct timespec monotonic_time_start;
    // CLOCK_REALTIME at application startup
    struct timespec realtime_time_start;
    bool running;

    void (*aike_update_window) (struct AikePlatform *platform, struct AikeWindow *win);
    void (*aike_init) (struct AikePlatform *platform);
    void (*aike_deinit) (struct AikePlatform *platform);
    void (*aike_update) (struct AikePlatform *platform);

    void *dlHandle;
    struct timespec libAikeLastMod;
};

void x11_swap_buffers(struct LinuxPlatformState *platform, struct X11WindowState *win);
void x11_set_cursor(struct AikePlatform *platform, uint32_t cursor);


typedef GLXContext (GLAPIENTRY *glXCreateContextAttribsARB_t) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static glXCreateContextAttribsARB_t glXCreateContextAttribsARB;

void linux_show_error(const char* title, const char* message)
{
    printf("%s Message: %s\n", title, message);
    exit(-1);
}

LinuxPlatformState linux;

void platform_fatal(const char *errorStr)
{
    fprintf(stderr, "%s\n", errorStr);
    abort();
}

void aike_stub()
{
    platform_fatal("Application libary is not loaded!");
}

void x11_init(LinuxPlatformState* platform)
{
    Display *display = XOpenDisplay(NULL);
    if(!display)
    {
        platform_fatal("Failed to open display");
        return;
    }
    if(!glXQueryExtension(display, 0, 0))
    {
        platform_fatal("X Server doesn't support GLX extension!");
        return;
    }
    // TODO: multi screen?
    platform->x11.screen = DefaultScreen(display);
    platform->x11.display = display;

    platform->x11.horcursor = XCreateFontCursor(platform->x11.display, XC_sb_h_double_arrow);
    platform->x11.vercursor = XCreateFontCursor(platform->x11.display, XC_sb_v_double_arrow);
}

void x11_free_resources(LinuxPlatformState* lp)
{
    XFreeCursor(lp->x11.display, lp->x11.horcursor);
    XFreeCursor(lp->x11.display, lp->x11.vercursor);
    XCloseDisplay(lp->x11.display);
}

bool x11_create_window(Display* display, int screen, AikeWindow *win, int width, int height, const char* title, bool nodecorations = false)
{
    Window root_window = RootWindow(display, screen);
    int attrib_list[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER, False,
        GLX_SAMPLE_BUFFERS, 0, // 1 and 4 for 4xAA ??
        //GLX_SAMPLES, 0,
        None
    };

    win->doubleBuffered = false; 

    int nElements = 0;
    GLXFBConfig* fbConfig = glXChooseFBConfig(display, screen, attrib_list, &nElements);
    if(!fbConfig)
    {
        linux_show_error("Error", "Failed to get FBConfig!");
        return false;
    }
    XVisualInfo* vinfo = glXGetVisualFromFBConfig(display, *fbConfig);
    if(!vinfo)
    {
        linux_show_error("Error", "Failed to get XVisualInfo");
        return false;
    }

    XSetWindowAttributes winAttr;
    winAttr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask;
    /*if(nodecorations)
        winAttr.override_redirect = true;*/
    winAttr.background_pixmap = None;
    winAttr.background_pixel = 0;
    winAttr.border_pixel = 0;
    winAttr.colormap = XCreateColormap(display, root_window, vinfo->visual, AllocNone);

    unsigned int mask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;
    /*if(nodecorations)
        mask |= CWOverrideRedirect;*/
    const int window_xpos = 150;
    const int window_ypos = 150;
    Window window = XCreateWindow(display, root_window, window_xpos, window_ypos, width, height, 
            0, vinfo->depth, InputOutput, vinfo->visual, mask, &winAttr);
    XStoreName(display, window, title);

    if(nodecorations)
    {
        Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
        long value = XInternAtom(display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
        XChangeProperty(display, window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
    }

    XMapWindow(display, window);

    X11WindowState *x11win = (X11WindowState*)malloc(sizeof(X11WindowState));
    win->nativePtr = (void*)x11win;
    x11win->wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(display, window, &x11win->wmDeleteWindow, 1);

    x11win->fbConfig = fbConfig;
    x11win->visualInfo = vinfo;

    win->screenRect = Rect(150.0f, 150.0f, width, height);

    x11win->window = window;
    return true;
}

void x11_make_window_current(AikePlatform *platform, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    glXMakeCurrent(linux.x11.display, x11win->window, linux.x11.glxContext);
    linux.aike_update_window(platform, win);
}

bool x11_create_gl_context(LinuxPlatformState *pstate, AikeWindow *win)
{
    Display* d = pstate->x11.display;
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    Window w = x11win->window;

    int attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    GLXContext ctx = glXCreateContextAttribsARB(
                        d,
                        *x11win->fbConfig,
                        NULL,
                        GL_TRUE,
                        attribs);

    //GLXContext ctx = glXCreateContext(d, pstate->x11.visualInfo, NULL, GL_TRUE);
    if(!glXMakeCurrent(d, w, ctx))
    {
        linux_show_error("Error", "glXMakeCurrent failed!");
        return false;
    }
    if(!glXIsDirect(d, glXGetCurrentContext()))
        printf("Indirect GL context obtained!\n");
    glViewport(0, 0, win->screenRect.width, win->screenRect.height);
    glColorMask(1, 1, 1, 1);
    glClearColor(0.7f, 0.7f, 0.7f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pstate->x11.glxContext = ctx;
    return true;
}

void x11_destroy_gl_context(LinuxPlatformState *pstate)
{
    glXDestroyContext(pstate->x11.display, pstate->x11.glxContext);
}

void x11_destroy_window(AikePlatform *platform, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    XDestroyWindow(linux.x11.display, x11win->window);
    XFree(x11win->visualInfo);
    XFree(x11win->fbConfig);
    free(win->nativePtr);
    win->nativePtr = NULL;
}

void x11_resize_window(AikePlatform *platform, AikeWindow *win, uint32_t newWidth, uint32_t newHeight)
{
    assert(newWidth > 0 && newHeight > 0);
    win->screenRect.width = newWidth;
    win->screenRect.height = newHeight;
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    XResizeWindow(linux.x11.display, x11win->window, newWidth, newHeight);
    linux.aike_update_window(platform, win);
}

void x11_move_window(AikePlatform *platform, AikeWindow *window, float x, float y)
{
    X11WindowState *x11win = (X11WindowState*)window->nativePtr;
    XMoveWindow(linux.x11.display, x11win->window, (int)x, (int)y);
}

void x11_set_cursor(AikePlatform *platform, uint32_t cursor)
{
    X11WindowState *x11win = (X11WindowState*)platform->mainWin.nativePtr;
    Window win = x11win->window;
    Display *display = linux.x11.display;
    switch(cursor)
    {
        case CURSOR_NONE:
            XDefineCursor(display, win, None);
            break;
        case CURSOR_HORIZONTAL_ARROWS:
            XDefineCursor(display, win, linux.x11.horcursor);
            break;
        case CURSOR_VERTICAL_ARROWS:
            XDefineCursor(display, win, linux.x11.vercursor);
            break;
    }
}

void x11_present_frame(AikePlatform *platform, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    if(win->doubleBuffered)
    {
        glXSwapBuffers(linux.x11.display, x11win->window);
    }
    else // single buffering
    {
        glFlush();
    }
}

void x11_do_events(AikePlatform *platform)
{
    Display *display = linux.x11.display;
    X11WindowState *x11win = (X11WindowState*)platform->mainWin.nativePtr;
    XEvent event;
    GLint gerror;
    while(XPending(display) > 0) // event loop
    {
        XNextEvent(display, &event);
        switch(event.type)
        {
            case Expose:
                glClear(GL_COLOR_BUFFER_BIT);
                gerror = glGetError();
                if(gerror != 0)
                {
                    printf("GL error occured! %04x\n", gerror);
                }
                break;
            case KeyPress:
                //x11_make_window_current(&pstate, &pstate.x11.ctxMenuWin);
                //window = pstate.x11.ctxMenuWin.window; // for glXSwapBuffers
                //x11_resize_window(pstate, &platform.mainWin.x11Window, 1200, 768);
                break;
            case ClientMessage:
                if(event.xclient.data.l[0] == x11win->wmDeleteWindow)
                    linux.running = false;
                break;
            case MotionNotify:
                //g_input->mousePos = Vec2(event.xmotion.x, 768.0f - event.xmotion.y);
                break;
            case ConfigureNotify: 
            {
                XConfigureEvent xce = event.xconfigure;

                if ((xce.width != platform->mainWin.screenRect.width || xce.height != platform->mainWin.screenRect.height) && xce.window == x11win->window) {

                    platform->mainWin.screenRect = Rect(xce.x, xce.y, xce.width, xce.height);
                    
                    linux.aike_update_window(platform, &platform->mainWin);
                }
            }
            break;
        }
    }
}

bool x11_create_borderless_window(AikeWindow *win, uint32_t width, uint32_t height)
{
    bool ret = x11_create_window(linux.x11.display, linux.x11.screen, win, width, height, "Unnamed", true);
    return ret;
}

void x11_screen_to_window_coord(AikePlatform *platform, AikeWindow *win, float x, float y, float *outx, float *outy)
{
    Display *d = linux.x11.display;
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    Window root = DefaultRootWindow(d);
    int destx, desty;
    Window child;

    if(XTranslateCoordinates(d, root, x11win->window, x, y, &destx, &desty, &child))
    {
        *outx = destx;
        *outy = desty;
        return;
    }
    fprintf(stderr, "Translating window coordinates failed! (Source window on other screen!)");
    *outx = 0.0f;
    *outy = 0.0f;
}


long x11_window_get_current_desktop(Window window, bool disp) 
{
	Atom actual_type_return;
	int actual_format_return = 0;
	unsigned long nitems_return = 0;
	unsigned long bytes_after_return = 0;
	long * desktop = 0;
	long ret;

    const char *atomname = disp ? "_NET_CURRENT_DESKTOP" : "_NET_WM_DESKTOP";

    // XLib is fucking insane, what the fuck
    Display *d = linux.x11.display;
	if(XGetWindowProperty(d, window, 
                XInternAtom(d, atomname, false), 0, 1, 
                false, XA_CARDINAL, &actual_type_return, &actual_format_return,
                &nitems_return, &bytes_after_return, 
                (unsigned char**)&desktop) != Success) 
    {
        return 0;
	}
	if(actual_type_return != XA_CARDINAL || nitems_return == 0) 
    {
		return 0;
	}

	ret = desktop[0];
	XFree(desktop);

	return ret;
}

bool x11_mouse_coord_valid(AikePlatform *platform, AikeWindow *win)
{
    long displayDesktop = x11_window_get_current_desktop(DefaultRootWindow(linux.x11.display), true);
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    long windowDesktop = x11_window_get_current_desktop(x11win->window, false);
    // TODO: same screen?
    return displayDesktop == windowDesktop;
}

AikeTime linux_get_monotonic_time(AikePlatform *platform)
{
    AikeTime ret;
    
    struct timespec curTime;
    clock_gettime(CLOCK_MONOTONIC, &curTime);

    struct timespec startTime = linux.monotonic_time_start;
    ret.sec = curTime.tv_sec - startTime.tv_sec;
    int64_t nsecSinceStart = curTime.tv_nsec - startTime.tv_nsec;
    if(nsecSinceStart < 0)
    {
        nsecSinceStart += 1000000000;
        ret.sec--;
    }
    ret.nsec = nsecSinceStart;
    assert(ret.sec >= 0 && nsecSinceStart >=0);
    return ret;
}

// TODO: this should be common among platforms?
static bool is_extension_supported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;
    for (start=extList;;) 
    {
        where = strstr(start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if ( where == start || *(where - 1) == ' ' )
            if ( *terminator == ' ' || *terminator == '\0' )
            return true;
        start = terminator;
    }
    return false;
}


static bool glx_check_extensions(Display* display, int screen)
{
    const char *extensionList = glXQueryExtensionsString(display, screen);
    printf("GLX Extensions: %s\n", extensionList);

#define CHECK_GLX_EXTENSION(x) if(! is_extension_supported(extensionList, x)) { \
        fprintf(stderr, "GLX Extension not supported: %s\n", x); \
        return false; }

    CHECK_GLX_EXTENSION("GLX_ARB_create_context");
    return true;
#undef CHECK_EXTENSION
}

static bool gl_check_extensions()
{
    GLint n;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);

    char buf[16384];
    buf[0] = 0;

    for(int i = 0; i < n; i++)
    {
        const char *ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if(ext == NULL)
            return false;
        if(strlen(ext) + strlen(buf) >= sizeof(buf))
        {
            fprintf(stderr, "Extension string buffer overflow");
            exit(-1);
        }
        strcat(buf, ext);
        strcat(buf, " ");
    }
    printf("GL Extensions: %s\n", buf);

    // TODO: this should be in application code, not here?
    #define CHECK_GL_EXTENSION(x) if(! is_extension_supported(buf, x)) { \
        fprintf(stderr, "GL Extension not supported: %s\n", x); \
        return false; }

#if AIKE_DEBUG
    CHECK_GL_EXTENSION("GL_ARB_debug_output");
#endif
    return true;
}

// TODO: this should be common among platforms?
static bool check_gl_version(int min_major_ver, int min_minor_ver)
{
	int major_ver, minor_ver;
	glGetIntegerv(GL_MAJOR_VERSION, &major_ver);
	glGetIntegerv(GL_MINOR_VERSION, &minor_ver);
	if (major_ver == GL_INVALID_ENUM || minor_ver == GL_INVALID_ENUM)
	{
		return false;
	}
	if (major_ver < min_major_ver)
	{
		return false;
	}
	if (major_ver == min_major_ver && minor_ver < min_minor_ver)
	{
		return false;
	}
    return true;
}

// TODO: this should be in game code??
void glMessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void* userParam )
{
    const char *typeStr;
    switch(type)
    {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeStr = "deprecated";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeStr = "undefined_beh";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeStr = "portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeStr = "performance";
            break;
        case GL_DEBUG_TYPE_OTHER:
            typeStr = "other";
            break;
        default:
            typeStr = "undefined";
            break;
    }
    const char *sevStr;
    switch(severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            sevStr = "high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            sevStr = "medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            sevStr = "low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            sevStr = "notification";
            break;
        default:
            sevStr = "undefined";
            break;
    }
    fprintf(stderr, "OpenGL:%s type = %s(0x%x), severity = %s, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "\x1B[31m** GL ERROR **\x1B[0m" : "" ), typeStr, type, sevStr, message);
}


void linux_sleep(uint32_t us)
{
    usleep(us);
}

void load_code()
{
    const char* codeFile = "./libAike.so";

    if(linux.dlHandle != 0)
    {
        //return;
        printf("Realoading game code!\n");
        int closed = dlclose(linux.dlHandle);
        if(closed != 0)
            printf("Unloading game code failed! %s\n",dlerror());
    }

    linux.dlHandle = dlopen (codeFile, RTLD_LAZY);
    while(!linux.dlHandle) {
        printf("Could not load libAike.so, retrying! %s\n",dlerror());
        usleep(1000000);
        linux.dlHandle = dlopen (codeFile, RTLD_LAZY);
    }

    struct stat attrib;
    stat(codeFile, &attrib);
    linux.libAikeLastMod = attrib.st_mtim;

    linux.aike_init = (void(*)(AikePlatform *)) dlsym(linux.dlHandle, "aike_init");
    linux.aike_deinit = (void(*)(AikePlatform *)) dlsym(linux.dlHandle, "aike_deinit");
    linux.aike_update = (void(*)(AikePlatform *)) dlsym(linux.dlHandle, "aike_update");
    linux.aike_update_window = (void(*)(AikePlatform *, AikeWindow *)) dlsym(linux.dlHandle, "aike_update_window");

    if(linux.aike_init == NULL)
        platform_fatal("aike_init not found in libAike.so");
    if(linux.aike_deinit == NULL)
        platform_fatal("aike_deinit not found in libAike.so");
    if(linux.aike_update == NULL)
        platform_fatal("aike_update not found in libAike.so");
    if(linux.aike_update_window == NULL)
        platform_fatal("aike_update_windwo not found in libAike.so");
    //dlclose(codeFile);

}

/////////////// HERE LIVES THE MIGHTY LIBINPUT /////////////////
// welcome to 2018 where doing basic input is rocket surgery

#define ANSI_RED        "\033[31m"
#define ANSI_HIGHLIGHT "\x1B[0;1;39m"
#define ANSI_NORMAL "\x1B[0m"

LIBINPUT_ATTRIBUTE_PRINTF(3, 0)
static void
log_handler(struct libinput *li,
	    enum libinput_log_priority priority,
	    const char *format,
	    va_list args)
{
	static int is_tty = -1;

	if (is_tty == -1)
		is_tty = isatty(STDOUT_FILENO);

	if (is_tty) {
		if (priority >= LIBINPUT_LOG_PRIORITY_ERROR)
			printf(ANSI_RED);
		else if (priority >= LIBINPUT_LOG_PRIORITY_INFO)
			printf(ANSI_HIGHLIGHT);
	}

	vprintf(format, args);

	if (is_tty && priority >= LIBINPUT_LOG_PRIORITY_INFO)
		printf(ANSI_NORMAL);
}

static int
open_restricted(const char *path, int flags, void *user_data)
{
	bool *grab = (bool*)user_data;
	int fd = open(path, flags);

	if (fd < 0)
		fprintf(stderr, "Failed to open %s (%s)\n",
			path, strerror(errno));
	else if (*grab && ioctl(fd, EVIOCGRAB, (void*)1) == -1)
		fprintf(stderr, "Grab requested, but failed for %s (%s)\n",
			path, strerror(errno));

	return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *user_data)
{
	close(fd);
}

static const struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};

static struct libinput *
p_libinput_init(bool verbose, bool grab)
{
    const char *seat = "seat0";
	struct libinput *li;
	struct udev *udev = udev_new();

	if (!udev) {
		fprintf(stderr, "Failed to initialize udev\n");
		return NULL;
	}

	li = libinput_udev_create_context(&interface, &grab, udev);
	if (!li) {
		fprintf(stderr, "Failed to initialize context from udev\n");
		goto out;
	}

	if (verbose) {
        // TODO: do these need to be unbound?
		libinput_log_set_handler(li, log_handler);
		libinput_log_set_priority(li, LIBINPUT_LOG_PRIORITY_DEBUG);
	}

	if (libinput_udev_assign_seat(li, seat)) {
		fprintf(stderr, "Failed to set seat\n");
		libinput_unref(li);
		li = NULL;
		goto out;
	}

out:
	udev_unref(udev);
	return li;
}

void p_libinput_destroy(struct libinput* li)
{
    libinput_unref(li);
}

// TODO: shared platform code
void platform_init(AikePlatform *platform)
{
   memset((void*)platform, 0, sizeof(AikePlatform)); 
}

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

int main(int argc, char *argv[])
{
    AikePlatform platform;
    platform_init(&platform);

    char buf[1024];  
    readlink("/proc/self/exe", buf, sizeof(buf));  
    char *dname = dirname(buf);
    chdir(dname);
    printf("Changing running directory to %s\n", dname);

    // I don't know why I'm doing this, it would just crash by calling NULL anyways
    linux.aike_init = (void(*)(AikePlatform*))aike_stub;
    linux.aike_deinit = (void(*)(AikePlatform*))aike_stub;
    linux.aike_update = (void(*)(AikePlatform*))aike_stub;
    linux.aike_update_window = (void(*)(AikePlatform*, AikeWindow*))aike_stub;

    x11_init(&linux);
    x11_create_window(linux.x11.display, linux.x11.screen, &platform.mainWin, 1024, 768, "Image editor");
    clock_gettime(CLOCK_MONOTONIC, &linux.monotonic_time_start);
    clock_gettime(CLOCK_REALTIME, &linux.realtime_time_start);

    AikeTime lieStart = linux_get_monotonic_time(&platform);
    linux.libInput = p_libinput_init(true, false);
    double liedif = aike_timedif_sec(lieStart, linux_get_monotonic_time(&platform));
    printf("Libinput took %f seconds to initialize\n", liedif);

    if(!glx_check_extensions(linux.x11.display, linux.x11.screen))
    {
        linux_show_error("Error", "Required GLX extensions not supported!");
        exit(-1);
    }

    glXCreateContextAttribsARB = (glXCreateContextAttribsARB_t)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
    if(glXCreateContextAttribsARB == NULL)
    {
        platform_fatal("Failed to get glXCreateContextAttribsARB pointer");
    }

    if(!x11_create_gl_context(&linux, &platform.mainWin))
    {
        linux_show_error("Error", "Creating GL context failed!");
        exit(-1);
    }
    if(!check_gl_version(3, 3))
    {
        linux_show_error("Error", "OpenGL 3.3 not supported!");
        exit(-1);
    }

    platform.create_borderless_window = x11_create_borderless_window;
    platform.destroy_window = x11_destroy_window;
    platform.resize_window = x11_resize_window;
    platform.move_window = x11_move_window;
    platform.screen_to_window_coord = x11_screen_to_window_coord;
    platform.window_to_screen_coord = NULL; // TODO
    platform.mouse_coord_valid = x11_mouse_coord_valid;
    platform.set_cursor = x11_set_cursor;
    platform.present_frame = x11_present_frame;
    platform.make_window_current = x11_make_window_current;
    platform.get_monotonic_time = linux_get_monotonic_time;
    platform.sleep = linux_sleep;

    bool anyErrors = false;
#define GLX_GET_POINTER(glFunc) anyErrors = (glFunc = (PFNGL ## glFunc)glXGetProcAddress((const GLubyte*)#glFunc)) == NULL || anyErrors;
    GLX_GET_POINTER(glGenVertexArrays);
    GLX_GET_POINTER(glDeleteVertexArrays);
    GLX_GET_POINTER(glCreateShader);
	GLX_GET_POINTER(glCreateShader);
	GLX_GET_POINTER(glGenBuffers);
	GLX_GET_POINTER(glDeleteBuffers);
	GLX_GET_POINTER(glBindBuffer);
	GLX_GET_POINTER(glBufferData);
    GLX_GET_POINTER(glBufferSubData);
	GLX_GET_POINTER(glEnableVertexAttribArray);
	GLX_GET_POINTER(glCreateShader);
    GLX_GET_POINTER(glDeleteShader);
	GLX_GET_POINTER(glShaderSource);
	GLX_GET_POINTER(glCompileShader);
	GLX_GET_POINTER(glGetShaderiv );
	GLX_GET_POINTER(glGetShaderInfoLog);
	GLX_GET_POINTER(glAttachShader);
	GLX_GET_POINTER(glGetProgramiv);
	GLX_GET_POINTER(glGetProgramInfoLog);
	GLX_GET_POINTER(glDetachShader);
	GLX_GET_POINTER(glCreateProgram);
    GLX_GET_POINTER(glDeleteProgram);
	GLX_GET_POINTER(glLinkProgram);
	GLX_GET_POINTER(glVertexAttribPointer);
	GLX_GET_POINTER(glUseProgram);
	GLX_GET_POINTER(glBindVertexArray);
    GLX_GET_POINTER(glUniform1i);
    GLX_GET_POINTER(glGetUniformLocation);
    GLX_GET_POINTER(glUniformMatrix3fv);
    GLX_GET_POINTER(glUniformMatrix4fv);
    GLX_GET_POINTER(glVertexAttribIPointer);
    GLX_GET_POINTER(glGetStringi);
#if defined(AIKE_DEBUG) || defined(_DEBUG)
    GLX_GET_POINTER(glDebugMessageCallback);
#endif
#undef GLX_GET_POINTER

    if(anyErrors)
        platform_fatal("Failed to get all necessary OpenGL functions!");

    if(!gl_check_extensions())
    {
        linux_show_error("Error", "Required GL extensions not supported!");
        exit(-1);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC) glMessageCallback, 0);
    
    const unsigned char* string = glGetString(GL_VERSION);
    printf("%s\n", string);
    linux.running = true;
    linux.dlHandle = NULL;

    XEvent event;
    int gerror;
    AikeTime start = platform.get_monotonic_time(&platform);
    platform.startTime = start;

    X11WindowState *x11win = (X11WindowState*)platform.mainWin.nativePtr;

    load_code();

    linux.aike_init(&platform);
    linux.aike_update_window(&platform, &platform.mainWin);

    while(linux.running)
    {
        AikeTime curTime = platform.get_monotonic_time(&platform);
        double elapsed = aike_timedif_sec(start, curTime);
        platform.dt = elapsed;

        x11_do_events(&platform);

        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        if(XQueryPointer(linux.x11.display, x11win->window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
        {
            platform.mouseX = win_x;
            platform.mouseY = win_y;
            platform.mouseScreenX = root_x;
            platform.mouseScreenY = root_y;
            uint32_t inputM = 0;
            if((mask & Button1Mask) != 0)
                inputM |= AIKE_MOUSEB1_BIT;
            if((mask & Button3Mask) != 0)
                inputM |= AIKE_MOUSEB2_BIT;
            platform.mouseButtons = inputM;
        }

        platform.mouseVerAxis = 0.0;
        platform.mouseHorAxis = 0.0;

        libinput_dispatch(linux.libInput);
        struct libinput_event* lie;
        while((lie = libinput_get_event(linux.libInput)))
        {
            auto type = libinput_event_get_type(lie);
            switch(type)
            {
                case LIBINPUT_EVENT_KEYBOARD_KEY:
                    {

                        struct libinput_event_keyboard *ke = libinput_event_get_keyboard_event(lie);
                        uint32_t k = libinput_event_keyboard_get_key(ke);
                        libinput_key_state state = libinput_event_keyboard_get_key_state(ke);
                        if(state == LIBINPUT_KEY_STATE_PRESSED && k < AIKE_KEY_COUNT)
                        {
                            platform.keyStates[k] = 1;
                        }
                        else if(k < AIKE_KEY_COUNT)
                        {
                            platform.keyStates[k] = 0;
                        }
                    }
                    break;
                case LIBINPUT_EVENT_POINTER_BUTTON:
                    {
                        // TODO: maybe we also need pointer position here??
                        struct libinput_event_pointer *pe = libinput_event_get_pointer_event(lie);
                        uint32_t btn = libinput_event_pointer_get_button(pe);
                        libinput_button_state state = libinput_event_pointer_get_button_state(pe);
                        if(state == LIBINPUT_BUTTON_STATE_PRESSED && btn < AIKE_KEY_COUNT)
                        {
                            platform.keyStates[btn] = 1;
                        }
                        else if(btn < AIKE_KEY_COUNT)
                        {
                            platform.keyStates[btn] = 0;
                        }
                    } break;
                case LIBINPUT_EVENT_POINTER_AXIS:
                    {
                        struct libinput_event_pointer *ae = libinput_event_get_pointer_event(lie);
                        libinput_pointer_axis_source asrc = libinput_event_pointer_get_axis_source(ae);
                        if(libinput_event_pointer_has_axis(ae, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
                            platform.mouseVerAxis = libinput_event_pointer_get_axis_value_discrete(ae, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);

                        if(libinput_event_pointer_has_axis(ae, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
                            platform.mouseHorAxis = libinput_event_pointer_get_axis_value_discrete(ae, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
                    } break;
                default:
                    break;
            }
        }
        
        linux.aike_update(&platform);

    }

    linux.aike_deinit(&platform);

    p_libinput_destroy(linux.libInput);

    x11_destroy_gl_context(&linux);
    x11_destroy_window(&platform, &platform.mainWin);
    x11_free_resources(&linux);
}
