#include "player.h"

#include "../GameWorld/world.h"
#include "../Physics/character.h"
#include <SDL2/SDL.h>

Player::Player()
{
    this->camera.zNear = 0.1f;
    this->camera.zFar = 100.0f;
    this->transform.position = Vec3(0.0f, 0.0f, 0.0f);
    this->transform.rotation = Quaternion::Identity();
}

void Player::init(World *world)
{
    this->world = world;
    character = new BtCharacter(world);
    character->init();
}

void Player::deinit()
{
    character->deinit();
    delete character;
}

void Player::update(float dt, Vec2 mouseDelta, PlayerInput &input)
{
    if(!this->isActive)
        return;

    Vec3 cv = (input.rotation * Vec3(0.0f, 0.0f, 1.0f)).normalized();
    Vec3 forward = Vec3(cv.x, 0.0f, cv.z).normalized();
    Vec3 right = Vec3::Cross(forward, Vec3(0.0f, 1.0f, 0.0f)).normalized();

    Vec3 moveVector(0.0f, 0.0f, 0.0f);

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (input.moveForward)
        moveVector += forward;
    if (input.moveBackward)
        moveVector -= forward;
    if (input.moveRight)
        moveVector -= right;
    if (input.moveLeft)
        moveVector += right;

    character->setVel(moveVector.normalized() * 0.1f);

    if (input.jump)
    {
        character->jump();
    }
    this->camera.transform.position = character->getPosition() + Vec3(0.0f, 1.6f, 0.0f);
    this->camera.transform.rotation = input.rotation;
}

void Player::setPosition(Vec3 *pos)
{
    character->setPosition(pos);
}

void Player::setActive(bool active)
{
    isActive = active;
    character->setActive(active);
}

