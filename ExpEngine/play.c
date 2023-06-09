void play_update(TessClient *client, double dt, uint16_t frameId);
void update_transform(GameClient *gc, uint16_t frameId, void *data, uint32_t dataLen);
void process_packet(GameClient *gc, void *data, size_t dataLen);
void query_required_server_properties(GameClient *gc);
void query_server_inputs(GameClient *gc);
void notify_world_ready(GameClient *gc);
void game_client_tick(GameClient *client);

void load_map(TessClient *client, TessMapAsset *map) {
    printf("Game client load map %s with %d object and %d entities\n", map->asset.assetId->cstr, map->mapObjectCount, map->mapEntityCount);
    tess_reset_world(&client->gameSystem);
    for(int i = 0; i < map->mapObjectCount; i++) {
        TessMapObject *obj = map->objects + i;
        tess_register_object(&client->gameSystem, obj->objectid, obj->assetId);
    }
    for(int i = 0; i < map->mapEntityCount; i++) {
        Mat4 modelToWorld;
        TessMapEntity *ent = map->entities + i;
        mat4_trs(&modelToWorld, ent->position, ent->rotation, ent->scale);
        tess_create_entity(&client->gameSystem, ent->objectId, ent->position, ent->rotation, ent->scale, &modelToWorld);
    }
}

void game_client_yield(GameClient *gclient) {

    ENetEvent event;
    // TODO: this should be done at start of the frame!
    while(gclient->connected && enet_host_service(gclient->eClient, &event, 0) > 0)
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_DISCONNECT:
                gclient->connected = false;
                printf("Disconnected!\n");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                //assert(event.packet->dataLength == 30);
                //float *data = (float*)event.packet->data;
                //update_transform(gclient, (uint16_t)frameId, event.packet->data, event.packet->dataLength);
                process_packet(gclient, event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
        }
    }

    scheduler_yield();
}

