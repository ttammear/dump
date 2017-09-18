#pragma once

#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "../Maths/maths.h"
#include "chunk.h"
#include "chunkmanager.h"
#include "worldgenerator.h" 
#include <assert.h>

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

class World
{
public:
    World(class Renderer *renderer, class BlockStore *blockStore);
    ~World();

    class ChunkData* getOrCreateChunkData(IVec3 chunkId);
    void markChunkDirty(IVec3 chunkId);
    uint8_t getBlockId(IVec3 block);
    uint8_t setBlockId(IVec3 block, uint8_t newId);
    bool lineCast(RaycastHit& hit, Vec3 start, Vec3 end);

    void update(Camera *cam);
    void render();

    std::unordered_map<IVec3, ChunkData*> chunks;
    std::vector<class Camera*> cameras;

    class Renderer *renderer;
    class BlockStore *blockStore;
    ChunkData *lastChunkAccess = nullptr;
    ChunkManager chunkManager;
    WorldGenerator worldGenerator;

    Vec3 gravity;
};
