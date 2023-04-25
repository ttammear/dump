#pragma once

#include <stdint.h>
#include "libs/klib/khash.h"
#include "ttmath.h"
#include "resourceformat.c"
//#include <coro.h>
#include <enet/enet.h>

#define TESS_LOADED_FILE_POOL_INITIAL_SIZE 100
#define TESS_MESH_POOL_INITIAL_SIZE 100
#define TESS_TEXTURE_POOL_INITIAL_SIZE 100
#define TESS_OBJECT_POOL_INITIAL_SIZE 500
#define TESS_LOADING_ASSET_POOL_INITIAL_SIZE 1000
#define TESS_ASSET_LOOKUP_ENTRY_POOL_INITIAL_SIZE 1000
#define TESS_ASSET_LOOKUP_CACHE_POOL_INITIAL_SIZE 100

#define POOL_FROM_ARENA(pool, arena, size) (pool_init_with_memory((pool), arena_push_size((arena), pool_calc_size((pool), (size)), 8), (size)), pool_set_allocator((pool), pool_arena_allocator), pool_set_user_data((pool), arena))

#define TESS_MAX_OBJECTS 5000 // TODO: use hash table to lower this
#define TESS_MAX_ENTITIES 1000

#define TESS_SERVER_MAX_PEERS 64
#define TESS_SERVER_MAX_DYN_ENTITIES 128

#define TESS_CLIENT_MAX_DYN_ENTITIES 128

#define MAX_MESH_SECTIONS 48

#define internal static
#define global static

void *pool_arena_allocator(void *usrData, size_t size);

enum TessFilePipeline
{
    Tess_File_Pipeline_None,
    Tess_File_Pipeline_Task,
    Tess_File_Pipeline_Count,
};

typedef struct TStr // interned string
{
    uint32_t len;
    char cstr[0];
} TStr;

typedef struct TessFile
{
    AikeIORequest req;
    uint32_t pipeline;
    void *userData;
    char filePath[AIKE_MAX_PATH];
} TessFile;

typedef struct TessTTRContext {
    struct TessAssetSystem *as;
    TTRHeader *header;
    TTRDescTbl *tbl;
    TTRImportTbl *imTbl;
    TStr *package;
} TessTTRContext;

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

// --------- SCHEDULER --------

extern struct TessScheduler *g_scheduler;

typedef void (*TaskFunc)(void*);

typedef enum SchedulerState {
    SCHEDULER_STATE_MAIN,
    SCHEDULER_STATE_EDITOR,
    SCHEDULER_STATE_GAME,
    SCHEDULER_STATE_TASK,
    SCHEDULER_STATE_EDITOR_SERVER,
} SchedulerState;

typedef enum SchedulerEvent {
    SCHEDULER_EVENT_NONE,
    SCHEDULER_EVENT_RENDER_MESSAGE,
    SCHEDULER_EVENT_FILE_LOADED,
} SchedulerEvent;

typedef enum SchedulerTaskFlag {
    Scheduler_Task_Flag_None = 0,
    Scheduler_Task_Flag_Cancelled = 1<<0,
    Scheduler_Task_Flag_Started = 1<<1,
} SchedulerTaskFlag;

typedef struct SchedulerTask {
    uint32_t flags;
    TaskFunc func;
    void *data;
    struct SchedulerTask *next;
    const char* name;
} SchedulerTask;

typedef struct YieldedTask {
    struct TaskContext *ctx;
    struct YieldedTask *next;
} YieldedTask;

typedef struct AsyncTask {
   struct TaskContext *ctx;
   atomic_bool done;
   const char *name;
   union {
       struct RenderMessage *renderMsg;
       struct TessFile *file;
   }; 
} AsyncTask;

typedef struct TaskContext {
    coro_context ctx;
    const char *name;
} TaskContext;

