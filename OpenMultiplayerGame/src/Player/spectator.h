#pragma once

#include "../camera.h"
#include "../Maths/maths.h"

class Spectator
{
public:
    Spectator();
    void update(float dt, Vec2 mouseDelta);

    Camera camera;
    Transform transform;
    Vec2 camRot;
};
