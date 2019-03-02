enum ClientEntityFlag
{
    Client_Entity_Flag_Created = 1 << 0,
};


void tess_create_editor_server(struct TessEditorServer *eserver, struct TessFixedArena *arena);
void tess_destroy_editor_server(struct TessEditorServer *eserver);
uint32_t tess_editor_server_create_entity(struct TessEditorServer *server, uint32_t objectId, struct V3 position);
struct TessEditorServerEntity* tess_editor_server_get_entity(struct TessEditorServer *server, uint32_t entityId);

void tess_server_init(struct TessServer *server, AikePlatform *platform)
{
    memset(server, 0, sizeof(struct TessServer));
    server->platform = platform;

    fixed_arena_init(platform, &server->arena, 3 * 1024 * 1024); 
    // init tempstack
    void *stackMem = arena_push_size(&server->arena, 1024*1024, 1);
    stack_init(&server->tempStack, stackMem, 1024*1024);

    // Init strings
    //
    tess_strings_init(&server->strings, &server->arena);

    // Init file system
    //
    server->fileSystem.platform = platform;
    // Virtual table tells file system where to pass the data once it's done loading
    server->fileSystem.pipeline_vtbl[Tess_File_Pipeline_None] = NULL;
    tess_file_system_init(&server->fileSystem, &server->arena);

    // Init asset system
    //
    server->assetSystem.fileSystem = &server->fileSystem;
    server->assetSystem.tstrings = &server->strings;
    server->assetSystem.renderer = NULL;
    tess_asset_system_init(&server->assetSystem, &server->arena);

    // Init editor server
    //
    server->editorServer.tstrings = &server->strings;
    server->editorServer.platform = platform;
    server->editorServer.assetSystem = &server->assetSystem;
    server->editorServer.arena = &server->arena;

    // Init game server
    //
    POOL_FROM_ARENA(server->gameServer.peerPool, &server->arena, TESS_SERVER_MAX_PEERS);
    POOL_FROM_ARENA(server->gameServer.dynEntityPool, &server->arena, TESS_SERVER_MAX_DYN_ENTITIES);
    server->gameServer.tempStack = &server->tempStack;
    server->gameServer.platform = server->platform;
    game_server_init(&server->gameServer);

}

void tess_server_destroy(struct TessServer *server)
{
    game_server_destroy(&server->gameServer);

    tess_destroy_editor_server(&server->editorServer);

    // Destroy file system
    
    // Destroy asset system
    tess_asset_system_destroy(&server->assetSystem);

    // Destroy strings
    tess_strings_destroy(&server->strings);

    assert(server->tempStack.start == server->tempStack.cur); // something was not freed on temp stack
}

void tess_create_editor_server(struct TessEditorServer *eserver, struct TessFixedArena *arena)
{
    eserver->tcpServer = eserver->platform->tcp_listen(eserver->platform, 7879);
    if(NULL != eserver->tcpServer)
    {
        printf("Editor server listening on port 7879\n");
    }
    else
    {
        fprintf(stderr, "Could not listen on port 7879! Is some other application using it?\n");
    }
    eserver->activeEntities = NULL;
    eserver->activeClients = NULL;
    POOL_FROM_ARENA(eserver->clientPool, arena, TESS_EDITOR_SERVER_MAX_CLIENTS);
    POOL_FROM_ARENA(eserver->entityPool, arena, TESS_MAX_ENTITIES);
    memset(&eserver->objectTable, 0, sizeof(eserver->objectTable));

    //    TStr *intr = tess_intern_string(eserver->tstrings, "First/object0");
    //   eserver->objectTable[1] = intr;

    TessAssetSystem *as = eserver->assetSystem;
    TessStrings *strings = eserver->tstrings;
    TStr *sponzaStr = tess_intern_string(strings, "Sponza/Sponza");
    tess_queue_asset(as, sponzaStr); 
    while(!tess_is_asset_loaded(as, sponzaStr)) {
        scheduler_yield();
    }
    TessAsset *mapAsset = tess_get_asset(as, sponzaStr);
    assert(mapAsset);
    assert(mapAsset->type == Tess_Asset_Map);
    TessMapAsset *map = (TessMapAsset*)mapAsset;

    // load map!
    for(int i = 0; i < map->mapObjectCount; i++) {
        TessMapObject *obj = map->objects + i;
        eserver->objectTable[obj->objectid] = map->objects[i].assetId;
    }
    for(int i = 0; i < map->mapEntityCount; i++) {
        TessMapEntity *ent = map->entities + i;
        uint32_t eid = tess_editor_server_create_entity(eserver, ent->objectId, ent->position);
        auto sent = tess_editor_server_get_entity(eserver, eid);
        sent->scale = ent->scale;
    }
}

