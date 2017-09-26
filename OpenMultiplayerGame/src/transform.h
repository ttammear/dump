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
    // for the love of god, use this only for C# interop
    Vec3 getPosition();
    void setRotation(Quaternion *rot);
    Quaternion getRotation();

    Vec3 position;
    Quaternion rotation;
    Vec3 scale;
};
