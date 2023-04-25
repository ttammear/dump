void game_server_update_input_config(ServerInputConfig *si) {
    int tid = 0;
    for(int i = 0; i < SERVER_MAX_INPUTS; i++) {
        if(i < si->inputCount && si->inputs[i].type == SERVER_INPUT_TYPE_KEY 
                && (si->inputs[i].flags&SERVER_INPUT_KEY_FLAG_TRACK_STATE)) {
            si->trackedKeyToInputId[tid++] = i; 
        }    
    }
    for(int i = tid; i < SERVER_MAX_INPUTS; i++) {
        si->trackedKeyToInputId[i] = 255;
    }
    si->trackedKeyCount = tid;
}

void game_server_default_input_config(ServerInputConfig *si) {
    si->inputCount = 7;

    si->inputs[0].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[0].tag = SERVER_INPUT_TAG_WALK_FORWARD;
    si->inputs[0].flags = SERVER_INPUT_KEY_FLAG_TRACK_STATE;

    si->inputs[1].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[1].tag = SERVER_INPUT_TAG_WALK_BACKWARD;
    si->inputs[1].flags = SERVER_INPUT_KEY_FLAG_TRACK_STATE;

    si->inputs[2].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[2].tag = SERVER_INPUT_TAG_WALK_RIGHT;
    si->inputs[2].flags = SERVER_INPUT_KEY_FLAG_TRACK_STATE;

    si->inputs[3].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[3].tag = SERVER_INPUT_TAG_WALK_LEFT;
    si->inputs[3].flags = SERVER_INPUT_KEY_FLAG_TRACK_STATE;

    si->inputs[4].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[4].tag = SERVER_INPUT_TAG_WALK_ACTION;
    si->inputs[4].flags = SERVER_INPUT_KEY_FLAG_TRACK_UP_EVENT;

    si->inputs[5].type = SERVER_INPUT_TYPE_KEY;
    si->inputs[5].tag = SERVER_INPUT_TAG_WALK_JUMP;
    si->inputs[5].flags = SERVER_INPUT_KEY_FLAG_TRACK_STATE;

    si->inputs[6].type = SERVER_INPUT_TYPE_SPHERICAL;
    si->inputs[6].tag = SERVER_INPUT_TAG_WALK_MOUSELOOK;
    si->inputs[6].flags = SERVER_INPUT_SPHERE_FLAG_TRACK_STATE;

    game_server_update_input_config(si);
}


void game_server_init(GameServer *gs)
{
    gs->mapAssetId = NULL;
    coreclr_init(gs->platform, gs);

    game_server_default_input_config(&gs->inputConfig);

    uint32_t size = create_physx_physics(NULL);
    gs->physics = malloc(size);
    create_physx_physics(gs->physics);
    gs->physics->init(gs->physics, false);

    assert(gs->mapAssetId != NULL); // Managed code should set map assetId
    // load map (colliders and other server side stuff)
    gs->mapReference = add_asset_reference(gs->assetSystem, gs->mapAssetId);    
    gs->mapStatus = 1;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 7777;

    gs->eServer = enet_host_create(&address, 32, 3, 0, 0);
    if(NULL == gs->eServer)
    {
        fprintf(stderr, "Failed to create ENet host for server! Assuming server is already up...\n");
    }
    else
    {
        printf("Game server listening at %s:%d\n", "localhost", 7777);
    }

    gs->connectedPeers = NULL;
    gs->dynEntities = NULL;

    gs->updateProgress = 0.0;

    // box from sky
    __auto_type ent = pool_allocate(gs->dynEntityPool);
    ent->id = 0;
    ent->objectId = 10;
    ent->position = (V3){0.0f, 110.0f, 0.0f};
    ent->rotation = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
    ent->scale = (V3){1.0f, 1.0f, 1.0f};
    buf_push(gs->dynEntities, ent);
    TessPhysicsSystem *p = gs->physics;
    auto body = physx_create_dynamic_body(p, ent->position, ent->rotation, 1.0f, (void*)(0x80000000|0));
    physx_attach_box_collider(p, body, V3_ZERO, QUAT_IDENTITY, (V3){0.5f, 0.5f, 0.5f});
    physx_add_body_to_scene(p, body);

    // ground
    ent = pool_allocate(gs->dynEntityPool);
    ent->id = 1;
    ent->objectId = 100;
    ent->position = (V3){0.0f, 0.0f, 0.0f};
    ent->rotation = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
    ent->scale = (V3){100.0f, 0.5f, 100.0f};
    buf_push(gs->dynEntities, ent);
    body = physx_create_static_body(p, ent->position, ent->rotation, (void*)(0x80000000|1));
    physx_attach_box_collider(p, body, V3_ZERO, QUAT_IDENTITY, (V3){50.0f, 0.25f, 50.0f});
    physx_add_body_to_scene(p, body);

    // dummy player
    ent = pool_allocate(gs->dynEntityPool);
    ent->id = 2;
    ent->objectId = 100;
    ent->position = (V3){0.0f, 5.0f, 0.0f};
    ent->rotation = QUAT_IDENTITY;
    ent->scale = (V3){0.5f, 1.8f, 0.5f};
    buf_push(gs->dynEntities, ent);
    auto controller = physx_create_capsule_controller(p, ent->position, (void*)(0x80000000|2));

}

