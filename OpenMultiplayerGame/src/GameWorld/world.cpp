#include "world.h"
#include <math.h>

#include "../Maths/maths.h"
#include "../camera.h"
#include "macros.h"
#include "../Renderer/mesh.h"
#include "../Renderer/renderer.h"
#include "../Physics/physics.h"
#include "../Player/player.h"

static Vec3 vertices[] = 
{
	{ -0.5f,-0.5f,  0.5f },
	{ 0.5f, -0.5f,  0.5f },
	{ 0.5f,  0.5f,  0.5f },
	{-0.5f,  0.5f,  0.5f },
	{ 0.5f, -0.5f,  0.5f },
	{ 0.5f, -0.5f, -0.5f },
	{ 0.5f,  0.5f, -0.5f },
	{ 0.5f,  0.5f,  0.5f },
	{ 0.5f, -0.5f, -0.5f },
	{-0.5f, -0.5f, -0.5f },
	{-0.5f,  0.5f, -0.5f },
	{ 0.5f,  0.5f, -0.5f },
	{-0.5f, -0.5f, -0.5f },
	{-0.5f, -0.5f,  0.5f },
	{-0.5f,  0.5f,  0.5f },
	{-0.5f,  0.5f, -0.5f },
	{-0.5f,  0.5f,  0.5f },
	{ 0.5f,  0.5f,  0.5f },
	{ 0.5f,  0.5f, -0.5f },
	{-0.5f,  0.5f, -0.5f },
	{-0.5f, -0.5f, -0.5f },
	{ 0.5f, -0.5f, -0.5f },
	{ 0.5f, -0.5f,  0.5f },
	{-0.5f, -0.5f,  0.5f }
};

static Vec3 texCoords[] = 
{
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f }
};

static unsigned short indices[]
{
	2, 1, 0, 0, 3, 2,
	6, 5, 4, 4, 7, 6,
	10, 9, 8, 8, 11, 10,
	14, 13, 12, 12, 15, 14,
	18, 17, 16, 16, 19, 18,
	22, 21, 20, 20, 23, 22
};

struct WeirdComponent : public Component
{
    double timePassed = 0.0;
    float i = 0.0f;

    void onUpdate(double dt)
    {
        transform->position = Vec3(i, sin(this->timePassed+i) + 3.0f, 0.0f);
        transform->rotation = Quaternion::AngleAxis(sin(this->timePassed) * 90.0f, Vec3(0.0f, 1.0f, 0.0f));
        this->timePassed += dt;
    }
};

#include "../Physics/btrigidbodycomponent.h"

World::World(Renderer *renderer)
{
    gravity = {0.0f, -9.8f, 0.0f};
    cubeMesh = new Mesh();
    cubeMesh->copyVertices(vertices, ARRAY_COUNT(vertices));
    cubeMesh->copyIndices(indices, ARRAY_COUNT(indices));
    cubeMesh->copyTexCoords(texCoords, ARRAY_COUNT(texCoords));
    cubeMesh->calculateNormals();

    physics = new Physics();
    physics->init();

    for(int i = 0; i < 10; i++)
    {
        auto entity = createEntity();
        entity->transform.position = Vec3(0.0f, 50.0f, 0.0f);
        entity->transform.rotation = Quaternion::Identity();
#ifdef SERVER
        auto comp = entity->addComponent<BtRigidBodyComponent>();        
#endif
        //comp->i = (float)i;
    }
}

World::~World()
{
    delete cubeMesh;

    physics->deinit();
    delete physics;
}

Entity* World::createEntity()
{
    for(int i = 0; i < MAX_ENTITIES; i++)
    {
        if(entities[i].inUse == false)
        {
            printf("Created entity %d\n", i);
            Entity *ret = &entities[i];
            ret->id = i;
            ret->inUse = true;
            ret->world = this;
            return ret;
        }
    }
    return NULL;
}

void World::destroyEntity(Entity *entity)
{
    entity->reset();
}

