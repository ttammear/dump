#include "application.h"

#include <SDL2/SDL.h>
#include "Renderer/renderer.h"
#include "game.h"

Application::Application()
{
    initialized = false;
    window = NULL;
    startTime = std::chrono::steady_clock::now();
}

Application::~Application()
{
    this->exit();
}

bool Application::isRunning()
{
    /*if(initialized)
        return window->isOpen();
    return false;*/
    return true; // TODO
}

void Application::start()
{
    SDL_Window *window = SDL_CreateWindow(
        "SDL2/OpenGL Demo", 0, 0, 1024, 768, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Create an OpenGL context associated with the window.
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    this->window = window;
    this->mainRenderer = new Renderer(1024.0f, 768.0f);
    this->game = new Game();
    this->game->window = this->window;
    initialized = true;

    glewInit();
}

void Application::doEvents()
{
    if(!initialized)
        return;

    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT:
                this->exit();
	            return;

            case SDL_KEYDOWN:
            {
                if(e.key.keysym.sym == SDLK_ESCAPE && SDL_GetMouseFocus() != NULL)
                {
                    this->exit();
                    break;
                }
                game->keyPress(e.key.keysym.sym);
            } break;

            case SDL_WINDOWEVENT_RESIZED:
                this->mainRenderer->resize(e.window.data1, e.window.data2);
                break;

            case SDL_MOUSEBUTTONDOWN:
                switch (e.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        game->mouseClick(0);
                        break;
                    case SDL_BUTTON_RIGHT:
                        game->mouseClick(1);
                        break;
                    default:
                        break;
                }
                break;

            case SDL_MOUSEWHEEL:
                game->mouseScroll(e.wheel.y);
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
        std::chrono::steady_clock::time_point curTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> frameTime = curTime - startTime;
        double dt = frameTime.count();
        startTime = curTime;
        //dt = 1.0f / 60.0f;
        //printf("dt %f\n", dt);
        game->updateAndRender(mainRenderer, dt);
        mainRenderer->presentFrame(this->window);
    }
}

void Application::exit()
{
    if(initialized)
    {
        SDL_DestroyWindow(window);
        delete this->game;
        this->game = NULL;
        delete this->mainRenderer;
        this->mainRenderer = NULL;
        delete this->window;
        this->window = NULL;
        initialized = false;
    }
}

