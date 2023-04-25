#if 0
set -e
set -x
gcc-8 -ggdb main.c -o truevoxel -lSDL2 -l:libGLEW.so.2.0 -lGL -lm
gdb ./truevoxel
#./truevoxel
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

#include <float.h>
#include <assert.h>
#include <stdbool.h>
#define TTMATH_IMPLEMENTATION
#include "ttmath.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

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

uint8_t *framebuffer;

const char *vertShaderSrc = "#version 330 core\r\n\
layout (location = 0) in vec2 position;\
layout (location = 1) in vec2 texcoord;\
out vec2 v_texcoord; \
void main() {\
    gl_Position = vec4(position, 0.0f, 1.0f);\
    v_texcoord = texcoord;\
}";

const char *fragShaderSrc = "#version 330 core\r\n\
in vec2 v_texcoord;\
out vec4 outColor;\
uniform sampler2D _MainTex;\
void main() {\
    vec2 tc = vec2(v_texcoord.x, 1.0 - v_texcoord.y);\
    outColor = texture(_MainTex, tc);\
    //outColor = vec4(0.0, 1.0, 0.0, 1.0);\r\n\
}";

const char *fragShaderRay = "#version 330 core\r\n\
in vec2 v_texcoord;\
out vec4 outColor;\
uniform usampler3D _VoxelTex;\
uniform mat3 _TexToWorld;\
uniform vec3 _CamPos;\
void main() {\
    vec3 dir = normalize(_TexToWorld * vec3(v_texcoord, 1.0));\
    vec3 origin = _CamPos;\
\
    vec3 tLeft = step(0.0, dir)*(ceil(origin)-origin) + step(0.0, -dir)*fract(origin);\
    vec3 tMax = abs(tLeft / dir);\
    vec3 tDelta = abs(1.0 / dir);\
    ivec3 istep = ivec3(sign(dir));\
    ivec3 pos = ivec3(floor(origin));\
    int steps = 0;\
    uint val = uint(0);\
    do {\
        bvec3 mask = lessThanEqual(tMax.xyz, min(tMax.yzx, tMax.zxy));\
        tMax += vec3(mask)*tDelta;\
        pos += ivec3(mask)*istep;\
        val = texelFetch(_VoxelTex, pos, 0).r;\r\n\
        steps++;\
    } while(val == uint(0) && steps < 100);\
\
    //outColor = vec4(0.0, 1.0, 0.0, 1.0);\r\n\
    float r = float(val&uint(255))/255.0;\
    float g = float((val&uint(0xFF00))>>8)/255.0f;\
    float b = float((val>>16)&uint(255))/255.0f;\
    //float a = float((val>>uint(24))&uint(255))/255.0f;\r\n\
    float a = 0.0;\
    outColor = vec4(r, g, b, a);\
}";




struct Camera {
    V3 pos;
    Quat rot;
    Mat4 worldToCamera;
};

struct WorldState {
    struct Camera cam;
    uint32_t voxels[100][100][100];
    bool voxelsDirty;
};

void init_world(struct WorldState *ws) {
    ws->cam.pos = (V3){0.0f, 10.0f, 0.0f};
    quat_angle_axis(&ws->cam.rot, 90.0f, (V3){1.0f, 0.0f, 0.0f});
    V3 invPos = v3_scale(ws->cam.pos, -1.0f);
    Quat invRot = ws->cam.rot;
    invRot.w *= -1.0f;
    mat4_rt(&ws->cam.worldToCamera, invRot, invPos);
    // TODO: init voxels
    int x, y, n;
    uint8_t *pixels = (uint8_t*)stbi_load("cat.jpg", &x, &y, &n, 4);
    if(pixels == NULL) {
        fatal("Failed to load voxel data\r\n");
    }
    uint32_t (*voxels)[100][100] = ws->voxels;
    for(int i = 0; i < 100; i++) 
    for(int j = 0; j < 100; j++) 
    for(int k = 0; k < 100; k++) 
    {
        if(j == 0) {
            float u = (float)k/100.0f;
            float v = (float)i/100.0f;
            int px = (int)floorf(u*(float)x);
            int py = (int)floorf(v*(float)y);
            voxels[i][j][k] = *((uint32_t*)&pixels[py*x*4 + px*4]);
        } else {
            voxels[i][j][k] = 0;
        }
    }
    for(int i = 0; i < 100; i++) {
        for(int j = 0; j < 100; j++) {
            float d = sqrtf((i-20.0f)*(i-20.0f)+(j-20.0f)*(j-20.0f));
            if(d <= 20.0f) {
                voxels[i][10][j] = CLAMP((d / 20.0f), 0, 1)*255;
            }
        }
    }
    stbi_image_free(pixels);
    ws->voxelsDirty = true;
}

