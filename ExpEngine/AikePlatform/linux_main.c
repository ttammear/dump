#define _GNU_SOURCE

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <assert.h>

// pthread
#include <pthread.h>

// input
#include <libinput.h>
#include <libudev.h>
#include <fcntl.h>
#include <errno.h>
#include <fnmatch.h>
#include <linux/input.h>

// memory
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aike_platform.h"
#include "common_platform.h"

#define __gl_h_ // avoids GL/gl.h
#include <GL/glx.h>

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#ifdef AIKE_AIO

#if defined(__linux__) && defined(LINUX_USE_KERNEL_AIO)
#define LINUX_AIO_IMPLEMENTATION
#include "linux_async_io.h"
#elif defined(__linux__)
#define POSIX_AIO_IMPLEMENTATION
#include "posix_aio.h"
#else 
#error No implementation for AIO on this OS
#endif

// TODO: doesn't belong here! (NOT AIO!)
#if defined(__linux__)
#define POSIX_IO_IMPLEMENTATION
#include "posix_io.h"
#endif

#endif

//static const char* codeFile = "./libAike.so";

static char *codeFile;

#define GL_FUNC_VAR(fname) PFNGL ## fname fname;
GL_FUNC_VAR(glGenVertexArrays);
GL_FUNC_VAR(glDeleteVertexArrays);
GL_FUNC_VAR(glGenBuffers);
GL_FUNC_VAR(glDeleteBuffers);
GL_FUNC_VAR(glBindBuffer);
GL_FUNC_VAR(glBufferData);
GL_FUNC_VAR(glBufferSubData);
GL_FUNC_VAR(glEnableVertexAttribArray);
GL_FUNC_VAR(glDisableVertexAttribArray);
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
GL_FUNC_VAR(glBindAttribLocation);
GL_FUNC_VAR(glGetActiveUniform);
GL_FUNC_VAR(glIsProgram);
GL_FUNC_VAR(glIsShader);
GL_FUNC_VAR(glBindBufferBase);
GL_FUNC_VAR(glGetActiveUniformsiv);
GL_FUNC_VAR(glGetUniformBlockIndex);
GL_FUNC_VAR(glUniform4fv);
GL_FUNC_VAR(glUniformBlockBinding);
GL_FUNC_VAR(glDrawElementsInstanced);
GL_FUNC_VAR(glGenerateMipmap);
GL_FUNC_VAR(glMapBuffer);
GL_FUNC_VAR(glMapBufferRange);
GL_FUNC_VAR(glUnmapBuffer);
GL_FUNC_VAR(glClientWaitSync);
GL_FUNC_VAR(glWaitSync);
GL_FUNC_VAR(glFenceSync);
GL_FUNC_VAR(glDeleteSync);
GL_FUNC_VAR(glGenFramebuffers);
GL_FUNC_VAR(glBindFramebuffer);
GL_FUNC_VAR(glBindRenderbuffer);
GL_FUNC_VAR(glFramebufferTexture2D);
GL_FUNC_VAR(glGenRenderbuffers);
GL_FUNC_VAR(glRenderbufferStorage);
GL_FUNC_VAR(glFramebufferRenderbuffer);
GL_FUNC_VAR(glCheckFramebufferStatus);
GL_FUNC_VAR(glDeleteFramebuffers);
GL_FUNC_VAR(glDeleteRenderbuffers);
GL_FUNC_VAR(glBlitFramebuffer);
GL_FUNC_VAR(glGetBufferSubData);
GL_FUNC_VAR(glDrawBuffers);
GL_FUNC_VAR(glClearBufferiv);
GL_FUNC_VAR(glClearBufferfv);
GL_FUNC_VAR(glActiveTexture);
GL_FUNC_VAR(glBindTexture);
GL_FUNC_VAR(glGenTextures);
GL_FUNC_VAR(glTexImage2D);
GL_FUNC_VAR(glTexParameteri);
GL_FUNC_VAR(glEnable);
GL_FUNC_VAR(glDisable);
GL_FUNC_VAR(glBlendEquation);
GL_FUNC_VAR(glBlendFunc);
GL_FUNC_VAR(glScissor);
GL_FUNC_VAR(glDrawElements);
GL_FUNC_VAR(glDeleteTextures);
GL_FUNC_VAR(glGetIntegerv);
GL_FUNC_VAR(glClearColor);
GL_FUNC_VAR(glClear);
GL_FUNC_VAR(glGetTexImage);
GL_FUNC_VAR(glViewport);
GL_FUNC_VAR(glFlush);
GL_FUNC_VAR(glColorMask);
GL_FUNC_VAR(glGetError);
GL_FUNC_VAR(glGetString);


