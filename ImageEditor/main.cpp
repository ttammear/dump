#include <cstdio>
#include <cstdlib>
#include <X11/Xlib.h>
#include <GL/glx.h>

struct linux_platform_state
{
    Display* display;
    Window window;
    int screen;
    int width;
    int height;

    Atom wmDeleteWindow;
};

void linux_show_error(const char* title, const char* message)
{
    printf("%s Message: %s\n", title, message);
    exit(-1);
}

void linux_create_window(linux_platform_state* platform, int width, int height, const char* title)
{

    Display* display = XOpenDisplay(":0.0");
    if(!display)
    {
       linux_show_error("Error", "Unable to create display device");
       return;
    }
    int screen = DefaultScreen(display);
    Window root_window = RootWindow(display, screen);
    if(!glXQueryExtension(display, 0, 0))
    {
        linux_show_error("Error", "X Server doesn't support GLX extension!");
        return;
    }
    int attrib_list[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER, True,
        GLX_SAMPLE_BUFFERS  , 0, // 1 and 4 for 4xAA ??
        GLX_SAMPLES         , 0,
        None
    };
    int nElements = 0;
    GLXFBConfig* fbConfig = glXChooseFBConfig(display, screen, attrib_list, &nElements);
    if(!fbConfig)
    {
        linux_show_error("Error", "Failed to get FBConfig!");
        return;
    }
    XVisualInfo* vinfo = glXGetVisualFromFBConfig(display, *fbConfig);
    if(!vinfo)
    {
        linux_show_error("Error", "Failed to get XVisualInfo");
        return;
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
    XMapWindow(display, window);

    platform->wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(display, window, &platform->wmDeleteWindow, 1);

    XFree(fbConfig);
    XFree(vinfo);
    
    platform->display = display;
    platform->window = window;
    platform->screen = screen;
    platform->width = width;
    platform->height = height;
}

int main()
{
    linux_platform_state pstate;
    linux_create_window(&pstate, 1024, 768, "Image editor");

    bool running = true;

    XEvent event;
    while (running) 
    {
        XNextEvent(pstate.display, &event);
        switch(event.type)
        {
            case Expose:
                XFillRectangle(pstate.display, pstate.window, DefaultGC(pstate.display, pstate.screen), 0, 0, pstate.width, pstate.height);
                break;
            case KeyPress:
                break;
            case ClientMessage:
                if(event.xclient.data.l[0] == pstate.wmDeleteWindow)
                    running = false;
                break;
        }
    }
    XCloseDisplay(pstate.display);
    return 0;
}

