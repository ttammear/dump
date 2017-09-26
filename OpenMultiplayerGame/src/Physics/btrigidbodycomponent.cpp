#include "btrigidbodycomponent.h"

#include <btBulletDynamicsCommon.h>
#include "../GameWorld/world.h"
#include "physics.h"

BtRigidBodyComponent::BtRigidBodyComponent()
{
    
}

BtRigidBodyComponent::~BtRigidBodyComponent()
{
    entity->world->physics->world->removeRigidBody(rigidBody);
    delete rigidBody->getMotionState();
    delete rigidBody;
    delete shape;
}

void BtRigidBodyComponent::init()
{
    shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    Quaternion erot = entity->transform.rotation;
    Vec3 epos = entity->transform.position;
    btQuaternion rot(erot.x, erot.y, erot.z, erot.w);
    btVector3 pos(epos.x, epos.y, epos.z);
    motionState = new btDefaultMotionState(btTransform(rot, pos));
    btScalar mass = 1.0f;
    btVector3 inertia(0, 0, 0);
    shape->calculateLocalInertia(mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
    rigidBodyCI.m_friction = 1.5f;
    rigidBody = new btRigidBody(rigidBodyCI);
    entity->world->physics->world->addRigidBody(rigidBody);
}

void BtRigidBodyComponent::setPosition(Vec3 *position)
{
    btTransform btrans;
    rigidBody->getMotionState()->getWorldTransform(btrans);
    btrans.setOrigin(btVector3(position->x, position->y, position->z));
    rigidBody->getMotionState()->setWorldTransform(btrans);
    rigidBody->setCenterOfMassTransform(btrans);
}

void BtRigidBodyComponent::setRotation(Quaternion *rotation)
{
    btTransform btrans;
    rigidBody->getMotionState()->getWorldTransform(btrans);
    btrans.setRotation(btQuaternion(rotation->x, rotation->y, rotation->z, rotation->w));
    rigidBody->getMotionState()->setWorldTransform(btrans);
    rigidBody->setCenterOfMassTransform(btrans);
}

void BtRigidBodyComponent::onPhysicsUpdate(double dt)
{
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    btQuaternion rot = trans.getRotation();

    entity->transform.position = Vec3(pos.m_floats[0], pos.m_floats[1], pos.m_floats[2]);
    entity->transform.rotation = Quaternion(rot.m_floats[3], rot.m_floats[0], rot.m_floats[1], rot.m_floats[2]);
}