void game_server_destroy(GameServer *gs)
{
    if(NULL != gs->eServer)
    {
        enet_host_destroy(gs->eServer);
        printf("Game server closed!\n");
    }
    buf_free(gs->connectedPeers);
    buf_free(gs->dynEntities);
    gs->physics->destroy(gs->physics);
}

void game_server_send_all_dyn_create(GameServer *gs, ServerPeer *speer)
{
    MsgCreateDynEntities msg;
    int writeId = 0;
    for(int i = 0; i < msg.count; i++) {
        GameServerDynEntity *ent = gs->dynEntities[i];
        auto went = msg.entities + writeId;
        went->objectId = ent->objectId;
        went->entityId = ent->id;
        went->position = ent->position;
        went->scale = ent->scale;
        went->rotation = ent->rotation;

        writeId ++;
        if(writeId >= ARRAY_COUNT(msg.entities)) {
            msg.count = writeId;
            writeId = 0;
            server_message_send(gs->tempStack, &msg, speer->ePeer);
        }
    }
    if(writeId > 0) {
        msg.count = writeId;
        server_message_send(gs->tempStack, &msg, speer->ePeer);
    }
}

void game_server_send_dyn_create(GameServer *gs, ServerPeer *speer, GameServerDynEntity *entity)
{
    MsgCreateDynEntities msg;
    msg.count = 1;
    msg.entities[0].objectId = entity->objectId;
    msg.entities[0].entityId = entity->id;
    msg.entities[0].position = entity->position;
    msg.entities[0].scale = entity->scale;
    msg.entities[0].rotation = entity->rotation;
    server_message_send(gs->tempStack, &msg, speer->ePeer);
}

void game_server_send_dyn_destroy(GameServer *gs, ServerPeer *speer, GameServerDynEntity *entity)
{
    MsgDestroyDynEntities msg;
    msg.count = 1;
    msg.entities[0].entityId = entity->id;
    server_message_send(gs->tempStack, &msg, speer->ePeer);
}

void game_server_send_player_reflection(GameServer *gs, ServerPeer *peer) { 
    ServerPlayer *p = &peer->player;
    MsgPlayerReflection msg;
    auto entity = gs->dynEntityPool + p->dynId;
    msg.position = entity->position;
    server_message_send(gs->tempStack, &msg, peer->ePeer);
}

void game_server_send_player_spawn(GameServer *gs, ServerPeer *speer) {
    MsgPlayerSpawn msg;
    ServerPlayer *p = &speer->player;
    auto dyn = &gs->dynEntityPool[p->dynId];
    if(p->status != Server_Player_Status_Created) {
        fprintf(stderr, "Trying to send spawn message to player when not spawned!\r\n");
        return;
    }
    msg.position = dyn->position;
    server_message_send(gs->tempStack, &msg, speer->ePeer);
}