void game_client_coroutine(GameClient *gclient)
{
    ByteStream stream;
    // NOTE: GameClient dependencies are filled and
    // rest of the memory is zero initialized
game_client_start:
    gclient->init = true;
    gclient->serverDynEntityMap = kh_init(uint32);
    gclient->tickRate = 10.0;
    gclient->tickProgress = 0.0;
    gclient->inputSystem.headless = true;
    gclient->inputSystem.platform = gclient->platform;
    tess_input_init(&gclient->inputSystem);

    gclient->localCharacter.characterController = NULL;

    memset(&gclient->inputConfig, 0, sizeof(ClientInputConfig));
    memset(gclient->inputHistory, 0, sizeof(gclient->inputHistory));

    gclient->physics = gclient->client->gameSystem.physics;

    gclient->eClient = enet_host_create(NULL, 1, 3, 0, 0);
    if(NULL == gclient->eClient)
    {
        fprintf(stderr, "Failed to create ENet client for gameClient!\n");
        goto game_client_done;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;

    enet_address_set_host(&address, gclient->ipStr);
    address.port = gclient->port;

    int retries = 3;
    while(retries > 0) // reconnect loop
    {
        peer = enet_host_connect(gclient->eClient, &address, 3, 0);
        assert(NULL != peer);
        enet_peer_timeout(peer, 0, 0, 5000);

        bool connecting = true;
        while(connecting) // completion poll loop
        {
            if(key(&gclient->client->inputSystem, AIKE_KEY_ESC)) // early out with esc
            {
                retries = 0;
                connecting = false;
                printf("Connect cancelled by user\n");
            }

            int res = enet_host_service(gclient->eClient, &event, 0);
            if(res > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
            {
                printf("Game client connected to %s:%u\n", gclient->ipStr, gclient->port);
                gclient->eServerPeer = peer;
                gclient->connected = true;
                retries = 0;
                connecting = false;
            }
            else if(res > 0)
            {
                printf("Connection to %s:%u failed... Retrying... %d\n", gclient->ipStr, gclient->port, retries);
                connecting = false;
                retries --;
            }
            scheduler_yield();
        }
    }

    // get server properties and load map
    if(gclient->connected)
    {
        query_required_server_properties(gclient);
        query_server_inputs(gclient);
        while((gclient->flags & Game_Client_Flag_Have_Map_AssetId) == 0) {
            // TODO: option to cancel or at least a timeout!
            game_client_yield(gclient);
        }
        notify_world_ready(gclient);

        TessAssetSystem *as = gclient->assetSystem;
        TessStrings *strings = gclient->strings;
        gclient->mapReference = add_asset_reference(as, gclient->mapAssetId);
        TessAssetStatus mapStatus;
        do{
            scheduler_yield();
            mapStatus = tess_get_asset_status(as, gclient->mapAssetId);
        } while(mapStatus != Tess_Asset_Status_Fail && mapStatus != Tess_Asset_Status_Loaded);
        TessAsset *map = tess_get_asset(as, gclient->mapAssetId);
        if(map == NULL || map->type != Tess_Asset_Map) {
            // TODO: proper disconnect
            fprintf(stderr, "Map asset requested by server was not found or was not a map asset!\n");
            gclient->connected = false;
        } else {
            load_map(gclient->client, (TessMapAsset*)map);
            gclient->eServerPeer = peer;
        }
    }
    else
        gclient->eServerPeer = NULL;

    AikeTime start = gclient->platform->get_monotonic_time(gclient->platform);
    unsigned int frameId = 0;

    while(gclient->connected) // update loop
    {
        AikeTime curTime = gclient->platform->get_monotonic_time(gclient->platform);
        double dt = aike_timedif_sec(start, curTime);
        start = curTime;

        gclient->tickProgress += dt;
        const double tickTime = 1.0/gclient->tickRate;
        while(gclient->tickProgress >= tickTime) {
            game_client_tick(gclient);
            gclient->tickProgress -= tickTime;
        }

        if(gclient->connected) {
            play_update(gclient->client, dt, frameId);
        }
        frameId++;
        game_client_yield(gclient);
    }

    tess_reset_world(&gclient->client->gameSystem);
    enet_host_destroy(gclient->eClient);
game_client_done:
    gclient->init = false;
    buf_free(gclient->dynEntities);
    kh_destroy(uint32, gclient->serverDynEntityMap);
    scheduler_set_mode(Tess_Client_Mode_Menu);
    scheduler_yield();
    goto game_client_start;
    assert(0); // returning to coroutine after final yield
}

void game_exit(GameClient *gclient)
{
    if(gclient->connected)
    {
        // this should raise disconnect event and that should terminate client
        enet_peer_disconnect(gclient->eServerPeer, 0);
    }
}

GameClientDynEntity* game_client_get_dynamic_entity(GameClient *gc, uint32_t id)
{
    assert(id >= 0 && id < TESS_CLIENT_MAX_DYN_ENTITIES);
    return &gc->dynEntityPool[id];
}

void game_client_destroy_dyn_ent(GameClient *gc, uint32_t id)
{
    auto dent = game_client_get_dynamic_entity(gc, id);
    int index = buf_find_idx(gc->dynEntities, dent);
    if(index != -1)
    {
        tess_destroy_entity(&gc->client->gameSystem, dent->entityId);
        buf_remove_at(gc->dynEntities, index);
        pool_free(gc->dynEntityPool, dent);
    }
}

uint16_t defaultKeycodeByTag(uint8_t tag) {
    switch(tag) {
        case SERVER_INPUT_TAG_WALK_FORWARD:
            return AIKE_KEY_W;
        case SERVER_INPUT_TAG_WALK_BACKWARD:
            return AIKE_KEY_S;
        case SERVER_INPUT_TAG_WALK_RIGHT:
            return AIKE_KEY_D;
        case SERVER_INPUT_TAG_WALK_LEFT:
            return AIKE_KEY_A;
        case SERVER_INPUT_TAG_WALK_JUMP:
            return AIKE_KEY_SPACE; 
        case SERVER_INPUT_TAG_WALK_ACTION:
            return AIKE_KEY_E;
        default:
            return AIKE_KEY_RESERVED;
    }
}

void game_client_start_tracking_input(GameClient *gc) {
    auto ic = &gc->inputConfig;
    for(int i = 0; i < ic->inputCount; i++) {
        auto inpt = ic->serverInputs[i];
        switch(inpt.type) {
            case SERVER_INPUT_TYPE_KEY:
                if(inpt.flags&SERVER_INPUT_KEY_FLAG_TRACK_STATE) {
                    int id = ic->trackedKeyCount++;
                    ic->trackedKeys[id].inputId = i;
                    ic->trackedKeys[id].platformKeycode = defaultKeycodeByTag(inpt.tag);
                }
                if(inpt.flags&(SERVER_INPUT_KEY_FLAG_TRACK_DOWN_EVENT|SERVER_INPUT_KEY_FLAG_TRACK_UP_EVENT)) {
                    int id = ic->eventKeyCount++;
                    ic->eventKeys[id].inputId = i;
                    ic->eventKeys[id].platformKeycode = defaultKeycodeByTag(inpt.tag);
                }
                break;
            case SERVER_INPUT_TYPE_ANALOG:
                // TODO: implement
                break;
            case SERVER_INPUT_TYPE_SPHERICAL:
                // TODO: implement
                break;
            default:
                break;
        };
    }
}

// TODO: just have a string library with len+string?
void process_server_property_string(GameClient *gc, uint32_t property, const char *str, uint32_t len) {
    switch(property) {
        case Game_Server_Property_Map_AssetId:
            gc->mapAssetId = tess_intern_string_s(gc->strings, str, len);
            printf("map set to %s\n", gc->mapAssetId->cstr);
            gc->flags |= Game_Client_Flag_Have_Map_AssetId;
            break;
    }
    free((void*)str);
}

void query_required_server_properties(GameClient *gc) {
    // format: id (uint16_t), count(uint8_t), [array of properties](uint8_t)
    // mapAssetId
    // TODO: dont use program stack
    uint8_t mem[300];
    ByteStream stream;
    init_byte_stream(&stream, mem, sizeof(mem));
    stream_write_uint16(&stream, Game_Client_Command_Get_Server_Properties);
    stream_write_uint8(&stream, 1);
    stream_write_uint8(&stream, Game_Server_Property_Map_AssetId);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(gc->eServerPeer, 0, packet);
}

void query_server_inputs(GameClient *gc) {
    // TODO: dont use program stack
    uint8_t mem[300];
    ByteStream stream;
    init_byte_stream(&stream, mem, sizeof(mem));
    stream_write_uint16(&stream, Game_Client_Command_Get_Server_Inputs);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(gc->eServerPeer, 0, packet);
}

void notify_world_ready(GameClient *gc) {
    // TODO: dont use program stack
    uint8_t mem[64];
    ByteStream stream;
    init_byte_stream(&stream, mem, sizeof(mem));
    stream_write_uint16(&stream, Game_Client_Command_World_Ready);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(gc->eServerPeer, 0, packet);
}

void local_spawn(GameClient *gc, V3 pos) {
    first_person_controller_init(&gc->localCharacter, gc->physics, pos, 0x0);
    if(gc->localCharacter.characterController != NULL) {
        fprintf(stderr, "Overwriting localCharacter when it's not NULL, possible memory leak!!!\r\n");
    }
}

void local_despawn(GameClient *gc) {
    if(gc->localCharacter.characterController != NULL) {
        first_person_controller_destroy(&gc->localCharacter);
    }
    printf("I got destroyed!!!\r\n");
}


void process_packet(GameClient *gc, void *data, size_t dataLen)
{
    uint8_t buf[65536];

    ByteStream stream;
    init_byte_stream(&stream, data, dataLen);
    bool success = true;
    parse_client_msg(&stream, buf, 65536, gc);
    stream_go_to_offset(&stream, 0);
    uint16_t cmdId;
    success &= stream_read_uint16(&stream, &cmdId);
    switch(cmdId)
    {
        case Game_Server_Command_Server_Property_Response:
            {
                uint8_t type;
                uint8_t property;
                success &= stream_read_uint8(&stream, &property);
                if(!success) break;
                success &= stream_read_uint8(&stream, &type);
                if(!success) break;
                switch(type) {
                    case Game_Server_Data_Type_String:
                    {
                        uint16_t strLen = 0;
                        success &= stream_read_uint16(&stream, &strLen);
                        if(!success) break;
                        // TODO: don't use malloc?
                        char *buf = malloc(strLen);
                        success &= stream_read_str(&stream, buf, strLen);
                        if(success) {
                            process_server_property_string(gc, property, buf, strLen);
                        } else {
                            free(buf);
                        }
                    } break;
                    default:
                        fprintf(stderr, "Unhandled server property response for proprty %d with type %d\n", property, type);
                        break;
                }
            } break;
        default:
            break;
    }
}

char *asset_status_to_string(enum TessAssetStatus status) {
    switch(status) {
        case Tess_Asset_Status_None: // uninitialized state
            return "Tess_Asset_Status_None";
        case Tess_Asset_Status_InQueue: // the task to load the asset has been queued, but not started yet
            return "Tess_Asset_Status_InQueue";
        case Tess_Asset_Status_Loading: // loading asset has started, but not yet finished
            return "Tess_Asset_Status_Loading";
        case Tess_Asset_Status_Loaded: // asset is ready
            return "Tess_Asset_Status_Loaded";
        case Tess_Asset_Status_Fail: // failure, asset can not be loaded
            return "Tess_Asset_Status_Fail";
        case Tess_Asset_Status_Pending_Destroy:
            return "Tess_Asset_Status_Pendind_Destroy";
        default:
            return "N/A";
    }
}

char *asset_type_to_string(enum TessAssetType type) {
    switch(type) {
        case Tess_Asset_None:
            return "Tess_Asset_None";
        case Tess_Asset_Unknown:
            return "Tess_Asset_Unknown";
        case Tess_Asset_Mesh:
            return "Tess_Asset_Mesh";
        case Tess_Asset_Texture:
            return "Tess_Asset_Texture";
        case Tess_Asset_Material:
            return "Tess_Asset_Material";
        case Tess_Asset_Sound:
            return "Tess_Asset_Sound";
        case Tess_Asset_Object:
            return "Tess_Asset_Object";
        case Tess_Asset_Map:
            return "Tess_Asset_Map";
        case Tess_Asset_Collider:
            return "Tess_Asset_Collider";
        default:
            return "N/A";
    }
}


void draw_game_debug_ui(TessClient *client/*, TessServer *server*/) {
    struct nk_context *ctx = &client->uiSystem.nk_ctx; 
    char buf[512];
    TessAssetSystemMetrics asMetrics;
    tess_get_asset_metrics(&client->assetSystem, &asMetrics);
    if(nk_begin(ctx, "Test window", nk_rect(0, 0, 250, 400), NK_WINDOW_NO_SCROLLBAR)) {
        sprintf(buf, "Loading assets: %d", asMetrics.numLoadingAssets);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, buf, NK_TEXT_ALIGN_CENTERED);
        sprintf(buf, "Loaded assets: %d", asMetrics.numLoadedAssets);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, buf, NK_TEXT_ALIGN_CENTERED);
        sprintf(buf, "Open asset files: %d", asMetrics.numOpenedAssetFiles);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, buf, NK_TEXT_ALIGN_CENTERED);
        sprintf(buf, "Total asset file loads: %d", asMetrics.totalFileLoads);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, buf, NK_TEXT_ALIGN_CENTERED);
    }
    nk_end(ctx);

    uint32_t count = tess_get_asset_list(&client->assetSystem, NULL);
    TessListedAsset *assets = malloc(count * sizeof(TessListedAsset));
    uint32_t count2 = tess_get_asset_list(&client->assetSystem, assets);
    assert(count2 == count);

    if(nk_begin(ctx, "Asset list", nk_rect(0, 350, 900, 600),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE)) {

        if(nk_tree_push(ctx, NK_TREE_TAB, "clientxx", NK_MINIMIZED)) {
            for(int i = 0; i < count2; i++) {
                nk_layout_row_dynamic(ctx, 15, 5);
                nk_label(ctx, assets[i].assetId->cstr, NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_status_to_string(assets[i].status), NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_status_to_string(assets[i].target), NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_type_to_string(assets[i].type), NK_TEXT_ALIGN_LEFT);
                nk_labelf(ctx, NK_TEXT_LEFT, "%d", assets[i].refCount); 
            }
            nk_tree_pop(ctx);
        }
        /*count = tess_get_asset_list(&client->server->assetSystem, NULL);
        TessListedAsset *assets2 = malloc(count * sizeof(TessListedAsset));
        count2 = tess_get_asset_list(&client->server->assetSystem, assets2);
        if(nk_tree_push(ctx, NK_TREE_TAB, "serverxx", NK_MINIMIZED)) {
            for(int i = 0; i < count2; i++) {
                nk_layout_row_dynamic(ctx, 15, 5);
                nk_label(ctx, assets2[i].assetId->cstr, NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_status_to_string(assets2[i].status), NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_status_to_string(assets2[i].target), NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, asset_type_to_string(assets2[i].type), NK_TEXT_ALIGN_LEFT);
                nk_labelf(ctx, NK_TEXT_LEFT, "%d", assets2[i].refCount); 
            }
            nk_tree_pop(ctx);
        }
        free(assets2);*/
    }
    free(assets);
    nk_end(ctx);
}

