#include "application.h"

#include "game.h"
#include <SDL2/SDL.h>
#include "Renderer/renderer.h"
#include <SDL2/SDL_opengl.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

Application::Application()
{
    initialized = false;
    running = true;
    displayRenderer = NULL;
}

Application::~Application()
{
    this->exit();
}

bool Application::isRunning()
{
    return initialized;
}

#ifdef __EMSCRIPTEN__
int pointerLockChanged(int eventType, const EmscriptenPointerlockChangeEvent *e, void *userData)
{
    if(!e->isActive)
        emscripten_request_pointerlock("#canvas", true);
    return false;
}
#endif

void Application::start()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Initializing SDL2 failed: %s\n", SDL_GetError());
        abort();
    }

    printf("Application was run from: %s\n", SDL_GetBasePath());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_RendererInfo displayRendererInfo;
    SDL_CreateWindowAndRenderer(1024, 768, SDL_WINDOW_OPENGL, &displayWindow, &displayRenderer);


    //SDL_GLContext ctx = SDL_GL_CreateContext(displayWindow); // TODO: delete?
    //SDL_GL_MakeCurrent(displayWindow, ctx);

    SDL_GetRendererInfo(displayRenderer, &displayRendererInfo);
    if((displayRendererInfo.flags & (SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE)) != (SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE))
    {
        fprintf(stderr, "SDL2 not hardware accelerated or no target surface!\n");
        abort();
    }

    int ret = SDL_SetRelativeMouseMode(SDL_TRUE);
    if(ret != 0)
    {
        fprintf(stderr, "Relative mouse mode not supported!\n");
    }
#ifdef __EMSCRIPTEN__
    emscripten_set_pointerlockchange_callback(NULL, NULL, true, pointerLockChanged);
#endif

    GLenum err = glewInit();
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    if(GLEW_OK != err) {
        printf("GLEW init failed!\n");
    }

    this->mainRenderer = new Renderer(1024.0f, 768.0f);
    this->game = new Game();
    this->game->window = this->displayWindow;
    this->startTime = SDL_GetTicks();
    initialized = true;
}

void Application::doEvents()
{
    if(!initialized)
        return;

    game->mouseMotion(0.0, 0.0f);

    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT:
                this->exit();
                break;
            case SDL_KEYDOWN:
                if(e.key.keysym.sym == SDLK_ESCAPE)
                {
                    this->exit();
                    return;
                }
                game->keyPress(e.key.keysym.sym);
                break;
            case SDL_WINDOWEVENT:
                {
                    switch(e.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                            this->mainRenderer->resize(e.window.data1, e.window.data2);
                            break;
                        case SDL_WINDOWEVENT_CLOSE:
                            this->exit();
                            break;
                        default:
                            break;
                    }
                }
                break;
            case SDL_MOUSEWHEEL:
                game->mouseScroll(e.wheel.y);
                break;
            case SDL_MOUSEMOTION:
                game->mouseMotion(e.motion.xrel, e.motion.yrel);
                break;
            case SDL_MOUSEBUTTONUP:
                if(e.button.button == SDL_BUTTON_RIGHT)
                    game->mouseClick(1);
                if(e.button.button == SDL_BUTTON_LEFT)
                    game->mouseClick(0);
                break;
            default:
                break;
        }
    }
}

void Application::doFrame()
{
    if(initialized)
    {
        // TODO: variable frame rate!
        unsigned int curTime = SDL_GetTicks();
        unsigned int dif = curTime - this->startTime;
        float dt = (float)dif/1000.0f;
        game->updateAndRender(mainRenderer, dt);
        mainRenderer->presentFrame(this->displayRenderer);
        this->startTime = curTime;
    }
}

void Application::exit()
{
    if(initialized)
    {
        SDL_SetRelativeMouseMode(SDL_FALSE);

        delete this->game;
        this->game = NULL;
        delete this->mainRenderer;
        this->mainRenderer = NULL;

        SDL_DestroyWindow(displayWindow);
        SDL_DestroyRenderer(displayRenderer);

        SDL_Quit();

        running = false;
        initialized = false;
    }
}

