#pragma once

#ifdef __linux__
#define AIKE_LINUX
#else
#error Unknown platform
#endif

#include "aike_math.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define aike_alloc(size) debug_alloc(size, __COUNTER__, __FILE__, __LINE__)
#define aike_free(ptr) debug_free(ptr)

#define aike_temp_alloc(size) temp_alloc(size)
#define aike_temp_pop() temp_pop()

#define COND_PRINT(x, ...) if(x) printf(__VA_ARGS__);
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define aike_log_warning(string, ...) (printf(string, __VA_ARGS__))
#define aike_fatal(msg, ...) (printf(msg"\n", ##__VA_ARGS__), exit(-1))

#ifdef AIKE_DEBUG
#define DEBUGINT(x) int32_t x;
#define TRACK_RESOURCE(desc) debug_track_resource(__COUNTER__, desc, __FILE__, __LINE__)
#define TRACK_RESOURCE_SET(var, desc) var = TRACK_RESOURCE(desc)
#define FREE_RESOURCE(uuid) debug_untrack_resource(uuid)
#else
#define DEBUGINT(x)
#define TRACK_RESOURCE(desc)
#define TRACK_RESOURCE_SET(var, desc)
#define FREE_RESOURCE(uuid)
#endif

//typedef void (*Action) (void);
typedef void (*Action) (void *data);

/// PLATFORM //
//
//

AikePlatform *g_platform;

///////// UI STUFF //////////////////////////

#pragma pack(push, 1)
struct QuadVertex
{
    Vec3 position;
    Vec4 color;
    Vec3 uv;
    uint32_t mode;
};
#pragma pack(pop)


struct QuadBuffer
{
    GLuint vao;
    GLuint vbo;
    QuadVertex vertices[4096];
    uint32_t vboSize;
    uint32_t vboDataPerVertex;
    uint32_t numQuads;
    DEBUGINT(vaouuid);
    DEBUGINT(vbouuid);

    uint32_t layer;
};

#pragma pack(push, 1)
struct SlowQuad
{
    Vec2 min;
    Vec2 max;
    GLuint texture;
    bool stretch;
    float imgWidth;
    float imgHeight;
};
#pragma pack(pop)

struct SlowQuadBuffer
{
    GLuint vao;
    GLuint vbo;
    uint32_t numQuads;
    uint32_t bufSize;
    SlowQuad quads[64];
    DEBUGINT(vbouuid);
    DEBUGINT(vaouuid);
    uint32_t layer;
};

struct CachedFont
{
    char name[256];
    uint32_t size;

    uint32_t atlasLayer;
    stbtt_packedchar charData[128];
};

enum TextAlignment
{
    TextAlignment_Left,
    TextAlignment_Center,
    TextAlignment_Right
};

struct FontManager
{
    CachedFont* cachedFonts[1024];
    uint32_t numCachedFonts;

    //QuadBuffer qBuf;
};

//extern FontManager* g_fontManager;


struct Renderer
{
    FontManager fontManager;
    Mat4 projection;

    uint32_t curMatrix;
    Mat3 immediateMatrixStack[6];
    Mat3 currentImmMatrix;

    // resources for rendering solid shapes
    //QuadBuffer solidBuffer;
    QuadBuffer layerBuffers[32];
    SlowQuadBuffer slowLayerBuffers[32];

    int32_t currentLayer;
    // TODO: have rendertarget instead of window
    AikeWindow *curWindow;

    GLuint mainProgram;
    GLuint slowQuadProgram;

    GLuint textureAtlasArray;
    uint32_t textureAtlasNumLayers;
    uint32_t textureAtlasUsedLayers;
    DEBUGINT(atlasdebug);
};

struct ContextMenu
{
    AikeWindow window;
    Vec2 position;
    char names[16][32];
    Action actions[16];
    void *actionData[16]; 
    uint32_t numOptions;
    bool rootMenu;
    uint64_t createdFrame;
};

extern Renderer* g_renderer;

enum LayoutTreeNodeType
{
    NT_None, // thanks X11 for defining None macro...
    Horizontal_Split,
    Vertical_Split,
    Leaf
};

struct LayoutTreeNode
{
    LayoutTreeNodeType type;
    Rect rect;
    float firstWeight; // for internal nodes
    bool uiS;
    struct LayoutTreeNode* children[2];
    struct ContextArea *context;
    struct AikeViewState *viewState;
};

#define LAYOUT_TREE_MAX_NODES_LOG2 6



struct LayoutTree
{
    LayoutTreeNode *rootNode;
    LayoutTreeNode nodePool[1<<LAYOUT_TREE_MAX_NODES_LOG2];
    uint8_t allocMap[1<<(LAYOUT_TREE_MAX_NODES_LOG2-3)]; // 0 - allocated, 1 - free
};

