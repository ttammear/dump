#include "world.h"
#include <math.h>
#include <assert.h>

#include "../Maths/maths.h"
#include "../camera.h"
#include "../macros.h"

World::World(Renderer *renderer, BlockStore *blockStore, Camera *cam)
    : chunkManager(renderer, cam, blockStore, this), worldGenerator(this)
{
    this->mainCam = cam;
    gravity = {0.0f, -9.8f, 0.0f};
}

World::~World()
{
    for(auto it = chunks.begin(); it != chunks.end(); ++it )
    {
        delete it->second;
    }
    chunks.clear();
}

ChunkData* World::getOrCreateChunkData(IVec3 chunkId)
{
    if(this->lastChunkAccess != nullptr 
            && this->lastChunkAccess->offset == chunkId)
    {
        return this->lastChunkAccess;
    }

    ChunkData *ret;
    auto fchunk = this->chunks.find(chunkId);
    if(fchunk != this->chunks.end())
    {
        ret = fchunk->second;
    }
    else
    {
        ret = new ChunkData(chunkId);
        this->chunks.insert({chunkId, ret});
        worldGenerator.fillChunk(chunkId);
    }
    this->lastChunkAccess = ret;
    return ret;
}


uint8_t World::getBlockId(IVec3 block)
{
    IVec3 chunkId = Chunk::getStoreChunkId(block);
    IVec3 localOffset(block.x - chunkId.x, block.y - chunkId.y, block.z - chunkId.z);
    auto fchunk = this->chunks.find(chunkId);
    
    ChunkData *cdata = getOrCreateChunkData(chunkId);

    const int s = CHUNK_STORE_SIZE;
    uint8_t ret = cdata->data[localOffset.x*s*s + localOffset.y*s + localOffset.z];
    return ret;
}

void World::fillFromSingleChunk(IVec3 startBlockId, IVec3 counts, IVec3 arrayOffsets, uint8_t *data)
{
    int xo, yo, zo, im, jm, km;
    const int s = CHUNK_STORE_SIZE;
    IVec3 lo = Chunk::getStoreLocalOffset(startBlockId);
    ChunkData *cdata = getOrCreateChunkData(Chunk::getStoreChunkId(startBlockId)); 
    assert(Chunk::getStoreChunkId(startBlockId) == Chunk::getStoreChunkId(startBlockId + counts + IVec3(-1, -1, -1)));
    im = (lo.x+counts.x)*s*s;
    jm = (lo.y+counts.y)*s;
    km = lo.z+counts.z;
    assert(lo.x < 16 && lo.y < 16 && lo.z < 16);
    for(int i = lo.x*s*s, xo = 0; i < im; i+=s*s, xo += arrayOffsets.x)
    for(int j = lo.y*s, yo = 0; j < jm; j+=s, yo += arrayOffsets.y)
    for(int k = lo.z, zo = 0; k < km; k++, zo += arrayOffsets.z)
    {
        data[xo+yo+zo] = cdata->data[i+j+k];
    }
}

void World::fillBlockCache(IVec3 originBlock, int size, uint8_t *data)
{
    assert(size < 16*16);
    IVec3 chunkId = Chunk::getStoreChunkId(originBlock);
    IVec3 localOffset = Chunk::getStoreLocalOffset(originBlock);

    struct AxisSection {
        int start;
        int count;
    };

    // remaining block count in origin chunk
    int cx = MIN(CHUNK_STORE_SIZE - localOffset.x, size), 
        cy = MIN(CHUNK_STORE_SIZE - localOffset.y, size), 
        cz = MIN(CHUNK_STORE_SIZE - localOffset.z, size);

    AxisSection xs[16], ys[16], zs[16];
    int xc = 1, yc = 1, zc = 1;
    xs[0] = {originBlock.x, cx};
    ys[0] = {originBlock.y, cy};
    zs[0] = {originBlock.z, cz};

    while(cx < size) {
        xs[xc].start = originBlock.x + cx;
        cx += (xs[xc++].count = MIN(size-cx, CHUNK_STORE_SIZE));
    }

    while(cy < size) {
        ys[yc].start = originBlock.y + cy;
        cy += (ys[yc++].count = MIN(size - cy, CHUNK_STORE_SIZE));
    }

    while(cz < size) {
        zs[zc].start = originBlock.z + cz;
        cz += (zs[zc++].count = MIN(size - cz, CHUNK_STORE_SIZE));
    }

    for(int i = 0; i < xc; i++)
    for(int j = 0; j < yc; j++)
    for(int k = 0; k < zc; k++)
    {
        IVec3 curOrigin(xs[i].start, ys[j].start, zs[k].start);
        IVec3 counts(xs[i].count, ys[j].count, zs[k].count);
        uint8_t *curData = data + ((curOrigin.x - originBlock.x)*size*size + (curOrigin.y - originBlock.y)*size + (curOrigin.z - originBlock.z));
        assert(curData < (data + size*size*size));
        fillFromSingleChunk(curOrigin, counts, IVec3(size*size, size, 1), curData);
    }
}

uint8_t World::setBlockId(IVec3 block, uint8_t newId)
{
    IVec3 chunkId = Chunk::getStoreChunkId(block);
    IVec3 localOffset(block.x - chunkId.x, block.y - chunkId.y, block.z - chunkId.z);
    
    ChunkData *cdata = getOrCreateChunkData(chunkId);

    const int s = CHUNK_STORE_SIZE;
    uint8_t ret = cdata->data[localOffset.x*s*s + localOffset.y*s + localOffset.z];
    cdata->data[localOffset.x*s*s + localOffset.y*s + localOffset.z] = newId;
    if(ret != newId)
        chunkManager.blockChanged(block);
    return ret;
}

bool World::lineCast(RaycastHit &hit, Vec3 start, Vec3 end)
{
    const float step = 0.01f;
    int steps = (start-end).length() / step;
    float progress = 0.0f;

    Vec3 ray = (end - start).normalized();

    IVec3 lastBlock;
    for(int i = 0; i < steps; i++)
    {
        Vec3 point = start + progress*ray;
        IVec3 blockV((int)floor(point.x), (int)floor(point.y), (int)floor(point.z));
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
}

void World::update()
{
    chunkManager.update();
}

void World::render()
{
    chunkManager.render();
}

void World::setCamera(Camera *cam)
{
    this->mainCam = cam;    
    chunkManager.camera = cam;
}

