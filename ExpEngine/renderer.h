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


struct Mesh
{
    u16 numVertices;
    u32 numIndices;

    // TODO: this is ugly, use unions?
    uintptr_t rendererHandle;
    uintptr_t rendererHandle2;
    uintptr_t rendererHandle3;

    uint8_t attribTypes[MAX_ATTRIBUTE_BUFFERS];
    uint32_t attribOffsets[MAX_ATTRIBUTE_BUFFERS];

    uint32_t vertexStride;
    uint32_t vertexBufferSize;
    uint32_t indexBufferSize;

    //TODO: temp
    struct Texture *tex;
};

struct Texture
{
    u16 format;
    u32 width;
    u32 height;

    uintptr_t rendererHandle;
    uintptr_t rendererHandle2;
    uint32_t bufferSize;
};

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

enum RenderMessageType
{
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
};

struct Shader
{
    uintptr_t rendererHandle;
    u32 type;
};

struct Material
{
    b32 isValid;
    uintptr_t rendererHandle;
    uint32_t perInstanceDataSize;
    struct Shader shaders[MATERIAL_MAX_SHADERS];
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
    void *vertBufPtr;
    void *idxBufPtr;
    void *dataBufPtr;
};

struct MeshUpdate
{
    uint32_t meshId;
};

struct MaterialQuery
{
    void *userData;
    uint32_t materialId;
    uint8_t shaderTypes[MATERIAL_MAX_SHADERS];
    const void *shaderCodes[MATERIAL_MAX_SHADERS];
    uint32_t shaderLengths[MATERIAL_MAX_SHADERS];
};

struct MaterialQueryDone
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

typedef struct RenderMessage
{
    uint32_t type;
    union 
    {
        struct MeshQuery meshQuery;
        struct MeshQueryResult meshQueryResult;
        struct MeshUpdate meshUpdate;
        struct MaterialQuery materialQuery;
        struct MaterialQueryDone materialQueryDone;
        struct TextureQuery texQ;
        struct TextureQueryResponse texQR;
        struct TextureUpdate texU;
    };
} RenderMessage;

DEFINE_RING_QUEUE(RenderMessage, 10);

struct RenderMessageChannel
{
    RING_QUEUE_TYPE(RenderMessage) toRenderer;
    RING_QUEUE_TYPE(RenderMessage) fromRenderer;
};

struct Renderer
{
    uint32_t type;
    struct Material fallbackMaterial;
    struct Mesh *meshes;
    struct Material *materials;
    struct Texture *textures;

    struct RenderMessageChannel ch;
};

struct OpenGLRenderer
{
    struct Renderer renderer;

    // max size for unitform buffer
    u32 uniBufSize;

    // buffer that stores per instance data
    // (except MVP matrix)
    u32 instanceDataBuffers[INSTANCE_BUFFER_COUNT];
    u32 curInstanceBufferIdx;

    // buffer for MVP matrix
    u32 matrixBuffers[MATRIX_BUFFER_COUNT];
    u32 curMatrixBufferIdx;
};

struct Renderer *create_renderer(u32 rendererType);
struct Renderer *create_opengl_renderer();
void destroy_opengl_renderer(struct OpenGLRenderer *glrend);
void destroy_renderer(struct Renderer *renderer);
void opengl_render_view(struct OpenGLRenderer *renderer, struct RenderViewBuffer *rbuf);