void set_voxel(struct WorldState *ws, uint32_t X, uint32_t Y, uint32_t Z, uint32_t value) {
    if(X >=0 && X < 100 && Y >= 0 && Y < 100 && Z >= 0 && Z < 100) {
        ws->voxels[Z][Y][X] = value;
    }
    ws->voxelsDirty = true;
}

struct RaycastInfo {
    uint32_t hitX;
    uint32_t hitY;
    uint32_t hitZ;
    uint32_t face; // 0 -x, 1 x, 2 -y, 3 y, 4 -z, 5 z
    uint32_t value;
};

uint32_t voxel_raycast_info(struct WorldState *ws, V3 o, V3 d, struct RaycastInfo* rc) {
    float xLeft = d.x >= 0 ? ceilf(o.x) - o.x : o.x - floorf(o.x);
    float yLeft = d.y >= 0 ? ceilf(o.y) - o.y : o.y - floorf(o.y);
    float zLeft = d.z >= 0 ? ceilf(o.z) - o.z : o.z - floorf(o.z);
    // TODO: d component could be 0!
    float tMaxX = fabs(xLeft / d.x);
    float tMaxY = fabs(yLeft / d.y);
    float tMaxZ = fabs(zLeft / d.z);
    float tDeltaX = fabs(1.0f / d.x);
    float tDeltaY = fabs(1.0f / d.y);
    float tDeltaZ = fabs(1.0f / d.z);
    float stepX = d.x >= 0.0f ? 1.0f : -1.0f;
    float stepY = d.y >= 0.0f ? 1.0f : -1.0f;
    float stepZ = d.z >= 0.0f ? 1.0f : -1.0f;
    // TODO: this is wrong !
    //int X = CLAMP(floorf(o.x), 0, 99), Y = CLAMP(floorf(o.y), 0, 99), Z = CLAMP(floorf(o.z), 0, 99);
    int X = floorf(o.x), Y = floorf(o.y), Z = floorf(o.z);
    // TODO: could be out of bounds!
    //uint32_t val = ws->voxels[Z][Y][X];
    int val = 0;
    int steps = 0;
    int face = -1;
    do {
        if(tMaxX < tMaxY) {
            if(tMaxX < tMaxZ) {
                X = X + stepX;
                tMaxX = tMaxX + tDeltaX;
                face = d.x > 0 ? 0 : 1;
            } else {
                Z = Z + stepZ;
                tMaxZ = tMaxZ + tDeltaZ;
                face = d.z > 0 ? 4 : 5;
            }
        } else {
            if(tMaxY < tMaxZ) {
                Y = Y + stepY;
                tMaxY = tMaxY + tDeltaY;
                face = d.y > 0 ? 2 : 3;
            } else {
                Z = Z + stepZ;
                tMaxZ = tMaxZ + tDeltaZ;
                face = d.z > 0 ? 4 : 5;
            }
        }
        if(Z >= 0 && Z < 100 && Y >= 0 && Y < 100 && X >= 0 && Y < 100) {
            val = ws->voxels[Z][Y][X];
        } else {
            val = 0;
        }
        if(steps >= 100) {
            break;
        }
        steps++;
    } while(val == 0);
    if(val != 0) {
        rc->face = face;
        rc->hitX = X;
        rc->hitY = Y;
        rc->hitZ = Z;
        return true;
    } else {
        return false;
    }
}

