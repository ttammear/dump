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
    uint8_t destroyCount;
    uint32_t entityIds[];
    
Command update dynamic entities
   uint8_t updateCount;
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
    ent->id = 333;
    ent->objectId = 2;
    ent->position = (V3){1.0f, 2.0f, 3.0f};
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

void game_server_create_dynamic_objects(GameServer *gs, ServerPeer *speer)
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
            printf("send pfff %d %d\n", writtenCount, size);
            enet_peer_send(speer->ePeer, 1, packet);

            writtenCount = 0;
            if(!success) i--;// if did not fit force to reiterate current (we flushed so it fits now)
        }
    }

    stack_pop(gs->tempStack);
}

void game_server_update_dynamic_objects(GameServer *gs, ServerPeer *speer)
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

void game_server_update(GameServer *gs, double dt)
{
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
                        speer->seq = 0;
                        buf_push(gs->connectedPeers, speer);

                        game_server_create_dynamic_objects(gs, speer);
                    }
                    else
                    {
                        printf("Server full, connect denied!\n");
                        enet_peer_disconnect(event.peer, 0);
                    }
                }break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Received packet. Size: %zu\n", event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
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
        //printf("update\n");
        int count = buf_len(gs->connectedPeers);
        for(int i = 0; i < count; i++)
        {
            ServerPeer *speer = gs->connectedPeers[i];
#pragma pack(push, 1)
            struct {
                uint16_t seq;
                V3 pos;
                Quat rot;
            } data;
#pragma pack(pop)
            memset(&data, 0, sizeof(data));
            data.seq = (uint16_t)speer->seq;
            data.pos.x = sinf(elapsed) * 10.0f;
            //ENetPacket *packet = enet_packet_create(&data, sizeof(data), 0);
            //enet_peer_send(speer->ePeer, 0, packet);
            gs->dynEntities[0]->position.x = data.pos.x;

            game_server_update_dynamic_objects(gs, speer);

            enet_host_flush(gs->eServer);
            speer->seq++;
        }

        count = buf_len(gs->dynEntities);
        for(int i = 0; i < count; i++)
        {
            gs->dynEntities[i]->updateSeq++;
        }

        gs->updateProgress -= rate;
    }
}
