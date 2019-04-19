#ifdef AIKE_ANDROID
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#define AIKE_GLES
#endif

enum GLMeshState
{
    GL_Mesh_State_Init = 0,
    GL_Mesh_State_Dirty,
    GL_Mesh_State_Wait_Sync,
    GL_Mesh_State_Ready
};

enum GLMaterialState
{
    GL_Material_State_Init,
    GL_Material_State_Wait_Sync,
    GL_Material_State_Ready
};

enum GLTextureState
{
    GL_Texture_State_Init = 0,
    GL_Texture_State_Dirty,
    GL_Texture_State_Wait_Sync,
    GL_Texture_State_Ready
};

typedef struct GLMesh
{
    uint32_t id;
    uint32_t state;

    // TODO: move somwhere else
    void *userData;
    void *userData2;
    MReady_A onComplete;

    uint32_t numVertices; // total number of vertices
    uint32_t numIndices; // total number of indices

    GLuint glVbo;
    GLuint glEbo;
    GLuint glVao;
    GLsync fence;

    uint8_t attribTypes[MAX_ATTRIBUTE_BUFFERS];
    uint32_t attribOffsets[MAX_ATTRIBUTE_BUFFERS];

    uint32_t vertexStride;
    uint32_t vertexBufferSize;
    uint32_t indexBufferSize;
    // TODO: uint32_t indexStride;

    uint32_t numMeshSections;
    MeshSection sections[MAX_MESH_SECTIONS];
} GLMesh;

typedef struct GLTexture
{
    uint32_t id;
    uint32_t state;

    // TODO: move somewhere else
    void *userData;
    void *userData2;
    TReady_A onComplete;

    u16 format;
    u16 filter;
    u32 width;
    u32 height;

    GLuint glTex;
    GLuint glPub; // pixel unpack buffer
    GLsync fence;

    uint32_t bufferSize;
} GLTexture;

typedef struct GLShader
{
    GLuint glShader;
    u32 type;
} GLShader;

typedef struct GLMaterial
{
    uint32_t id;
    uint32_t shaderId;
    GLuint glProgram;

    void *userData;
    void *userData2;
    
    uint32_t perInstanceDataSize;
    union InstanceData iData;
} GLMaterial;

struct OpenGLRenderer;
typedef void (*OnSyncAction_t)(struct OpenGLRenderer *renderer, void *data);

typedef struct GLSyncPoint
{
    bool active;
    GLsync sync;
    OnSyncAction_t onSync;
    void *userData;
} GLSyncPoint;

#define GL_RENDERER_MAX_SYNC_POINTS 1000

typedef struct OpenGLRenderer
{
    Renderer renderer;

    // max size for unitform buffer
    u32 uniBufSize;

    // buffer that stores per instance data
    // (except MVP matrix)
    u32 instanceDataBuffers[INSTANCE_BUFFER_COUNT];
    u32 curInstanceBufferIdx;

    // buffer for MVP matrix
    u32 matrixBuffers[MATRIX_BUFFER_COUNT];
    u32 curMatrixBufferIdx;

    u32 immVAO;
    u32 immVertBuf;
    u32 immIndexBuf;
    u32 immVertBufSize;
    u32 immIndexBufSize;

    u32 fboColorTex;
    u32 fbo;
    u32 fboDepthBuffer;
    u32 fboWidth;
    u32 fboHeight;

    // only if the feature is enabled
    bool renderObjectId;
    u32 objectIdPbo;
    u32 objectIdInstanceBuf;
    u32 objectIdFboTex;

    // TODO: remove once you have render targets
    u32 windowWidth;
    u32 windowHeight;

    GLuint builtinPrograms[Shader_Type_Count];
    GLuint uiProgram;

    uint32_t numFreeSyncPoints;
    uint32_t syncPointFreeList[GL_RENDERER_MAX_SYNC_POINTS];
    GLSyncPoint syncPoints[GL_RENDERER_MAX_SYNC_POINTS];

    khash_t(32) *meshMap;
    khash_t(32) *materialMap;
    khash_t(32) *textureMap;

    struct GLMaterial *materialPool;
    struct GLTexture *texturePool;
    struct GLMesh *meshPool;

    GLMesh defaultMesh;
    uint32_t meshId;
    GLTexture defaultTexture;
    uint32_t textureId;
    GLMaterial fallbackMaterial;
    uint32_t materialId;

    struct SwapBuffer *swapbuf;
    struct RenderViewBuffer *curView;
} OpenGLRenderer;

#ifdef _DEBUG
#define CHECK_BUF_ALIGN(x) assert(((x) & BUF_ALIGN_MASK) == 0)
#else
#define CHECK_BUF_ALIGN(x)
#endif
