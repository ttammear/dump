#include "../Maths/maths.h"

#include <BulletDynamics/Dynamics/btActionInterface.h>

class BtCharacter : public btActionInterface
{
public:
    BtCharacter(class World *world);
    void init();
    void deinit();
    void setVel(Vec3 vel);
    void jump();
    Vec3 getPosition();
    void setPosition(Vec3 *pos);

    void setActive(bool active);

    virtual void updateAction(class btCollisionWorld* collisionWorld, btScalar deltaTimeStep);
	virtual void debugDraw(class btIDebugDraw* debugDrawer);

    bool isActive = true;
    class btKinematicCharacterController *kinematicController;
    class btConvexShape *shape;
    class btPairCachingGhostObject *ghost;
    class World *world;

    struct Entity *entity;

};