void tess_destroy_editor_server(struct TessEditorServer *eserver)
{
    buf_free(eserver->activeEntities);

    // TODO: platform should do this automatically on tcp_close_server
    uint32_t count = buf_len(eserver->activeClients);
    for(int i = 0; i < count; i++)
    {
        eserver->platform->tcp_close_connection(eserver->platform, eserver->activeClients[i]->connection);
    }
    buf_free(eserver->activeClients);
    
    if(NULL != eserver->tcpServer)
        eserver->platform->tcp_close_server(eserver->platform, eserver->tcpServer);
}

void tess_editor_server_send_command(struct TessEditorServer *server, struct TessEditorServerClient *client, uint8_t *data, uint32_t size)
{
    assert(size > 0 && size < TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    server->platform->tcp_send(server->platform, client->connection, data, size); 
}

static inline void tess_editor_server_send_stream(struct TessEditorServer *server, struct TessEditorServerClient *client, struct ByteStream *stream)
{
    tess_editor_server_send_command(server, client, stream->start, stream_get_offset(stream));
}

uint32_t tess_editor_server_create_entity(struct TessEditorServer *server, uint32_t objectId, struct V3 position)
{
    struct TessEditorServerEntity *edEnt = pool_allocate(server->entityPool);
    if(edEnt == NULL)
    {
        fprintf(stderr, "Entity pool full! Entity creation ignored!\n");
        return 0;
    } 
    if(objectId >= TESS_MAX_OBJECTS)
    {
        fprintf(stderr, "Can't create entity: objectId out of range!\n");
        return 0;
    }
    edEnt->position = position;
    edEnt->eulerRotation = make_v3(0.0f, 0.0f, 0.0f);
    edEnt->scale = make_v3(1.0f, 1.0f, 1.0f);
    edEnt->objectId = objectId;
    edEnt->id = edEnt - server->entityPool;
    edEnt->version = 0;

    buf_push(server->activeEntities, edEnt);
    return edEnt->id;
}

void tess_editor_server_destroy_entity(struct TessEditorServer *server, uint32_t entityId)
{
    struct TessEditorServerEntity *edEnt = tess_editor_server_get_entity(server, entityId);
    int idx;
    if(edEnt != NULL && (idx = buf_find_idx(server->activeEntities, edEnt)) != -1)
    { 
        buf_remove_at(server->activeEntities, idx);
        pool_free(server->entityPool, edEnt);

        ARRAY_COMMAND_START(stream);
        ARRAY_COMMAND_HEADER_END(stream);
        bool written = true;
        written &= stream_write_uint32(&stream, entityId);
        ARRAY_COMMAND_INC(stream);
        ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Destroy_Entities, 1);

        int count = buf_len(server->activeClients);
        for(int i = 0 ; i < count; i++)
        {
            auto client = server->activeClients[i];
            tess_editor_server_send_stream(server, client, &stream);
            CLEAR_BIT(client->entityFlags[entityId], Client_Entity_Flag_Created);
        }
    }
}