void game_server_send_player_despawn(GameServer *gs, ServerPeer *speer) {
    MsgPlayerDespawn msg;
    ServerPlayer *p = &speer->player;
    auto dyn = &gs->dynEntityPool[p->dynId];
    if(p->status != Server_Player_Status_Created) {
        fprintf(stderr, "Trying to send despawn message to player when stil spawned!\r\n");
        return;
    }
    server_message_send(gs->tempStack, &msg, speer->ePeer);
}

GameServerDynEntity* game_server_create_dyn(GameServer *gs, uint32_t objectId, V3 pos, Quat rot)
{
    auto ent = pool_allocate(gs->dynEntityPool);
    assert(ent);
    ent->id = ent - gs->dynEntityPool;
    ent->objectId = objectId;
    ent->position = pos;
    ent->rotation = rot;
    ent->updateSeq = 0;
    buf_push(gs->dynEntities, ent);
    int count = buf_len(gs->connectedPeers);
    for(int i = 0; i < count; i++)
    {
        game_server_send_dyn_create(gs, gs->connectedPeers[i], ent);
    }
    return ent;
}

void game_server_destroy_dyn(GameServer *gs, uint32_t id)
{
    int count = buf_len(gs->connectedPeers);
    GameServerDynEntity *entity = gs->dynEntityPool + id;
    for(int i = 0; i < count; i++)
    {
        game_server_send_dyn_destroy(gs, gs->connectedPeers[i], entity);
    }
    int idx = buf_find_idx(gs->dynEntities, entity);
    assert(idx >= 0);
    buf_remove_at(gs->dynEntities, idx);
    pool_free(gs->dynEntityPool, entity);
}

void game_server_send_dyn_updates(GameServer *gs, ServerPeer *speer)
{
    MsgUpdateDynEntities msg;
    uint32_t count = buf_len(gs->dynEntities);
    uint32_t writeId = 0;
    for(int i = 0; i < count; i++) {
        auto ent = gs->dynEntities[i];
        auto went = msg.entities + writeId;
        went->entityId = ent->id;
        went->sequence = ent->updateSeq;
        went->position = ent->position;
        went->rotation = ent->rotation;

        writeId++;
        if(writeId >= ARRAY_COUNT(msg.entities)) {
            msg.count = writeId;
            writeId = 0;
            server_message_send(gs->tempStack, &msg, speer->ePeer);
        }
    }
    if(writeId > 0) {
        msg.count = writeId;
        writeId = 0;
        server_message_send(gs->tempStack, &msg, speer->ePeer);
    }
}

void server_send_property(GameServer *gs, ServerPeer *sp, uint8_t prop) {
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4);
    ByteStream stream;
    init_byte_stream(&stream, mem, 65536);
    stream_write_uint16(&stream, Game_Server_Command_Server_Property_Response);
    stream_write_uint8(&stream, prop);
    bool send = false;
    switch(prop) {
        case Game_Server_Property_Map_AssetId:
            {
                stream_write_uint8(&stream, Game_Server_Data_Type_String);
                assert(gs->mapAssetId->len < 65536);
                stream_write_uint16(&stream, gs->mapAssetId->len);
                stream_write_str(&stream, gs->mapAssetId->cstr, gs->mapAssetId->len);
                send = true;
            } break;
    }
    if(send) {
        ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(sp->ePeer, 1, packet);
    }
    stack_pop(gs->tempStack);
}

void server_send_input_config(GameServer *gs, ServerPeer *sp) {
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4);
    MsgServerInputConfig ic;
    ic.count = gs->inputConfig.inputCount;
    for(int i = 0; i < ic.count; i++) {
        ic.inputs[i].type = gs->inputConfig.inputs[i].type;
        ic.inputs[i].tag = gs->inputConfig.inputs[i].tag;
        ic.inputs[i].flags = gs->inputConfig.inputs[i].flags;
    }
    server_message_send(gs->tempStack, &ic, sp->ePeer);
}

void server_queue_input_event(GameServer *gs, ServerPeer *sp, ServerPlayerInputEvent* e) {
    // TODO: implement this
    if(sp->player.input.pendingEventCount < SERVER_PLAYER_INPUT_MAX_EVENTS) {
        uint32_t id = sp->player.input.pendingEventCount++;
        // TODO: additional checks ?
        sp->player.input.pendingEvents[id] = *e;
    }
}

