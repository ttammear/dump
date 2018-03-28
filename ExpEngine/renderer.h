#pragma once

#define RENDERER_TYPE_OPENGL 1
#define RENDERER_TYPE_HEADLESS 2

#define MAX_ATTRIBUTE_BUFFERS 8

#define tt_render_warning(x)
#define tt_render_fatal(x)

#define MATERIAL_MAX_SHADERS 8

#define renderer_queue_message(r, m) ring_queue_enqueue(RenderMessage, &r->ch.toRenderer, m)
#define renderer_next_message(r, m) ring_queue_dequeue(RenderMessage, &r->ch.fromRenderer, m)

// TODO:????
#define INSTANCE_BUFFER_COUNT 3
#define MAX_INSTANCE_BUFFERS 3

#define MATRIX_BUFFER_COUNT 1
#define MAX_MATRIX_BUFFERS  1

struct MeshQueryResult;
struct MeshReady;
struct Renderer;
typedef void (*MQComplete_A)(struct Renderer* rend, struct MeshQueryResult *mqr, void *userData);
typedef void (*MReady_A)(struct Renderer *rend, struct MeshReady *mr, void *userData);

enum ShaderType_E
{
    ShaderType_None,
    ShaderType_GLSL_ES_Frag,
    ShaderType_GLSL_ES_Vert,
    ShaderType_GLSL_Frag,
    ShaderType_GLSL_Vert,
    ShaderType_Count
};

enum VertexAttributeType
{
    Vertex_Attribute_Type_None = 0,
    Vertex_Attribute_Type_Vec4,
    Vertex_Attribute_Type_Vec3,
    Vertex_Attribute_Type_Vec2,
    Vertex_Attribute_Type_Float
};

enum TextureFormat
{
    Texture_Format_None,
    Texture_Format_RGBA,
    Texture_Format_RGBA32F,
    Texture_Format_R32F,
    Texture_Format_R32I,
    Texture_Format_R8F,
    Texture_Format_R8U,
};

enum TextureFilter
{
    Texture_Filter_None,
    Texture_Filter_Bilinear,
    Texture_Filter_Trilinear
};

enum RenderMessageType
{
    Render_Message_None,
    Render_Message_Mesh_Query,
    Render_Message_Mesh_Query_Result,
    Render_Message_Mesh_Update,
    Render_Message_Mesh_Ready,
    Render_Message_Material_Query,
    Render_Message_Material_Ready,
    Render_Message_Texture_Query,
    Render_Message_Texture_Query_Response,
    Render_Message_Texture_Update,
    Render_Message_Texture_Ready,

    Render_Message_Screen_Resize,
    Render_Message_Stop
};

static const uint32_t s_vertexAttributeTypeSizes[] = 
{
    [Vertex_Attribute_Type_None] = 0,
    [Vertex_Attribute_Type_Vec4] = 16,
    [Vertex_Attribute_Type_Vec3] = 12,
    [Vertex_Attribute_Type_Vec2] = 8,
    [Vertex_Attribute_Type_Float] = 4,
};

static const uint32_t s_vertexAttributeTypePrims[] = 
{
    [Vertex_Attribute_Type_None] = 0,
    [Vertex_Attribute_Type_Vec4] = 4,
    [Vertex_Attribute_Type_Vec3] = 3,
    [Vertex_Attribute_Type_Vec2] = 2,
    [Vertex_Attribute_Type_Float] = 1,
};

struct MeshQuery
{
    void *userData;
    MQComplete_A onComplete;
    uint32_t meshId;
    uint32_t vertexCount;
    uint32_t indexCount;
    b32 largeIndices;
    uint32_t dataStorageSize;
    uint8_t attributeTypes[MAX_ATTRIBUTE_BUFFERS];
};

struct MeshQueryResult
{
    uint32_t meshId;
    void *userData;
    MQComplete_A onComplete;
    void *vertBufPtr;
    void *idxBufPtr;
    void *dataBufPtr;
};

struct MeshUpdate
{
    uint32_t meshId;
    void *userData;
    MReady_A onComplete;
};

struct MeshReady
{
    uint32_t meshId;
    void *userData;
    MReady_A onComplete;
};

struct MaterialQuery
{
    void *userData;
    uint32_t materialId;
    uint8_t shaderTypes[MATERIAL_MAX_SHADERS];
    const void *shaderCodes[MATERIAL_MAX_SHADERS];
    uint32_t shaderLengths[MATERIAL_MAX_SHADERS];
};

struct MaterialReady
{
    uint32_t materialId;
    void *userData;
};

struct TextureQuery
{
    void *userData;
    uint32_t textureId;
    uint32_t width;
    uint32_t height;
    uint32_t filter;
    enum TextureFormat format;
};

struct TextureQueryResponse
{
    uint32_t textureId;
    void *userData;
    void *textureDataPtr;
};

struct TextureUpdate
{
    uint32_t textureId;
    void *userData;
};

struct TextureReady
{
    uint32_t textureId;
};

struct ScreenResize
{
    uint32_t width;
    uint32_t height;
};

typedef struct RenderMessage
{
    uint32_t type;
    union 
    {
        struct MeshQuery meshQuery;
        struct MeshQueryResult meshQueryResult;
        struct MeshUpdate meshUpdate;
        struct MeshReady meshR;
        struct MaterialQuery matQ;
        struct MaterialReady matR;
        struct TextureQuery texQ;
        struct TextureQueryResponse texQR;
        struct TextureUpdate texU;
        struct TextureReady texR;
        struct ScreenResize screenR;
    };
} RenderMessage;

DEFINE_RING_QUEUE(RenderMessage, 10);

struct RenderMessageChannel
{
    RING_QUEUE_TYPE(RenderMessage) toRenderer;
    RING_QUEUE_TYPE(RenderMessage) fromRenderer;
};

typedef void* (*render_thread_proc_t)(void*);

struct Renderer
{
    uint32_t type;
    struct RenderMessageChannel ch;

    render_thread_proc_t threadProc;
    AikeThread *renderThread;
};

struct Renderer *create_renderer(u32 rendererType, AikePlatform *platform, struct SwapBuffer *sbuf);
void destroy_renderer(struct Renderer *renderer);

struct Renderer *create_opengl_renderer(AikePlatform *platform, struct SwapBuffer *sbuf);
void destroy_opengl_renderer(struct Renderer *glrend);

// TODO: remove
void *opengl_proc(void *data);