struct TessEditorServerEntity* tess_editor_server_get_entity(struct TessEditorServer *server, uint32_t entityId)
{
    if(entityId >= pool_cap(server->entityPool))
        return NULL;
    return server->entityPool + entityId;
}

void editor_server_send_debug_message(struct TessEditorServer *server, struct TessEditorServerClient *client, char *msg)
{
    uint32_t msgLen = strlen(msg);
    uint8_t buf[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];
    struct ByteStream stream;
    init_byte_stream(&stream, buf, TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    bool success = stream_write_uint16(&stream, 2 + 2 + 2 + msgLen);
    success &= stream_write_uint16(&stream, Editor_Server_Command_Debug_Message);
    success &= stream_write_uint16(&stream, msgLen);
    success &= stream_write_str(&stream, msg, msgLen);
    if(success)
        tess_editor_server_send_command(server, client, buf, stream_get_offset(&stream));
}

bool tess_editor_server_new_client(struct TessEditorServer *eserver, AikeTCPConnection *con)
{
    struct TessEditorServerClient *client = pool_allocate(eserver->clientPool);
    if(client == NULL)
        return false;
    client->connection = con;
    client->objectTableVersion = 0;
    editor_command_buf_reset(&client->cmdBuf, eserver, client, true);
    memset(&client->entityVersions, 0, sizeof(client->entityVersions));
    memset(&client->entityFlags, 0, sizeof(client->entityVersions));
    buf_push(eserver->activeClients, client);
    editor_server_send_debug_message(eserver, client, "Hello client!");
    return true;
}

void send_all_object_definitions(struct TessEditorServer *server, struct TessEditorServerClient *client)
{
    bool noneSent = true;
    uint32_t numEnts = 0;
    ARRAY_COMMAND_START(stream);
    ARRAY_COMMAND_HEADER_END(stream);
    for(int i = 0; i < ARRAY_COUNT(server->objectTable); i++)
    {
        TStr *assetIdStr = server->objectTable[i];
        if(assetIdStr != NULL)
        {
            bool res = true;
            res &= stream_write_uint32(&stream, i);
            assert(assetIdStr->len < 256);
            res &= stream_write_uint8(&stream, (uint8_t)assetIdStr->len);
            res &= stream_write_str(&stream, assetIdStr->cstr, assetIdStr->len);

            if(!res) // didnt fit, packet flushed, go again
            {
                ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Define_Objects, numEnts);
                tess_editor_server_send_command(server, client, stream.start, stream_get_offset(&stream)); 
                ARRAY_COMMAND_RESET(stream);
                noneSent = false;
                numEnts = 0;
                i--; // forces to process the entry again (because it didnt fit)
            }
            else
            {
                ARRAY_COMMAND_INC(stream);
                numEnts++;
            }
        }
    }
    if(numEnts > 0 || noneSent)
    {
        ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Define_Objects, numEnts);
        tess_editor_server_send_command(server, client, stream.start, stream_get_offset(&stream)); 
    }
}

void client_send_uncreated_entities(struct TessEditorServer *server, struct TessEditorServerClient *client)
{
    ARRAY_COMMAND_START(stream);
    ARRAY_COMMAND_HEADER_END(stream);
    uint32_t count = buf_len(server->activeEntities);
    uint32_t numEnts = 0;
    for(uint32_t i = 0; i < count; i++)
    {
        struct TessEditorServerEntity *entity = server->activeEntities[i];
        uint32_t entityId = entity->id;
        if(!BIT_IS_SET(client->entityFlags[entityId], Client_Entity_Flag_Created))
        {
            // TODO: create entity for client
            bool written = true;
            written &= stream_write_uint32(&stream, entityId);
            written &= stream_write_uint32(&stream, entity->objectId);
            written &= stream_write_v3(&stream, entity->position);
            written &= stream_write_v3(&stream, entity->eulerRotation);
            written &= stream_write_v3(&stream, entity->scale);
            if(!written) // force to reinterate current
            {
                ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Create_Entities, numEnts);
                tess_editor_server_send_stream(server, client, &stream);
                ARRAY_COMMAND_RESET(stream);
                numEnts = 0;
                i--;
            }
            else 
            {
                SET_BIT(client->entityFlags[entityId], Client_Entity_Flag_Created);
                client->entityVersions[entityId] = entity->version;
                ARRAY_COMMAND_INC(stream);
                numEnts++;
            }
        }
    }
    if(numEnts != 0)
    {
        ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Create_Entities, numEnts);
        tess_editor_server_send_stream(server, client, &stream);
    }
}

