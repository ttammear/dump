#pragma once

struct RenderMeshInstance
{
    struct Mat4 modelM;

    void *instanceDataPtr;
    u32 instanceDataSize;

    u32 numUniforms;
    char *uniformNames[16]; // shader uniform names
    u32 uniformTypes[16];   // uniform type - float, mat3, mat4 etc
    void *uniformData[16];  // pointer to uniform data in the data secion
};

struct RenderMeshEntry
{
    struct Mesh *mesh;
    struct Material *material;
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
    struct Mesh *mesh;
    struct Material *material;
    u32 instanceCount;
};

struct BuilderMeshInstance
{
    int mentryIdx;
    struct Mat4 modelM;
    u32 instanceDataIdx;
    u32 instanceDataSize;

    u32 numUniforms;
    char *uniformNames[16]; // shader uniform names
    u32 uniformTypes[16];   // uniform type - float, mat3, mat4 etc
    void *uniformData[16];  // pointer to uniform data in the data secion
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
void add_mesh_instance(struct RenderViewBuilder *builder, struct Mesh *mesh, struct Material *material, struct Mat4 *modelM, void *instanceData, u32 instanceDataSize);

void swap_buffer_init(struct SwapBuffer *vb);
void swap_buffer_destroy(struct SwapBuffer *sb);
// thread safe as long as its called before view_buffer_init
struct RenderViewBuffer* take_view_buffer(struct SwapBuffer *buf);
// thread safe
struct RenderViewBuffer* swap_view_for_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);
struct RenderViewBuffer* swap_view_if_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);    

