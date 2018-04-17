
void tess_create_editor_server(struct TessEditorServer *eserver, struct TessFixedArena *arena);
void tess_destroy_editor_server(struct TessEditorServer *eserver);

void tess_server_init(struct TessServer *server, AikePlatform *platform)
{
    memset(server, 0, sizeof(struct TessServer));
    server->platform = platform;

    fixed_arena_init(platform, &server->arena, 2 * 1024 * 1024); 

    // Init strings
    //
    tess_strings_init(&server->strings, &server->arena);

    // Init file system
    //
    server->fileSystem.platform = platform;
    // Virtual table tells file system where to pass the data once it's done loading
    server->fileSystem.pipeline_vtbl[Tess_File_Pipeline_None] = NULL;
    server->fileSystem.pipeline_vtbl[Tess_File_Pipeline_TTR] = (FilePipelineProc)tess_process_ttr_file;
    server->fileSystem.pipeline_ptrs[Tess_File_Pipeline_TTR] = &server->assetSystem;
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
    tess_create_editor_server(&server->editorServer, &server->arena);
}

void tess_server_destroy(struct TessServer *server)
{
    tess_destroy_editor_server(&server->editorServer);

    // Destroy file system
    
    // Destroy asset system
    tess_asset_system_destroy(&server->assetSystem);

    // Destroy strings
    tess_strings_destroy(&server->strings);
}

void tess_create_editor_server(struct TessEditorServer *eserver, struct TessFixedArena *arena)
{
    printf("Editor server listening on port 7879\n");
    eserver->tcpServer = eserver->platform->tcp_listen(eserver->platform, 7879);
    eserver->activeEntities = NULL;
    eserver->activeClients = NULL;
    POOL_FROM_ARENA(eserver->clientPool, arena, TESS_EDITOR_SERVER_MAX_CLIENTS);
    memset(&eserver->objectTable, 0, sizeof(eserver->objectTable));

    TStr *intr = tess_intern_string(eserver->tstrings, "First/object0");
    eserver->objectTable[1] = intr;
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
    
    eserver->platform->tcp_close_server(eserver->platform, eserver->tcpServer);
}

void tess_editor_server_send_command(struct TessEditorServer *server, struct TessEditorServerClient *client, uint8_t *data, uint32_t size)
{
    assert(size > 0 && size < TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    server->platform->tcp_send(server->platform, client->connection, data, size); 
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
    buf_push(eserver->activeClients, client);
    editor_server_send_debug_message(eserver, client, "Hello client!");
    return true;
}

void send_all_object_definitions(struct TessEditorServer *server, struct TessEditorServerClient *client)
{
    uint32_t numEntries, entryCountLoc, numPackets, curLoc;
    uint8_t buf[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];
    struct ByteStream stream;
    init_byte_stream(&stream, buf, sizeof(buf));
    stream_write_command_header(&stream, 0, Editor_Server_Command_Define_Objects);
    entryCountLoc = stream_get_offset(&stream);
    stream_advance(&stream, 2);
    numEntries = 0;
    numPackets = 0;
    curLoc = stream_get_offset(&stream);
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

            if(!res) // packet full, flush
            {
                stream_go_to_offset(&stream, 0);
                stream_write_command_header(&stream, curLoc, Editor_Server_Command_Define_Objects);
                stream_go_to_offset(&stream, entryCountLoc);
                stream_write_uint16(&stream, numEntries); 

                tess_editor_server_send_command(server, client, buf, curLoc);
                
                // reset / go back to first entry location
                stream_go_to_offset(&stream, entryCountLoc + 2);
                numEntries = 0;
                numPackets ++;
            }
            curLoc = stream_get_offset(&stream);
            numEntries++;
        }
    }
    if(numEntries > 0 || numPackets == 0)
    {
        printf("sent!\n");
        stream_go_to_offset(&stream, 0);
        stream_write_command_header(&stream, curLoc, Editor_Server_Command_Define_Objects);
        stream_go_to_offset(&stream, entryCountLoc);
        stream_write_uint16(&stream, numEntries); 
        tess_editor_server_send_command(server, client, buf, curLoc);
    }
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
        case Editor_Server_Command_Define_Objects:
            break;
        case Editor_Server_Command_Create_Entities:
            break;
        case Editor_Server_Command_Move_Entity:
            break;
        case Editor_Server_Command_Destroy_Entities:
            break;
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
        default:
            printf("Editor server: unknown command! %d\n", cmd);
            break;
    }
}

void tess_update_editor_server(struct TessEditorServer *eserver)
{
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
    // @OPTIMIZE: this is like n^3 worst case!
    for(int i = 0; i < removeCount; i++)
    {
        uint32_t idx = buf_find_idx(eserver->activeClients, removeList[i]);
        assert(idx != -1);
        buf_remove_at(eserver->activeClients, idx);
    }
}
