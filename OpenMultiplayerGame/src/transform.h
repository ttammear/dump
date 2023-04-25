#pragma once

#include "Maths/maths.h"

class Transform
{
public:
    Transform();
    Mat4 getModelMatrix();
    Vec3 forward();
    Vec3 right();

    void setPosition(Vec3 *pos);
    void setRotation(Quaternion *rot);

    // functions for managed interop
    static Vec3 tgetPosition(class Transform *transform);
    static void tsetPosition(class Transform *transform, Vec3 *pos);
    static Quaternion tgetRotation(class Transform *transform);
    static void tsetRotation(class Transform *transform, Quaternion *quaternion);


    Vec3 position;
    Quaternion rotation;
    Vec3 scale;
};