#define CONTEXT_AREA_MAX_OPTIONS 16

struct ContextArea
{
    Rect screenRect;
    char names[CONTEXT_AREA_MAX_OPTIONS][64];
    Action actions[CONTEXT_AREA_MAX_OPTIONS];
    void *userData[CONTEXT_AREA_MAX_OPTIONS];
    uint32_t numOptions;
};

static uint32_t zoomTable[]
{
       60,
       70,
       80,
      100,
      200,
      300,
      400,
      500,
      650,
      800,
     1000,
     1500,
     2000,
     3000,
     4500,
     6500,
     9500,
    14500,
    20000,
    40000
};

const uint32_t startZoom = 10;

static inline uint32_t getNextZoom(uint32_t current)
{
    for(uint32_t i = 0; i < ARRAY_COUNT(zoomTable); i++)
    {
        if(zoomTable[i] > current)
            return zoomTable[i];
    }
    return zoomTable[ARRAY_COUNT(zoomTable) - 1];
}

static inline uint32_t getPrevZoom(uint32_t current)
{
    for(int32_t i = ARRAY_COUNT(zoomTable)-1; i >= 0; i--)
    {
        if(zoomTable[i] < current)
            return zoomTable[i];
    }
    return zoomTable[0];
}

struct ImageView
{
    Vec2 offset;
    int32_t scale1000;
    Vec2 imageSpaceCenter;

    // grabbing state
    bool grabbing;
    Vec2 grabPoint;
    Vec2 offsetOnGrab;
};

struct AikeViewState
{
    enum AikeViewType
    {
        Unknown,
        ImageView,
        Count
    };

    uint32_t type;
    union 
    {
        struct ImageView imgView;
    };
};

#define AIKE_MAX_VIEWS 32

struct UserInterface
{
    LayoutTree layoutTree;
    uint32_t cursor;

    StructArray contextList;
    StructPool contextPool;

    int32_t numFreeViews;
    uint32_t viewFreeList[AIKE_MAX_VIEWS];
    AikeViewState viewPool[AIKE_MAX_VIEWS];
};


#define AIKE_IMG_CHUNK_SIZE 128 // 4 component 64K (aligns with common virtual memory page size)
#define AIKE_MAX_IMG_SIZE 1<<16
#define AIKE_MAX_AXIS_CHUNKS (AIKE_MAX_IMG_SIZE / AIKE_IMG_CHUNK_SIZE)

struct ImageTile
{
    uint32_t glLayer;
    int32_t tileX;
    int32_t tileY;
    uint8_t data[AIKE_IMG_CHUNK_SIZE][AIKE_IMG_CHUNK_SIZE][4];
};

#define AIKE_TILE_POOL_INIT_SIZE 256

struct AikeTilePool
{
    // TODO: definitely need dynamic pool here
    uint32_t numFree;
    uint32_t freeList[AIKE_TILE_POOL_INIT_SIZE];
    GLuint glTextureArray;
};

struct AikeImage
{
    uint32_t width;
    uint32_t height;
    uint32_t numComps;

    // REVIEW: should we also create a data structure for iteration or
    // is this fast enough?
    khash_t(ptr_t) *tile_hashmap; // contains void* of ImageTile*
};

struct OpenImages
{
    AikeImage images[10];
    bool imagePresent[10];
};

struct Aike
{
    OpenImages images;
    AikeTilePool tilePool;
};


extern UserInterface *g_ui;

////////////////////////////////////////////
//
static void layout_tree_tests(LayoutTree *ltree);
static void layout_tree_resize(LayoutTree *ltree, uint32_t width, uint32_t height);
static void layout_tree_draw(LayoutTreeNode *node);

static void aike_fatal_error(const char* error);

static CachedFont* font_manager_load_font(FontManager* fmgr, const char *path, uint32_t size);

static void renderer_init(Renderer *renderer);
static void renderer_free_resources(Renderer *renderer);
void quad_buffer_render(QuadBuffer *qbuf, GLint program, GLuint texture, GLuint texture2);
static void renderer_draw_quad(Renderer *renderer, Rect rect, Vec4 color);

static void user_interface_init(UserInterface *ui, uint32_t screenWidth, uint32_t screenHeight);
static void user_interface_free_resources(UserInterface *ui);
static void user_interface_pre_frame(UserInterface *ui);
static void user_interface_draw(UserInterface *ui);
static void layout_tree_init(LayoutTree* tree, uint32_t rootWidth, uint32_t rootHeight);

