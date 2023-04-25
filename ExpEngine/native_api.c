
// TODO: this should be nauto generated to be same as the one
// on managed code side
typedef enum NativeApiProc {
    Native_Api_Proc_Unkown,
    Native_Api_Proc_InternAssetId,
    Native_Api_Proc_SetMap,
    Native_Api_Proc_SetServerTitle,
    Native_Api_Proc_SetGamemodeTitle,
    Native_Api_Proc_SetMaxPlayers,
} NativeApiProc;

void* native_intern_asset_id(GameServer *server, const char *package, const char *asset) {
    TessAssetSystem *as = server->assetSystem;
    auto ret = intern_asset_id(as, package, asset);
    return ret;
}

void native_set_map(GameServer *gs, TStr *assetId) {
    gs->mapAssetId = assetId;
}

void native_set_server_title(GameServer *gs, const char *title) {
}

void native_set_gamemode_title(GameServer *gs, const char *title) {
}

void native_set_max_players(GameServer *gs, int maxPlayers) {
}

