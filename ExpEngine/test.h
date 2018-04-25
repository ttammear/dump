#pragma once

#pragma pack(push, 1)
typedef struct UIVertex
{
    V4 position;
    V2 texCoord;
    uint32_t color;
//    struct V4 color;
} UIVertex;
#pragma pack(pop)

typedef struct UIBatch
{
    uint32_t textureId;
    uint32_t scissorX0;
    uint32_t scissorY0;
    uint32_t scissorX1;
    uint32_t scissorY1;
    uint32_t indexStart;
    uint32_t indexCount;
} UIBatch;

typedef struct RenderMeshInstance
{
    u32 matrixIndex;
    u32 objectId;
} RenderMeshInstance;

typedef struct RenderMeshEntry
{
    uint32_t meshId;
    uint32_t materialId;
    u32 numInstances;
    RenderMeshInstance instances[];
} RenderMeshEntry;

typedef struct RenderSpace
{
    Mat4 viewProjection;
    u32 numEntries;
    struct RenderMeshEntry *meshEntries[];
    // Decals
    // Quads/Strps/Triangles/Whatever
} RenderSpace;

typedef struct RenderPostProc
{
    int dummy;
} RenderPostProc;

typedef struct RenderStep
{
    u32 type; // space, postproc
    u32 index;
} RenderStep;

typedef struct RenderView
{
    u32 numSpaces;
    u32 numPostProcs;
    u32 numSteps;

    uint32_t numVertices;
    struct UIVertex *vertices;
    uint32_t numIndices;
    uint16_t *indices;
    Mat4 orthoMatrix;

    uint32_t numUIBatches;
    struct UIBatch *uiBatches;

    uint32_t matrixCount;
    _Alignas(16) struct Mat4_sse2 *tmatrixBuf;

    Mat4_sse2 worldToClip;

    // Render features
    struct RenderSpace *space;
    //RenderPostProc postProc[];

    // Rendering order
    struct RenderStep *steps;
} RenderView;

typedef struct RenderViewBuffer
{
    u32 size;
    u8 *bottomPtr;

    _Alignas(64) RenderView view;
    u32 swapIndex;
    u8 end[4];
} RenderViewBuffer;

typedef struct BuilderMeshEntry
{
    uint32_t meshId;
    uint32_t materialId;
    u32 instanceCount;
} BuilderMeshEntry;

typedef struct BuilderMeshInstance
{
    int mentryIdx;
    Mat4 modelM;
    u32 objectId;
} BuilderMeshInstance;

typedef struct RenderViewBuilder
{
    struct BuilderMeshEntry *meshBuf;
    struct BuilderMeshInstance *instanceBuf;
    Mat4 viewProjection;

    Mat4 uiViewProjection;
    struct UIVertex *vertices;
    uint16_t *indices;
    uint32_t materialId;
    Mat4 orthoMatrix;
    uint32_t indexBase;

    struct UIBatch *batches;
    UIBatch curBatch;
    bool beginBatch;
} RenderViewBuilder;

typedef struct SwapBuffer
{
    struct RenderViewBuffer *viewBuffers[3];
    _Alignas(64) struct RenderViewBuffer *freeViewBuffer;
    _Alignas(64) volatile u32 viewBuffersTaken;
    _Alignas(64) volatile u32 numSwaps;
} SwapBuffer;

struct RenderViewBuffer* rview_buffer_init(void *memory, u32 size);
void* rview_buffer_destroy(struct RenderViewBuffer *rbuf);
void rview_buffer_clear(struct RenderViewBuffer *rbuf);
void rview_builder_reset(struct RenderViewBuilder *builder);
void build_view(struct RenderViewBuilder *builder, struct RenderViewBuffer *buf);
void add_mesh_instance(struct RenderViewBuilder *builder, uint32_t meshId, uint32_t materialId, struct Mat4 *modelM, u32 objectId);

void swap_buffer_init(struct SwapBuffer *vb);
void swap_buffer_destroy(struct SwapBuffer *sb);
// thread safe as long as its called before view_buffer_init
struct RenderViewBuffer* take_view_buffer(struct SwapBuffer *buf);
// thread safe
struct RenderViewBuffer* swap_view_for_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);
struct RenderViewBuffer* swap_view_if_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf);    

