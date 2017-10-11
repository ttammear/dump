#pragma once

#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "../Maths/maths.h"
#include <assert.h>
#include "transform.h"
#include "entity.h"

struct RaycastHit
{
    Vec3 point;
    IVec3 block;
    IVec3 faceDirection;
};

struct WorldPlayer
{
    class Player *player;
    void *usrPtr;
    void (*getInputFunc)(void*, struct PlayerInput*);
};

#define MAX_ENTITIES 100

class World
{
public:
    World(class Renderer *renderer);
    ~World();

    struct Entity* createEntity();
    void destroyEntity(struct Entity *entity);
    
//    class ChunkData* getOrCreateChunkData(IVec3 chunkId);
//    void markChunkDirty(IVec3 chunkId);
//    uint8_t getBlockId(IVec3 block);
//    uint8_t setBlockId(IVec3 block, uint8_t newId);
//    bool lineCast(RaycastHit& hit, Vec3 start, Vec3 end);

    void update(float dt, class Camera *cam);
    void render();

    std::vector<class Camera*> cameras;

    std::vector<WorldPlayer> players;

    class Renderer *renderer;
//    class BlockStore *blockStore;
    class Physics *physics;
//    ChunkManager chunkManager;
//    WorldGenerator worldGenerator;

    void (*onTick)(void *userPtr) = NULL;
    void *onTickUserPtr = NULL;

    Entity entities[MAX_ENTITIES];
    class Mesh *cubeMesh;

    Vec3 gravity;

    double timePassed = 0.0f;
};
