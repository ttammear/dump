#include "application.h"

#include <SDL2/SDL.h>
#include "Renderer/renderer.h"
#include "game.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

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
    return initialized;
}

void checkSDLError(int line = -1)
{
	const char *error = SDL_GetError();
	if (*error != '\0')
	{
		printf("SDL Error: %s\n", error);
		if (line != -1)
			printf(" + line: %i\n", line);
		SDL_ClearError();
	}
}

void Application::start()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        __builtin_trap();
    }

    SDL_Window *window = SDL_CreateWindow(
        "SDL2/OpenGL Demo", 0, 0, 1024, 768, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    // Create an OpenGL context associated with the window.
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if(glcontext == NULL)
    {
        fprintf(stderr, "Failed to create OpenGL 3.3 core context\n");
        __builtin_trap();
    }

    this->window = window;
    this->mainRenderer = new Renderer(1024.0f, 768.0f);
    this->game = new Game();
    this->game->window = this->window;
    initialized = true;

    ImGui_ImplSdlGL3_Init(window);


    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK)
    {
        fprintf(stderr, "glewInit() failed!\n");
        __builtin_trap();
    }
}

void Application::doEvents()
{
    if(!initialized)
        return;

    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        ImGui_ImplSdlGL3_ProcessEvent(&e);

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

            case SDL_WINDOWEVENT:
            switch(e.window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_MINIMIZED:
                    this->mainRenderer->resize(e.window.data1, e.window.data2);
                    break;

            } break;

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

        ImGui_ImplSdlGL3_NewFrame(window);
        bool show_test_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImColor(114, 144, 154);

        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        ImGui::Render();

        mainRenderer->presentFrame(this->window);
    }
}

void Application::exit()
{
    if(initialized)
    {
        ImGui_ImplSdlGL3_Shutdown();
        SDL_DestroyWindow(window);
        delete this->game;
        this->game = NULL;
        delete this->mainRenderer;
        this->mainRenderer = NULL;
        this->window = NULL;
        initialized = false;
        SDL_Quit();
    }
}

