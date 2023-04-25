#include "aike_platform.h"
#include <GLES2/gl2.h>

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{

}

void aike_init(AikePlatform *platform)
{
    platform->create_opengl_context(&platform->mainWin);
}

void aike_deinit(AikePlatform *platform)
{

}

void aike_update(AikePlatform *platform)
{
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    platform->present_frame(platform, &platform->mainWin);
}
