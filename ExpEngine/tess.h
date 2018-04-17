#define TESS_LOADED_FILE_POOL_SIZE 100
#define TESS_MESH_POOL_SIZE 100
#define TESS_OBJECT_POOL_SIZE 100
#define TESS_LOADING_ASSET_POOL_SIZE 100
#define TESS_DEP_NODE_POOL_SIZE 1000
#define TESS_ASSET_LOOKUP_ENTRY_POOL_SIZE 1000
#define TESS_ASSET_LOOKUP_CACHE_POOL_SIZE 100

#define POOL_FROM_ARENA(pool, arena, size) (pool_init_with_memory((pool), arena_push_size((arena), pool_calc_size((pool), (size))), (size)))

#define TESS_MAX_OBJECTS 10
#define TESS_MAX_ENTITIES 100


enum TessFilePipeline
{
    Tess_File_Pipeline_None,
    Tess_File_Pipeline_TTR,
    Tess_File_Pipeline_Count,
};

typedef struct TStr // interned string
{
    uint32_t len;
    char cstr[];
} TStr;

struct TessFile
{
    AikeIORequest req;
    uint32_t pipeline;
    void *userData;
    char filePath[AIKE_MAX_PATH];
};

struct LoadTTRJob
{
    struct TessFile file;
    const char *fileName;

};

typedef void (*FilePipelineProc)(void *subsystem, struct TessFile *file);

struct TessFileSystem
{
    AikePlatform *platform;
    struct TessFile *loadedFilePool;

    FilePipelineProc pipeline_vtbl[Tess_File_Pipeline_Count];
    void *pipeline_ptrs[Tess_File_Pipeline_Count];
};

// --------- ASSET SYSTEM ----------

enum TessAssetType
{
    Tess_Asset_None,
    Tess_Asset_Unknown,
    Tess_Asset_Mesh,
    Tess_Asset_Texture,
    Tess_Asset_Material,
    Tess_Asset_Sound,
    Tess_Asset_Object
};

enum TessAssetDependencyStatus
{
    Tess_Asset_Dependency_Status_None,
    Tess_Asset_Dependency_Status_Loading,
    Tess_Asset_Dependency_Status_Done,
    Tess_Asset_Dependency_Status_Extern,
    Tess_Asset_Dependency_Status_Fail,
};

enum TessObjectFlags
{
    Tess_Object_Flag_Registered = 1<<0,
    Tess_Object_Flag_Loaded     = 1<<1,
};

struct TessAsset
{
    TStr *assetId;
    uint32_t type;
};

struct TessMeshAsset
{
    struct TessAsset asset;
    uint32_t meshId;
};

struct TessObjectAsset
{
    struct TessAsset asset;
    struct TessMeshAsset *mesh;
    // TODO: material?
};

struct TessMeshDepData
{
    struct TessAssetSystem *as;
    struct TTRMesh *tmesh;
};

struct TessObjectDepData
{
    TStr *meshAssetId;
};

struct TessLoadingAsset
{
    struct TessFile *file;
    uint32_t status;
    TStr *assetId;
    struct TessAsset *asset;
    uint32_t type;
    union
    {
        struct TessMeshDepData meshData;
        struct TessObjectDepData objectData;
    };
};

struct AssetLookupEntry
{
    TStr *packageName;
    TStr *fileName;
    TStr *assetName;
    uint32_t assetType;
};

struct AssetLookupCache
{
    struct AssetLookupEntry **entries;
};

struct TessAssetSystem
{
    struct TessFileSystem *fileSystem;
    struct TessAsset **loadedAssets;
    struct TessMeshAsset *meshPool;
    struct TessObjectAsset *objectPool;
    struct TessLoadingAsset *loadingAssetPool;
    struct TessLoadingAsset **loadingAssets;
    struct AssetLookupEntry *assetLookupEntryPool;
    struct AssetLookupCache *assetLookupCachePool;

    khash_t(str) *packageAssetMap;
    khash_t(64) *loadedAssetMap; // TStr assetId, TessAsset*
    khash_t(64) *loadingAssetMap; // TStr assetId, TessLoadingAsset*

