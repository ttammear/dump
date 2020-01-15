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

    memset(&gclient->inputConfig, 0, sizeof(ClientInputConfig));

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

void process_packet(GameClient *gc, void *data, size_t dataLen)
{
    ByteStream stream;
    init_byte_stream(&stream, data, dataLen);
    bool success = true;
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
        case Game_Server_Command_Server_Input_Config:
            {
                uint8_t icount;
                uint8_t type;
                uint8_t tag;
                uint8_t flags;
                success = stream_read_uint8(&stream, &icount);
                auto ic = &gc->inputConfig;
                if(success) {
                    for(int iidx = 0; iidx < icount; iidx++) {
                        success &= stream_read_uint8(&stream, &type);
                        success &= stream_read_uint8(&stream, &tag);
                        success &= stream_read_uint8(&stream, &flags);
                        if(!success)
                            break;
                        else {
                            ic->serverInputs[iidx].type = type;
                            ic->serverInputs[iidx].tag = tag;
                            ic->serverInputs[iidx].flags = flags;
                        }
                    }
                }
                if(success) {
                    ic->inputCount = icount;
                    game_client_start_tracking_input(gc);
                }
            } break;
        case Game_Server_Command_Create_DynEntities:
            {
                uint16_t count;
                success &= stream_read_uint16(&stream, &count);
                if(!success) break;
                printf("Client create %hu dynamic entities, size: %zu\n", count, dataLen);
                V3 pos, scale; Quat rot;
                Mat4 mat;
                uint32_t entityId, objectId;
                for(int i = 0; i < count; i++)
                {
                    success &= stream_read_uint32(&stream, &objectId);
                    success &= stream_read_uint32(&stream, &entityId);
                    success &= stream_read_v3(&stream, &pos);
                    success &= stream_read_v3(&stream, &scale);
                    success &= stream_read_quat(&stream, &rot);
                    if(!success)
                        break;

                    mat4_trs(&mat, pos, rot, scale);
                    auto dynEnt = pool_allocate(gc->dynEntityPool);
                    assert(dynEnt);
                    dynEnt->id = dynEnt - gc->dynEntityPool;
                    dynEnt->serverId = entityId;
                    dynEnt->entityId = tess_create_entity(gc->world, objectId, pos, rot, scale, &mat);
                    assert(dynEnt->entityId != 0); // world in invalid state, possibly world not reset before started loading?
                    transform_init(&dynEnt->ntransform, pos, rot, scale);
                    buf_push(gc->dynEntities, dynEnt);

                    int dummy;
                    khiter_t k = kh_put(uint32, gc->serverDynEntityMap, entityId, &dummy);
                    kh_val(gc->serverDynEntityMap, k) = dynEnt->id;

                    printf("entity %d obj: %d pos: %f %f %f rot: %f %f %f %f\n", entityId, objectId, pos.x, pos.y, pos.z, rot.w, rot.x, rot.y, rot.z);
                }
            }break;
        case Game_Server_Command_Update_DynEntities:
            {
                uint16_t count;
                success &= stream_read_uint16(&stream, &count);
                if(!success) break;
                //printf("Client update %hu dynamic entities, size: %zu\n", count, dataLen);
                V3 pos; Quat rot;
                Mat4 mat;
                uint32_t entityId;
                uint16_t seq;
                for(int i = 0; i < count; i++)
                {
                    success &= stream_read_uint32(&stream, &entityId);
                    success &= stream_read_uint16(&stream, &seq);
                    success &= stream_read_v3(&stream, &pos);
                    success &= stream_read_quat(&stream, &rot);
                    if(!success) break;

                    //printf("update %d seq %hu pos %f %f %f\n", entityId, seq, pos.x, pos.y, pos.z);

                    khiter_t k = kh_get(uint32, gc->serverDynEntityMap, entityId);
                    // not created, skip
                    if(k == kh_end(gc->serverDynEntityMap)) 
                        continue;
                    uint32_t dynEntId = kh_val(gc->serverDynEntityMap, k);
                    auto dynEnt = game_client_get_dynamic_entity(gc, dynEntId);
                    assert(dynEnt);
                    transform_update(&dynEnt->ntransform, seq, 0, pos, rot);
                }
            } break;
        case Game_Server_Command_Destroy_DynEntities:
            {
                uint16_t count;
                success &= stream_read_uint16(&stream, &count);
                if(!success) break;
                uint32_t entityId;
                for(int i = 0; i < count; i++)
                {
                    success &= stream_read_uint32(&stream, &entityId);
                    if(!success) break;
                    game_client_destroy_dyn_ent(gc, entityId);
                    printf("client destroy dyn entityid %d\n", entityId);
                }
            } break;
        case Game_Server_Command_Player_Reflection:
            {
                V3 pos;
                success &= stream_read_v3(&stream, &pos);
                v3_add(&pos, pos, (V3){0.0f, 1.5f, 0.0f});
                TessCamera *cam = &gc->client->gameSystem.defaultCamera;
                cam->position = pos;
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


void draw_game_debug_ui(TessClient *client, TessServer *server) {
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
        count = tess_get_asset_list(&client->server->assetSystem, NULL);
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
        free(assets2);
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

    ClientInputConfig *ic = &gclient->inputConfig;
    int inputBitfieldLen = (ic->trackedKeyCount/8) + (ic->trackedKeyCount%8 != 0);
    uint8_t bitfield[256];
    memset(bitfield, 0, inputBitfieldLen);
    for(int i = 0; i < ic->trackedKeyCount; i++) {
        int idx = i/8;
        int bit = i%8;
        auto val = in->keyStates[ic->trackedKeys[i].platformKeycode] != 0; 
        bitfield[idx] |= (val<<bit);
        //printf("%d", val);
    }
    //printf(" (%d bytes)\r\n", inputBitfieldLen);

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

    if(key(input, AIKE_KEY_TAB)) {
        if(key(input, AIKE_KEY_W))
            v3_add(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_S))
            v3_sub(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_D))
            v3_add(&cam->position, cam->position, right);
        if(key(input, AIKE_KEY_A))
            v3_sub(&cam->position, cam->position, right);
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
        game_exit(&client->gameClient);
    }

    int count = buf_len(gc->dynEntities);
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
    draw_game_debug_ui(client, client->server);
}

