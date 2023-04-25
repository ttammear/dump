#if 0
set -e
set -x
g++ -std=c++11 main.cpp -ggdb3 -o phys -lSDL2 -l:libGLEW.so.2.0 -lGL
./phys
exit
#endif

#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include <float.h>

#define TTMATH_IMPLEMENTATION
#include "ttmath.h"
#undef TTMATH_IMPLEMENTATION

#include "main.h"
#include "phys.h"

#include "phys.cpp"

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    exit(-1);
}

void gl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void *userParam) {
    printf("GL ERROR: %s\r\n", message);
}

const char *vertShaderSrc = R"foo(#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec4 color;
out vec2 v_texcoord;
out vec4 v_color;
uniform mat4 ModelToClip;
void main() {
    gl_Position = ModelToClip * vec4(position, 1.0f);
    v_texcoord = texcoord;
    v_color = color;
})foo";

const char *fragShaderSrc = R"foo(#version 330 core
in vec2 v_texcoord;
in vec4 v_color;
out vec4 outColor;
uniform sampler2D _MainTex;
void main() {
    vec2 tc = vec2(v_texcoord.x, 1.0 - v_texcoord.y);
    outColor = v_color;
    //outColor = vec4(0.0, 1.0, 0.0, 1.0);
})foo";


static struct BasicShapes *shapes;

#define MAX_LINES 500

uint32_t lineCount;
static V2 lines[MAX_LINES*2]; // each line is 2 vectors
static uint32_t lineColors[MAX_LINES*2];
uint32_t lineVao, lineVbo;

struct Rectangle2D* create_rectangle(V3 pos, float rot, V2 scale) {
    struct Rectangle2D *ret = &shapes->rects[shapes->rectCount++];
    ret->pos = pos;
    ret->rot = rot;
    ret->scale = scale;
    ret->color = 0xFFFFFFFF;
    return ret;
}

struct Circle2D* create_circle(V3 pos, float scale) {
    struct Circle2D *ret = &shapes->circles[shapes->circleCount++];
    ret->pos = pos;
    ret->radius = scale;
    ret->color = 0xFFFFFFFF;
    return ret;
}

void draw_line(V2 start, V2 end, uint32_t color) {
    lines[lineCount*2 + 0] = start;
    lines[lineCount*2 + 1] = end;
    lineColors[lineCount*2 + 0] = color;
    lineColors[lineCount*2 + 1] = color;
    lineCount++;
}

void create_solid_mesh(struct SolidMesh *mesh, float *vertices, uint32_t numVertices, uint16_t *indices, uint32_t numIndices) {
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numVertices*3, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)*numIndices, indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindVertexArray(0);
}

void create_shapes(DefaultMeshes *dm) {
    float rectVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
    };
    uint16_t rectIndices[] = {
        0, 1, 2, 1, 3, 2
    };
    create_solid_mesh(&dm->rectangle, rectVertices, 4, rectIndices, 6);


    float circleVertices[3*73];
    uint16_t circleIndices[71*3];
    // center
    circleVertices[0] = 0.0f;
    circleVertices[1] = 0.0f;
    circleVertices[2] = 0.0f;
    const float r = 0.5f;
    for(int i = 1; i < 73; i++) {
        float angle = ((float)(i-1) / 36.0f) * M_PI;
        circleVertices[i*3 + 0] = cosf(angle)*r;
        circleVertices[i*3 + 1] = sinf(angle)*r;
        circleVertices[i*3 + 2] = 0.0f;
        if(i > 1) {
            circleIndices[(i-2)*3 + 0] = i-1;
            circleIndices[(i-2)*3 + 1] = i;
            circleIndices[(i-2)*3 + 2] = 0;
        };
    }
    circleIndices[213] = 72;
    circleIndices[214] = 1;
    circleIndices[215] = 0;
    create_solid_mesh(&dm->circle, circleVertices, 73, circleIndices, 216);
}

