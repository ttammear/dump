#include <map>
#include <assert.h>
#include "../macros.h"

// 0 - right face
// 1 - left face
// 2 - top face
// 3 - bottom face
// 4 - front face
// 5 - back face

class Block
{
public:
    const char *name;
    int faceTextureLayers[6];
};

class BlockStore
{
public:
    enum Flags
    {
        Initialized = 1<<0   
    };

    void createBlock(int blockId, const Block &block)
    {
        assert(blockId < 256);
        blockFlags[blockId] |= Flags::Initialized;
        blocks[blockId] = block;
    }

    inline Block *getBlock(int blockId)
    {
        assert(blockId < 256);
        if(FLAGSET(blockFlags[blockId], Flags::Initialized))
        {
           return &blocks[blockId]; 
        }
        else
        {
            // TODO: return dummy block
            //fprintf(stderr, "Block %d not loaded\n", blockId);
            return NULL;
        }
    }

    Block blocks[256];
    int blockFlags[256];
};