static void context_menu_create(ContextMenu *menu, Vec2 position, bool root);
static void context_menu_free_resources(ContextMenu *menu);
static void context_menu_add_option(ContextMenu *menu, const char *text, Action onClick, void *onClickData);
static void context_menu_render(struct Renderer *renderer, ContextMenu *menu);


static void cached_font_unload(CachedFont *font);
static void text_width(CachedFont *font, Vec2 position, const char *text);
static void renderer_render_text(Renderer *renderer, CachedFont* font, Rect rect, const char *text, TextAlignment alignment);

// Memory

void* temp_alloc(uint32_t size);
void temp_pop();

struct AutoAlloc
{
    AutoAlloc(uint32_t size)
    {
        ptr = aike_temp_alloc(size);
    }

    ~AutoAlloc()
    {
        aike_temp_pop();
    }

    void *ptr;
};

struct MemoryManager
{
    uint32_t num_current_allocations;
    struct AllocationEntry *entries;
    struct AEFreeListEntry *freeListHead;
    struct AllocatedListEntry *allocationListHead;
    uint32_t capacity;
    uint32_t maxIndex;

    uint8_t *stackStart;
    uint8_t *stackPointer;
    uint32_t stackSize;
};


extern MemoryManager *g_memoryManager;
extern Aike *g_aike;

void* debug_alloc(size_t size, uint32_t uuid, const char *fileName, int lineNumber);
void debug_free(void *ptr);
struct MemoryManager* memory_manager_initialize();
void memory_manager_free_resources(MemoryManager *memMgr);
void memory_manager_print_entries(MemoryManager *memMgr, bool breakIfAnyEntries);
uint32_t debug_track_resource(uint32_t uuid, const char *description, const char *fileName, int lineNumber);
void debug_untrack_resource(uint32_t uuid);

size_t strlcpy(char *dst, const char *src, size_t siz);


#define MOUSE1() ((g_input->inputStates & AIKE_MOUSEB1_BIT) != 0)
#define MOUSE2() ((g_input->inputStates & AIKE_MOUSEB2_BIT) != 0)
#define MOUSE1_DOWN() ((g_input->inputStatesPrev & AIKE_MOUSEB1_BIT) == 0 && (g_input->inputStates & AIKE_MOUSEB1_BIT) != 0)
#define MOUSE2_DOWN() ((g_input->inputStatesPrev & AIKE_MOUSEB2_BIT) == 0 && (g_input->inputStates & AIKE_MOUSEB2_BIT) != 0)
#define MOUSE1_UP() ((g_input->inputStatesPrev & AIKE_MOUSEB1_BIT) != 0 && (g_input->inputStates & AIKE_MOUSEB1_BIT) == 0)
#define MOUSE2_UP() ((g_input->inputStatesPrev & AIKE_MOUSEB2_BIT) != 0 && (g_input->inputStates & AIKE_MOUSEB2_BIT) == 0)

#ifndef AIKE_DEBUG
#define KEY(x) ((g_input->keyStates[(x)] != 0))
#else
#define KEY(x) ({bool keyret_ = g_input->keyStates[(x)]; assert(x < AIKE_KEY_COUNT); keyret_;})
#endif

#define BOUNDKEY(x) ((g_input->keyBindStates[(x)] != 0))
#define BOUNDKEY_DOWN(x) ((g_input->keyBindStatesPrev[(x)] == 0) && (g_input->keyBindStates[(x)] != 0))
#define BOUNDKEY_UP(x) ((g_input->keyBindStatesPrev[(x)] != 0) && (g_input->keyBindStates[(x)] == 0))

#define AIKE_MAX_KEYBINDS 2

// Input
struct AikeInput
{
    enum KeyBinds
    {
        KB_None,
        KB_Grab,
        KB_ZoomIn,
        KB_ZoomOut,
        KB_Count
    };

    Vec2 mousePos;
    Vec2 mouseScreenPos;
    double mouseVerticalAxis;
    double mouseHorizontalAxis;

    uint16_t keyBinds[KB_Count][AIKE_MAX_KEYBINDS];
    uint16_t keyBindStates[KB_Count];
    uint16_t keyBindStatesPrev[KB_Count];

    // key states from platform
    uint16_t keyStates[AIKE_KEY_COUNT];
    uint16_t keyStatesPrev[AIKE_KEY_COUNT];

    uint32_t inputStatesPrev;
    uint32_t inputStates;
};
extern AikeInput *g_input;

void input_init(AikeInput *input);
void input_update(AikeInput *input, AikePlatform *platform);

// Events

enum Event
{
    Event_Mouse1Down,
    Event_Mouse2Down,
    Event_Mouse1Up,
    Event_Mouse2Up,
    Event_Count
};

typedef bool (*EventAction) (void *data);

