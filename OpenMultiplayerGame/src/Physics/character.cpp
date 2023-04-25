#include "character.h"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include "../GameWorld/world.h"
#include "../Physics/physics.h"

BtCharacter::BtCharacter(World *world)
{
    this->world = world;
}

void BtCharacter::init()
{
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(btVector3(0.0f, 30.0f, 0.0f));

    shape = new btCapsuleShape(0.5f, 1.8f);

    ghost = new btPairCachingGhostObject();
    ghost->setCollisionShape(shape);
    ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    ghost->setActivationState(DISABLE_DEACTIVATION);
    ghost->setWorldTransform(trans);


    auto btWorld = world->physics->world;

    btWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback( new btGhostPairCallback() );
    btWorld->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);

    kinematicController = new btKinematicCharacterController(ghost, shape, 0.35f);
    btVector3 vel(0.0f, 0.0f, 0.0f);
    //kinematicController->setVelocityForTimeInterval(vel, 10.0f);
    kinematicController->setWalkDirection(vel);
    kinematicController->setGravity(btVector3(0.0f, -9.8f, 0.0f));
    kinematicController->setJumpSpeed(5.5f);

    btWorld->addAction(this);

    entity = world->createEntity();
}

void BtCharacter::deinit()
{
    delete kinematicController;
    auto btWorld = world->physics->world;
    btWorld->removeCollisionObject(ghost);
    delete ghost;
    delete shape;
}

void BtCharacter::updateAction(btCollisionWorld* collisionWorld, btScalar dt)
{
    auto btWorld = world->physics->world;
    kinematicController->updateAction(btWorld, dt);

    btTransform trans;
    trans = ghost->getWorldTransform();
    btVector3 bpos = trans.getOrigin();
    Vec3 pos(bpos.m_floats[0], bpos.m_floats[1], bpos.m_floats[2]);
    entity->transform.setPosition(&pos);
}

void BtCharacter::debugDraw(class btIDebugDraw *debugDrawer)
{

}

void BtCharacter::setVel(Vec3 vel)
{
    btVector3 bvel(vel.x, vel.y, vel.z);
    kinematicController->setWalkDirection(bvel);
}

Vec3 BtCharacter::getPosition()
{
    btVector3 wtrans = ghost->getWorldTransform().getOrigin();
    Vec3 ret(wtrans.m_floats[0], wtrans.m_floats[1], wtrans.m_floats[2]);
    return ret;
}

void BtCharacter::setPosition(Vec3 *pos)
{
    btTransform trans = ghost->getWorldTransform();
    trans.setOrigin(btVector3(pos->x, pos->y, pos->z));
    ghost->setWorldTransform(trans);
}

void BtCharacter::jump()
{
    kinematicController->jump();
}

void BtCharacter::setActive(bool active)
{
    if(active == isActive)
        return;
    auto btWorld = world->physics->world;
    if(active)
    {
        btWorld->addAction(this);
    }
    else
    {
        btWorld->removeAction(this);
    }
    entity->active = active;
    isActive = active;
}