typedef struct TessScheduler {
    uint32_t mode;
    SchedulerState state;

    TaskContext *curTaskCtx;
    khash_t(64) *waitingTaskSet; // hashmap used as a set

    SchedulerTask *taskQueueHead;
    SchedulerTask *taskQueueTail;

    YieldedTask *yieldedTasksHead;
    YieldedTask *yieldedTasksTail;

    SchedulerTask *freeTasks;
    TaskContext *ctxPool;
    uint8_t *taskMemory;
    uint32_t taskMemorySize;

    struct TessClient *client;
    
    TessFixedArena arena;

    // three contexts: editor, game, menu(mainCtx)
    coro_context mainCtx;
    coro_context editorCtx;
    coro_context gameCtx;

    // TODO: editor server and game server should be run on a separate
    // process (from client) or at the very least separate thread
    // them sharing data with client is not that important
    coro_context editorServerCtx;

    void *mainStack;
    size_t mainStackSize;
    void *editorStack;
    size_t editorStackSize;
    void *gameStack;
    size_t gameStackSize;
    void *editorServerStack;
    size_t editorServerStackSize;
} TessScheduler;

// --------- ASSET SYSTEM ----------

enum TessAssetType
{
    Tess_Asset_None,
    Tess_Asset_Unknown,
    Tess_Asset_Mesh,
    Tess_Asset_Texture,
    Tess_Asset_Material,
    Tess_Asset_Sound,
    Tess_Asset_Object,
    Tess_Asset_Map,
    Tess_Asset_Collider,
};


typedef enum TessAssetStatus
{
    Tess_Asset_Status_None, // uninitialized state
    Tess_Asset_Status_InQueue, // the task to load the asset has been queued, but not started yet
    Tess_Asset_Status_Loading, // loading asset has started, but not yet finished
    Tess_Asset_Status_Loaded, // asset is ready
    Tess_Asset_Status_Fail, // failure, asset can not be loaded
    Tess_Asset_Status_Pending_Destroy,


    Tess_Asset_Status_Count, // must be last!
} TessAssetStatus;

typedef enum TessAssetAction {
    A_Noop, // do nothing (wait)
    A_Enqueue_Load, // enqueue asset to the load queue
    A_Enqueue_Destroy, // enqueue asset to the destroy queue
    A_Invalid_Target, // targeting intermediate statuses is not allowed (loading, inqueue etc)
    A_Failure, // put the asset in failur state (as of now can not recover from that)
    A_Cancel_Load, // cancel load task
    A_Cancel_Destroy, // cancel destroy task
} TessAssetAction;


enum TessObjectFlags
{
    Tess_Object_Flag_Loaded     = 1<<1,
};

// TODO: should get rid of this (uses malloc)
struct UnloadAssetData {
    struct TessAssetSystem *as;
    TStr *assetId;
    SchedulerTask *task;
};


typedef struct TessAsset {
    TStr *assetId;
    uint32_t type;
    struct UnloadAssetData *ul;
} TessAsset;

// if someone loads an asset, they get this to let asset system
// know once they're done with the asset (so it can be freed)
typedef struct AssetReference {
    TStr *assetId;
} AssetReference;

typedef struct TessMaterial {
    uint32_t materialId;
    AssetReference *texRef;
} TessMaterial;

typedef struct TessMeshAsset {
    // NOTE: asset is first for "inheritance"
    // (we can cast pointer of this structure to an TessAsset pointer)
    TessAsset asset; 
    uint32_t meshId;
    uint32_t numSections;
    TessMaterial materials[MAX_MESH_SECTIONS];
} TessMeshAsset;

typedef struct TessTextureAsset {
    TessAsset asset;
    uint32_t textureId;
} TessTextureAsset;

typedef struct TessColliderAsset {
    TessAsset asset;
    uint32_t shapeCount;
    CPxShape *shapes;
} TessColliderAsset;

typedef struct TessObjectAsset {
    TessAsset asset;
    TessMeshAsset *mesh;
    TessColliderAsset *collider;
    AssetReference *meshRef;
    AssetReference *colliderRef;
} TessObjectAsset;