struct ListenEvent
{
    ListenEvent *next;
    void *userData;
    EventAction onTrigger;
    int priority;
};

struct EventManager
{
    ListenEvent listeners[Event_Count];
};

static void event_manager_init(EventManager *emgr);
static void event_manager_free_resources(EventManager *emgr);
static void event_manager_event(EventManager *emgr, Event event);
static void event_manager_add_listener(EventManager *emgr, Event event, EventAction onTrigger, void *userData, int priority);
static void event_manager_remove_listener(EventManager *emgr, Event event, EventAction onTrigger, void *userData);

extern EventManager *g_emgr;

// preprocessor defintions
// AIKE_DEBUG - debug build
// AIKE_SLOW - speed not important, slow sanity checks may be ran


////////////////////////// OPENGL ////////////////////////////////

/*#define GL_FUNC_VAR(fname) extern PFNGL ## fname fname;

GL_FUNC_VAR(glGenVertexArrays);
GL_FUNC_VAR(glDeleteVertexArrays);
GL_FUNC_VAR(glGenBuffers);
GL_FUNC_VAR(glDeleteBuffers);
GL_FUNC_VAR(glBindBuffer);
GL_FUNC_VAR(glBufferData);
GL_FUNC_VAR(glBufferSubData);
GL_FUNC_VAR(glEnableVertexAttribArray);
GL_FUNC_VAR(glCreateShader);
GL_FUNC_VAR(glDeleteShader);
GL_FUNC_VAR(glShaderSource);
GL_FUNC_VAR(glCompileShader);
GL_FUNC_VAR(glGetShaderiv);
GL_FUNC_VAR(glGetShaderInfoLog);
GL_FUNC_VAR(glAttachShader);
GL_FUNC_VAR(glGetProgramiv);
GL_FUNC_VAR(glGetProgramInfoLog);
GL_FUNC_VAR(glDetachShader);
GL_FUNC_VAR(glCreateProgram);
GL_FUNC_VAR(glDeleteProgram);
GL_FUNC_VAR(glLinkProgram);
GL_FUNC_VAR(glVertexAttribPointer);
GL_FUNC_VAR(glVertexAttribIPointer);
GL_FUNC_VAR(glUseProgram);
GL_FUNC_VAR(glBindVertexArray);
GL_FUNC_VAR(glUniform1i);
GL_FUNC_VAR(glGetUniformLocation);
GL_FUNC_VAR(glUniformMatrix3fv);
GL_FUNC_VAR(glUniformMatrix4fv);*/

/*GL_FUNC_VAR(glViewport);
GL_FUNC_VAR(glClearColor);
GL_FUNC_VAR(glClear);
GL_FUNC_VAR(glGetError);
GL_FUNC_VAR(glGenTextures);
GL_FUNC_VAR(glBindTexture);
GL_FUNC_VAR(glTexImage3D);
GL_FUNC_VAR(glTexParameteri);
GL_FUNC_VAR(glDeleteTextures);
GL_FUNC_VAR(glEnable);
GL_FUNC_VAR(glBlendFunc);
GL_FUNC_VAR(glActiveTexture);
GL_FUNC_VAR(glDrawArrays);
GL_FUNC_VAR(glTexSubImage3D);*/

// TODO: is there any way we could get rid of this?? (using the above declarations fails to link)
#include <GL/gl.h>


static inline uint32_t aike_tile_hash(int32_t x, int32_t y)
{
    uint32_t ret = (((int16_t)y)<<16) | (int16_t)x;
    return ret;
}
void aike_init_tile_pool(AikeTilePool *pool);
AikeImage *aike_alloc_image_slot(Aike *aike);
void aike_free_image_slot(Aike *aike, AikeImage *slot);
AikeImage *aike_get_first_image(Aike *aike);
void aike_destroy_tile_pool(AikeTilePool *pool);

AikeImage *aike_open_image(Aike *aike, uint32_t width, uint32_t height, uint32_t numcomps, void *memory, bool seamless);
void aike_close_image(Aike *aike, AikeImage *img);

void aike_return_tile(Aike *aike, ImageTile* tile);
void aike_update_tile(Aike *aike, ImageTile* tile);

GLuint opengl_load_texture(uint32_t width, uint32_t height, uint32_t numcomps, void *data);
GLuint opengl_load_seamless_texture(uint32_t width, uint32_t height, uint32_t numcomps, void *data);
GLuint opengl_create_texture_array(uint32_t width, uint32_t height, uint32_t layers);
void opengl_destroy_texture_array(GLuint texarr);
void opengl_copy_texture_array_layer(GLuint tex, uint32_t width, uint32_t height, uint32_t layer, void *data);
