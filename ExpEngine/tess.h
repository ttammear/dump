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

#define internal static
#define global static


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

typedef struct TessFile
{
    AikeIORequest req;
    uint32_t pipeline;
    void *userData;
    char filePath[AIKE_MAX_PATH];
} TessFile;

typedef struct LoadTTRJob
{
    struct TessFile file;
    const char *fileName;

} LoadTTRJob;

typedef void (*FilePipelineProc)(void *subsystem, struct TessFile *file);

typedef struct TessFileSystem
{
    AikePlatform *platform;
    struct TessFile *loadedFilePool;

    FilePipelineProc pipeline_vtbl[Tess_File_Pipeline_Count];
    void *pipeline_ptrs[Tess_File_Pipeline_Count];
} TessFileSystem;

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

enum TessAssetStatus
{
    Tess_Asset_Status_None,
    Tess_Asset_Status_Loading,
    Tess_Asset_Status_Loaded,
    Tess_Asset_Status_Fail
};

enum TessObjectFlags
{
    Tess_Object_Flag_Registered = 1<<0,
    Tess_Object_Flag_Loaded     = 1<<1,
};

typedef struct TessAsset
{
    TStr *assetId;
    uint32_t type;
} TessAsset;

typedef struct TessMeshAsset
{
    TessAsset asset;
    uint32_t meshId;
} TessMeshAsset;

typedef struct TessObjectAsset
{
    TessAsset asset;
    TessMeshAsset *mesh;
    // TODO: material?
} TessObjectAsset;

typedef struct TessMeshDepData
{
    struct TessAssetSystem *as;
    struct TTRMesh *tmesh;
} TessMeshDepData;

typedef struct TessObjectDepData
{
    TStr *meshAssetId;
} TessObjectDepData;

typedef struct TessLoadingAsset
{
    struct TessFile *file;
    uint32_t status;
    TStr *assetId;
    struct TessAsset *asset;
    uint32_t type;
    union
    {
        TessMeshDepData meshData;
        TessObjectDepData objectData;
    } ;
}TessLoadingAsset;

typedef struct AssetLookupEntry
{
    TStr *packageName;
    TStr *fileName;
    TStr *assetName;
    uint32_t assetType;
} AssetLookupEntry;

typedef struct AssetLookupCache
{
    struct AssetLookupEntry **entries;
} AssetLookupCache;

typedef struct TessAssetSystem
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
    khash_t(uint32) *assetStatusMap; // TStr assetId, uint32

    struct TessStrings *tstrings;
    struct Renderer *renderer;
} TessAssetSystem;

typedef struct TessStrings
{
    TessFixedArena stringArena;
    TStr **internedStrings;
} TessStrings;

// --------------- GAME WORLD --------

typedef struct TessObject
{
    uint32_t id;
    uint32_t flags;
    TStr *assetId; // TessObjectAsset
    struct TessObjectAsset *asset;
} TessObject;

typedef struct TessEntity
{
    uint32_t id;
    uint32_t objectId;
    struct Mat4 objectToWorld;
} TessEntity;

typedef struct TessCamera
{
    V3 position;
    Quat rotation;

    float aspectRatio;
    float FOV;
    float nearPlane;
    float farPlane;

    Mat4 viewToClip;
} TessCamera;

typedef struct TessGameSystem
{
    struct TessObject objectTable[TESS_MAX_OBJECTS];
    struct TessEntity *entityPool;
    struct TessEntity **activeEntities;
    struct TessAssetSystem *assetSystem;    

    struct TessCamera *activeCamera;
    struct TessCamera defaultCamera;

    struct TessRenderSystem *renderSystem;
} TessGameSystem;

// --------------- INPUT -------------------

typedef struct TessInputSystem
{
    uint16_t keyStates[AIKE_KEY_COUNT];
    uint16_t keyStatesPrev[AIKE_KEY_COUNT];

    AikePlatform *platform;
} TessInputSystem;

#define mouse_left_down(x) ((x)->keyStates[AIKE_BTN_LEFT] && !(x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse1_right_down(x) ((x)->keyStates[AIKE_BTN_RIGHT] && !(x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left_up(x) ((x)->keyStates[AIKE_BTN_LEFT] && !(x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse1_right_up(x) (!(x)->keyStates[AIKE_BTN_RIGHT] && (x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left(x) ((x)->keyStates[AIKE_BTN_LEFT] != 0)
#define mouse_right(x) ((x)->keyStates[AIKE_BTN_RIGHT] != 0)