void game_client_add_event(GameClient *gclient, GameClientEvent *event) {
    uint32_t id = gclient->frameClientEventCount++;
    assert(id < GAME_CLIENT_MAX_EVENTS_PER_FRAME);
    if(id < GAME_CLIENT_MAX_EVENTS_PER_FRAME) {
        gclient->frameClientEvents[id] = *event;
    }
}

// this is game logic update (synced to server tickrate)
void game_client_tick(GameClient *gclient) {
    TessInputSystem *in = &gclient->inputSystem;
    tess_input_begin(in);

    // TODO: compose update packet (input, events)

    // calculate input bitfield to send and store
    ClientInputConfig *ic = &gclient->inputConfig;
    int inputBitfieldLen = (ic->trackedKeyCount/8) + (ic->trackedKeyCount%8 != 0);
    uint8_t bitfield[MAX_TRACKED_KEY_BYTES];
    // TODO: don't allow this to happen! (64 keys should be plenty..)
    assert(ic->trackedKeyCount <= MAX_TRACKED_KEY_BYTES);
    memset(bitfield, 0, inputBitfieldLen);
    for(int i = 0; i < ic->trackedKeyCount; i++) {
        int idx = i/8;
        int bit = i%8;
        auto val = in->keyStates[ic->trackedKeys[i].platformKeycode] != 0; 
        bitfield[idx] |= (val<<bit);
        //printf("%d", val);
    }

    // store to input history
    int historyIdx = gclient->tickId % MAX_INPUT_HISTORY_FRAMES;
    for(int i = 0; i < MAX_TRACKED_KEY_BYTES; i++) {
        gclient->inputHistory[historyIdx].keyBits[i] = bitfield[i];
    }
    gclient->inputHistory[historyIdx].tickId = gclient->tickId;

    GameClientEvent e;
    for(int i = 0; i < ic->eventKeyCount; i++) {
        uint8_t serverId = ic->eventKeys[i].inputId;
        uint8_t flags = ic->serverInputs[serverId].flags;
        uint8_t keycode = ic->eventKeys[i].platformKeycode;
        if(flags&SERVER_INPUT_KEY_FLAG_TRACK_UP_EVENT){ 
            if(in->keyStates[keycode] == 0 && in->keyStatesPrev[keycode] != 0) {
                //printf("tracked key up\r\n");
                e.type = Game_Client_Event_Key_Up;
                e.keyEdge.serverInputId = serverId;
                game_client_add_event(gclient, &e);
            }
        }
        if(flags&SERVER_INPUT_KEY_FLAG_TRACK_DOWN_EVENT) {
            if(in->keyStates[keycode] != 0 && in->keyStatesPrev[keycode] == 0) {
                // TODO: queue event
                //printf("tracked key down\r\n");
                e.type = Game_Client_Event_Key_Down;
                e.keyEdge.serverInputId = serverId;
                game_client_add_event(gclient, &e);
            }
        }
    }


    // TODO: use temp memory
    uint8_t mem[1024];
    ByteStream stream;

    // send update data that does not need to be resent if lost
    init_byte_stream(&stream, mem, sizeof(mem));
    stream_write_uint16(&stream, Game_Client_Command_Update_Unreliable);
    stream_write_uint16(&stream, (uint16_t)gclient->tickId);
    stream_write_uint8(&stream, inputBitfieldLen);
    for(int i = 0; i < inputBitfieldLen; i++) {
        stream_write_uint8(&stream, bitfield[i]);
    }
    stream_write_v2(&stream, gclient->camRot);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), 0);
    enet_peer_send(gclient->eServerPeer, 1, packet);

    // send data that needs to be reliable
    init_byte_stream(&stream, mem, sizeof(mem));
    stream_write_uint16(&stream, Game_Client_Command_Update_Reliable);
    stream_write_uint16(&stream, (uint16_t)gclient->tickId);
    stream_write_uint8(&stream, gclient->frameClientEventCount);
    // TODO: just split into multiple packets when this happens
    assert(gclient->frameClientEventCount <= 255);
    for(int i = 0; i < gclient->frameClientEventCount; i++) {
        auto e = &gclient->frameClientEvents[i];
        stream_write_uint8(&stream, e->type);
        switch(e->type) {
            case Game_Client_Event_Key_Up:
            case Game_Client_Event_Key_Down:
                stream_write_uint8(&stream, e->keyEdge.serverInputId);
                break;
        }
    }
    if(stream_get_offset(&stream) > 5) { // no point sending empty packet!
        packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(gclient->eServerPeer, 0, packet);
    }

    tess_input_end(in);
    gclient->tickId++;
    gclient->frameClientEventCount = 0;
}

