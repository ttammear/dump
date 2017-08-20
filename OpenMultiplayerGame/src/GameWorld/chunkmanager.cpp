#include "chunkmanager.h"

#include "chunk.h"
#include "../macros.h"
#include "../Renderer/renderer.h"
#include "../camera.h"

ChunkManager::ChunkManager(Renderer *renderer, Camera *camera, BlockStore *bs, World *world)
{
    this->renderer = renderer;
    this->camera = camera;
    this->blockStore = bs;
    this->world = world;
}

ChunkManager::~ChunkManager()
{
    // TODO: delete all chunks
}

void ChunkManager::loadChunk(IVec3 chunkId)
{
    Chunk *chunk;
    chunk = new Chunk(blockStore, world, Vec3(-10.0f, -10.0f, -10.0f), 20);
    chunk->regenerateMesh();
    loadedChunks.insert({chunkId, chunk});
}

void ChunkManager::blockChanged(IVec3 block)
{
    //IVec3 chunkId = Chunk::getChunkId(block);
    IVec3 chunkId(-10, -10, -10);

    auto fchunk = loadedChunks.find(chunkId);
    if(fchunk != loadedChunks.end())
    {
        fchunk->second->flags |= Chunk::Flags::Dirty;
    }
    else
    {
        // not found
    }
}

void ChunkManager::update()
{
    for(auto it = loadedChunks.begin(); it != loadedChunks.end(); ++it )
    {
        Chunk *chunk = it->second;
        if(FLAGSET(chunk->flags, Chunk::Flags::Dirty))
        {
            chunk->regenerateMesh();
        }
    }
}

void ChunkManager::render()
{
    Mat4 world_to_clip = camera->getViewProjectionMatrix();
    for(auto it = loadedChunks.begin(); it != loadedChunks.end(); ++it )
    {
        // TODO: only need transform
        Mat4 model_to_world = Mat4::TRS(it->second->offset, Quaternion::Identity(), Vec3(1.0f, 1.0f, 1.0f));
        Mat4 model_to_clip = world_to_clip * model_to_world;
        renderer->renderMesh(it->second->mesh, renderer->defaultMaterial, &model_to_clip);
    }
}

