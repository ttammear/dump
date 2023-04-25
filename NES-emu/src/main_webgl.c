#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <assert.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "libs/stb_image.h"
#include "ttTypes.h"
#include "nes.h"

#ifdef __WASM__
#include "../../wasmtest/WebGL.h"
#include "sys/browser.h"
#endif

#define FALSE 0
#define TRUE 1

int iddd = 0;
u8 buttons = 0;

u32 curSample = 0;

static b32 g_running;

/*void InitAudio()
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
	    printf("Couldn't open audio: %s\n", SDL_GetError());
	    exit(-1);
	}

    SDL_PauseAudio(0);
}*/

/*void CleanupAudio()
{
    SDL_CloseAudio();
}*/

unsigned int fb;
unsigned int tex;
unsigned int tex2;
unsigned int program;

unsigned int drawTex;

void emu_frame() {
    //printf("Render frame!");
    nes_frame();

#if 0

    WebGL_bindFramebuffer(WEBGL_FRAMEBUFFER, fb);
    WebGL_bindFramebuffer(WEBGL_READ_FRAMEBUFFER, fb);
    WebGL_bindFramebuffer(WEBGL_DRAW_FRAMEBUFFER, 0);
    WebGL_blitFramebuffer(0, 0, 256, 240, 0, 0, 256, 240, WEBGL_COLOR_BUFFER_BIT, WEBGL_NEAREST);
#else
    WebGL_clear(WEBGL_COLOR_BUFFER_BIT);
    WebGL_bindTexture(WEBGL_TEXTURE_2D, drawTex);
    WebGL_texSubImage2D(WEBGL_TEXTURE_2D, 0, 0, 0, 256, 240, WEBGL_RGBA, WEBGL_UNSIGNED_BYTE, displayBuffer, 240*256*4);
    // we dont use any other buffers, so don't need it
    //WebGL_bindBuffer(WEBGL_ARRAY_BUFFER, vbo);
    unsigned int texLoc = WebGL_getUniformLocation(program, "tex");
    WebGL_uniform1i(texLoc, 0);
    WebGL_activeTexture(WEBGL_TEXTURE0);
    WebGL_bindTexture(WEBGL_TEXTURE_2D, drawTex);
    WebGL_bindFramebuffer(WEBGL_FRAMEBUFFER, 0);
    WebGL_drawArrays(WEBGL_TRIANGLES, 0, 6);
    drawTex = drawTex == tex ? tex2 : tex;
#endif


}

int main(void)
{
    printf("Starting NES emulator!");

    WebGL_createContext("#nesemu");

    nes_init();
    nes_frame();

    float vertices[24] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    unsigned int vbo = WebGL_createBuffer(0);
    WebGL_bindBuffer(WEBGL_ARRAY_BUFFER, vbo);
    WebGL_bufferData(WEBGL_ARRAY_BUFFER, sizeof(float) * 24, vertices, WEBGL_STATIC_DRAW);
    WebGL_enableVertexAttribArray(0);
    WebGL_enableVertexAttribArray(1);
    WebGL_vertexAttribPointer(0, 2, WEBGL_FLOAT, 0, 16, 0);
    WebGL_vertexAttribPointer(1, 2, WEBGL_FLOAT, 0, 16, 8);

    const char *vertexShader = "#version 300 es\r\n\
                                in vec4 position;\
                                in vec2 uv_v;\
                                out vec2 UV;\
                                void main() {\
                                    gl_Position = position;\
                                    UV = uv_v;\
                                }";
    const char *fragmentShader = "#version 300 es\r\n\
                              precision highp float;\r\n\
                              in vec2 UV; \r\n\
                              uniform sampler2D tex; \r\n\
                              out vec4 outColor;\r\n\
                              void main() {\r\n\
                                  vec2 texCoord = vec2(UV.x, 1.0f-UV.y);\r\n\
                                  //outColor = vec4(1.0, 0.0, 0.0, 1.0);\r\n\
                                  outColor = texture(tex, texCoord); \r\n\
                              }";

    unsigned int vertShader = WebGL_createShader(WEBGL_VERTEX_SHADER);
    unsigned int fragShader = WebGL_createShader(WEBGL_FRAGMENT_SHADER);

    int vertLen = strlen(vertexShader);
    int fragLen = strlen(fragmentShader);
    WebGL_shaderSource(vertShader, vertexShader);
    WebGL_shaderSource(fragShader, fragmentShader);
    WebGL_compileShader(vertShader);
    WebGL_compileShader(fragShader);
    program = WebGL_createProgram(0);
    WebGL_attachShader(program, vertShader);
    WebGL_attachShader(program, fragShader);
    WebGL_linkProgram(program);
    WebGL_detachShader(program, vertShader);
    WebGL_detachShader(program, fragShader);
    WebGL_deleteShader(vertShader);
    WebGL_deleteShader(fragShader);

    WebGL_useProgram(program);

    tex = WebGL_createTexture(0);
    WebGL_bindTexture(WEBGL_TEXTURE_2D, tex);
    WebGL_texImage2D(WEBGL_TEXTURE_2D, 0, WEBGL_RGBA, 256, 240, 0, WEBGL_RGBA, WEBGL_UNSIGNED_BYTE, displayBuffer, 256*240*4);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_MIN_FILTER, WEBGL_LINEAR);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_WRAP_S, WEBGL_CLAMP_TO_EDGE);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_WRAP_T, WEBGL_CLAMP_TO_EDGE);
    tex2 = WebGL_createTexture(0);
    WebGL_bindTexture(WEBGL_TEXTURE_2D, tex2);
    WebGL_texImage2D(WEBGL_TEXTURE_2D, 0, WEBGL_RGBA, 256, 240, 0, WEBGL_RGBA, WEBGL_UNSIGNED_BYTE, displayBuffer, 256*240*4);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_MIN_FILTER, WEBGL_LINEAR);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_WRAP_S, WEBGL_CLAMP_TO_EDGE);
    WebGL_texParameteri(WEBGL_TEXTURE_2D, WEBGL_TEXTURE_WRAP_T, WEBGL_CLAMP_TO_EDGE);
    drawTex = tex;


    //WebGL_clearColor(1.0, 0.0, 0.0, 1.0);
    //WebGL_clear(WEBGL_COLOR_BUFFER_BIT);

    browser_frameCallback(emu_frame);

    //InitAudio();

    /*SDL_Window* displayWindow;
    SDL_RendererInfo displayRendererInfo;
    displayWindow = SDL_CreateWindow("NES emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, 0);
    displayRenderer = SDL_CreateRenderer(displayWindow, -1, SDL_RENDERER_SOFTWARE);*/
//    SDL_CreateWindowAndRenderer(800, 600, 0, &displayWindow, &displayRenderer);
/*    SDL_GetRendererInfo(displayRenderer, &displayRendererInfo);
    //TODO: Check that we have OpenGL */
    /*if ((displayRendererInfo.flags & SDL_RENDERER_ACCELERATED) == 0 || 
        (displayRendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) == 0) {
        //TODO: Handle this. We have no render surface and not accelerated.
        printf("WARNING: unaccelerated renderer\n");
    }*/
    
    /*i32 x,y,n;
    unsigned char *data = stbi_load("tux.png", &x, &y, &n, 3);
    assert(n == 3);*/


    //u32 startTime = SDL_GetTicks();
    //u32 timeOff = SDL_GetTicks();
    
    //texture = SDL_CreateTexture(displayRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240); 

    //SDL_DestroyTexture(texture);

    //CleanupAudio();
    //SDL_DestroyWindow(displayWindow);
   
    //SDL_Quit();
    
    return 0;
}