uint32_t voxel_raycast(struct WorldState *ws, V3 o, V3 d, uint32_t *val) {
    float xLeft = d.x >= 0 ? ceilf(o.x) - o.x : o.x - floorf(o.x);
    float yLeft = d.y >= 0 ? ceilf(o.y) - o.y : o.y - floorf(o.y);
    float zLeft = d.z >= 0 ? ceilf(o.z) - o.z : o.z - floorf(o.z);
    // TODO: d component could be 0!
    float tMaxX = fabs(xLeft / d.x);
    float tMaxY = fabs(yLeft / d.y);
    float tMaxZ = fabs(zLeft / d.z);
    float tDeltaX = fabs(1.0f / d.x);
    float tDeltaY = fabs(1.0f / d.y);
    float tDeltaZ = fabs(1.0f / d.z);
    float stepX = d.x >= 0.0f ? 1.0f : -1.0f;
    float stepY = d.y >= 0.0f ? 1.0f : -1.0f;
    float stepZ = d.z >= 0.0f ? 1.0f : -1.0f;
    // TODO: this is wrong !
    //int X = CLAMP(floorf(o.x), 0, 99), Y = CLAMP(floorf(o.y), 0, 99), Z = CLAMP(floorf(o.z), 0, 99);
    int X = floorf(o.x), Y = floorf(o.y), Z = floorf(o.z);
    // TODO: could be out of bounds!
    //uint32_t val = ws->voxels[Z][Y][X];
    *val = 0;
    int steps = 0;
    do {
        if(tMaxX < tMaxY) {
            if(tMaxX < tMaxZ) {
                X = X + stepX;
                /*if(X == -1 || X == 100)
                    return false;*/
                tMaxX = tMaxX + tDeltaX;
            } else {
                Z = Z + stepZ;
                /*if(Z == -1 || Z == 100)
                    return false;*/
                tMaxZ = tMaxZ + tDeltaZ;
            }
        } else {
            if(tMaxY < tMaxZ) {
                Y = Y + stepY;
                /*if(Y == -1 || Y == 100)
                    return false;*/
                tMaxY = tMaxY + tDeltaY;
            } else {
                Z = Z + stepZ;
                /*if(Z == -1 || Z == 100)
                    return false;*/
                tMaxZ = tMaxZ + tDeltaZ;
            }
        }
        if(Z >= 0 && Z < 100 && Y >= 0 && Y < 100 && X >= 0 && Y < 100) {
            *val = ws->voxels[Z][Y][X];
        } else {
            *val = 0;
        }
        if(steps >= 100) {
            break;
        }
        steps++;
    } while(*val == 0);
    return *val != 0;
}

// software renderer
void render(struct WorldState *ws, uint8_t *framebuffer, int w, int h) {
    V3 origin = ws->cam.pos;
    float fx = 1200.0f;
    float cx = (float)w / 2.0f;
    float cy = (float)h / 2.0f;

    Mat3 cam2Screen = { // column major order
        fx, 0.0f, 0.0f, 0.0f, -fx, 0.0f, cx, cy, 1.0f
    };  
    Mat3 screen2Cam = {            
        1.0f/fx, 0.0f, 0.0f, 0.0f, -1.0f/fx, 0.0f, -cx/fx, -cy/fx, 1.0f
    };
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            uint32_t *tt = (uint32_t*)&framebuffer[i*w*4 + j*4];
            V3 screenDir = (V3){(float)j, (float)i, 1.0f};
            V3 rayDir; 
            // camera space ray
            mat3_v3_mul(&rayDir, &screen2Cam, screenDir);
            v3_normalize(&rayDir);
            // world space ray>
            quat_v3_mul_dir(&rayDir, ws->cam.rot, rayDir);
            uint32_t color;
            bool hit = voxel_raycast(ws, origin, rayDir, &color);
            if(hit) {
                *tt = color;
            } else {
                *tt = 0;
            }
        }
    }
}