void game_server_create_player(GameServer *gs, ServerPlayer *sp) {
    assert(sp->status == Server_Player_Status_None);
    //V3 pos = (V3){-1500.0f, 50.0f, -900.0f};
    V3 pos = (V3) {-1245.153320, 100.902983, -1070.290527};
    TessPhysicsSystem *p = gs->physics;

    GameServerDynEntity *ent = game_server_create_dyn(gs, 200, pos, QUAT_IDENTITY);
    ent->scale = (V3){0.5f, 1.8f, 0.5f};
    first_person_controller_init(&sp->fpc, p, pos, (void*)(intptr_t)(0x80000000|ent->id));
    sp->status = Server_Player_Status_Created;
    sp->dynId = ent->id;

    // reset input
    sp->input.sphericalState = (V2){0.0f, 0.0f};
    memset(sp->input.keyStates, 0, sizeof(sp->input.keyStates));
    sp->input.pendingEventCount = 0;

}

void game_server_destroy_player(GameServer *gs, ServerPlayer *sp) {
    TessPhysicsSystem *p = gs->physics;

    //game_server_destroy_dyn(gs, entity);
    assert(sp->status == Server_Player_Status_Created);
    physx_destroy_capsule_controller(p, sp->fpc.characterController);
    sp->fpc.characterController = NULL;
    game_server_destroy_dyn(gs, sp->dynId);
    sp->dynId = -1;

    sp->status = Server_Player_Status_None;
}

void server_process_packet(GameServer *gs, ServerPeer *sp, void* data, size_t dataLen) {
    ByteStream stream;
    init_byte_stream(&stream, data, dataLen);
    bool success = true;
    uint16_t cmdId;
    success &= stream_read_uint16(&stream, &cmdId);
    if(!success) return;
    switch(cmdId) {
        case Game_Client_Command_Get_Server_Properties:
            {
                // TODO: use bitfield to set the property dirty and then periodically send the dirty properties (much easier to rate-limit in case of malicious/abnormal client)
                printf("Game client requested server properties\n");
                uint8_t count;
                success &= stream_read_uint8(&stream, &count);
                if(!success) break;
                for(int i = 0; i < count; i++) {
                    uint8_t prop = 0;
                    success &= stream_read_uint8(&stream, &prop);
                    if(!success) return;
                    server_send_property(gs, sp, prop);
                }
            } break;
        case Game_Client_Command_World_Ready:
            {
                if(sp->player.status == Server_Player_Status_None) {
                    game_server_create_player(gs, &sp->player);
                    game_server_send_player_spawn(gs, sp);
                }
                if((sp->flags&Server_Peer_Flag_World_Ready) == 0) {
                    game_server_send_all_dyn_create(gs, sp);
                    sp->flags |= Server_Peer_Flag_World_Ready;
                }
            } break;
        case Game_Client_Command_Get_Server_Inputs:
            {
                server_send_input_config(gs, sp);
            } break;
        case Game_Client_Command_Update_Reliable:
            {
                // 2 byte tickId
                // 1 byte clientEventCount
                // [ client events ]
                uint16_t tickId;
                uint8_t eventCount;
                ServerPlayerInputEvent e;
                ServerInputConfig *ic = &gs->inputConfig;
                success &= stream_read_uint16(&stream, &tickId);
                success &= stream_read_uint8(&stream, &eventCount);
                for(int i = 0; i < eventCount && success; i++) {
                    uint8_t eventId;
                    uint8_t inputId;
                    success &= stream_read_uint8(&stream, &eventId);
                    if(!success) break;
                    switch(eventId) {
                        case Game_Client_Event_Key_Down:
                        case Game_Client_Event_Key_Up:
                            e.type = eventId == Game_Client_Event_Key_Up ? Server_Player_Input_Event_Key_Up : Server_Player_Input_Event_Key_Down;
                            success &= stream_read_uint8(&stream, &inputId);
                            if(success && inputId < ic->inputCount) {
                                e.keyEdge.serverInputId = inputId;
                                server_queue_input_event(gs, sp, &e);
                            }
                            break;
                        default:
                            goto reliable_update_done;
                    }
                }
            } 
reliable_update_done:
            break;
        case Game_Client_Command_Update_Unreliable:
            {
                // 2 byte tickId
                // 1 byte input bitfield length
                // [ bitfield bytes ]
                uint16_t tickId;
                V2 camRot;
                ServerInputConfig *ic = &gs->inputConfig;
                ServerPlayerInput *spi = &sp->player.input;
                uint8_t bcount;
                success &= stream_read_uint16(&stream, &tickId);
                success &= stream_read_uint8(&stream, &bcount);
                uint8_t safeBcount = MIN(SERVER_MAX_INPUTS/8, bcount);
                uint8_t bitfield[SERVER_MAX_INPUTS/8];
                for(int i = 0; i < safeBcount && success; i++){ 
                    success &= stream_read_uint8(&stream, bitfield + i);
                }
                success &= stream_read_v2(&stream, &camRot);
                if(success) {
                    for(int i = 0; i < ic->trackedKeyCount; i++) {
                        uint32_t b = (bitfield[i/8]>>(i%8)) & 1;
                        uint32_t inputId = ic->trackedKeyToInputId[i];
                        if(inputId < ic->inputCount) {
                            spi->keyStates[inputId] = b;
                        }
                    }
                    spi->sphericalState = camRot;
                }
            } break;
        default:
            return;
    }
}

