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

Chunk::Chunk(BlockStore *blockStore, class World *world, IVec3 offset)
{
    this->flags = 0;
    this->offset = offset;

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

static inline bool isTransparent(uint8_t blockId)
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

    const int size = CHUNK_SIZE;
    const int sizep1 = CHUNK_SIZE+1;

    // cache all blockIds
    uint8_t blockIds[sizep1*sizep1*sizep1];
    world->fillBlockCache(offset, CHUNK_SIZE+1, blockIds);

    int blockIdx;

    for(int dir = 0; dir < 3; dir++) { // sweeping face
        for(int i = 0; i < size; i++) { // blockX
            for(int j = 0; j < size; j++) { // blockY
                blockIdx = i*sizep1*sizep1+j*sizep1;
                for(int k = 0; k < size; k++, blockIdx++) { // blockZ
                    int blockId = blockIds[blockIdx];
                    bool blockEmpty = blockId == 0;
                    bool blockTransparent = isTransparent(blockId);
                    
                    Block *block = blockStore->getBlock(blockId);

                    // id of next block in current direction
                    uint8_t dirBlockId;
                    switch(dir) {
                        case 0:
                            dirBlockId = blockIds[blockIdx+sizep1*sizep1];
                            break;
                        case 1:
                            dirBlockId = blockIds[blockIdx+sizep1];
                            break;
                        case 2:
                            dirBlockId = blockIds[blockIdx+1];
                            break;
                    }

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
                        if(faceBlock == block) // swap face normal?
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