void client_sync_entities(struct TessEditorServer *server, struct TessEditorServerClient *client)
{
    ARRAY_COMMAND_START(stream);
    ARRAY_COMMAND_HEADER_END(stream);
    uint32_t count = buf_len(server->activeEntities);
    uint32_t numEnts = 0;
    for(uint32_t i = 0; i < count; i++)
    {
        struct TessEditorServerEntity *entity = server->activeEntities[i];
        uint32_t entityId = entity->id;
        bool created = BIT_IS_SET(client->entityFlags[entityId], Client_Entity_Flag_Created);
        if(created && client->entityVersions[entityId] != entity->version)
        {
            bool written = true;
            printf("sync %p %d to %d\n", client, entityId, entity->version);
            written &= stream_write_uint32(&stream, entityId);
            written &= stream_write_v3(&stream, entity->position);
            written &= stream_write_v3(&stream, entity->eulerRotation);
            written &= stream_write_v3(&stream, entity->scale);
            if(!written)
            {
                ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Transform_Entities, numEnts);
                tess_editor_server_send_stream(server, client, &stream);
                ARRAY_COMMAND_RESET(stream);
                numEnts = 0;
                i--;
            }
            else
            {
                client->entityVersions[entityId] = entity->version;
                ARRAY_COMMAND_INC(stream);
                numEnts++;
            }
        }
    }
    if(numEnts != 0)
    {
        ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Transform_Entities, numEnts);
        tess_editor_server_send_stream(server, client, &stream);
    }
}

void client_update(struct TessEditorServer *server, struct TessEditorServerClient *client)
{
    client_send_uncreated_entities(server, client);
    client_sync_entities(server, client);
}

void editor_server_process_client_command(struct TessEditorServer *server, struct TessEditorServerClient *client, uint32_t cmd, uint8_t *data, uint32_t dataSize)
{
    struct ByteStream stream;
    init_byte_stream(&stream, data, dataSize);
    switch(cmd)
    {
        case Editor_Server_Command_Nop:
            break;
        case Editor_Server_Command_Request_Object_Definitions:
            printf("Editor server: client requested object definitions\n");
            send_all_object_definitions(server, client);
            break;
        case Editor_Server_Command_Server_Create_Entities:
            {
                struct V3 pos, rot, scale;
                uint32_t objectId;
                uint16_t numEntries = 0;
                bool dataRead = true;
                dataRead &= stream_read_uint16(&stream, &numEntries);
                if(!dataRead) return;
                for(int i = 0; i < numEntries; i++)
                {
                    dataRead &= stream_read_uint32(&stream, &objectId);
                    dataRead &= stream_read_v3(&stream, &pos);
                    dataRead &= stream_read_v3(&stream, &rot);
                    dataRead &= stream_read_v3(&stream, &scale);
                    if(!dataRead) return;
                    //printf("create entity obj: %d pos: %f %f %f rot %f %f %f sc %f %f %f\n", objectId, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, scale.x, scale.y, scale.z);
                    tess_editor_server_create_entity(server, objectId, pos);
                }
            }
            break;
        case Editor_Server_Command_Destroy_Entities:
            {
                uint16_t numEntries = 0;
                uint32_t entityId;
                bool dataRead = true;
                dataRead &= stream_read_uint16(&stream, &numEntries);
                if(!dataRead) return;
                for(int i = 0; i < numEntries; i++)
                {
                    dataRead &= stream_read_uint32(&stream, &entityId);
                    if(!dataRead) return;
                    tess_editor_server_destroy_entity(server, entityId);
                }
            } break;
        case Editor_Server_Command_Debug_Message:
            {
                char buf[4096];
                uint16_t len;
                bool res = stream_read_uint16(&stream, &len);
                if(!res) return;
                if(len >= sizeof(buf)) return;
                res = stream_read_str(&stream, buf, len);
                if(!res) return;
                buf[len] = 0;
                printf("Editor server debug msg: %s\n", buf);
            }break;
        case Editor_Server_Command_Transform_Entities:
            {
                struct V3 pos, rot, scale;
                uint32_t entityId;
                uint16_t numEntries = 0;
                bool dataRead = true;
                dataRead &= stream_read_uint16(&stream, &numEntries);
                if(!dataRead) return;
                for(int i = 0; i < numEntries; i++)
                {
                    dataRead &= stream_read_uint32(&stream, &entityId);
                    dataRead &= stream_read_v3(&stream, &pos);
                    dataRead &= stream_read_v3(&stream, &rot);
                    dataRead &= stream_read_v3(&stream, &scale);
                    if(!dataRead) return;
                    struct TessEditorServerEntity *edEnt;
                    edEnt = tess_editor_server_get_entity(server, entityId);
                    if(NULL == edEnt)
                        return;         
                    edEnt->position = pos;
                    edEnt->eulerRotation = rot;
                    edEnt->scale = scale;
                    edEnt->version++;
                    // the client that sent us the change should be up to date
                    client->entityVersions[entityId] = edEnt->version;
                }
            }break;
        default:
            printf("Editor server: unknown command! %d\n", cmd);
            break;
    }
}

