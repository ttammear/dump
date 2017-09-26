#include "physics.h"

#include <btBulletDynamicsCommon.h>

void Physics::init()
{
    collisionConf = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConf);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver;
    world = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConf);
    world ->setGravity(btVector3(0, -9.8f, 0));

    // TODO: free
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
    btRigidBody::btRigidBodyConstructionInfo
            groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    world->addRigidBody(groundRigidBody);

    lastStep = std::chrono::steady_clock::now();
}

void Physics::deinit()
{
    for(int i = world->getNumCollisionObjects()-1; i >= 0; i--)
    {
        btCollisionObject *obj = world->getCollisionObjectArray()[i];
        auto body = btRigidBody::upcast(obj);
        if(body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        world->removeCollisionObject(obj);
        delete obj;
    }

    /*for(int j = 0; j < collisionShapes.size(); j++)
    {
        auto shape = collisionSHapes[j];
        collisionShapes[j] = 0;
        delete shape;
    }*/

    delete world;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConf;
    //collisionShapes.clear();
}

inline void measureTime(Physics *physics)
{
    std::chrono::steady_clock::time_point curT = std::chrono::steady_clock::now();
    double timeSinceLast = std::chrono::duration<double>(curT - physics->lastStep).count();
    physics->offsetFromRealtime += timeSinceLast;
    physics->lastStep = curT;
}

bool Physics::simulate(bool& again)
{
    measureTime(this);
    if(offsetFromRealtime > -timestep)
    {
        world->stepSimulation(timestep, 10);
        offsetFromRealtime -= timestep;
        stepCount++;
        measureTime(this);
        again = offsetFromRealtime > -timestep;
        return true;
    }
    else 
    {
        again = false;
        return false;
    }
}