int main(void) {
    struct WorldState *world = (struct WorldState*)malloc(sizeof(struct WorldState));
    init_world(world);

    framebuffer = (uint8_t*)malloc(2048*2048*4);
    if(framebuffer == NULL) {
        fatal("Not enough memory for framebuffer\r\n");
    }

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        SDL_Quit();
        fatal("Failed to initialize SDL: %s\r\n", SDL_GetError());
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_CORE);

    int winWidth = 800;
    int winHeight = 800;

    SDL_Window *win = SDL_CreateWindow("TrueVoxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winWidth, winHeight, SDL_WINDOW_OPENGL);
    if(win == NULL) {
        fatal("Failed to create SDL2 window!\r\n");
    }
    SDL_GLContext ctx = SDL_GL_CreateContext(win);

    GLenum err = glewInit();
    if(err != GLEW_OK) {
        fatal("Failed to initialize GLEW. Error: %s\r\n", glewGetErrorString(err));
    }

    int grabbed = SDL_CaptureMouse(true);
    printf("grabbed: %d\r\n", grabbed);

    // create texture
    int texwidth = winWidth;
    int texheight = winHeight;
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texwidth, texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint voxelTex;
    glGenTextures(1, &voxelTex);
    glBindTexture(GL_TEXTURE_3D, voxelTex);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, 100, 100, 100, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    // create quad to draw mesh
    float quadVertices[] = {
        -1.0f, -1.0f, 
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
         // tex
         0.0f,  0.0f,
         1.0f,  0.0f,
         0.0f,  1.0f,
         1.0f,  0.0f,
         1.0f,  1.0f,
         0.0f,  1.0f
    };
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(float), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)(12*sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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
    src = fragShaderRay;
    srcLen = strlen(fragShaderRay);
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

    uint32_t running = 1;
    float camX = 0.0f, camY = 0.0f;
    while(running) {

        if(world->voxelsDirty) {
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, 100, 100, 100, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
            glBindTexture(GL_TEXTURE_3D, voxelTex);
            glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 100, 100, 100, GL_RED_INTEGER, GL_UNSIGNED_INT, world->voxels);
            world->voxelsDirty = false;
            printf("voxels updated\r\n");
        }

        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_KEYDOWN:
                    if(e.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    camX += e.motion.xrel;
                    camY += e.motion.yrel;
                    camX = fmodf(camX, 360.0f);
                    camY = fmodf(camY, 360.0f);
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
                case SDL_MOUSEBUTTONUP:
                    {
                        V3 forward2 = (V3){0.0f, 0.0f, 1.0f};
                        quat_v3_mul_dir(&forward2, world->cam.rot, forward2);
                        if(e.button.button == SDL_BUTTON_LEFT) {
                            struct RaycastInfo ri;
                            bool hit = voxel_raycast_info(world, world->cam.pos, forward2, &ri);
                            int setX = ri.hitX;
                            int setY = ri.hitY;
                            int setZ = ri.hitZ;
                            switch(ri.face) {
                                case 0:
                                    setX -=1;
                                    break;
                                case 1:
                                    setX += 1;
                                    break;
                                case 2:
                                    setY -= 1;
                                    break;
                                case 3:
                                    setY += 1;
                                    break;
                                case 4:
                                    setZ -= 1;
                                    break;
                                case 5:
                                    setZ += 1;
                                    break;
                            }
                            set_voxel(world, setX, setY, setZ, 0xFFFFFFFF);
                        } else if(e.button.button == SDL_BUTTON_RIGHT) {
                            struct RaycastInfo ri;
                            bool hit = voxel_raycast_info(world, world->cam.pos, forward2, &ri);
                            if(hit) {
                                set_voxel(world, ri.hitX, ri.hitY, ri.hitZ, 0);
                            }
                        }
                    }
                    break;
            }
        }
        // update texture
        glViewport(0, 0, winWidth, winHeight);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        V3 forward = (V3){0.0f, 0.0f, 1.0f};
        quat_v3_mul_dir(&forward, world->cam.rot, forward);
        V3 right = (V3){1.0f, 0.0f, 0.0f};
        quat_v3_mul_dir(&right, world->cam.rot, right);

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if(state[SDL_SCANCODE_W]) {
            v3_add(&world->cam.pos, world->cam.pos, forward);
        }
        if(state[SDL_SCANCODE_S]) {
            v3_sub(&world->cam.pos, world->cam.pos, forward);
        }
        if(state[SDL_SCANCODE_D]) {
            v3_add(&world->cam.pos, world->cam.pos, right);
        }
        if(state[SDL_SCANCODE_A]) {
            v3_sub(&world->cam.pos, world->cam.pos, right);
        }
        Quat rotX, rotY, camRot;
        quat_angle_axis(&rotX, camX, (V3){0.0f, 1.0f, 0.0f});
        quat_angle_axis(&rotY, camY, (V3){1.0f, 0.0f, 0.0f});
        quat_mul(&camRot, rotX, rotY);
        world->cam.rot = camRot;

        if(state[SDL_SCANCODE_F]) {
        }

        //render(world, framebuffer, texwidth, texheight);

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texwidth, texheight, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

        glUseProgram(program);
        /*glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        int texloc = glGetUniformLocation(program, "_MainTex");
        glUniform1i(texloc, 0);*/
        int voxelTexLoc = glGetUniformLocation(program, "_VoxelTex");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxelTex);
        glUniform1i(voxelTexLoc, 0);

        int texToWorldLoc = glGetUniformLocation(program, "_TexToWorld");
        Mat3 camToWorld, texToCam, texToWorld;
        mat3_rotation(&camToWorld, &world->cam.rot);
        float fx = 1.0f;
        texToCam = (Mat3){1.0f/fx, 0.0f, 0.0f, 0.0f, 1.0f/fx, 0.0f, -0.5f/fx, -0.5f/fx, 1.0f};
        mat3_mul(&texToWorld, &camToWorld, &texToCam);
        glUniformMatrix3fv(texToWorldLoc, 1, GL_FALSE, &texToWorld.m11);
        
        int camPosLoc = glGetUniformLocation(program, "_CamPos");
        glUniform3fv(camPosLoc, 1, &world->cam.pos.x);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SDL_GL_SwapWindow(win);
    }
    SDL_Quit();
    return 0;
}
