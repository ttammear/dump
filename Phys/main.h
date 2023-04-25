#pragma once 

#include <float.h>
#include "stdbool.h"
#include "ttmath.h"


struct Circle2D* create_circle(V3 pos, float scale);
struct Rectangle2D* create_rectangle(V3 pos, float rot, V2 scale);
void draw_line(V2 start, V2 end, uint32_t color);

struct SolidMesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
};

struct DefaultMeshes {
    struct SolidMesh rectangle;
    struct SolidMesh circle;
};

struct Rectangle2D {
    V3 pos;
    V2 scale;
    float rot;
    uint32_t color;
};

struct Circle2D {
    V3 pos;
    float radius;
    uint32_t color;
};

struct BasicShapes {
    struct Rectangle2D rects[1024];
    uint32_t rectCount;
    struct Circle2D circles[1000];
    uint32_t circleCount;
};

struct Camera2D {
    V3 pos;
    float rot;
    Mat4 worldToCam;
    Mat4 camToClip;
    Mat4 worldToClip;
};