GL_FUNC_VAR(glDebugMessageCallback);


typedef struct X11WindowState
{
    Window window;
    XVisualInfo *visualInfo;
    GLXFBConfig *fbConfig;
    Atom wmDeleteWindow;

    GLXContext glContext;
} X11WindowState;

typedef struct X11State
{
    Display* display;
    int screen;

    Cursor horcursor;
    Cursor vercursor;
} X11State;

typedef struct LinuxPlatformState
{
    X11State x11;
    struct libinput *libInput;
    // CLOCK_MONOTONIC at application startup
    struct timespec monotonic_time_start;
    // CLOCK_REALTIME at application startup
    struct timespec realtime_time_start;
    bool running;

    uint32_t numPendingCharacters;
    uint32_t pendingCharacters[64];

    void (*aike_update_window) (struct AikePlatform *platform, struct AikeWindow *win);
    void (*aike_init) (struct AikePlatform *platform);
    void (*aike_deinit) (struct AikePlatform *platform);
    void (*aike_update) (struct AikePlatform *platform);
    void (*aike_begin_hot_reload) (struct AikePlatform *platform);
    void (*aike_end_hot_reload) (struct AikePlatform *platform);

    void *dlHandle;
    struct timespec libAikeLastMod;
} LinuxPlatformState;

void x11_swap_buffers(struct LinuxPlatformState *platform, struct X11WindowState *win);
void x11_set_cursor(struct AikePlatform *platform, uint32_t cursor);


typedef GLXContext (GLAPIENTRY *glXCreateContextAttribsARB_t) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static glXCreateContextAttribsARB_t glXCreateContextAttribsARB;

typedef void (GLAPIENTRY *glXSwapIntervalEXT_t)(Display *dpy, GLXDrawable drawable, int interval);
typedef void (GLAPIENTRY *glXSwapIntervalMESA_t)(int interval);
static glXSwapIntervalEXT_t glXSwapIntervalEXT;
static glXSwapIntervalMESA_t glXSwapIntervalMESA2;

uint32_t temp_keymap[AIKE_KEY_COUNT] =
{
    [AIKE_KEY_Q] = 'q',
    [AIKE_KEY_W] = 'w',
    [AIKE_KEY_E] = 'e',
    [AIKE_KEY_R] = 'r',
    [AIKE_KEY_T] = 't',
    [AIKE_KEY_Y] = 'y',
    [AIKE_KEY_U] = 'u',
    [AIKE_KEY_I] = 'i',
    [AIKE_KEY_O] = 'o',
    [AIKE_KEY_P] = 'p',
    [AIKE_KEY_A] = 'a',
    [AIKE_KEY_S] = 's',
    [AIKE_KEY_D] = 'd',
    [AIKE_KEY_F] = 'f',
    [AIKE_KEY_G] = 'g',
    [AIKE_KEY_H] = 'h',
    [AIKE_KEY_J] = 'j',
    [AIKE_KEY_K] = 'k',
    [AIKE_KEY_L] = 'l',
    [AIKE_KEY_Z] = 'z',
    [AIKE_KEY_X] = 'x',
    [AIKE_KEY_C] = 'c',
    [AIKE_KEY_V] = 'v',
    [AIKE_KEY_B] = 'b',
    [AIKE_KEY_N] = 'n',
    [AIKE_KEY_M] = 'm',
    [AIKE_KEY_1] = '1',
    [AIKE_KEY_2] = '2',
    [AIKE_KEY_3] = '3',
    [AIKE_KEY_4] = '4',
    [AIKE_KEY_5] = '5',
    [AIKE_KEY_6] = '6',
    [AIKE_KEY_7] = '7',
    [AIKE_KEY_8] = '8',
    [AIKE_KEY_9] = '9',
    [AIKE_KEY_0] = '0',
    [AIKE_KEY_SPACE] = ' ',
    [AIKE_KEY_DOT] = '.',
};

