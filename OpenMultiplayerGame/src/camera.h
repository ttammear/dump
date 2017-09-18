#pragma once

#include "Maths/maths.h"
#include "transform.h"

class Camera
{
public:

    enum Flags
    {
        Disabled = 1 << 0
    };

    Camera();
    Camera(float targetWidth, float targetHeight);
    Mat4 getViewProjectionMatrix();

    float FOV = 90.0f;
    float zNear = 0.1f;
    float zFar = 1000.0f; 

    // temporary until there's proper rendertarget!
    float targetWidth;
    float targetHeight;

    unsigned int flags = 0;

    Transform transform;
};