// this is visual/render update
void play_update(TessClient *client, double dt, uint16_t frameId)
{
    TessCamera *cam = &client->gameSystem.defaultCamera;
    TessInputSystem *input = &client->inputSystem;
    GameClient *gc = &client->gameClient;

    V3 forward = make_v3(0.0f, 0.0f, 0.1f);
    V3 right;
    quat_v3_mul_dir(&forward, cam->rotation, make_v3(0.0f, 0.0f, 0.01f));
    quat_v3_mul_dir(&right, cam->rotation, make_v3(0.01f, 0.0f, 0.0f));

    // TODO: fix timestep!!!!!
    physx_simulate(gc->physics, dt);
    if(gc->localCharacter.characterController != NULL) {
        FirstPersonControls ctrl = {
            .forward = key(input, AIKE_KEY_W),
            .backward = key(input, AIKE_KEY_S),
            .right = key(input, AIKE_KEY_D),
            .left = key(input, AIKE_KEY_A),
            .jump = key_down(input, AIKE_KEY_SPACE),
            .yawRad = gc->camRot.x,
        };
        // TODO: move this to firstpersoncontroller.c
        first_person_update(&gc->localCharacter, dt, &ctrl);
    }

    V3 pos;
    void *usrPtr;
    uint32_t count = physx_get_controllers(gc->physics, &pos, &usrPtr, 1);
    if(count == 1) {
        cam->position = pos;
    }

    V2 scaledMouseD;
    v2_scale(&scaledMouseD, TT_PI32/180.0f, input->mouseDelta);
    v2_add(&gc->camRot, gc->camRot, scaledMouseD);
    gc->camRot = make_v2(gc->camRot.x, MAX(gc->camRot.y, -90.0f*TT_DEG2RAD_F));
    gc->camRot = make_v2(gc->camRot.x, MIN(gc->camRot.y, 90.0f*TT_DEG2RAD_F));
    gc->camRot.x = fmodf(gc->camRot.x, TT_PI32*2.0f);
    gc->camRot.y = fmodf(gc->camRot.y, TT_PI32*2.0f);

    Quat xRot;
    quat_euler(&xRot, make_v3(gc->camRot.y, gc->camRot.x, 0.0f));
    cam->rotation = xRot;

    if(key(input, AIKE_KEY_ESC))
    {
        local_despawn(gc);
        game_exit(&client->gameClient);
    }

    count = buf_len(gc->dynEntities);
    for(int i = 0; i < count; i++)
    {
        GameClientDynEntity *dent = gc->dynEntities[i];
        transform_advance(&dent->ntransform, dt, frameId);
        TessEntity *ent = tess_get_entity(gc->world, dent->entityId);
        V3 npos, nscale; Quat nrot;
        transform_get(&dent->ntransform, &npos, &nrot, &nscale);
        //printf("pos %f %f %f\n", npos.x, npos.y, npos.z);
        mat4_trs(&ent->objectToWorld, npos, nrot, nscale);
    }
    draw_game_debug_ui(client/*, client->server*/);
}