void linux_show_error(const char* title, const char* message)
{
    printf("%s Message: %s\n", title, message);
    exit(-1);
}

LinuxPlatformState linuxPState;

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

void x11_maximize_window(Display *display, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;

    XEvent xev;
    Atom wm_state  =  XInternAtom(display, "_NET_WM_STATE", True);
    Atom max_horz  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
    Atom max_vert  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", True);

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = x11win->window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
    xev.xclient.data.l[1] = max_horz;
    xev.xclient.data.l[2] = max_vert;

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);
}

bool x11_create_window(Display* display, int screen, AikeWindow *win, int width, int height, const char* title, bool nodecorations)
{
    Window root_window = RootWindow(display, screen);
    win->doubleBuffered = true; 

    int attrib_list[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER, win->doubleBuffered ? True : False,
        GLX_SAMPLE_BUFFERS, 0, // 1 and 4 for 4xAA ??
        //GLX_SAMPLES, 0,
        None
    };


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
    winAttr.background_pixmap = None;
    winAttr.background_pixel = 0;
    winAttr.border_pixel = 0;
    winAttr.colormap = XCreateColormap(display, root_window, vinfo->visual, AllocNone);

    unsigned int mask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;
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

    X11WindowState *x11win = (X11WindowState*)malloc(sizeof(X11WindowState));

    XMapWindow(display, window);

    win->nativePtr = (void*)x11win;
    x11win->wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(display, window, &x11win->wmDeleteWindow, 1);

    x11win->fbConfig = fbConfig;
    x11win->visualInfo = vinfo;

    win->screenX = 150.0f;
    win->screenY = 150.0f;
    win->width = width;
    win->height = height;

    x11win->window = window;
    return true;
}

// TODO: this could be called from different thread??
void x11_make_window_current(AikePlatform *platform, AikeWindow *win)
{
    if(win != NULL)
    {
        X11WindowState *x11win = (X11WindowState*)win->nativePtr;
        glXMakeCurrent(linuxPState.x11.display, x11win->window, x11win->glContext);
        // TODO: don't call this when not main thread
        linuxPState.aike_update_window(platform, win);
    }
    else
    {
        glXMakeCurrent(linuxPState.x11.display, None, NULL);
    }
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

    x11win->glContext = ctx;
    return true;
}

void x11_destroy_window(AikePlatform *platform, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    XDestroyWindow(linuxPState.x11.display, x11win->window);
    XFree(x11win->visualInfo);
    XFree(x11win->fbConfig);
    free(win->nativePtr);
    win->nativePtr = NULL;
}

void x11_resize_window(AikePlatform *platform, AikeWindow *win, uint32_t newWidth, uint32_t newHeight)
{
    assert(newWidth > 0 && newHeight > 0);
    // TODO: window screen coordinates never get updated!
    win->width = newWidth;
    win->height = newHeight;
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    XResizeWindow(linuxPState.x11.display, x11win->window, newWidth, newHeight);
    linuxPState.aike_update_window(platform, win);
}

void x11_move_window(AikePlatform *platform, AikeWindow *window, float x, float y)
{
    X11WindowState *x11win = (X11WindowState*)window->nativePtr;
    XMoveWindow(linuxPState.x11.display, x11win->window, (int)x, (int)y);
}

void x11_set_cursor(AikePlatform *platform, uint32_t cursor)
{
    X11WindowState *x11win = (X11WindowState*)platform->mainWin.nativePtr;
    Window win = x11win->window;
    Display *display = linuxPState.x11.display;
    switch(cursor)
    {
        case CURSOR_NONE:
            XDefineCursor(display, win, None);
            break;
        case CURSOR_HORIZONTAL_ARROWS:
            XDefineCursor(display, win, linuxPState.x11.horcursor);
            break;
        case CURSOR_VERTICAL_ARROWS:
            XDefineCursor(display, win, linuxPState.x11.vercursor);
            break;
    }
}

