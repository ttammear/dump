#include "chunk.h"
#include "../macros.h"

#include <assert.h>
#include "../Renderer/mesh.h"
#include "blockstore.h"
#include "world.h"
#include <cstring>

namespace
{
    extern Vec3 sideOffsets[6];
    extern Vec3 sideQuads[3][4];
    extern Vec3 directions[3];
    extern Vec3 sideTexCoords[3][4];
}

Chunk::Chunk(BlockStore *blockStore, class World *world, IVec3 offset, int size)
{
    this->flags = 0;
    this->offset = offset;
    this->size = size;

    this->mesh = nullptr;
    this->waterMesh = nullptr;
    this->blockStore = blockStore;
    this->world = world;
}

Chunk::~Chunk()
{
    if(this->mesh != nullptr)
        delete this->mesh;
    if(this->waterMesh != nullptr)
        delete this->waterMesh;
}

IVec3 Chunk::getChunkId(IVec3 block)
{
    IVec3 ret;
    ret.x = ((int)floor(block.x / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
    ret.y = ((int)floor(block.y / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
    ret.z = ((int)floor(block.z / (float)CHUNK_STORE_SIZE)) * CHUNK_STORE_SIZE;
    return ret;
}

IVec3 Chunk::getLocalOffset(IVec3 block)
{
    IVec3 chunk = getChunkId(block);
    return IVec3(block.x - chunk.x, block.y - chunk.y, block.z - chunk.z);
}

bool isTransparent(uint8_t blockId)
{
    switch(blockId)
    {
        case 0:
            return true;
        case 9:
            return true;
        case 18:
            return true;
        default:
            return false;
    };
}

void Chunk::regenerateMesh()
{
    Vec3 vertices[65536];
    Vec3 texCoords[65536];
    uint16_t indices[65536];
    int vertCount = 0;
    int indexCount = 0;

    Vec3 transVertices[65536];
    Vec3 transTexCoords[65536];
    uint16_t transIndices[65536];
    int transVertCount = 0;
    int transIndexCount = 0;

    int size = this->size;
    int sizep1 = this->size+1;

    // cache all blockIds
    uint8_t blockIds[sizep1*sizep1*sizep1];
    for(int i = 0; i < sizep1; i++)
    for(int j = 0; j < sizep1; j++)
    for(int k = 0; k < sizep1; k++)
    {
        blockIds[i*sizep1*sizep1+j*sizep1+k] = world->getBlockId(IVec3((int)offset.x+i, (int)offset.y+j, (int)offset.z+k));
    }

    for(int i = 0; i < size; i++)
    for(int j = 0; j < size; j++)
    for(int k = 0; k < size; k++)
    {
        int blockId = blockIds[i*sizep1*sizep1+j*sizep1+k];
        bool blockEmpty = blockId == 0;
        bool blockTransparent = isTransparent(blockId);
        
        Block *block = blockStore->getBlock(blockId);

        uint8_t dirBlockIds[3];
        dirBlockIds[0] = blockIds[(i+1)*sizep1*sizep1+j*sizep1+k];
        dirBlockIds[1] = blockIds[i*sizep1*sizep1+(j+1)*sizep1+k];
        dirBlockIds[2] = blockIds[i*sizep1*sizep1+j*sizep1+(k+1)];
        for(int dir = 0; dir < 3; dir++)
        {
            uint8_t dirBlockId = dirBlockIds[dir];
            Block *dirBlock = blockStore->getBlock(dirBlockId);
            bool dirEmpty = dirBlockId == 0;
            bool dirTransparent = isTransparent(dirBlockId);
            // is block side visible?
            if(blockEmpty != dirEmpty || blockTransparent != dirTransparent)
            {
                Block* faceBlock;
                if(blockEmpty != dirEmpty)
                    faceBlock = blockEmpty ? dirBlock : block;
                else if(blockTransparent != dirTransparent)
                    faceBlock = blockTransparent ? dirBlock : block;

                uint8_t faceBlockId = faceBlock == block ? blockId : dirBlockId;
                bool faceTransparent = (isTransparent(faceBlockId) && faceBlockId != 0);

                Vec3 *vertArr = faceTransparent ? transVertices : vertices;
                Vec3 *texArr = faceTransparent ? transTexCoords : texCoords;
                uint16_t *indexArr = faceTransparent ? transIndices : indices;
                uint32_t tempVertCount = faceTransparent ? transVertCount : vertCount;
                uint32_t tempIdxCount = faceTransparent ? transIndexCount : indexCount;
                if(faceTransparent)
                {
                    transVertCount += 4;
                    transIndexCount += 6;
                }
                else
                {
                    vertCount += 4;
                    indexCount += 6;
                }

                for(int vert = 0; vert < 4; vert++)
                {
                    vertArr[tempVertCount+vert] = Vec3(i, j, k) + sideQuads[dir][vert] + directions[dir];
                    texArr[tempVertCount+vert] = sideTexCoords[dir][vert];
                    texArr[tempVertCount+vert].z = faceBlock->faceTextureLayers[dir*2 + (faceBlock == block ? 0 : 1)];
                }
                if(faceBlock == block)
                {
                    indexArr[tempIdxCount+0] = tempVertCount + 0;
                    indexArr[tempIdxCount+1] = tempVertCount + 1;
                    indexArr[tempIdxCount+2] = tempVertCount + 2;
                    indexArr[tempIdxCount+3] = tempVertCount + 0;
                    indexArr[tempIdxCount+4] = tempVertCount + 2;
                    indexArr[tempIdxCount+5] = tempVertCount + 3;
                }
                else
                {
                    indexArr[tempIdxCount+0] = tempVertCount + 3;
                    indexArr[tempIdxCount+1] = tempVertCount + 2;
                    indexArr[tempIdxCount+2] = tempVertCount + 0;
                    indexArr[tempIdxCount+3] = tempVertCount + 2;
                    indexArr[tempIdxCount+4] = tempVertCount + 1;
                    indexArr[tempIdxCount+5] = tempVertCount + 0;
                }
            }
        }
    }

    Mesh *mesh, *transMesh;

    if(vertCount > 0) {
        if(this->mesh != nullptr)
            mesh = this->mesh;
        else {
            mesh = new Mesh();
            this->mesh = mesh;
        }
        mesh->copyVertices(vertices, vertCount);
        mesh->copyTexCoords(texCoords, vertCount);
        mesh->copyIndices(indices, indexCount);
        mesh->calculateNormals();
    }
    else
        this->mesh = nullptr;
    if(transVertCount > 0) {
        if(this->waterMesh != nullptr)
            transMesh = this->waterMesh;
        else {
            transMesh = new Mesh();
            this->waterMesh = transMesh;
        }
        transMesh->copyVertices(transVertices, transVertCount);
        transMesh->copyTexCoords(transTexCoords, transVertCount);
        transMesh->copyIndices(transIndices, transIndexCount);
        transMesh->calculateNormals();
    }
    else
        this->waterMesh = nullptr;

    if(transVertCount > 0 || vertCount > 0)
        this->flags &= ~Flags::Dirty;
}

namespace
{
    Vec3 directions [3] =
    {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    Vec3 sideQuads[3][4] =
    {
        { // right/left
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f}
        },
        { // top/bottom
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f}
        },
        { // front/back
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f}
        }
    };

    Vec3 sideTexCoords [3][4] =
    {
        { // right/left
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        },
        { // top/bottom
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        },
        { // front/back
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f}
        }
    };
}

