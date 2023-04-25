#pragma once

#include "../Maths/maths.h"
#include "../macros.h"
#include <stdint.h>

#define CHUNK_STORE_SIZE    16  // dimensions of stored chunk
#define CHUNK_SIZE          16  // dimensions of rendered chunk

class Chunk
{
public:
    enum Flags
    {
        Dirty           = 1 << 0
    };

    Chunk(class BlockStore *blockStore, class World *world, IVec3 offset);
    ~Chunk();

    void regenerateMesh();

    static inline IVec3 getChunkId(IVec3 block)
    {
        IVec3 ret;
        ret.x = ((int)USFLOOR(block.x / (float)CHUNK_SIZE)) * CHUNK_SIZE;
        ret.y = ((int)USFLOOR(block.y / (float)CHUNK_SIZE)) * CHUNK_SIZE;
        ret.z = ((int)USFLOOR(block.z / (float)CHUNK_SIZE)) * CHUNK_SIZE;
        return ret;
    }

    static inline IVec3 getLocalOffset(IVec3 block)
    {
        IVec3 chunk = getChunkId(block);
        return IVec3(block.x - chunk.x, block.y - chunk.y, block.z - chunk.z);
    }

    static inline IVec3 getStoreChunkId(IVec3 block)
    {
        IVec3 ret;
        ret.x = ((int)USFLOOR(block.x / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
        ret.y = ((int)USFLOOR(block.y / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
        ret.z = ((int)USFLOOR(block.z / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
        return ret;
    }

    static inline IVec3 getStoreLocalOffset(IVec3 block)
    {
        IVec3 chunk = getStoreChunkId(block);
        return IVec3(block.x - chunk.x, block.y - chunk.y, block.z - chunk.z);
    }

    IVec3 offset;
    int flags;
    class Mesh *mesh;
    class Mesh *waterMesh;
    class BlockStore *blockStore;
    class World *world;
};