typedef struct TessMapObject {
    uint32_t objectid;
    TStr *assetId;
} TessMapObject;

typedef struct TessMapEntity {
    V3 position;
    Quat rotation;
    V3 scale;
    uint32_t objectId;
} TessMapEntity;

typedef struct TessMapData {
} TessMapData;

typedef struct TessMapAsset
{
    TessAsset asset;
    uint32_t mapObjectCount;
    uint32_t mapEntityCount;
    TessMapObject *objects;
    TessMapEntity *entities;
    TessMapData *data;
} TessMapAsset;

typedef struct TessLoadingAsset
{
    struct TessAssetSystem *as;
    TStr *fileName;
    struct TessFile *file;
    struct SchedulerTask *task;
    TStr *assetId;
    uint32_t type;
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

typedef void (*OnAssetLoaded_t) (void *, struct TessAssetSystem *, struct TessAsset *);

typedef struct TessAssetSystemMetrics {
    int numLoadingAssets;
    int numLoadedAssets;
    int numOpenedAssetFiles;
    int totalFileLoads;
} TessAssetSystemMetrics;

typedef struct TessListedAsset {
    TStr *assetId;
    TessAssetStatus status;
    TessAssetStatus target;
    enum TessAssetType type;
    uint32_t refCount;
} TessListedAsset;

typedef struct TessAssetSystem
{
    TessAsset nullAsset;
    TessObjectAsset nullObject;
    TessMeshAsset nullMesh;
    bool isServer;

    DELEGATE(onAssetLoaded, OnAssetLoaded_t, 4);

    struct TessFileSystem *fileSystem;
    struct TessMeshAsset *meshPool;
    struct TessTextureAsset *texturePool;
    struct TessObjectAsset *objectPool;
    struct TessLoadingAsset *loadingAssetPool;
    struct AssetLookupEntry *assetLookupEntryPool;    
    struct AssetLookupCache *assetLookupCachePool;

    AssetReference *assetRefPool;

    int numLoadingAssets;
    int numLoadedAssets;
    int numOpenAssetFiles;
    int totalFileLoads;

    khash_t(str) *packageAssetMap;
    // currenlty loaded assets
    khash_t(64) *loadedAssetMap; // TStr assetId, TessAsset*
    // is asset loaded, failed, loading etc
    khash_t(uint32) *assetStatusMap; // TStr assetId, TessAssetStatus
    khash_t(uint32) *assetTargetStatusMap; // assetId->TessAssetStatus
    // this keeps track of how many things reference an asset
    khash_t(ptrToU32) *refCountMap;
    // AssetId -> TessLoadingAsset*
    khash_t(64) *loadingAssetMap;

    TStr **packageList;

    struct TessStrings *tstrings;
    struct Renderer *renderer;
    struct TessPhysicsSystem *physics;
} TessAssetSystem;

typedef struct TessStrings
{
    TessFixedArena stringArena;
    khash_t(str) *stringHashMap;
    TStr **internedStrings;
    TStr *empty;
} TessStrings;

// --------------- GAME WORLD --------

typedef struct TessObject
{
    uint32_t id;
    uint32_t flags;
    TStr *assetId; // TessObjectAsset
    AssetReference *ref;
    struct TessObjectAsset *asset;
} TessObject;

typedef struct TessEntity
{
    uint32_t id;
    uint32_t objectId;
    // TODO: should either be pos,rot,scale OR Mat4 to keep the structure lean
    V3 pos;
    Quat rot;
    V3 scale;
    CPxRigidActor physicsActor;
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
    struct TessStrings *tstrings;
    struct TessPhysicsSystem *physics;
} TessGameSystem;

// --------------- INPUT -------------------

typedef struct TessInputSystem
{
    uint16_t keyStates[AIKE_KEY_COUNT];
    uint16_t keyStatesPrev[AIKE_KEY_COUNT];

    V2 mouseDelta;
    V2 normMouseDelta; // mouse delta in range 0...1
    V2 mousePos;
    V2 normMousePos;
    V2 mousePrev;
    V2 scroll;

    bool headless;

    AikePlatform *platform;
    struct TessRenderSystem *renderSystem;
} TessInputSystem;

#define mouse_left_down(x) ((x)->keyStates[AIKE_BTN_LEFT] && !(x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse_right_down(x) ((x)->keyStates[AIKE_BTN_RIGHT] && !(x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left_up(x) (!(x)->keyStates[AIKE_BTN_LEFT] && (x)->keyStatesPrev[AIKE_BTN_LEFT])
#define mouse_right_up(x) (!(x)->keyStates[AIKE_BTN_RIGHT] && (x)->keyStatesPrev[AIKE_BTN_RIGHT])
#define mouse_left(x) ((x)->keyStates[AIKE_BTN_LEFT] != 0)
#define mouse_right(x) ((x)->keyStates[AIKE_BTN_RIGHT] != 0)

#define key(x, k) ((x)->keyStates[(k)])
#define key_down(x, k) ((x)->keyStates[(k)] && !(x)->keyStatesPrev[(k)])

// --------------- RENDERING -------------------

typedef struct TessRenderSystem
{
    Mat3 windowToScreen;

    int rtW;
    int rtH;

    struct SwapBuffer *viewSwapBuffer;
    struct RenderViewBuilder *viewBuilder;
    struct RenderViewBuffer *gameRenderView;
    struct Renderer *renderer;
    AikePlatform *platform;
    Mat4 worldToClip;
} TessRenderSystem;

// --------------- UI ---------------------

#ifndef TESS_HEADLESS

typedef struct {
    uint32_t state; // 0 - nothing selected, 1 - package selected, 2 - asset selected no confirm
    TStr *packageName;
    TStr *assetName;
    TStr *result;
    b32 show;
} AssetPickerState;

typedef struct TessUISystem
{
    uint32_t width;
    uint32_t height;

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
    struct TessInputSystem *inputSystem;
    struct AikePlatform *platform;
} TessUISystem;

#endif

// --------------- PHYSICS ----------------

#include "physics.h"

// --------------- MENU -------------------

typedef struct TessMainMenu
{
    struct nk_context *nk_ctx;
    uint32_t mode;

    char gameIpStrBuf[256];
    char editorIpStrBuf[256];
    char gamePortStrBuf[7];
    char editorPortStrBuf[7];
    int gamePortStrLen;
    int editorPortStrLen;

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

typedef struct FirstPersonController {
    V3 velocity;
    CPxController characterController;
    TessPhysicsSystem *phys;
} FirstPersonController;

typedef enum GameClientEventType {
    Game_Client_Event_Key_Down,
    Game_Client_Event_Key_Up,
} GameClientEventType;

#ifdef TESS_CLIENT

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

typedef struct CreateAssetState
{
    bool showAddWindow;
    int createType;
    char assetNameBuf[64];
    char objectAssetNameBuf[128];
    AssetPickerState pickerState;
} CreateAssetState;

typedef struct TessEditor
{
    b32 init;
    b32 connected;

    char ipStr[256];
    uint16_t port;
    AikeTCPConnection *tcpCon;

    V2 normalizedCursorPos;

    // renderer writes the currently pointed objectid here
    int32_t cursorObjectIdBuf;
    // safe to use version
    int32_t cursorObjectId;

    bool objectSelected;
    int32_t selectedEditorEntityId;
    int32_t selectedObjectId;

    bool moving;
    V3 moveDirection;

    bool enteringCommand;
    char cmdStr[256];
    bool profilerOpen;
    bool debugLogOpen;
    bool assetWinOpen;
    CreateAssetState createAssetState;

    struct TessCamera *cam;
    V2 camRot;
    bool camLocked;

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
    TessStrings *tstrings;
} TessEditor;

// Game client
//

#define NTRANS_BUF_SIZE 4
#define GAME_CLIENT_MAX_EVENTS_PER_FRAME 256

typedef struct NetworkedTransform
{
    V3 scale;
    V3 positions[NTRANS_BUF_SIZE];
    Quat rotations[NTRANS_BUF_SIZE];
    uint16_t seqs[NTRANS_BUF_SIZE];
    double progress;
    uint16_t fromSeq;
} NetworkedTransform;

typedef struct GameClientDynEntity
{
    uint32_t id;
    uint32_t entityId;
    uint32_t serverId;
    NetworkedTransform ntransform;
} GameClientDynEntity;

typedef enum GameClientFlags {
    Game_Client_Flag_Have_Map_AssetId = 1<<0,
} GameClientFlags;

typedef struct GameClientEvent {
    uint8_t type;
    union {
        struct {
            uint8_t serverInputId;
        } keyEdge;
    };
} GameClientEvent;

#define MAX_INPUT_HISTORY_FRAMES 10
#define MAX_TRACKED_KEY_BYTES 8

typedef struct ClientFrameInput {
    uint8_t keyBits[MAX_TRACKED_KEY_BYTES];
    uint32_t tickId;
} ClientFrameInput;

typedef struct GameClient
{
    ENetHost *eClient;
    ENetPeer *eServerPeer;
    bool init;
    bool connected;

    double tickRate;
    double tickProgress;
    int tickId;

    // max 64 bits per frame
    ClientFrameInput inputHistory[MAX_INPUT_HISTORY_FRAMES];

    char ipStr[256];
    uint16_t port;

    uint32_t frameClientEventCount;
    GameClientEvent frameClientEvents[GAME_CLIENT_MAX_EVENTS_PER_FRAME];

    uint32_t flags;
    TStr *mapAssetId;
    AssetReference *mapReference;
    ClientInputConfig inputConfig;

    V2 camRot;

    FirstPersonController localCharacter;

    GameClientDynEntity **dynEntities;
    GameClientDynEntity *dynEntityPool;

    khash_t(uint32) *serverDynEntityMap; // serverId -> dynEntityId

    // NOTE: this is input system for server tracked inputs
    // local client uses different input system (to decouple from server tickrate)

    TessPhysicsSystem *physics;

    TessInputSystem inputSystem;

    struct TessStrings *strings;
    struct TessAssetSystem *assetSystem;
    struct TessClient *client;
    struct TessGameSystem *world;
    struct AikePlatform *platform;
} GameClient;

#endif

// --------------- STATE --------------

#ifdef TESS_CLIENT

enum TessClientMode
{
    Tess_Client_Mode_Menu,
    Tess_Client_Mode_Editor,
    Tess_Client_Mode_CrazyTown,
    Tess_Client_Mode_Game,
};

typedef struct TessClient
{
    TessFileSystem fileSystem;
    TessAssetSystem assetSystem;
    TessGameSystem gameSystem;
    TessRenderSystem renderSystem;
    TessUISystem uiSystem;
    TessInputSystem inputSystem;

    GameClient gameClient;

    TessMainMenu mainMenu;
    TessEditor editor;

    TessStrings strings;

    TessFixedArena arena;

    AikePlatform *platform;
} TessClient;

#endif

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

    AssetReference *mapReference;
    struct TessStrings *tstrings;
    AikePlatform *platform;
    TessAssetSystem *assetSystem;
    struct TessFixedArena *arena;

    uint16_t objectTableVersion;
    TStr *objectTable[TESS_MAX_OBJECTS];
    struct TessEditorServerEntity **activeEntities;
    struct TessEditorServerClient **activeClients;
    struct TessEditorServerClient *clientPool;
    struct TessEditorServerEntity *entityPool;
} TessEditorServer;

typedef enum GameClientCommand {
    Game_Client_Command_Get_Server_Inputs,
    Game_Client_Command_Get_Server_Properties,
    Game_Client_Command_World_Ready,
    Game_Client_Command_Update_Unreliable,
    Game_Client_Command_Update_Reliable,
} GameClientCommand;


enum GameServerDataType {
    Game_Server_Data_Type_None,
    Game_Server_Data_Type_Uint8,
    Game_Server_Data_Type_Int8,
    Game_Server_Data_Type_Int32,
    Game_Server_Data_Type_Uint32,
    Game_Server_Data_Type_Float32,
    Game_Server_Data_Type_Float64,
    Game_Server_Data_Type_String,
};

enum GameServerProperty {
    Game_Server_Property_Map_AssetId,
    Game_Server_Property_Max_Players,
    Game_Server_Property_Server_Title,
    Game_Server_Property_Gamemode_Title,
};

enum GameServerCommand
{
    Game_Server_Command_Nop,
    Game_Server_Command_Create_DynEntities,
    Game_Server_Command_Destroy_DynEntities,
    Game_Server_Command_Update_DynEntities,
    Game_Server_Command_Server_Property_Response,
    Game_Server_Command_Server_Input_Config,
    Game_Server_Command_Player_Reflection,
    Game_Server_Command_Player_Spawn,
    Game_Server_Command_Player_Despawn,
};

enum ServerPeerFlag {
    Server_Peer_Flag_World_Ready = 1<<0,
};

enum ServerClientType {
    Server_Client_Type_None,
    Server_Client_Type_Player,
    Server_Client_Type_Count,
} ServerClientType;

enum ServerPlayerStatus {
    Server_Player_Status_None,
    Server_Player_Status_Created,
    Server_Player_Status_Count,
} ServerPlayerStatus;

typedef struct FirstPersonControls {
    bool forward;
    bool backward;
    bool right;
    bool left;
    bool jump;
    float yawRad;
}FirstPersonControls;

typedef struct ServerPlayer
{
    uint32_t status;
    ServerPlayerInput input; 
    FirstPersonController fpc;
    uint32_t dynId;
} ServerPlayer;

typedef struct ServerPeer
{
    ENetPeer *ePeer;
    uint32_t clientType;
    // TODO: pointer makes more sense
    union {
        ServerPlayer player;
    };
    uint32_t flags;
} ServerPeer;

typedef struct GameServerDynEntity
{
    uint32_t id;
    uint32_t objectId;
    uint32_t updateSeq;
    V3 position;
    V3 scale;
    Quat rotation;
} GameServerDynEntity;

// Game server
typedef struct GameServer
{
    ENetHost *eServer;

    AikePlatform *platform;
    TessAssetSystem *assetSystem;
    ServerPeer **connectedPeers;
    ServerPeer *peerPool;

    TessPhysicsSystem *physics;

    uint32_t flags;
    TStr *mapAssetId;
    uint32_t mapStatus; // 1 - map asset loaded, 2 - load kicked off

    AssetReference *mapReference;
    ServerInputConfig inputConfig;

    GameServerDynEntity **dynEntities;
    GameServerDynEntity *dynEntityPool;

    TessStrings *strings;

    double updateProgress;
    TessStack *tempStack;

    TessGameSystem gameSystem;
} GameServer;


typedef struct TessServer
{
    TessFileSystem fileSystem;
    TessAssetSystem assetSystem;

    TessEditorServer editorServer;
    GameServer gameServer;

    TessStrings strings;

    TessFixedArena arena;
    TessStack tempStack;

    AikePlatform *platform;
} TessServer;

void first_person_update(FirstPersonController *c, float dt, FirstPersonControls *i);
void first_person_controller_init(FirstPersonController *fpc, TessPhysicsSystem *ps, V3 pos, void *usrPtr);
void first_person_controller_destroy(FirstPersonController *fpc);

bool tess_load_file(TessFileSystem *fs, const char* fileName, uint32_t pipeline, void *userData);

TessAsset* tess_process_asset_from_ttr(struct TessAssetSystem *as, struct TessFile *tfile, struct TessLoadingAsset*);
TStr* tess_intern_string_s(struct TessStrings *tstrings, const char *string, uint32_t maxlen);
TStr* tess_intern_string(struct TessStrings *tstrings, const char *string);
bool get_asset_file(struct TessAssetSystem *as, TStr *assetId, TStr **result);
TStr *tess_get_asset_id(struct TessAssetSystem *as, TStr *package, TStr *asset);
TStr *tess_get_asset_name_from_id(struct TessAssetSystem *as, TStr *assetId);
TStr *tess_get_asset_package_from_id(struct TessAssetSystem *as, TStr *assetId);
TStr* intern_asset_id(TessAssetSystem *as, const char *package, const char *asset);
AssetReference* add_asset_reference(TessAssetSystem *as, TStr *assetId);
void remove_asset_reference(TessAssetSystem *as, AssetReference *ref);

// for debug only
void tess_get_asset_metrics(struct TessAssetSystem *as, struct TessAssetSystemMetrics *tasm);
uint32_t tess_get_asset_list(struct TessAssetSystem *as, TessListedAsset* listedAssets);

internal inline bool tess_is_asset_loaded(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadedAssetMap);
}
internal inline TessAssetStatus tess_get_asset_status(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(uint32, as->assetStatusMap, (intptr_t)assetId);
    return k == kh_end(as->assetStatusMap) ? Tess_Asset_Status_None : kh_value(as->assetStatusMap, k);
}