// --------------- RENDERING -------------------

typedef struct TessRenderSystem
{
    struct SwapBuffer *viewSwapBuffer;
    struct RenderViewBuilder *viewBuilder;
    struct RenderViewBuffer *gameRenderView;
    struct Renderer *renderer;
    Mat4 worldToClip;
} TessRenderSystem;

// --------------- UI ---------------------

typedef struct TessUISystem
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
} TessUISystem;

// --------------- MENU -------------------

typedef struct TessMainMenu
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
} TessMainMenu;

// --------------- EDITOR ------------------

#define TESS_EDITOR_SERVER_MAX_COMMAND_SIZE 65536
#define TESS_EDITOR_SERVER_MAX_CLIENTS 16

typedef struct TessEditorCommandBuf // network command buffer
{
    // those 3 are needed onle because i wanted to share some code.. (editor_flush_command, editor_append_cmd_data)
    void *usrPtr;
    void *usrClientPtr;
    bool isServer;
    uint32_t currentCommandBytes;
    uint32_t currentCommandSize;
    uint8_t currentCommand[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];
} TessEditorCommandBuf;

typedef struct TessEditorEntity
{
    uint32_t id;
    uint32_t serverId;
    uint32_t entityId;

    V3 position;
    V3 scale;
    V3 eulerRotation;

    uint32_t localDirty;
    uint32_t remoteDirty;
} TessEditorEntity;

typedef struct TessEditor
{
    b32 init;
    b32 connected;

    AikeTCPConnection *tcpCon;

    V2 normalizedCursorPos;

    // renderer writes the currently pointed objectid here
    int32_t cursorObjectIdBuf;
    // safe to use version
    int32_t cursorObjectId;

    bool objectSelected;
    int32_t selectedObjectId;

    khash_t(uint32) *entityMap; // entityId -> editorEntityId
    khash_t(uint32) *serverEntityMap; // serverId -> editorEntityId
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

    TessEditorCommandBuf cmdBuf;

    TessFixedArena arena;
} TessEditor;

// --------------- STATE --------------

enum TessClientMode
{
    Tess_Client_Mode_Menu,
    Tess_Client_Mode_Editor,
    Tess_Client_Mode_CrazyTown,
};

typedef struct TessClient
{
    TessFileSystem fileSystem;
    TessAssetSystem assetSystem;
    TessGameSystem gameSystem;
    TessRenderSystem renderSystem;
    TessUISystem uiSystem;
    TessInputSystem inputSystem;

    TessMainMenu mainMenu;
    TessEditor editor;

    TessStrings strings;

    TessFixedArena arena;

    uint32_t mode;

    AikePlatform *platform;
} TessClient;

// Server

typedef struct TessEditorServerEntity
{
    uint32_t id;
    uint32_t objectId;
    uint16_t version;
    V3 position;
    V3 eulerRotation;
    V3 scale;
} TessEditorServerEntity;

typedef struct TessEditorServerClient
{
    AikeTCPConnection *connection;

    uint8_t entityFlags[TESS_MAX_ENTITIES];
    uint16_t entityVersions[TESS_MAX_ENTITIES];
    uint16_t objectTableVersion;

    // TODO: this is more than 65k bytes, should be stored separately
    TessEditorCommandBuf cmdBuf;
} TessEditorServerClient;

typedef struct TessEditorServer
{
    AikeTCPServer *tcpServer;

    struct TessStrings *tstrings;
    AikePlatform *platform;

    uint16_t objectTableVersion;
    TStr *objectTable[TESS_MAX_OBJECTS];
    struct TessEditorServerEntity **activeEntities;
    struct TessEditorServerClient **activeClients;
    struct TessEditorServerClient *clientPool;
    struct TessEditorServerEntity *entityPool;
} TessEditorServer;

typedef struct TessServer
{
    TessFileSystem fileSystem;
    TessAssetSystem assetSystem;
    TessGameSystem gameSystem;

    TessEditorServer editorServer;

    TessStrings strings;

    TessFixedArena arena;

    AikePlatform *platform;
} TessServer;

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

extern struct TessVtable *g_tessVtbl;
