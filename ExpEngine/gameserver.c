/*
Packets contain commands
    uint16_t commandId;
    uint8_t data[];

Command load map
    uint8_t assetIdLen;
    char assetId[];

Command reset world - unloads everything (map, entities, all)
    [no data]

Command create dynamic entities
    uint16_t createCount;
    struct
    {
        uint32_t objectId;
        uint32_t entityId;
        V3 position;
        Quat rotation;
    } entities[];

Command destroy dynamic entities
    uint16_t destroyCount;
    uint32_t entityIds[];
    
Command update dynamic entities
   uint16_t updateCount;
   struct
   {
        uint32_t entityId;
        uint16_t seq;
        V3 position;
        Quat rotation;
   } entities[]; 

*/


void game_server_init(GameServer *gs)
{
    coreclr_init(gs->platform, gs);

    assert(gs->mapAssetId); // Managed code should set map assetId

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 7777;

    gs->eServer = enet_host_create(&address, 32, 2, 0, 0);
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

    __auto_type ent = pool_allocate(gs->dynEntityPool);
    ent->id = 0;
    ent->objectId = 2;
    ent->position = (V3){0.0f, 0.0f, 0.0f};
    ent->rotation = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
    buf_push(gs->dynEntities, ent);
    ent = pool_allocate(gs->dynEntityPool);
    ent->id = 1;
    ent->objectId = 2;
    ent->position = (V3){0.0f, 0.0f, 0.0f};
    ent->rotation = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
    buf_push(gs->dynEntities, ent);
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
}

void game_server_send_all_dyn_create(GameServer *gs, ServerPeer *speer)
{
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4); 
        
    ByteStream stream;
    init_byte_stream(&stream, mem, 65536);
    stream_write_uint16(&stream, Game_Server_Command_Create_DynEntities);
    uint32_t countLoc = stream_get_offset(&stream);
    stream_write_uint16(&stream, 0);

    uint32_t count = buf_len(gs->dynEntities);
    uint32_t writtenCount = 0;
    uint32_t size;
    for(int i = 0; i < count; i++)
    {
        bool success = true;
        GameServerDynEntity *ent = gs->dynEntities[i];
        success &= stream_write_uint32(&stream, ent->objectId);
        success &= stream_write_uint32(&stream, ent->id);
        success &= stream_write_v3(&stream, ent->position);
        success &= stream_write_quat(&stream, ent->rotation);
        if(success)
        {
            size = stream_get_offset(&stream);
            writtenCount++;
        }
        if(!success || i+1 >= count) // if did not fit or last element, send packet
        {
            stream_go_to_offset(&stream, countLoc);
            assert(writtenCount < 65536);
            stream_write_uint16(&stream, writtenCount);

            ENetPacket *packet = enet_packet_create(stream.start, size, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(speer->ePeer, 1, packet);

            writtenCount = 0;
            if(!success) i--;// if did not fit force to reiterate current (we flushed so it fits now)
        }
    }

    stack_pop(gs->tempStack);
}

void game_server_send_dyn_create(GameServer *gs, ServerPeer *speer, GameServerDynEntity *entity)
{
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4);

    ByteStream stream;
    init_byte_stream(&stream, mem, 65536);
    stream_write_uint16(&stream, Game_Server_Command_Create_DynEntities);
    stream_write_uint16(&stream, 1);
    stream_write_uint32(&stream, entity->objectId);
    stream_write_uint32(&stream, entity->id);
    stream_write_v3(&stream, entity->position);
    stream_write_quat(&stream, entity->rotation);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(speer->ePeer, 1, packet);

    stack_pop(gs->tempStack);
}

void game_server_send_dyn_destroy(GameServer *gs, ServerPeer *speer, GameServerDynEntity *entity)
{
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4);

    ByteStream stream;
    init_byte_stream(&stream, mem, 65536);
    stream_write_uint16(&stream, Game_Server_Command_Destroy_DynEntities);
    stream_write_uint16(&stream, 1);
    stream_write_uint32(&stream, entity->id);
    ENetPacket *packet = enet_packet_create(stream.start, stream_get_offset(&stream), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(speer->ePeer, 1, packet);

    stack_pop(gs->tempStack); 
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

void game_server_destroy_dyn(GameServer *gs, GameServerDynEntity *entity)
{
    int count = buf_len(gs->connectedPeers);
    for(int i = 0; i < count; i++)
    {
        game_server_send_dyn_destroy(gs, gs->connectedPeers[i], entity);
    }
    int index = buf_find_idx(gs->dynEntities, entity);
    assert(index != -1);
    buf_remove_at(gs->dynEntities, index);
    pool_free(gs->dynEntityPool, entity);
}

void game_server_send_dyn_updates(GameServer *gs, ServerPeer *speer)
{
    uint8_t *mem = stack_push(gs->tempStack, 65536, 4);

    ByteStream stream;
    init_byte_stream(&stream, mem, 65536);
    stream_write_uint16(&stream, Game_Server_Command_Update_DynEntities);
    uint32_t countLoc = stream_get_offset(&stream);
    stream_write_uint16(&stream, 0);

    uint32_t count = buf_len(gs->dynEntities);
    uint32_t writtenCount = 0;
    uint32_t size;
    for(int i = 0; i < count; i++)
    {
        bool success = true;
        GameServerDynEntity *ent = gs->dynEntities[i];
        // TODO: write data
        success &= stream_write_uint32(&stream, ent->id);
        success &= stream_write_uint16(&stream, ent->updateSeq);
        success &= stream_write_v3(&stream, ent->position);
        success &= stream_write_quat(&stream, ent->rotation);
        if(success)
        {
            size = stream_get_offset(&stream);
            writtenCount++;
        }
        if(!success || i+1 >= count)
        {
            stream_go_to_offset(&stream, countLoc);
            assert(writtenCount < 65536);
            stream_write_uint16(&stream, writtenCount);

            ENetPacket *packet = enet_packet_create(stream.start, size, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(speer->ePeer, 0, packet);

            writtenCount = 0;
            if(!success) i--; 
        }
    }

    stack_pop(gs->tempStack);
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
                if((sp->flags&Server_Peer_Flag_World_Ready) == 0) {
                    game_server_send_all_dyn_create(gs, sp);
                    sp->flags |= Server_Peer_Flag_World_Ready;
                }
            } break;
        default:
            return;
    }
}

void game_server_update(GameServer *gs, double dt)
{
    PROF_BLOCK();
    if(NULL == gs->eServer)
        return;

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
                }break;
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    printf("Received packet. Size: %zu\n", event.packet->dataLength);
                    ServerPeer *speer = (ServerPeer*)event.peer->data;
                    server_process_packet(gs, speer, event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                {
                    ServerPeer *speer = (ServerPeer*)event.peer->data;
                    printf("Game client disconnected: %x:%u\n", event.peer->address.host, event.peer->address.port);
                    int idx = buf_find_idx(gs->connectedPeers, speer);
                    if(idx != -1)
                    {
                        buf_remove_at(gs->connectedPeers, idx);
                        pool_free(gs->peerPool, speer);
                    }
                    event.peer->data = NULL;
                }
                break;
            default: 
                break;
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

            gs->dynEntities[0]->position.x = sinf(elapsed)*10.0f;
            gs->dynEntities[1]->position.z = cosf(elapsed)*10.0f;

            game_server_send_dyn_updates(gs, speer);

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