internal inline TessAssetStatus tess_get_asset_target_status(TessAssetSystem *as, TStr *assetId) {

    khiter_t k = kh_get(uint32, as->assetTargetStatusMap, (intptr_t)assetId);
    return k == kh_end(as->assetTargetStatusMap) ? Tess_Asset_Status_None : kh_value(as->assetTargetStatusMap, k);
}

TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId);

void tess_refresh_package_list(TessAssetSystem *as);
void tess_gen_lookup_cache_for_package(TessAssetSystem *as, TStr *packageName);

void tess_world_init(TessGameSystem *gs);
TessEntity* tess_get_entity(TessGameSystem *gs, uint32_t id);
void tess_register_object(TessGameSystem *gs, uint32_t id, TStr *assetId);
uint32_t tess_create_entity(TessGameSystem *gs, uint32_t id, V3 pos, Quat rot, V3 scale, Mat4 *modelMatrix);
void tess_destroy_entity(TessGameSystem *gs, uint32_t id);
void tess_reset_world(TessGameSystem *gs);

void tess_input_init(struct TessInputSystem *input);
void tess_input_begin(struct TessInputSystem *input);
void tess_input_end(struct TessInputSystem *input);

#ifdef TESS_CLIENT
void tess_ui_init(struct TessUISystem *ui);
void tess_ui_destroy(struct TessUISystem *ui);
void tess_ui_begin(struct TessUISystem *ui);
void tess_ui_end(struct TessUISystem *ui);

void editor_coroutine(TessEditor *editor);

void tess_main_menu_init(struct TessMainMenu *menu);
void tess_main_menu_update(struct TessMainMenu *menu);

void editor_reset(TessEditor *editor);
#endif


void tess_render_entities(struct TessGameSystem *gs);

void scheduler_init(TessScheduler *ctx, AikePlatform *platform, struct TessClient *client, TessServer *server);
void scheduler_yield();
void scheduler_set_mode(uint32_t mode);
void scheduler_event(uint32_t type, void *data, struct AsyncTask *usrPtr);
void scheduler_wait_for(struct AsyncTask *task);
void scheduler_task_end();
SchedulerTask* scheduler_queue_task_(TaskFunc func, void *usrData, const char *name);
void scheduler_assert_task();
#define scheduler_queue_task(func, usrData) scheduler_queue_task_(func, usrData, #func)
void scheduler_cancel_task(SchedulerTask *task);

extern struct TessVtable *g_tessVtbl;

#define ASYNC // for now just an informative tag for the programmer
