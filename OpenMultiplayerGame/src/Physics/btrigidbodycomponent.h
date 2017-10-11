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

    // for managed interop
    static void tsetPosition(struct BtRigidBodyComponent* comp, Vec3 *pos);
    static void tsetRotation(struct BtRigidBodyComponent* comp, Quaternion *rot);
    
    class btMotionState *motionState;
    class btCollisionShape *shape;
    class btRigidBody *rigidBody;
};
