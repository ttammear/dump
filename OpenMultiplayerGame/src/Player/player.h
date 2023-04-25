#pragma once 

#include "../camera.h"
#include "inventory.h"
#include "../Maths/maths.h"

struct PlayerInput
{
    bool moveForward;
    bool moveBackward;
    bool moveRight;
    bool moveLeft;
    bool jump;
    Quaternion rotation;
};

class Player
{
public:
    Player();
    void init(class World *world);
    void deinit();
    void update(PlayerInput& input);
    void setActive(bool active);
    void setPosition(Vec3 *pos);

    Camera camera;
    Inventory inventory;
    Transform transform;
    Vec3 velocity;

    float rotX; // horizontal
    float rotY;

    class World *world;
    struct BtCharacter *character;
    bool isActive = true;
};