void x11_present_frame(AikePlatform *platform, AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    if(win->doubleBuffered)
    {
        glXSwapBuffers(linuxPState.x11.display, x11win->window);
    }
    else // single buffering
    {
        glFlush();
    }
}

void x11_do_events(AikePlatform *platform)
{
    Display *display = linuxPState.x11.display;
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
                    linuxPState.running = false;
                break;
            case MotionNotify:
                //g_input->mousePos = Vec2(event.xmotion.x, 768.0f - event.xmotion.y);
                break;
            case ConfigureNotify: 
            {
                XConfigureEvent xce = event.xconfigure;

                if ((xce.width != platform->mainWin.width || xce.height != platform->mainWin.height) && xce.window == x11win->window) {
                    platform->mainWin.screenX = xce.x;
                    platform->mainWin.screenY = xce.y;
                    platform->mainWin.width = xce.width;
                    platform->mainWin.height = xce.height;
                    
                    linuxPState.aike_update_window(platform, &platform->mainWin);
                }
            }
            break;
        }
    }
}

bool x11_create_borderless_window(AikeWindow *win, uint32_t width, uint32_t height)
{
    bool ret = x11_create_window(linuxPState.x11.display, linuxPState.x11.screen, win, width, height, "Unnamed", true);
    return ret;
}