void game_server_load_map(GameServer *gs, TessMapAsset *map) {
    tess_reset_world(&gs->gameSystem);
    for(int i = 0; i < map->mapObjectCount; i++) {
        TessMapObject *obj = map->objects + i;
        tess_register_object(&gs->gameSystem, obj->objectid, obj->assetId);
    }
    for(int i = 0; i < map->mapEntityCount; i++) {
        Mat4 modelToWorld;
        TessMapEntity *ent = map->entities + i;
        mat4_trs(&modelToWorld, ent->position, ent->rotation, ent->scale);
        tess_create_entity(&gs->gameSystem, ent->objectId, ent->position, ent->rotation, ent->scale, &modelToWorld);
    }
}

void game_server_update(GameServer *gs, double dt)
{
    PROF_BLOCK();
    if(NULL == gs->eServer)
        return;

    // TODO: just use coroutine ?
    if(gs->mapStatus == 1) {
        TessAssetStatus status = tess_get_asset_status(gs->assetSystem, gs->mapAssetId);
        if(status == Tess_Asset_Status_Fail || status == Tess_Asset_Status_Loaded) {
            TessAsset *map = tess_get_asset(gs->assetSystem, gs->mapAssetId);
            if(map == NULL) {
                fprintf(stderr, "Map asset could not be loaded!\r\n");
                assert(0);
            } else {
                gs->mapStatus = 2;
                game_server_load_map(gs, (TessMapAsset*)map);
            }
        }
        return;
    }

    static double physicsTime;
    const double physicsTimestep = 1.0/60.0;

    ENetEvent event;
    while(enet_host_service(gs->eServer, &event, 0) > 0)
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    ServerPeer *speer = pool_allocate(gs->peerPool);
                    if(NULL != speer)
                    {
                        printf("New game client from %x:%u\n", event.peer->address.host, event.peer->address.port);
                        event.peer->data = speer;
                        speer->ePeer = event.peer;
                        speer->flags = 0;
                        buf_push(gs->connectedPeers, speer);
                    }
                    else
                    {
                        printf("Server full, connect denied!\n");
                        enet_peer_disconnect(event.peer, 0);
                    }
                } break;
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    //printf("Server received packet. Size: %zu\n", event.packet->dataLength);
                    ServerPeer *speer = (ServerPeer*)event.peer->data;
                    server_process_packet(gs, speer, event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                {
                    ServerPeer *speer = (ServerPeer*)event.peer->data;
                    printf("Game client disconnected: %x:%u\n", event.peer->address.host, event.peer->address.port);

                    if(speer->player.status == Server_Player_Status_Created) {
                        game_server_destroy_player(gs, &speer->player);
                        game_server_send_player_despawn(gs, speer);
                    }

                    int idx = buf_find_idx(gs->connectedPeers, speer);
                    if(idx != -1)
                    {
                        buf_remove_at(gs->connectedPeers, idx);
                        pool_free(gs->peerPool, speer);
                    }
                    event.peer->data = NULL;
                } break;
            default: 
                break;
        }
    }

    // process player input events
    for(int i = 0; i < buf_len(gs->connectedPeers); i++) {
        auto peer = gs->connectedPeers[i];
        auto spi = &peer->player.input;
        for(int j = 0; j < spi->pendingEventCount; j++) {
            auto e = &spi->pendingEvents[j];
            switch(e->type) {
                case Server_Player_Input_Event_Key_Up:
                    break;
                case Server_Player_Input_Event_Key_Down:
                    break;
                default:
                    break;
            }
        }
        spi->pendingEventCount = 0;
    }

    TessPhysicsSystem *p = gs->physics;

    // move players
    for(int i = 0; i < buf_len(gs->connectedPeers); i++) {
        auto peer = gs->connectedPeers[i];
        auto sp = &peer->player;
        auto spi = &peer->player.input;
        if(peer->player.status == Server_Player_Status_Created) {

            FirstPersonControls ctrl = {
                .forward = spi->keyStates[0] != 0,
                .backward = spi->keyStates[1] != 0,
                .right = spi->keyStates[2] != 0,
                .left = spi->keyStates[3] != 0,
                .jump = spi->keyStates[5] != 0,
                .yawRad = spi->sphericalState.x,
            };


            first_person_update(&peer->player.fpc, dt, &ctrl);
        }
    }

    // simulate physics
    physicsTime += dt;
    while(physicsTime >= physicsTimestep) {
        physx_simulate(p, physicsTimestep);
        physicsTime -= physicsTimestep;

        V3 pos[10];
        Quat rot[10];
        void *usrPtr[10];
        uint32_t c = physx_get_active_bodies(p, pos, rot, usrPtr, 10);
        //printf("%d active bodies\r\n", c);
        for(int i = 0; i < c; i++) {
            uint32_t id = (uint32_t)usrPtr[i];
            if(id&0x80000000) {
                id &= 0x7FFFFFFF;
                gs->dynEntities[id]->position = pos[i];
                gs->dynEntities[id]->rotation = rot[i];
            }
            //printf("%d (%.2f %.2f %.2f) (%.2f %.2f %.2f %.2f)\r\n", i, pos[i].x, pos[i].y, pos[i].z, rot[i].w, rot[i].x, rot[i].y, rot[i].z);
        }
        c = physx_get_controllers(p, pos, usrPtr, 10);
        for(int i = 0; i < c; i++) {
            uint32_t id = (uint32_t)usrPtr[i];
            if(id&0x80000000) {
                id &= 0x7FFFFFFF;
                gs->dynEntities[id]->position = pos[i];
            }
        }
    }

    const double rate = 1.0/10.0;
    gs->updateProgress += dt;
    static float elapsed;
    elapsed += dt;
    while(gs->updateProgress >= rate)
    {
        int count = buf_len(gs->connectedPeers);
        for(int i = 0; i < count; i++)
        {
            ServerPeer *speer = gs->connectedPeers[i];

            game_server_send_dyn_updates(gs, speer);
            game_server_send_player_reflection(gs, speer);

            enet_host_flush(gs->eServer);
        }

        count = buf_len(gs->dynEntities);
        for(int i = 0; i < count; i++)
        {
            gs->dynEntities[i]->updateSeq++;
        }

        gs->updateProgress -= rate;
    }
}

static void message_get_server_properties(MsgGetServerProperties *msg, void *usrPtr) {
}
static void message_client_world_ready(MsgClientWorldReady *msg, void *usrPtr) {
}
static void message_get_server_inputs(MsgGetServerInputs *msg, void *usrPtr) {
}
static void message_update_reliable(MsgUpdateReliable *msg, void *usrPtr) {
}
static void message_update_unreliable(MsgUpdateUnreliable *msg, void *usrPtr) {
}

static void server_send_data(uint8_t *data, uint32_t len, void *usrPtr) {
    ENetPeer *p = (ENetPeer*)usrPtr;
    ENetPacket *packet = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(p, 1, packet);
}
