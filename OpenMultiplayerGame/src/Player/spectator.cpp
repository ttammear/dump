#include "spectator.h"

#include <SDL2/SDL.h>

Spectator::Spectator()
{
    this->camera.zNear = 0.1f;
    this->camera.zFar = 100.0f;
    this->transform.position = Vec3(0.0f, 0.0f, 0.0f);
    this->transform.rotation = Quaternion::Identity();
}

void Spectator::update(float dt, Vec2 mouseDelta)
{
    const float moveSpeed = 30.0f;
    const float rotSpeed = 0.4f;

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_W])
    {
        this->camera.transform.position += dt*moveSpeed*camera.transform.forward();
    }
    if (state[SDL_SCANCODE_S])
    {
        this->camera.transform.position -= dt*moveSpeed*camera.transform.forward();
    }
    if (state[SDL_SCANCODE_A])
    {
        this->camera.transform.position -= dt*moveSpeed*camera.transform.right();
    }
    if (state[SDL_SCANCODE_D])
    {
        this->camera.transform.position += dt*moveSpeed*camera.transform.right();
    }

    camRot += mouseDelta;

    Quaternion rotX = Quaternion::AngleAxis(rotSpeed * camRot.x, Vec3(0.0f, 1.0f, 0.0f));
    Quaternion rotY = Quaternion::AngleAxis(rotSpeed * camRot.y, Vec3(1.0f, 0.0f, 0.0f));
    this->camera.transform.rotation = rotX * rotY;
}
