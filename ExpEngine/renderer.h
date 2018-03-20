#pragma once

#define RENDERER_TYPE_OPENGL 1
#define RENDERER_TYPE_HEADLESS 2

#define RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS 1024

enum RenderEvent_E
{
    RenderEvent_MeshUpdate,
    RenderEvent_MeshDestroy,
    RenderEvent_TextureUpdate,
    RenderEvent_TextureDestroy,
    RenderEvent_MaterialUpdate,
    RenderEvent_MaterialDestroy,
    RenderEvent_Count
};

#define tt_render_warning(x)
#define tt_render_fatal(x)

enum MeshFlags
{ 
    MeshFlag_Dirty          = 1<<0,
    MeshFlag_HasVertices    = 1<<1,
    MeshFlag_HasIndices     = 1<<2,
    MeshFlag_HasTexcoords   = 1<<3,
    MeshFlag_HasNormals     = 1<<4,
    MeshFlag_HasColors      = 1<<5,
    MeshFlag_Initialized    = 1<<6
};

enum TextureFlags_E
{
    TextureFlag_Dirty           = 1<<0,
    TextureFlag_HasData         = 1<<1,
    TextureFlag_Initialized     = 1<<2
};

typedef void (*RenderEventCallback_F)(u32 eventId, void *eventData, void *usrPtr);


struct RenderEventSubEntry
{
    void *usrPtr;
    RenderEventCallback_F callback;
    struct RenderEventSubEntry *next;
    struct RenderEventSubEntry *prev;
};

struct RenderEventHandler
{
    u32 firstFree;
    u16 freelist[RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS];
    struct RenderEventSubEntry entryPool[RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS];
    struct RenderEventSubEntry *entries[RenderEvent_Count];
};


extern struct RenderEventHandler *g_renderEventHandler;

void init_render_event_handler();
void* subscribe_to_render_event(u32, RenderEventCallback_F, void* usrPtr);
void unsubscribe_from_render_event(u32 eventId, void *ptr);
void trigger_render_event(u32 eventId, void *eventData);

struct Mesh
{
    u16 flags;
    u16 numVertices;
    u32 numIndices;

    // TODO: this is ugly, use unions?
    uintptr_t rendererHandle;
    uintptr_t rendererHandle2;
    uintptr_t rendererHandle3;

    struct V3 *vertices;
    struct V3 *texCoords;
    struct V3 *normals;
    struct V4 *colors;
    u16 *indices;

    //TODO: temp
    struct Texture *tex;
};

enum TextureFormat_E
{
    TextureFormat_None,
    TextureFormat_RGB,
    TextureFormat_RGBA,
    TextureFormat_R8,
    TextureFormat_Count
};

struct Texture
{
    u16 flags;
    u16 format;
    u32 width;
    u32 height;

    uintptr_t rendererHandle;

    void *data;
};


// NOTES:
// should avoid doing immediate state changes in command buffer
// ideally a command buffer could be executed by multiple threads (Vulkan, DX12 etc)
// state changes like viewport or matrix change would require a fence
// the alternative would be to copy the data somehow
// for example a command buffer could also have a matrix buffer
// and each command requiring a matrix could store an index to that buffer
// that could probably be done atomically during command buffer generation

// TODO: RenderTarget
// push_render_target(); ?? 

enum ShaderType_E
{
    ShaderType_None,
    ShaderType_GLSL_ES_Frag,
    ShaderType_GLSL_ES_Vert,
    ShaderType_GLSL_Frag,
    ShaderType_GLSL_Vert,
    ShaderType_Count
};

#define MATERIAL_MAX_SHADERS 8

enum ShaderFlags
{
    ShaderFlag_Initialized = 1<<0,
    ShaderFlag_Dirty       = 1<<1,
    ShaderFlag_Active      = 1<<2,
};

enum MaterialFlags
{
    MaterialFlag_Dirty          =   1<<0,
    MaterialFlag_Initialized    =   1<<1,
    MaterialFlag_Valid          =   1<<2
};

struct Shader
{
    uintptr_t rendererHandle;
    uint32_t flags;
    u32 type;
    const char *source;
};

struct Material
{
    uintptr_t rendererHandle;
    uint32_t flags;
    uint32_t perInstanceDataSize;
    struct Shader shaders[MATERIAL_MAX_SHADERS];
};

struct Renderer
{
    uint32_t type;
    struct Material fallbackMaterial;
};

// TODO:????
#define INSTANCE_BUFFER_COUNT 3
#define MAX_INSTANCE_BUFFERS 3

#define MATRIX_BUFFER_COUNT 1
#define MAX_MATRIX_BUFFERS  1

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

    void *meshUpdateEventHandle;
    void *meshDestroyEventHandle;
    void *textureUpdateEventHandle;
    void *textureDestroyEventHandle;
    void *materialUpdateEventHandle;
    void *materialDestroyEventHandle;
};



struct Mesh *create_mesh();
struct Renderer *create_renderer(u32 rendererType);
struct Texture *create_texture();

struct Renderer *create_opengl_renderer();

void destroy_mesh(struct Mesh *mesh);
void destroy_opengl_renderer(struct OpenGLRenderer *glrend);
void destroy_renderer(struct Renderer *renderer);
void destroy_texture(struct Texture *tex);

void mesh_copy_vertices(struct Mesh *mesh, struct V3 *vertices, u32 count);
void mesh_copy_texcoords(struct Mesh *mesh, struct V3 *texCoords, u32 count);
void mesh_copy_normals(struct Mesh *mesh, struct V3 *normals, u32 count);
void mesh_copy_colors(struct Mesh *mesh, struct V4 *colors, u32 count);
void mesh_copy_indices(struct Mesh *mesh, u16 *indices, u32 count);
void mesh_queue_for_update(struct Mesh *mesh);
u32 tex_format_get_comps(u16 format);
void texture_copy_data(struct Texture *tex, u32 width, u32 height, u16 format, void *data);
void texture_queue_for_update(struct Texture *tex);

void material_init(struct Material *mat);
void material_destroy(struct Material *mat);
void material_add_shader(struct Material *mat, u32 type, const char *source);
void material_remove_shader(struct Material *mat, u32 index);
void material_queue_for_update(struct Material *mat);
b32 material_find_shader_of_type(struct Material *mat, u32 type, u32 *ret);

void material_init(struct Material *mat);

void opengl_render_view(struct OpenGLRenderer *renderer, struct RenderViewBuffer *rbuf);

