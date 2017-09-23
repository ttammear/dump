#include "player.h"

#include "../GameWorld/world.h"
#include <SDL2/SDL.h>

Player::Player()
{
    this->camera.zNear = 0.1f;
    this->camera.zFar = 100.0f;
    this->transform.position = Vec3(0.0f, 0.0f, 0.0f);
    this->transform.rotation = Quaternion::Identity();
}

void Player::update(float dt, Vec2 mouseDelta)
{
    // rotation
    rotX += mouseDelta.x;
    rotY += mouseDelta.y;

    const float rotSpeed = 0.2f;
    transform.rotation = Quaternion::AngleAxis(rotSpeed * rotX, Vec3(0.0f, 1.0f, 0.0f));
    Quaternion vertRot = Quaternion::AngleAxis(rotSpeed * rotY, Vec3(1.0f, 0.0f, 0.0f));

    Vec3 forward = transform.forward();
    Vec3 right = transform.right();

    // basic collision detection
    Vec3 playerBase = transform.position + Vec3(0.0f, -1.7f, 0.0f);
    Vec3 groundedRayDest = playerBase + Vec3(0.0f, -0.1f, 0.0f);
    
    Vec3 moveBase = transform.position + Vec3(0.0f, -1.0f, 0.0f);
    Vec3 forwardRayDest = moveBase + (0.4f * forward);
    Vec3 backwardRayDest = moveBase - (0.4f * forward);
    Vec3 rightRayDest = moveBase + (0.4f * right);
    Vec3 leftRayDest = moveBase - (0.4f * right);

    RaycastHit hit;
    bool grounded = false;
    if(world->lineCast(hit, playerBase, groundedRayDest)) // grounded
    {
        if(velocity.y < 0.0f)
            velocity = Vec3(0.0f, 0.0f, 0.0f);
        grounded = true;
    }
    else // in air
    {
        velocity += world->gravity*dt;
    }

    bool canForward = !world->lineCast(hit, moveBase + (forward*0.2f), forwardRayDest);
    bool canBackward = !world->lineCast(hit, moveBase - (forward*0.2f), backwardRayDest);
    bool canRight = !world->lineCast(hit, moveBase + (right*0.2f), rightRayDest);
    bool canLeft = !world->lineCast(hit, moveBase - (right*0.2f), leftRayDest);

    Vec3 moveVector(0.0f, 0.0f, 0.0f);

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    // last day, no time for anything prettier
    if (state[SDL_SCANCODE_W])
    {
        moveVector += forward;
    }
    if (state[SDL_SCANCODE_S])
    {
        moveVector -= forward;
    }
    if (state[SDL_SCANCODE_A])
    {
        moveVector -= right;
    }
    if (state[SDL_SCANCODE_D])
    {
        moveVector += right;
    }

    if (state[SDL_SCANCODE_SPACE] && grounded)
    {
        velocity.y = 5.0f;
    }

    this->transform.position += velocity*dt;
    this->transform.position += moveVector*dt*8.0f;
    // TODO: ability to attach transform to eachother
    this->camera.transform.position = transform.position;
    this->camera.transform.rotation = transform.rotation * vertRot;
}