static void message_player_reflection(MsgPlayerReflection *msg, void *usrPtr) {
    // TODO: sync local character
}

static void message_server_input_config(MsgServerInputConfig *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    auto ic = &gc->inputConfig;
    for(int i = 0; i < msg->count; i++) {
        ic->serverInputs[i].type = msg->inputs[i].type;
        ic->serverInputs[i].tag = msg->inputs[i].tag;
        ic->serverInputs[i].flags = msg->inputs[i].flags;
    }
    ic->inputCount = msg->count;
    game_client_start_tracking_input(gc);
}

static void message_create_dyn_entities(MsgCreateDynEntities *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    Mat4 mat;
    for(int i = 0; i < msg->count; i++) {
        auto ed = msg->entities + i;
        mat4_trs(&mat, ed->position, ed->rotation, ed->scale);
        auto dynEnt = pool_allocate(gc->dynEntityPool);
        assert(dynEnt);
        dynEnt->id = dynEnt - gc->dynEntityPool;
        dynEnt->serverId = ed->entityId;
        dynEnt->entityId = tess_create_entity(gc->world, ed->objectId, ed->position, ed->rotation, ed->scale, &mat);
        assert(dynEnt->entityId != 0);
        transform_init(&dynEnt->ntransform, ed->position, ed->rotation, ed->scale);
        buf_push(gc->dynEntities, dynEnt);
        int dummy;
        khiter_t k = kh_put(uint32, gc->serverDynEntityMap, ed->entityId, &dummy);
        kh_val(gc->serverDynEntityMap, k) = dynEnt->id;
        printf("entity %d obj: %d pos: %f %f %f rot: %f %f %f %f\n", ed->entityId, ed->objectId, ed->position.x, ed->position.y, ed->position.z, ed->rotation.w, ed->rotation.x, ed->rotation.y, ed->rotation.z);
    }
}