/*bool World::lineCast(RaycastHit &hit, Vec3 start, Vec3 end)
{
    const float step = 0.01f;
    int steps = (start-end).length() / step;
    float progress = 0.0f;

    Vec3 ray = (end - start).normalized();

    IVec3 lastBlock;
    for(int i = 0; i < steps; i++)
    {
        Vec3 point = start + progress*ray;
        IVec3 blockV((int)myfloorf(point.x), (int)myfloorf(point.y), (int)myfloorf(point.z));
        if(!(blockV == lastBlock))
        {
            uint8_t bid = getBlockId(blockV);    
            if(bid != 0 && bid != 9)
            {
                hit.point = point;
                hit.block = blockV;
                
                // determine block face TODO: seems buggy sometimes!
                float offsetx = fabs(blockV.x - point.x);
                float offsety = fabs(blockV.y - point.y);
                float offsetz = fabs(blockV.z - point.z);
                Vec3 offset0(fabs(0.0f - offsetx), fabs(0.0f - offsety), fabs(0.0f - offsetz));
                Vec3 offset1(fabs(1.0f - offsetx), fabs(1.0f - offsety), fabs(1.0f - offsetz));
                Vec3 minOffset(mymin(offset0.x, offset1.x), mymin(offset0.y, offset1.y), mymin(offset0.z, offset1.z));
                if(minOffset.x < minOffset.y && minOffset.x < minOffset.z)
                    hit.faceDirection = (offset0.x < offset1.x ? IVec3(-1, 0, 0) : IVec3(1, 0, 0));
                else if(minOffset.y < minOffset.z)
                    hit.faceDirection = (offset0.y < offset1.y ? IVec3(0, -1, 0) : IVec3(0, 1, 0));
                else
                    hit.faceDirection = (offset0.z < offset1.z ? IVec3(0, 0, -1) : IVec3(0, 0, 1));

                return true;
            }
            lastBlock = blockV;
        }
        progress += step;
    }
    return false;
}*/

void World::update(float dt, Camera *cam)
{
    //chunkManager.viewerPosition = cam->transform.position;
    //chunkManager.update();

    timePassed += (double)dt;

    bool result, again;
    int curCount = 0;
    do
    {
        // update player inputs
        for(auto& player : players)
        {
            PlayerInput pinput;
            player.getInputFunc(player.usrPtr, &pinput);
            player.player->update(dt, Vec2(0.0f, 0.0f), pinput);
        }

        result = physics->simulate(again);
        if(onTick != NULL)
            onTick(onTickUserPtr);
        if(result)
        {
            for(int i = 0; i < MAX_ENTITIES; i++)
            {
                if(entities[i].inUse)
                {
                    for(auto component : entities[i].components)
                    {
                        component->onPhysicsUpdate(dt);
                    }
                }
            }
        }
        curCount++;
        if(curCount > 3)
        {
            fprintf(stderr, "Simulated more than 3 physics steps in a row!\n");
            fprintf(stderr, "Seems like the physics simulation can't keep up!\n");
            break;
        }
    } while(again);

    //printf("phys %f %f\n", 1.0 / (timePassed / physics->stepCount), physics->offsetFromRealtime);

    for(int i = 0; i < MAX_ENTITIES; i++)
    {
        if(entities[i].inUse)
        {
            for(auto component : entities[i].components)
            {
                component->onUpdate(dt);
            }
        }
    }

}

void World::render()
{
    //Mat4 id = Mat4::TRS(Vec3(0.0f, 3.0f, 1.0f), Quaternion::Identity(), Vec3(1.0f, 1.0f, 1.0f));

    for(auto cam : cameras)
    {
        if((cam->flags & Camera::Flags::Disabled) == 0)
        {
            //chunkManager.render(cam);
            Mat4 vp = cam->getViewProjectionMatrix();
            for(int i = 0; i < MAX_ENTITIES; i++)
            {
                if(entities[i].inUse && entities[i].active)
                {
                    Mat4 id = entities[i].transform.getModelMatrix();
                    renderer->renderMesh(cubeMesh, renderer->defaultMaterial, &id, &vp);
                }
            }
        }
    }

}