void tess_update_editor_server(struct TessEditorServer *eserver)
{
    if(NULL == eserver->tcpServer)
        return;

    PROF_BLOCK();
    uint8_t buf[1024];
    AikePlatform *pl = eserver->platform;
    
    AikeTCPConnection *con;
    while((con = pl->tcp_accept(pl, eserver->tcpServer)) != NULL)
    {
        printf("new connection to %d:%d\n", *((int*)con->destAddr), con->destPort); 
        bool success = tess_editor_server_new_client(eserver, con);
        if(!success) // server full
            pl->tcp_close_connection(pl, con);
    }

    uint32_t count = buf_len(eserver->activeClients);

    uint32_t removeCount = 0;
    struct TessEditorServerClient *removeList[count];

    // receive and process data from clients
    for(int i = 0; i < count; i++)
    {
        struct TessEditorServerClient *client = eserver->activeClients[i];
        con = client->connection; 
        uint32_t nBytes;
        while(pl->tcp_recv(pl, con, buf, sizeof(buf), &nBytes))
        {
            if(nBytes == 0)
            {
                // TODO: should platform close it automatically?
                printf("closed connection to %d:%d\n", *((int*)con->destAddr), con->destPort);
                pl->tcp_close_connection(pl, con);
                pool_free(eserver->clientPool, client);
                removeList[removeCount++] = client;
                break;
            }
            else
            {
                editor_append_cmd_data(&client->cmdBuf, buf, nBytes);
            }
        }
    }
    // remove closed clients
    // @OPTIMIZE: this is like n^3 worst case!
    for(int i = 0; i < removeCount; i++)
    {
        uint32_t idx = buf_find_idx(eserver->activeClients, removeList[i]);
        assert(idx != -1);
        buf_remove_at(eserver->activeClients, idx);
    }

    count = buf_len(eserver->activeClients);
    for(int i = 0; i < count; i++)
    {
        client_update(eserver, eserver->activeClients[i]);
    }
}

void editor_server_coroutine(void *data) {
    TessEditorServer *eserver = (TessEditorServer*)data;
    tess_create_editor_server(eserver, eserver->arena);
    while(1) {
        tess_update_editor_server(eserver);
        scheduler_yield();
    }
}
