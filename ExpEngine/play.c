void play_update(TessClient *client, double dt, uint16_t frameId);
void update_transform(GameClient *gc, uint16_t frameId, void *data, uint32_t dataLen);
void process_packet(GameClient *gc, void *data, size_t dataLen);

void load_map(TessClient *client)
{
    tess_reset_world(&client->gameSystem);
    void *buf = malloc(1024*1024);
    FILE *file = fopen("Packages/Sponza/map.ttm", "rb");
    if(file != NULL)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
        fread(buf, 1, size, file);
        fclose(file);

        MapReader reader;
        bool res = map_reader_begin(&reader, buf, size);
        if(!res)
        {
            fprintf(stderr, "Loading map failed!\n");
            goto load_done;
        }
        MapObject obj;
        while(map_next_object(&reader, &obj) == Map_Error_Code_Success)
        {
            TStr *assetId = tess_intern_string_s(&client->strings, obj.assetId, sizeof(obj.assetId));
            tess_register_object(&client->gameSystem, obj.objectId, assetId);
        }
        MapEntity ent;
        while(map_next_entity(&reader, &ent) == Map_Error_Code_Success)
        {
            Mat4 modelToWorld;
            mat4_trs(&modelToWorld, ent.pos, ent.rot, ent.scale);
            tess_create_entity(&client->gameSystem, ent.objectId, &modelToWorld);
        }
    }
load_done:
    free(buf);
}

void game_client_coroutine(GameClient *gclient)
{
game_client_start:
    gclient->init = true;
    gclient->connected = false;
    gclient->dynEntities = NULL;
    gclient->serverDynEntityMap = kh_init(uint32);

    gclient->eClient = enet_host_create(NULL, 1, 2, 0, 0);
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
        peer = enet_host_connect(gclient->eClient, &address, 2, 0);
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

    if(gclient->connected)
    {
        load_map(gclient->client);
        gclient->eServerPeer = peer;
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

        while(enet_host_service(gclient->eClient, &event, 0) > 0)
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

        if(gclient->connected)
            play_update(gclient->client, dt, frameId);
        frameId++;
        scheduler_yield();
    }

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
        enet_peer_disconnect(gclient->eServerPeer, 0);
        // this should raise disconnect event and that should terminate client
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

void process_packet(GameClient *gc, void *data, size_t dataLen)
{
    ByteStream stream;
    init_byte_stream(&stream, data, dataLen);
    bool success = true;
    uint16_t cmdId;
    success &= stream_read_uint16(&stream, &cmdId);
    switch(cmdId)
    {
        case Game_Server_Command_Create_DynEntities:
            {
                uint16_t count;
                success &= stream_read_uint16(&stream, &count);
                if(!success) break;
                printf("Client create %hu dynamic entities, size: %zu\n", count, dataLen);
                V3 pos; Quat rot;
                Mat4 mat;
                uint32_t entityId, objectId;
                for(int i = 0; i < count; i++)
                {
                    success &= stream_read_uint32(&stream, &objectId);
                    success &= stream_read_uint32(&stream, &entityId);
                    success &= stream_read_v3(&stream, &pos);
                    success &= stream_read_quat(&stream, &rot);
                    if(!success)
                        break;

                    mat4_trs(&mat, pos, rot, (V3){1.0f, 1.0f, 1.0f});
                    auto dynEnt = pool_allocate(gc->dynEntityPool);
                    assert(dynEnt);
                    dynEnt->id = dynEnt - gc->dynEntityPool;
                    dynEnt->serverId = entityId;
                    dynEnt->entityId = tess_create_entity(gc->world, objectId, &mat);
                    assert(dynEnt->entityId != 0);
                    transform_init(&dynEnt->ntransform, pos, rot);
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
                    if(k == kh_end(gc->serverDynEntityMap)) break;
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
                    printf("destroy %d\n", entityId);
                }
            } break;
        default:
            break;
    }
}

void draw_game_debug_ui(TessClient *client) {
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
}

void play_update(TessClient *client, double dt, uint16_t frameId)
{
    static V2 camRot;

    TessCamera *cam = &client->gameSystem.defaultCamera;
    TessInputSystem *input = &client->inputSystem;

    V3 forward = make_v3(0.0f, 0.0f, 0.1f);
    V3 right;
    quat_v3_mul_dir(&forward, cam->rotation, make_v3(0.0f, 0.0f, 0.01f));
    quat_v3_mul_dir(&right, cam->rotation, make_v3(0.01f, 0.0f, 0.0f));

        if(key(input, AIKE_KEY_W))
            v3_add(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_S))
            v3_sub(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_D))
            v3_add(&cam->position, cam->position, right);
        if(key(input, AIKE_KEY_A))
            v3_sub(&cam->position, cam->position, right);

        v2_add(&camRot, camRot, input->mouseDelta);
        camRot = make_v2(camRot.x, MAX(camRot.y, -90.0f));
        camRot = make_v2(camRot.x, MIN(camRot.y, 90.0f));

    Quat xRot, yRot;
    quat_euler_deg(&xRot, make_v3(0.0f, camRot.y, camRot.x));
    cam->rotation = xRot;

    if(key(input, AIKE_KEY_ESC))
    {
        game_exit(&client->gameClient);
    }

    GameClient *gc = &client->gameClient;
    int count = buf_len(gc->dynEntities);
    for(int i = 0; i < count; i++)
    {
        GameClientDynEntity *dent = gc->dynEntities[i];
        transform_advance(&dent->ntransform, dt, frameId);
        TessEntity *ent = tess_get_entity(gc->world, dent->entityId);
        Quat rot = {1.0f, 0.0f, 0.0f, 0.0f};
        V3 npos; Quat nrot;
        transform_get(&dent->ntransform, &npos, &nrot);
        //printf("pos %f %f %f\n", npos.x, npos.y, npos.z);
        mat4_trs(&ent->objectToWorld, npos, rot, (V3){1.0f, 1.0f, 1.0f});
    }
    draw_game_debug_ui(client);
}

