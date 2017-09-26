#pragma once

#include "../entity.h"

struct BtRigidBodyComponent : Component
{
    BtRigidBodyComponent();
    ~BtRigidBodyComponent();

    void init();
    void onPhysicsUpdate(double dt);  
    void setPosition(Vec3 *position);
    void setRotation(Quaternion *rotation);
    
    class btMotionState *motionState;
    class btCollisionShape *shape;
    class btRigidBody *rigidBody;
};
