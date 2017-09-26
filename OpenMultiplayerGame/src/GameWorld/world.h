#pragma once

#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "../Maths/maths.h"
#include "chunk.h"
#include "chunkmanager.h"
#include "worldgenerator.h" 
#include <assert.h>
#include "transform.h"
#include "entity.h"

struct ChunkData
{
    enum Flags
    {
        Populated = 1 << 0
    };

    ChunkData(IVec3 offset)
    {
        this->offset = offset;
    }

    void setBlock(int x, int y, int z, uint8_t value)
    {
        assert(x < CHUNK_STORE_SIZE && y < CHUNK_STORE_SIZE && z < CHUNK_STORE_SIZE);
        data[x*CHUNK_STORE_SIZE*CHUNK_STORE_SIZE + y*CHUNK_STORE_SIZE + z] = value;
    }

    int flags;
    IVec3 offset;
    uint8_t data[CHUNK_STORE_SIZE*CHUNK_STORE_SIZE*CHUNK_STORE_SIZE];
};

struct RaycastHit
{
    Vec3 point;
    IVec3 block;
    IVec3 faceDirection;
};

#define MAX_ENTITIES 100

class World
{
public:
    World(class Renderer *renderer, class BlockStore *blockStore);
    ~World();

    struct Entity* createEntity();
    void destroyEntity(struct Entity *entity);
    
    class ChunkData* getOrCreateChunkData(IVec3 chunkId);
    void markChunkDirty(IVec3 chunkId);
    uint8_t getBlockId(IVec3 block);
    uint8_t setBlockId(IVec3 block, uint8_t newId);
    bool lineCast(RaycastHit& hit, Vec3 start, Vec3 end);

    void update(float dt, Camera *cam);
    void render();

    std::unordered_map<IVec3, ChunkData*> chunks;
    std::vector<class Camera*> cameras;

    class Renderer *renderer;
    class BlockStore *blockStore;
    class Physics *physics;
    ChunkData *lastChunkAccess = nullptr;
    ChunkManager chunkManager;
    WorldGenerator worldGenerator;

    Entity entities[MAX_ENTITIES];
    class Mesh *cubeMesh;

    Vec3 gravity;

    double timePassed = 0.0f;
};