    struct TessStrings *tstrings;
    struct Renderer *renderer;
};

struct TessStrings
{
    struct TessFixedArena stringArena;
    TStr **internedStrings;
};

// --------------- GAME WORLD --------

struct TessObject
{
    uint32_t id;
    uint32_t flags;
    TStr *assetId; // TessObjectAsset
    struct TessObjectAsset *asset;
};

struct TessEntity
{
    uint32_t id;
    uint32_t objectId;
    struct Mat4 objectToWorld;
};

struct TessCamera
{
    struct V3 position;
    struct Quat rotation;

    float aspectRatio;
    float FOV;
    float nearPlane;
    float farPlane;

    struct Mat4 viewToClip;
};

struct TessGameSystem
{
    struct TessObject objectTable[TESS_MAX_OBJECTS];
    struct TessEntity *entityPool;
    struct TessEntity **activeEntities;
    struct TessAssetSystem *assetSystem;    

    struct TessCamera *activeCamera;
    struct TessCamera defaultCamera;

    struct TessRenderSystem *renderSystem;
};

// --------------- INPUT -------------------

struct TessInputSystem
{
    uint16_t keyStates[AIKE_KEY_COUNT];
    uint16_t keyStatesPrev[AIKE_KEY_COUNT];

    AikePlatform *platform;
};

#define mouse_left_down(x) ((x)->keyStates[AIKE_BTN_LEFT] && !(x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse1_right_down(x) ((x)->keyStates[AIKE_BTN_RIGHT] && !(x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left_up(x) ((x)->keyStates[AIKE_BTN_LEFT] && !(x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse1_right_up(x) (!(x)->keyStates[AIKE_BTN_RIGHT] && (x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left(x) ((x)->keyStates[AIKE_BTN_LEFT] != 0)
#define mouse_right(x) ((x)->keyStates[AIKE_BTN_RIGHT] != 0)

// --------------- RENDERING -------------------

struct TessRenderSystem
{
    struct SwapBuffer *viewSwapBuffer;
    struct RenderViewBuilder *viewBuilder;
    struct RenderViewBuffer *gameRenderView;
    struct Renderer *renderer;
    struct Mat4 worldToClip;
};

// --------------- UI ---------------------

struct TessUISystem
{
    struct nk_context nk_ctx;
    struct nk_font_atlas nk_atlas;
    struct nk_font *nk_font;
    struct nk_draw_null_texture nk_null;
    struct nk_buffer nk_cmds;

    // these probably don't belong here!
    int32_t nk_w;
    int32_t nk_h;
    const void *image;

    bool loaded;

    struct TessRenderSystem *renderSystem;
    struct AikePlatform *platform;
};

// --------------- MENU -------------------

struct TessMainMenu
{
    struct nk_context *nk_ctx;
    uint32_t mode;

    char ipStrBuf[256];
    char portStrBuf[7];
    int portStrLen;

    const char *statusStr;

    struct TessUISystem *uiSystem;
    struct TessInputSystem *inputSystem;
    struct TessClient *client;
};

// --------------- EDITOR ------------------

#define TESS_EDITOR_SERVER_MAX_COMMAND_SIZE 65536
#define TESS_EDITOR_SERVER_MAX_CLIENTS 16

struct TessEditorCommandBuf // network command buffer
{
    // those 3 are needed onle because i wanted to share some code.. (editor_flush_command, editor_append_cmd_data)
    void *usrPtr;
    void *usrClientPtr;
    bool isServer;
    uint32_t currentCommandBytes;
    uint32_t currentCommandSize;
    uint8_t currentCommand[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];
};

struct TessEditorEntity
{
    uint32_t id;
    uint32_t entityId;

    struct V3 position;
    struct V3 scale;
    struct V3 eulerRotation;

    uint32_t dirty;
};

struct TessEditor
{
    b32 init;
    b32 connected;

    AikeTCPConnection *tcpCon;

    struct V2 normalizedCursorPos;

    // renderer writes the currently pointed objectid here
    int32_t cursorObjectIdBuf;
    // safe to use version
    int32_t cursorObjectId;

    bool objectSelected;
    int32_t selectedObjectId;

