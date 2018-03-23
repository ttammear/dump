#pragma once

struct RenderMeshInstance
{
    u32 matrixIndex;

    void *instanceDataPtr;
    u32 instanceDataSize;
};

struct RenderMeshEntry
{
    uint32_t meshId;
    uint32_t materialId;
    u32 numInstances;
    struct RenderMeshInstance instances[];
};

struct RenderSpace
{
    struct Mat4 viewProjection;
    u32 numEntries;
    struct RenderMeshEntry *meshEntries[];
    // Decals
    // Quads/Strps/Triangles/Whatever
};

struct RenderPostProc
{
    int dummy;
};

struct RenderStep
{
    u32 type; // space, postproc
    u32 index;
};

struct RenderView
{
    u32 numSpaces;
    u32 numPostProcs;
    u32 numSteps;

    struct Mat4_sse2 *tmatrixBuf;

    struct Mat4_sse2 worldToClip;

    // Render features
    struct RenderSpace *space;
    //RenderPostProc postProc[];

    // Rendering order
    struct RenderStep *steps;
};

struct RenderViewBuffer
{
    u32 size;
    u8 *bottomPtr;

    struct RenderView view;
    u32 swapIndex;
    u8 end[4];
};

struct BuilderMeshEntry
{
    uint32_t meshId;
    uint32_t materialId;
    u32 instanceCount;
};

struct BuilderMeshInstance
{
    int mentryIdx;
    struct Mat4 modelM;
    u32 instanceDataIdx;
    u32 instanceDataSize;
};

struct RenderViewBuilder
{
    struct BuilderMeshEntry *meshBuf;
    struct BuilderMeshInstance *instanceBuf;
    u8 *instanceDataBuf;;
};

struct SwapBuffer
{
    struct RenderViewBuffer *viewBuffers[3];
    struct RenderViewBuffer *freeViewBuffer;
    volatile u32 viewBuffersTaken;
    volatile u32 numSwaps;
};

struct RenderViewBuffer* rview_buffer_init(void *memory, u32 size);
void* rview_buffer_destroy(struct RenderViewBuffer *rbuf);
void rview_buffer_clear(struct RenderViewBuffer *rbuf);
void rview_builder_reset(struct RenderViewBuilder *builder);
void build_view(struct RenderViewBuilder *builder, struct RenderViewBuffer *buf);
void add_mesh_instance(struct RenderViewBuilder *builder, uint32_t meshId, uint32_t materialId, struct Mat4 *modelM, void *instanceData, u32 instanceDataSize);

void swap_buffer_init(struct SwapBuffer *vb);
void swap_buffer_destroy(struct SwapBuffer *sb);
// thread safe as long as its called before view_buffer_init
struct RenderViewBuffer* take_view_buffer(struct SwapBuffer *buf);
// thread safe
struct RenderViewBuffer* swap_view_for_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);
struct RenderViewBuffer* swap_view_if_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);    