int main(void) {

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        SDL_Quit();
        fatal("Failed to initialize SDL: %s\r\n", SDL_GetError());
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_CORE);

    int winWidth = 1024;
    int winHeight = 768;

    SDL_Window *win = SDL_CreateWindow("TrueVoxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winWidth, winHeight, SDL_WINDOW_OPENGL);
    if(win == NULL) {
        fatal("Failed to create SDL2 window!\r\n");
    }
    SDL_GLContext ctx = SDL_GL_CreateContext(win);

    GLenum err = glewInit();
    if(err != GLEW_OK) {
        fatal("Failed to initialize GLEW. Error: %s\r\n", glewGetErrorString(err));
    }

    struct DefaultMeshes defaultMeshes;
    create_shapes(&defaultMeshes);
    shapes = (struct BasicShapes*)malloc(sizeof(struct BasicShapes));
    shapes->rectCount = 0;
    shapes->circleCount = 0;

    struct Camera2D camera;
    camera.pos = (V3){0.0f, 0.0f, 0.0f};
    //mat4_perspective(&camera.camToClip, 45.0f, (float)winWidth/winHeight, 0.1f, 1000.0f);
    mat4_ortho(&camera.camToClip, 0.0f, winWidth, winHeight, 0.0f, -1.0f, 1.0f);
    mat4_rt(&camera.worldToCam, (Quat){1.0f, 0.0f, 0.0f, 0.0f}, v3_scale(camera.pos, -1.0f));
    mat4_mul(&camera.worldToClip, &camera.camToClip, &camera.worldToCam);

    glDebugMessageCallback(gl_debug_proc, NULL);
    glEnable(GL_DEBUG_OUTPUT);

    int vertShader = glCreateShader(GL_VERTEX_SHADER);
    int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = vertShaderSrc;
    int srcLen = strlen(vertShaderSrc);
    glShaderSource(vertShader, 1, &src, &srcLen); 
    int status = GL_FALSE;
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        int infoLen;
        GLchar infoLog[1024];
        glGetShaderInfoLog(vertShader, 1024, &infoLen, infoLog); 
        fatal("Failed to compile vertex shader!\r\n%s", infoLog);
    }
    src = fragShaderSrc;
    srcLen = strlen(fragShaderSrc);
    glShaderSource(fragShader, 1, &src, &srcLen);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        fatal("Failed to compile fragment shader!\r\n");
    }
    int program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status != GL_TRUE) {
        fatal("Failed to link GL program\r\n");
    }

    glGenVertexArrays(1, &lineVao);
    glGenBuffers(1, &lineVbo);
    glBindVertexArray(lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*MAX_LINES+sizeof(uint32_t)*MAX_LINES*2, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)(uintptr_t)(sizeof(float)*4*MAX_LINES));
    glBindVertexArray(0);

    init();

    uint32_t running = 1;
    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_MOUSEBUTTONDOWN:
                    switch(e.button.button) {
                        case SDL_BUTTON_LEFT:
                            button_down(0, (V2){(float)e.button.x, (float)e.button.y});
                        break;
                        case SDL_BUTTON_RIGHT:
                            button_down(1, (V2){(float)e.button.x, (float)e.button.y});
                        break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    switch(e.button.button){ 
                        case SDL_BUTTON_LEFT:
                            button_up(0, (V2){(float)e.button.x, (float)e.button.y});
                            break;
                        case SDL_BUTTON_RIGHT:
                            button_up(1, (V2){(float)e.button.x, (float)e.button.y});
                            break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    mouse_move((V2){(float)e.motion.x, (float)e.motion.y}, (V2){(float)e.motion.xrel, (float)e.motion.yrel});
                    break;
                case SDL_KEYDOWN:
                    if(e.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_WINDOWEVENT:
                    if(e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        winWidth = e.window.data1;
                        winHeight = e.window.data2;
                        glViewport(0, 0, winWidth, winHeight);
                    }
                    break;
            }
        }
        // update texture
        glViewport(0, 0, winWidth, winHeight);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        update();

        glBindVertexArray(defaultMeshes.rectangle.vao);
        glUseProgram(program);
        GLint matLoc = glGetUniformLocation(program, "ModelToClip");
        if(matLoc == -1) {
            fatal("ModelToClip uniform doesn't exist in shader!\r\n");
        }
        for(int i = 0; i < shapes->rectCount; i++) {
            Mat4 modelToWorld, modelToClip;
            mat4_trs(&modelToWorld, shapes->rects[i].pos, (Quat){1.0f, 0.0f, 0.0f, 0.0f}, (V3){shapes->rects[i].scale.x, shapes->rects[i].scale.y, 1.0f});
            mat4_mul(&modelToClip, &camera.worldToClip, &modelToWorld);
            glUniformMatrix4fv(matLoc, 1, GL_FALSE, &modelToClip.m11);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
        }
        glBindVertexArray(defaultMeshes.circle.vao);
        glDisable(GL_CULL_FACE);
        for(int i = 0 ; i < shapes->circleCount; i++) {
            Circle2D *c = shapes->circles + i;
            float b = (c->color&0xFF)/255.0f, g = ((c->color>>8)&0xFF)/255.0f, r = ((c->color>>16)&0xFF)/255.0f, a = 1.0f;
            glVertexAttrib4f(2, r, g, b, a);
            Mat4 modelToWorld, modelToClip;
            float radius = c->radius;
            mat4_trs(&modelToWorld, c->pos, (Quat){1.0f, 0.0f, 0.0f, 0.0f}, (V3){radius*2.0f, radius*2.0f, 1.0f});
            mat4_mul(&modelToClip, &camera.worldToClip, &modelToWorld);
            glUniformMatrix4fv(matLoc, 1, GL_FALSE, &modelToClip.m11);
            glDrawElements(GL_TRIANGLES, 72*3, GL_UNSIGNED_SHORT, NULL);
        }
        glUniformMatrix4fv(matLoc, 1, GL_FALSE, &camera.worldToClip.m11);
        if(lineCount > 0) {
            void *vertData = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float)*4*MAX_LINES+sizeof(uint32_t)*MAX_LINES*2, GL_MAP_WRITE_BIT);
            memcpy(vertData, lines, sizeof(float)*4*lineCount);
            memcpy((uint8_t*)vertData + sizeof(float)*4*MAX_LINES, lineColors, sizeof(uint32_t)*lineCount*2);
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glBindVertexArray(lineVao);
            glDrawArrays(GL_LINES, 0, lineCount*2);
            glBindBuffer(GL_ARRAY_BUFFER, lineVbo);

            lineCount = 0;
        }

        /*glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 6);*/

        SDL_GL_SwapWindow(win);
    }
    SDL_Quit();
    return 0;
}