    khash_t(uint32) *entityMap; // entityId -> editorEntityId
    struct TessEditorEntity *edEntityPool;
    struct TessEditorEntity **edEntities;

    struct nk_context *nk_ctx;

    struct TessRenderSystem *renderSystem;
    struct TessUISystem *uiSystem;
    // TODO: this doens't belong here once you have input system
    struct AikePlatform *platform;
    struct TessInputSystem *inputSystem;
    struct TessClient *client;
    struct TessGameSystem *world;

    struct TessEditorCommandBuf cmdBuf;

    struct TessFixedArena arena;
};

// --------------- STATE --------------

enum TessClientMode
{
    Tess_Client_Mode_Menu,
    Tess_Client_Mode_Editor,
    Tess_Client_Mode_CrazyTown,
};

struct TessClient
{
    struct TessFileSystem fileSystem;
    struct TessAssetSystem assetSystem;
    struct TessGameSystem gameSystem;
    struct TessRenderSystem renderSystem;
    struct TessUISystem uiSystem;
    struct TessInputSystem inputSystem;

    struct TessMainMenu mainMenu;
    struct TessEditor editor;

    struct TessStrings strings;

    struct TessFixedArena arena;

    uint32_t mode;

    AikePlatform *platform;
};

// Server

struct TessEditorServerEntity
{
    uint32_t objectId;
    uint16_t version;
    struct V3 position;
    struct V3 eulerRotation;
    struct V3 scale;
};

struct TessEditorServerClient
{
    struct AikeTCPConnection *connection;

    uint16_t entityVersions[TESS_MAX_ENTITIES];
    uint16_t objectTableVersion;

    // TODO: this is more than 65k bytes, should be stored separately
    struct TessEditorCommandBuf cmdBuf;
};

struct TessEditorServer
{
    AikeTCPServer *tcpServer;

    struct TessStrings *tstrings;
    AikePlatform *platform;

    uint16_t objectTableVersion;
    TStr *objectTable[TESS_MAX_OBJECTS];
    struct TessEditorServerEntity entities[TESS_MAX_ENTITIES];
    struct TessEditorServerEntity **activeEntities;
    struct TessEditorServerClient **activeClients;
    struct TessEditorServerClient *clientPool;
};

struct TessServer
{
    struct TessFileSystem fileSystem;
    struct TessAssetSystem assetSystem;
    struct TessGameSystem gameSystem;

    struct TessEditorServer editorServer;

    struct TessStrings strings;

    struct TessFixedArena arena;

    AikePlatform *platform;
};

void tess_process_ttr_file(struct TessAssetSystem *as, struct TessFile *tfile);
TStr* tess_intern_string_s(struct TessStrings *tstrings, const char *string, uint32_t maxlen);
TStr* tess_intern_string(struct TessStrings *tstrings, const char *string);
bool get_asset_file(struct TessAssetSystem *as, TStr *assetId, TStr **result);
void tess_load_asset_if_not_loaded(struct TessAssetSystem *as, TStr *assetId);
TStr *tess_get_asset_id(struct TessAssetSystem *as, TStr *package, TStr *asset);
TStr *tess_get_asset_name_from_id(struct TessAssetSystem *as, TStr *assetId);
TStr *tess_get_asset_package_from_id(struct TessAssetSystem *as, TStr *assetId);

void tess_input_init(struct TessInputSystem *input);
void tess_input_begin(struct TessInputSystem *input);
void tess_input_end(struct TessInputSystem *input);

void tess_ui_init(struct TessUISystem *ui);
void tess_ui_destroy(struct TessUISystem *ui);
void tess_ui_begin(struct TessUISystem *ui);
void tess_ui_end(struct TessUISystem *ui);

void editor_update(struct TessEditor *editor);
bool editor_connect(struct TessEditor *editor, const char *ipStr, uint16_t port);
void editor_init(struct TessEditor *editor);
void editor_destroy(struct TessEditor *editor);
void tess_render_entities(struct TessGameSystem *gs);

void tess_main_menu_init(struct TessMainMenu *menu);
void tess_main_menu_update(struct TessMainMenu *menu);
