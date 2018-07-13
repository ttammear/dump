#include <GL/glew.h>
#include <stdio.h>
#include "application.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// day 2 start!
// day 3 start!
// day 4 start!
// day 5 start!
// day 6 start!
// day 7 start! Is this finally over?
// day 8 start - cheat day

static Application *app;

void loop()
{
#ifdef __EMSCRIPTEN__
    if(!app->isRunning())
        emscripten_pause_main_loop();
#endif
    app->doEvents();
    app->doFrame();
}

int main(int argc, char *argv[])
{
    app = new Application();
    app->start();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (app->isRunning())
    {
        loop();
    }
#endif
    delete app;

    return 0;
}