void x11_screen_to_window_coord(AikePlatform *platform, AikeWindow *win, float x, float y, float *outx, float *outy)
{
    Display *d = linuxPState.x11.display;
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
    Display *d = linuxPState.x11.display;
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
    long displayDesktop = x11_window_get_current_desktop(DefaultRootWindow(linuxPState.x11.display), true);
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

    struct timespec startTime = linuxPState.monotonic_time_start;
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

void reload_code(AikePlatform *platform)
{
    bool reloading = false;
    if(linuxPState.dlHandle != 0)
    {
        //return;
        printf("Realoading game code!\n");
        if(linuxPState.aike_begin_hot_reload != NULL)
            linuxPState.aike_begin_hot_reload(platform);
        reloading = true;
        int closed = dlclose(linuxPState.dlHandle);
        if(closed != 0)
            printf("Unloading game code failed! %s\n",dlerror());
    }

    linuxPState.dlHandle = dlopen (codeFile, RTLD_LAZY);
    while(!linuxPState.dlHandle) {
        printf("Could not load libAike.so, retrying! %s\n",dlerror());
        usleep(1000000);
        linuxPState.dlHandle = dlopen (codeFile, RTLD_LAZY);
    }

    struct stat attrib;
    stat(codeFile, &attrib);
    linuxPState.libAikeLastMod = attrib.st_mtim;

    linuxPState.aike_init = (void(*)(AikePlatform *)) dlsym(linuxPState.dlHandle, "aike_init");
    linuxPState.aike_deinit = (void(*)(AikePlatform *)) dlsym(linuxPState.dlHandle, "aike_deinit");
    linuxPState.aike_update = (void(*)(AikePlatform *)) dlsym(linuxPState.dlHandle, "aike_update");
    linuxPState.aike_update_window = (void(*)(AikePlatform *, AikeWindow *)) dlsym(linuxPState.dlHandle, "aike_update_window");
    linuxPState.aike_begin_hot_reload = (void(*)(AikePlatform*)) dlsym(linuxPState.dlHandle, "aike_begin_hot_reload");
    linuxPState.aike_end_hot_reload = (void(*)(AikePlatform*)) dlsym(linuxPState.dlHandle, "aike_end_hot_reload");

    if(linuxPState.aike_init == NULL)
        platform_fatal("aike_init not found in libAike.so");
    if(linuxPState.aike_deinit == NULL)
        platform_fatal("aike_deinit not found in libAike.so");
    if(linuxPState.aike_update == NULL)
        platform_fatal("aike_update not found in libAike.so");
    if(linuxPState.aike_update_window == NULL)
        platform_fatal("aike_update_windwo not found in libAike.so");
    /*
    // TODO: maybe we shouldn't make those mandatory
    if(linuxPState.aike_begin_hot_reload == NULL)
        platform_fatal("aike_begin_hot_reload not found in libAike.so");
    if(linuxPState.aike_end_hot_reload == NULL)
        platform_fatal("aike_end_hot_reload not found in libAike.so"); */
    

    if(reloading && linuxPState.aike_end_hot_reload != NULL)
        linuxPState.aike_end_hot_reload(platform);
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

typedef struct PthreadState
{
    pthread_t pthread;

} PthreadState;

AikeThread* create_thread(void *userData, thread_proc_t procedure)
{
    AikeThread *ret = (AikeThread*)malloc(sizeof(AikeThread));
    ret->exited = false;
    PthreadState *tstate = (PthreadState*)malloc(sizeof(PthreadState));
    ret->nativePtr = tstate;

    if(pthread_create(&tstate->pthread, NULL, procedure, userData))
    {
        free(tstate);
        free(ret);
        fprintf(stderr, "Creating thread failed!\n");
        return NULL;
    }

    return ret;
}

bool join_thread(AikeThread *thread, void **result)
{
    PthreadState *state = (PthreadState*)thread->nativePtr;
    void *ret;
    if(pthread_join(state->pthread, result))
    {
        return false;
    }
    free(thread->nativePtr);
    // TODO: we never free the AikeThread, is this ok?
    thread->exited = true;
    return true;
}

bool detach_thread(AikeThread *thread)
{
    PthreadState *state = (PthreadState*)thread->nativePtr;
    if(pthread_detach(state->pthread))
    {
        return false;
    }
    free(thread->nativePtr);
    // TODO: we never free the AikeThread, is this ok?
    thread->exited = true;
    return true;
}

void create_opengl_context(AikePlatform *platform, AikeWindow *window)
{
    if(!x11_create_gl_context(&linuxPState, window))
    {
        linux_show_error("Error", "Creating GL context failed!");
        exit(-1);
    }
    if(!check_gl_version(3, 3))
    {
        linux_show_error("Error", "OpenGL 3.3 not supported!");
        exit(-1);
    }

    if(!gl_check_extensions())
    {
        linux_show_error("Error", "Required GL extensions not supported!");
        exit(-1);
    }
#if AIKE_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC) glMessageCallback, 0);
#endif
    glViewport(0, 0, window->width, window->height);

    // clear back buffer from gibberish (twice in case its double buffered)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    x11_present_frame(platform, window);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    x11_present_frame(platform, window);

    const unsigned char* string = glGetString(GL_VERSION);
    printf("GL_VERSION: %s\n", string);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void destroy_opengl_context(AikeWindow *win)
{
    X11WindowState *x11win = (X11WindowState*)win->nativePtr;
    glXDestroyContext(linuxPState.x11.display, x11win->glContext);
}

bool swap_interval(AikeWindow *win, int i)
{
    if(glXSwapIntervalEXT != NULL)
    {
        X11WindowState *x11Win = (X11WindowState*)win->nativePtr;
        glXSwapIntervalEXT(linuxPState.x11.display, x11Win->window, i);
        return true;
    }
    else if(glXSwapIntervalMESA2 != NULL)
    {
        glXSwapIntervalMESA2(i);
        return true;
    }
    return false;
}

void p_libinput_destroy(struct libinput* li)
{
    libinput_unref(li);
}

AikeMemoryBlock *allocate_memory(AikePlatform *platform, size_t size, uint32_t flags)
{
    long pagesize = sysconf(_SC_PAGESIZE);
    assert(pagesize > 0);
    // align header to cache line
    uint32_t headerSize = ALIGN_UP(sizeof(AikeMemoryBlock), 8*AIKE_EXPECTED_CACHE_LINE_SIZE);
    size_t totalSize = ALIGN_UP(size + headerSize, 8*pagesize);
    int prot = PROT_READ | PROT_WRITE;
    if((flags & Aike_Memory_Flag_AllowExec) == 0)
        prot |= PROT_EXEC;
    int mmapflags = MAP_PRIVATE | MAP_ANONYMOUS;
    if((flags & Aike_Memory_Flag_Commit) != 0)
        mmapflags |= MAP_POPULATE;
    void *mem = mmap(NULL, totalSize, prot, mmapflags, -1, 0);
    if(mem)
    {
        // already zeroed, dont need to zero
        AikeMemoryBlock *block = (AikeMemoryBlock*)mem;
        block->flags = flags;
        block->size = size;
        block->realSize = totalSize;
        block->memory = ((uint8_t*)mem) + headerSize;
        assert(((uintptr_t)block->memory & (AIKE_EXPECTED_CACHE_LINE_SIZE-1)) == 0);
        return block;
    }
    fprintf(stderr, "Memory allocation failed! Size: %lu Flags: %d\n", size, flags);
    return NULL;
}

void free_memory(AikePlatform *platform, AikeMemoryBlock *block)
{
    munmap((void*)block, block->realSize);
};

uint32_t  next_character(AikePlatform *platform)
{
    if(linuxPState.numPendingCharacters > 0)
        return linuxPState.pendingCharacters[--linuxPState.numPendingCharacters];
    else
        return 0;
}

void aike_exit(AikePlatform *platform)
{
    linuxPState.running = false;
}

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

int main(int argc, char *argv[])
{
    AikePlatform platform;
    platform_init(&platform);

    if(argc < 2) {
        fprintf(stderr, "argc of at least 2 expected\r\n");
        exit(-1);
    }

    // TODO: use an actual command line option instead of assumine its the second arg
    codeFile = argv[1];

    char buf[1024];  
    for(int i = 0; i < ARRAY_COUNT(buf); i++) // make valgrind happy..
        buf[i] = 0;
    readlink("/proc/self/exe", buf, sizeof(buf));  
    char *dname = dirname(buf);
    chdir(dname);
    printf("Changing running directory to %s\n", dname);

    // I don't know why I'm doing this, it would just crash by calling NULL anyways
    linuxPState.aike_init = (void(*)(AikePlatform*))aike_stub;
    linuxPState.aike_deinit = (void(*)(AikePlatform*))aike_stub;
    linuxPState.aike_update = (void(*)(AikePlatform*))aike_stub;
    linuxPState.aike_update_window = (void(*)(AikePlatform*, AikeWindow*))aike_stub;

    XInitThreads(); 
    x11_init(&linuxPState);
    x11_create_window(linuxPState.x11.display, linuxPState.x11.screen, &platform.mainWin, 1024, 768, "Title", false);
    //x11_maximize_window(Display *display, AikeWindow *win);

    clock_gettime(CLOCK_MONOTONIC, &linuxPState.monotonic_time_start);
    clock_gettime(CLOCK_REALTIME, &linuxPState.realtime_time_start);

    AikeTime lieStart = linux_get_monotonic_time(&platform);
    linuxPState.libInput = p_libinput_init(true, false);
    double liedif = aike_timedif_sec(lieStart, linux_get_monotonic_time(&platform));
    printf("Libinput took %f seconds to initialize\n", liedif);

    if(!glx_check_extensions(linuxPState.x11.display, linuxPState.x11.screen))
    {
        linux_show_error("Error", "Required GLX extensions not supported!");
        exit(-1);
    }

    glXCreateContextAttribsARB = (glXCreateContextAttribsARB_t)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
    if(glXCreateContextAttribsARB == NULL)
    {
        platform_fatal("Failed to get glXCreateContextAttribsARB pointer");
    }
    glXSwapIntervalEXT = (glXSwapIntervalEXT_t)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
    glXSwapIntervalMESA2 = (glXSwapIntervalMESA_t)glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
    if(glXSwapIntervalEXT == NULL && glXSwapIntervalMESA2 == NULL)
    {
        fprintf(stderr, "Neither glXSwapIntervalEXT nor glXSwapIntervalMESA supported!\n");
    }

    platform.exit = aike_exit;
    platform.create_opengl_context = create_opengl_context;
    platform.destroy_opengl_context = destroy_opengl_context;
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
    platform.swap_interval = swap_interval;
    platform.create_thread = create_thread;
    platform.join_thread = join_thread;
    platform.detach_thread = detach_thread;
    platform.allocate_memory = allocate_memory;
    platform.free_memory = free_memory;
    platform.open_directory = open_directory;
    platform.next_files = next_files;
    platform.close_directory = close_directory;
    platform.map_file = map_file;
    platform.unmap_file = unmap_file;
    platform.next_character = next_character;

#ifdef AIKE_AIO
    platform.init_async_io = init_async_io;
    platform.submit_io_request = submit_io_request;
    platform.get_next_io_event = get_next_io_event;
    platform.destroy_async_io = destroy_async_io;

    platform.open_file = open_file;
    platform.close_file = close_file;

    platform.tcp_listen = tcp_listen;
    platform.tcp_close_server = tcp_close_server;
    platform.tcp_accept = tcp_accept;
    platform.tcp_connect = tcp_connect;
    platform.tcp_close_connection = tcp_close_connection;
    platform.tcp_recv = tcp_recv;
    platform.tcp_send = tcp_send;
#endif

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
    GLX_GET_POINTER(glDisableVertexAttribArray);
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
    GLX_GET_POINTER(glBindAttribLocation);
    GLX_GET_POINTER(glGetActiveUniform);
    GLX_GET_POINTER(glIsProgram);
    GLX_GET_POINTER(glIsShader);
    GLX_GET_POINTER(glBindBufferBase);
    GLX_GET_POINTER(glGetActiveUniformsiv);
    GLX_GET_POINTER(glGetUniformBlockIndex);
    GLX_GET_POINTER(glUniform4fv);
    GLX_GET_POINTER(glUniformBlockBinding);
    GLX_GET_POINTER(glDrawElementsInstanced);
    GLX_GET_POINTER(glGenerateMipmap);
    GLX_GET_POINTER(glMapBuffer);
    GLX_GET_POINTER(glMapBufferRange);
    GLX_GET_POINTER(glUnmapBuffer);
    GLX_GET_POINTER(glClientWaitSync);
    GLX_GET_POINTER(glWaitSync);
    GLX_GET_POINTER(glFenceSync);
    GLX_GET_POINTER(glDeleteSync);
    GLX_GET_POINTER(glGenFramebuffers);
    GLX_GET_POINTER(glBindFramebuffer);
    GLX_GET_POINTER(glBindRenderbuffer);
    GLX_GET_POINTER(glFramebufferTexture2D);
    GLX_GET_POINTER(glGenRenderbuffers);
    GLX_GET_POINTER(glRenderbufferStorage);
    GLX_GET_POINTER(glFramebufferRenderbuffer);
    GLX_GET_POINTER(glCheckFramebufferStatus);
    GLX_GET_POINTER(glDeleteFramebuffers);
    GLX_GET_POINTER(glDeleteRenderbuffers);
    GLX_GET_POINTER(glBlitFramebuffer);
    GLX_GET_POINTER(glGetBufferSubData);
    GLX_GET_POINTER(glDrawBuffers);
    GLX_GET_POINTER(glClearBufferiv);
    GLX_GET_POINTER(glClearBufferfv);
    GLX_GET_POINTER(glActiveTexture);
    GLX_GET_POINTER(glBindTexture);
    GLX_GET_POINTER(glGenTextures);
    GLX_GET_POINTER(glTexImage2D);
    GLX_GET_POINTER(glTexParameteri);
    GLX_GET_POINTER(glEnable);
    GLX_GET_POINTER(glDisable);
    GLX_GET_POINTER(glBlendEquation);
    GLX_GET_POINTER(glBlendFunc);
    GLX_GET_POINTER(glScissor);
    GLX_GET_POINTER(glDrawElements);
    GLX_GET_POINTER(glDeleteTextures);
    GLX_GET_POINTER(glGetIntegerv);
    GLX_GET_POINTER(glClearColor);
    GLX_GET_POINTER(glClear);
    GLX_GET_POINTER(glGetTexImage);
    GLX_GET_POINTER(glViewport);
    GLX_GET_POINTER(glFlush);
    GLX_GET_POINTER(glColorMask);
    GLX_GET_POINTER(glGetError);
    GLX_GET_POINTER(glGetString);

#if defined(AIKE_DEBUG) || defined(_DEBUG)
    GLX_GET_POINTER(glDebugMessageCallback);
#endif
#undef GLX_GET_POINTER

    if(anyErrors)
        platform_fatal("Failed to get all necessary OpenGL functions!");
    
    linuxPState.running = true;
    linuxPState.dlHandle = NULL;

    XEvent event;
    int gerror;
    AikeTime start = platform.get_monotonic_time(&platform);
    platform.startTime = start;

    X11WindowState *x11win = (X11WindowState*)platform.mainWin.nativePtr;

    reload_code(&platform);
    x11_do_events(&platform);

    linuxPState.aike_init(&platform);
    linuxPState.aike_update_window(&platform, &platform.mainWin);

    while(linuxPState.running)
    {
        AikeTime curTime = platform.get_monotonic_time(&platform);
        double elapsed = aike_timedif_sec(start, curTime);
        platform.dt = elapsed;
        start = curTime;

        struct stat statRes;
        stat(codeFile, &statRes);
        struct timespec codeModified = statRes.st_mtim;
        if((codeModified.tv_sec == linuxPState.libAikeLastMod.tv_sec && codeModified.tv_nsec > linuxPState.libAikeLastMod.tv_nsec) || codeModified.tv_sec > linuxPState.libAikeLastMod.tv_sec)
        {
            reload_code(&platform);
        }

        x11_do_events(&platform);
#ifdef AIKE_AIO
        linux_process_aio();
#endif

        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        if(XQueryPointer(linuxPState.x11.display, x11win->window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
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

        libinput_dispatch(linuxPState.libInput);
        struct libinput_event* lie;
        while((lie = libinput_get_event(linuxPState.libInput)))
        {
            __auto_type type = libinput_event_get_type(lie);
            switch(type)
            {
                case LIBINPUT_EVENT_KEYBOARD_KEY:
                    {

                        struct libinput_event_keyboard *ke = libinput_event_get_keyboard_event(lie);
                        uint32_t k = libinput_event_keyboard_get_key(ke);
                        enum libinput_key_state state = libinput_event_keyboard_get_key_state(ke);
                        if(state == LIBINPUT_KEY_STATE_PRESSED && k < AIKE_KEY_COUNT)
                        {
                            platform.keyStates[k] = 1;

                            if(temp_keymap[k] != 0)
                            {
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
                                memmove(&linuxPState.pendingCharacters[1], &linuxPState.pendingCharacters[0], sizeof(linuxPState.pendingCharacters[0])*MIN(linuxPState.numPendingCharacters, ARRAY_COUNT(linuxPState.pendingCharacters)-1));
                                linuxPState.numPendingCharacters++;
                                linuxPState.numPendingCharacters = MIN(linuxPState.numPendingCharacters, ARRAY_COUNT(linuxPState.pendingCharacters));
                                // TODO: temp hack to get shift working
                                if(!platform.keyStates[AIKE_KEY_LEFTSHIFT])
                                    linuxPState.pendingCharacters[0] = temp_keymap[k];
                                else 
                                    linuxPState.pendingCharacters[0] = temp_keymap[k]-32;
                            }
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
                        enum libinput_button_state state = libinput_event_pointer_get_button_state(pe);
                        if(state == LIBINPUT_BUTTON_STATE_PRESSED && btn < AIKE_KEY_COUNT)
                            platform.keyStates[btn] = 1;
                        else if(btn < AIKE_KEY_COUNT)
                            platform.keyStates[btn] = 0;
                    } break;
                case LIBINPUT_EVENT_POINTER_AXIS:
                    {
                        struct libinput_event_pointer *ae = libinput_event_get_pointer_event(lie);
                        enum libinput_pointer_axis_source asrc = libinput_event_pointer_get_axis_source(ae);
                        if(libinput_event_pointer_has_axis(ae, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
                            platform.mouseVerAxis = libinput_event_pointer_get_axis_value_discrete(ae, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);

                        if(libinput_event_pointer_has_axis(ae, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
                            platform.mouseHorAxis = libinput_event_pointer_get_axis_value_discrete(ae, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
                    } break;
                default:
                    break;
            }
            libinput_event_destroy(lie);
        }
        
        linuxPState.aike_update(&platform);

    }

    linuxPState.aike_deinit(&platform);

    p_libinput_destroy(linuxPState.libInput);

    x11_destroy_window(&platform, &platform.mainWin);
    x11_free_resources(&linuxPState);
}
