#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// SDL stuff
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <assert.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "libs/stb_image.h"
#include "ttTypes.h"
#include "nes.h"

#define FALSE 0
#define TRUE 1


static SDL_Renderer* displayRenderer;
static SDL_Texture* texture;
static SDL_Surface *surface;

u32 buffer[400][400];

int iddd = 0;
u8 buttons = 0;

static SDL_AudioSpec audio_spec;
u32 curSample = 0;

static b32 g_running;

void InitAudio()
{
    printf("Audio init\n");
    audio_spec.freq = SAMPLES_PER_SECOND;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 2048;
    audio_spec.callback = AudioCallback;
    audio_spec.userdata = NULL;

    //audio_pos = wav_buffer;
    //audio_len = wav_length;

    if ( SDL_OpenAudio(&audio_spec, NULL) < 0 ){
	    fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
	    exit(-1);
	}

    SDL_PauseAudio(0);
}

void CleanupAudio()
{
    SDL_CloseAudio();
}

void loop()
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT:

#ifdef __EMSCRIPTEN__
                emscripten_pause_main_loop();
#else
                g_running = FALSE;
#endif
                break;
            case SDL_KEYDOWN:
                {
                    switch (e.key.keysym.sym)
                    {
                        case SDLK_a:  buttons|=2; break;
                        case SDLK_d: buttons|=1; break;
                        case SDLK_w:    buttons|=8; break;
                        case SDLK_s:  buttons|=4; break;
                        case SDLK_LEFT:  buttons|=64; break;
                        case SDLK_RIGHT: buttons|=128; break;
                        case SDLK_RETURN: buttons|=16; break;
                        case SDLK_SPACE:  buttons|=32; break;
                    }
                } break;
            case SDL_KEYUP:
                {
                    switch (e.key.keysym.sym)
                    {
                        case SDLK_a:  buttons&=~2; break;
                        case SDLK_d: buttons&=~1; break;
                        case SDLK_w:    buttons&=~8; break;
                        case SDLK_s:  buttons&=~4; break;
                        case SDLK_LEFT:  buttons&=~64; break;
                        case SDLK_RIGHT: buttons&=~128; break;
                        case SDLK_RETURN: buttons&=~16; break;
                        case SDLK_SPACE:  buttons&=~32; break;
                    }

                } break;
        }

    }
    //SDL_RenderClear(displayRenderer);
    nes_frame();
    
    SDL_UpdateTexture(texture, NULL, displayBuffer, 256 * 4);
    SDL_RenderCopy(displayRenderer, texture, NULL, NULL);

    //Display_Render();
    SDL_RenderPresent(displayRenderer);
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        fprintf(stderr, "Initializing SDL2 failed: %s\n", SDL_GetError());
        return -1;
    }

    InitAudio();

    SDL_Window* displayWindow;
    SDL_RendererInfo displayRendererInfo;
    SDL_CreateWindowAndRenderer(800, 600, SDL_WINDOW_OPENGL, &displayWindow, &displayRenderer);
    SDL_GetRendererInfo(displayRenderer, &displayRendererInfo);
    /*TODO: Check that we have OpenGL */
    if ((displayRendererInfo.flags & SDL_RENDERER_ACCELERATED) == 0 || 
        (displayRendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) == 0) {
        /*TODO: Handle this. We have no render surface and not accelerated. */
    }
    
    /*i32 x,y,n;
    unsigned char *data = stbi_load("tux.png", &x, &y, &n, 3);
    assert(n == 3);*/
    nes_init();
    nes_frame();


    u32 startTime = SDL_GetTicks();
    u32 timeOff = SDL_GetTicks();
    
    texture = SDL_CreateTexture(displayRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240); 
    surface = SDL_CreateRGBSurfaceFrom(NULL, 256, 240, 32, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

#ifdef __EMSCRIPTEN__
    // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
    emscripten_set_main_loop(loop, 60, 1);
#else
    while(g_running)
    {
        u32 curTime = SDL_GetTicks();
        u32 dif = curTime - startTime;
        i32 usecs = (16-dif)*1000;
        if(usecs < 0)
        {
            timeOff = -(usecs/1000);
            usecs = 0;
        }

        usleep(usecs); // TODO: framerate

        curTime = SDL_GetTicks();
        //printf("frametime %dms\n", curTime-startTime);
        startTime = curTime;
    }
#endif

    CleanupAudio();
    SDL_DestroyWindow(displayWindow);
   
    SDL_Quit();
    
    return 0;
}
