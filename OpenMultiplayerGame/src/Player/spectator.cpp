#include "spectator.h"

#include <SFML/Window.hpp>

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
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        this->camera.transform.position += dt*moveSpeed*camera.transform.forward();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        this->camera.transform.position -= dt*moveSpeed*camera.transform.forward();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        this->camera.transform.position -= dt*moveSpeed*camera.transform.right();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        this->camera.transform.position += dt*moveSpeed*camera.transform.right();
    }

    camRot += mouseDelta;

    Quaternion rotX = Quaternion::AngleAxis(rotSpeed * camRot.x, Vec3(0.0f, 1.0f, 0.0f));
    Quaternion rotY = Quaternion::AngleAxis(rotSpeed * camRot.y, Vec3(1.0f, 0.0f, 0.0f));
    this->camera.transform.rotation = rotX * rotY;
}
