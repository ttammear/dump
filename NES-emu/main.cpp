#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// OpenGL stuff
#include <GL/glew.h>
#include <GL/glu.h>

// SDL stuff
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

#include <assert.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "libs/stb_image.h"
#include "ttTypes.h"
#include "nes.h"

#define FALSE 0
#define TRUE 1


static SDL_Renderer* displayRenderer;

u32 buffer[400][400];

int iddd = 0;
u8 buttons = 0;

b32 InitGL()
{
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return FALSE;
    }
    if(!glewIsSupported("GL_VERSION_3_0"))
    {
        fprintf(stderr, "Error: OpenGL 3.0 required\n");
        return FALSE;
    }
    return TRUE;
}

b32 CreateProgram(GLuint *program, const char *vertexShader, const char *fragmentShader)
{
    b32 ret = TRUE;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* vsP = vertexShader;
    const char* fsP = fragmentShader;
    const i32 vLen = strlen(vsP);
    const i32 fLen = strlen(fsP);
    glShaderSource(vs, 1, &vsP, &vLen);
    glShaderSource(fs, 1, &fsP, &fLen);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        char infoLog[1024];
        GLsizei length;
        glGetShaderInfoLog(vs, sizeof(infoLog), &length, infoLog);
        fprintf(stderr, "%s\n", infoLog);
        ret = FALSE;
    }
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        char infoLog[1024];
        GLsizei length;
        glGetShaderInfoLog(fs, sizeof(infoLog), &length, infoLog);
        fprintf(stderr, "%s\n", infoLog);
        ret = FALSE;
    }

    GLuint p = glCreateProgram();
    *program = p;
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &status);
    // TODO: exit on error
    if(status == GL_FALSE)
        ret = FALSE;
    glDetachShader(p, vs);
    glDetachShader(p, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ret;
}

b32 CreateVAO(GLuint *vao, void *vertBuf, u32 vertexCount, void *indexBuf, u32 indexCount, GLuint texCoordLoc)
{
    assert(glGetError() == 0);
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    GLuint vb, eb;
    glGenBuffers(1, &vb);
    glGenBuffers(1, &eb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(r32)*4, vertBuf, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(r32)*4, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(r32)*4, (GLvoid*)(2*sizeof(r32)));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(u16), indexBuf, GL_STATIC_DRAW);
    assert(glGetError() == 0);

    glBindVertexArray(0);
    return glGetError() == 0;
}

void BitmapToPixelBuffer(GLuint *pixBuf, void *src, u32 width, u32 height, u32 pixelSize)
{
    glGenBuffers(1, pixBuf);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, *pixBuf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width*height*pixelSize, src, GL_DYNAMIC_DRAW);
    assert(glGetError() == 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void UpdatePixelBuffer(GLuint pixBuf, GLuint tex, u32 width, u32 height, void *src)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixBuf);
    glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, width*height*sizeof(u32), src);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

static SDL_AudioSpec audio_spec;
u32 curSample = 0;


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

int
main(int argc, char *argv[])
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
    
    if(!InitGL())
    {
        fprintf(stderr, "Initializing OpenGL failed, exiting...\n");
        return -1;
    }

    glViewport(0, 0, 800, 600);
    const char *vertShader = 
        "#version 130\n \
        in vec4 position;\n\
        in vec2 texCoord; \n\
        out vec2 texCoordVout; \n\
        void main(void) { \n\
            gl_Position = position; \n\
            texCoordVout = texCoord; \n\
    }\n";
    const char *fragShader = 
        "#version 130\n \
        uniform sampler2D tex; \n\
        in vec2 texCoordVout; \n\
        void main(void) { \n\
            gl_FragColor = texture(tex, texCoordVout);\n \
    }\n";

    GLuint program;
    if(!CreateProgram(&program, vertShader, fragShader))
    {
        fprintf(stderr, "Failed to compile shaders, exiting...\n");
        return -1;
    }
    glUseProgram(program);
    GLuint texLoc = glGetUniformLocation(program, "tex");
    GLuint texCoordLoc = glGetAttribLocation(program, "texCoord");
    assert(texLoc != -1);
    assert(glGetError() == 0);
    
    /*i32 x,y,n;
    unsigned char *data = stbi_load("tux.png", &x, &y, &n, 3);
    assert(n == 3);*/
    nes_init();
    nes_frame();

    GLuint pb, tex;
    BitmapToPixelBuffer(&pb, displayBuffer, 256, 240, 4);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);


    r32 verts[] = { -1.0f, -1.0f, 0.0, 1.0, 
                    1.0f, -1.0f, 1.0, 1.0, 
                    1.0f, 1.0f, 1.0, 0.0, 
                    -1.0f, 1.0f, 0.0, 0.0};
    u16 indices[] = {0, 1, 2, 2, 3, 0};
    GLuint vao;
    if(!CreateVAO(&vao, verts, 4, indices, 6, texCoordLoc))
    {
        fprintf(stderr, "Failed to create VAO\n");
        return -1;
    }

    u32 startTime = SDL_GetTicks();
    u32 timeOff = SDL_GetTicks();

    b32 running = TRUE;
    while(running)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    running = FALSE;
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
        SDL_RenderClear(displayRenderer);
        glClearColor(0.0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nes_frame();
        UpdatePixelBuffer(pb, tex, 256, 240, displayBuffer); 

        glUseProgram(program);
        glUniform1i(texLoc, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0); 
        glBindVertexArray(0);

        assert(glGetError() == 0); 
        //Display_Render();
        SDL_RenderPresent(displayRenderer);

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

    CleanupAudio();
    SDL_DestroyWindow(displayWindow);
   
    SDL_Quit();
    
    return 0;
}
