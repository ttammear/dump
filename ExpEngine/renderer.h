#pragma once

#define RENDERER_TYPE_OPENGL 1
#define RENDERER_TYPE_HEADLESS 2

#define MAX_ATTRIBUTE_BUFFERS 8

#define tt_render_warning(x) (fprintf(stderr, x))
#define tt_render_fatal(x) (fprintf(stderr, x), exit(-1))

#define MATERIAL_MAX_SHADERS 8

#define renderer_queue_message(r, m) ring_queue_enqueue(RenderMessage, &r->ch.toRenderer, m)
#define renderer_next_message(r, m) ring_queue_dequeue(RenderMessage, &r->ch.fromRenderer, m)

// TODO:????
#define INSTANCE_BUFFER_COUNT 3
#define MAX_INSTANCE_BUFFERS 3

#define MATRIX_BUFFER_COUNT 3
#define MAX_MATRIX_BUFFERS  3

struct MeshQueryResult;
struct MeshReady;
struct Renderer;
struct TextureQueryResponse;
struct TextureReady;
struct ObjectIDSamplesReady;
struct MaterialReady;
typedef void (*MQComplete_A)(struct Renderer* rend, struct MeshQueryResult *mqr, void *userData);
typedef void (*MReady_A)(struct Renderer *rend, struct MeshReady *mr, void *userData);
typedef void (*TQComplete_A)(struct Renderer* rend, struct TextureQueryResponse *tqr, void *userData);
typedef void (*TReady_A)(struct Renderer *rend, struct TextureReady *tr, void *userData);
typedef void (*OSReady_A)(struct Renderer *rend, struct ObjectIDSamplesReady *tdr, void *userData);
typedef void (*MatReady_A)(struct Renderer *rend, struct MaterialReady *mr, void *userData);

enum ShaderType
{
    Shader_Type_None,
    Shader_Type_Unlit_Color, // Color
    Shader_Type_Unlit_Vertex_Color, // nothing
    Shader_Type_Unlit_Textured, // texture, tint?
    Shader_Type_Count,
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

    Render_Message_Sample_Object_Id,
    Render_Message_Sample_Object_Ready,

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

// Material instance data
//

#pragma pack(push, 1)
struct UnlitColorIData
{
    V4 color;
};

struct UnlitTexturedIData
{
    uint32_t textureId;
    V4 color;
};

struct UnlitVertexColor
{
    V4 color;
};

union InstanceData
{
    struct UnlitColorIData unlitColor;
    struct UnlitTexturedIData unlitTextured;
    struct UnlitVertexColor unlitVertexColor;
};
#pragma pack(pop)

// Renderer commands
//

typedef struct MeshSection
{
    uint32_t offset;
    uint32_t count;
    uint32_t materialId;
}MeshSection;

#define MAX_MESH_SECTIONS 4

typedef struct MeshQuery
{
    void *userData;
    MQComplete_A onComplete;
    uint32_t meshId;
    uint32_t vertexCount;
    uint32_t indexCount; // number of total indices
    uint8_t attributeTypes[MAX_ATTRIBUTE_BUFFERS];
} MeshQuery;

typedef struct MeshQueryResult
{
    uint32_t meshId;
    void *userData;
    MQComplete_A onComplete;
    void *vertBufPtr;
    void *idxBufPtr;
    void *dataBufPtr;
} MeshQueryResult;

typedef struct MeshUpdate
{
    uint32_t meshId;
    void *userData;
    MReady_A onComplete;
/*    uint16_t numSections;
    MeshSection sections[MAX_MESH_SECTIONS]; // TODO: this will make command union very large*/
} MeshUpdate;

typedef struct MeshReady
{
    uint32_t meshId;
    void *userData;
    MReady_A onComplete;
} MeshReady;

typedef struct MaterialQuery
{
    void *userData;
    MatReady_A onComplete;
    uint32_t materialId;
    uint32_t shaderId;
    union InstanceData iData;
} MaterialQuery;

typedef struct MaterialReady
{
    uint32_t materialId;
    void *userData;
    MatReady_A onComplete;
} MaterialReady;

typedef struct TextureQuery
{
    void *userData;
    TQComplete_A onComplete;
    uint32_t textureId;
    uint32_t width;
    uint32_t height;
    uint32_t filter;
    enum TextureFormat format;
} TextureQuery;

typedef struct TextureQueryResponse
{
    uint32_t textureId;
    void *userData;
    TQComplete_A onComplete;
    void *textureDataPtr;
} TextureQueryResponse;

typedef struct TextureUpdate
{
    uint32_t textureId;
    void *userData;
    TReady_A onComplete;
} TextureUpdate;

typedef struct TextureReady
{
    uint32_t textureId;
    void *userData;
    TReady_A onComplete;
} TextureReady;

typedef struct ScreenResize
{
    uint32_t width;
    uint32_t height;
} ScreenResize;

typedef struct SampleObjectId
{
    // texture coordinates for samples 0..1
    struct V2 *normalizedSampleCoords;
    uint32_t sampleCount;

    // buffer where n=sampleCount sample values will be writtern
    uint32_t *buffer;

    void *userData;
    void ** onComplete;

    uint32_t fromTextureId;
} SampleObjectId;

typedef struct ObjectIDSamplesReady
{
    void *userData;
    void ** onComplete;
} ObjectIDSamplesReady;

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
        struct SampleObjectId sampleO;
        struct ObjectIDSamplesReady sampleOR;
    };
} RenderMessage;

DEFINE_RING_QUEUE(RenderMessage, 10);

typedef struct RenderMessageChannel
{
    RING_QUEUE_TYPE(RenderMessage) toRenderer;
    RING_QUEUE_TYPE(RenderMessage) fromRenderer;
} RenderMessageChannel;

typedef void* (*render_thread_proc_t)(void*);

typedef struct Renderer
{
    uint32_t type;
    struct RenderMessageChannel ch;

    AikeThread *renderThread;
    AikePlatform *platform;

    struct SwapBuffer *swapBuffer;
} Renderer;

struct Renderer *create_renderer(u32 rendererType, AikePlatform *platform);
void destroy_renderer(struct Renderer *renderer);

struct Renderer *create_opengl_renderer(AikePlatform *platform);
void destroy_opengl_renderer(struct Renderer *glrend);
void start_opengl_renderer(struct Renderer *rend);
void stop_opengl_renderer(struct Renderer *rend);