static void message_update_dyn_entities(MsgUpdateDynEntities *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    for(int i = 0; i < msg->count; i++) {
        auto ent = msg->entities+i;
        khiter_t k = kh_get(uint32, gc->serverDynEntityMap, ent->entityId);
        if(k == kh_end(gc->serverDynEntityMap)) continue;
        uint32_t dynEntId = kh_val(gc->serverDynEntityMap, k);
        auto dynEnt = game_client_get_dynamic_entity(gc, dynEntId);
        assert(dynEnt);
        transform_update(&dynEnt->ntransform, ent->sequence, 0, ent->position, ent->rotation);
    }
}

static void message_destroy_dyn_entities(MsgDestroyDynEntities *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    for(int i = 0; i < msg->count; i++) {
        uint32_t entityId = msg->entities[i].entityId;
        game_client_destroy_dyn_ent(gc, entityId);
        printf("client destroy dyn entityid %d\n", entityId);
    }
}

static void message_player_spawn(MsgPlayerSpawn *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    local_spawn(gc, msg->position);
}

static void message_player_despawn(MsgPlayerDespawn *msg, void *usrPtr) {
    GameClient *gc = (GameClient*)usrPtr;
    local_despawn(gc);
}

static void client_send_data(uint8_t *data, uint32_t len, void *usrPtr) {
    ENetPeer *p = (ENetPeer*)usrPtr;
    ENetPacket *packet = enet_packet_create(data, len, 0);
    enet_peer_send(p, 1, packet);
}
