void objectid_callback(struct Renderer *renderer, struct ObjectIDSamplesReady *tdr, void *userData);
void editor_disconnect(struct TessEditor *editor);
void editor_connected(struct TessEditor *editor);

void query_objectid(struct TessEditor *editor)
{
    RenderMessage rmsg = {};
    rmsg.type = Render_Message_Sample_Object_Id;
    rmsg.sampleO.normalizedSampleCoords = &editor->normalizedCursorPos;
    rmsg.sampleO.sampleCount = 1;
    rmsg.sampleO.buffer = (uint32_t*)&editor->cursorObjectIdBuf;
    rmsg.sampleO.onComplete = objectid_callback;
    rmsg.sampleO.userData = editor;
    renderer_queue_message(editor->renderSystem->renderer, &rmsg);
}

void objectid_callback(struct Renderer *renderer, struct ObjectIDSamplesReady *tdr, void *userData)
{
    struct TessEditor *editor = (struct TessEditor *)userData;
    editor->cursorObjectId = editor->cursorObjectIdBuf;
    if(editor->connected)
        query_objectid(editor);
}

void editor_init(struct TessEditor *editor)
{
    fixed_arena_init(editor->platform, &editor->arena, 256 * 1024);
    POOL_FROM_ARENA(editor->edEntityPool, &editor->arena, 1000);

    editor->cursorObjectIdBuf = 0xFFFFFFFF;

    editor->nk_ctx = &editor->uiSystem->nk_ctx;

    editor->entityMap = kh_init(uint32);
    editor->edEntities = NULL;
    editor->tcpCon = NULL;

    editor->init = true;
}

void editor_destroy(struct TessEditor *editor)
{
    if(editor->connected)
        editor_disconnect(editor);

    kh_destroy(uint32, editor->entityMap);
    buf_free(editor->edEntities);

    fixed_arena_free(editor->platform, &editor->arena);
}

bool editor_connect(struct TessEditor *editor, const char *ipStr, uint16_t port)
{
    assert(editor->init);

    // TODO: terminate old connection and connect to new
    assert(editor->tcpCon == NULL);

    query_objectid(editor);

    editor_command_buf_reset(&editor->cmdBuf, editor, NULL, false);
    AikeTCPConnection *connection;
    connection = editor->platform->tcp_connect(editor->platform, ipStr, port);
    editor->tcpCon = connection;

    if(NULL != connection)
    {
        printf("Editor connected to %s:%d\n", ipStr, port); 
        editor->connected = true;
        editor_connected(editor);
        return true;
    }
    return false;
}

void editor_disconnect(struct TessEditor *editor)
{
    assert(editor->init && editor->connected);
    editor->platform->tcp_close_connection(editor->platform, editor->tcpCon);
    editor->tcpCon = NULL;
    editor->connected = false;
}

void editor_create_object(struct TessEditor *editor, uint32_t objectId)
{
    struct Mat4 identity;
    mat4_identity(&identity);
    uint32_t entityId = tess_create_entity(editor->world, objectId, &identity);
    struct TessEditorEntity *edEnt = pool_allocate(editor->edEntityPool);
    assert(edEnt); // TODO: deal with this properly!
    edEnt->entityId = entityId;
    edEnt->position = make_v3(0.0f, 0.0f, 0.0f);
    edEnt->scale = make_v3(1.0f, 1.0f, 1.0f);
    edEnt->eulerRotation = make_v3(0.0f, 0.0f, 0.0f);
    edEnt->id = edEnt - editor->edEntityPool;
    edEnt->dirty = false;
    
    int dummy;
    khiter_t k = kh_put(uint32, editor->entityMap, entityId, &dummy);
    assert(dummy > 0); // key already present in table
    kh_val(editor->entityMap, k) = edEnt->id;
    buf_push(editor->edEntities, edEnt);
}

void editor_send_debug_message(struct TessEditor *editor, char *msg);

void editor_draw_ui(struct TessEditor *editor)
{
    PROF_BLOCK();
    struct nk_context *ctx = editor->nk_ctx;

    if(nk_begin(ctx, "Editor", nk_rect(400, 50, 350, 300),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        if(nk_button_label(ctx, "Create object"))
        {
            editor_create_object(editor, 1);
            if(editor->connected)
                editor_send_debug_message(editor, "Client created object!");
        }
        if(nk_button_label(ctx, "Exit"))
        {
            if(editor->connected)
                editor_disconnect(editor);
            editor->client->mode = Tess_Client_Mode_Menu;
        }
        nk_property_int(ctx, "Mode", 0, (int*)&editor->client->mode, INT_MAX, 1, 1);
        nk_value_int(ctx, "Hover object:", editor->cursorObjectId);
    }
    nk_end(ctx);


    if(nk_begin(ctx, "Selection", nk_rect(0, 0, 200, 400), NK_WINDOW_NO_SCROLLBAR))
    {
        if(editor->objectSelected)
        {
            struct TessEditorEntity *edEnt = editor->edEntityPool + editor->selectedObjectId;
            struct TessEntity *ent = tess_get_entity(editor->world, edEnt->entityId);
            struct TessObject *obj = tess_get_object(editor->world, ent->objectId);
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            nk_layout_row_push(ctx, 50);
            nk_label(ctx, "Asset:", NK_TEXT_ALIGN_LEFT); 
            nk_layout_row_push(ctx, 150);
            nk_label(ctx, obj->assetId->cstr, NK_TEXT_ALIGN_LEFT); 
            nk_layout_row_end(ctx);

            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, "Position:", NK_TEXT_ALIGN_LEFT);
            nk_property_float(ctx, "#X", -99999.0f, &edEnt->position.x, 999999.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Y", -99999.0f, &edEnt->position.y, 999999.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Z", -99999.0f, &edEnt->position.z, 999999.0f, 0.1f, 0.1f);
            nk_label(ctx, "Rotation:", NK_TEXT_ALIGN_LEFT);
            nk_property_float(ctx, "#X", -99999.0f, &edEnt->eulerRotation.x, 999999.0f, 0.1f, 1.0f);
            nk_property_float(ctx, "#Y", -99999.0f, &edEnt->eulerRotation.y, 999999.0f, 0.1f, 1.0f);
            nk_property_float(ctx, "#Z", -99999.0f, &edEnt->eulerRotation.z, 999999.0f, 0.1f, 1.0f);
            edEnt->eulerRotation = normalize_degrees(edEnt->eulerRotation);
            edEnt->dirty = true;
        }
        else
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, "No object selected", NK_TEXT_ALIGN_CENTERED);
        }
    }
    nk_end(ctx);

    render_profiler(ctx);
}

void editor_client_send_command(struct TessEditor *editor, uint8_t *data, uint32_t size)
{
    if(!editor->connected)
    {
        fprintf(stderr, "Tried to send editor command when not connected, ignoring\n");
        return;
    }
    assert(size > 0 && size <= TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    editor->platform->tcp_send(editor->platform, editor->tcpCon, data, size);
}

void editor_client_send_empty_command(struct TessEditor *editor, uint16_t cmd)
{
    uint8_t buf[64];
    struct ByteStream stream;
    init_byte_stream(&stream, buf, sizeof(buf));
    int res = stream_write_empty_command(&stream, cmd);
    assert(res);
    editor_client_send_command(editor, buf, stream_get_offset(&stream));
}

void editor_send_debug_message(struct TessEditor *editor, char *msg)
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
        editor_client_send_command(editor, buf, stream_get_offset(&stream));
}

void editor_connected(struct TessEditor *editor)
{
    editor_client_send_empty_command(editor, Editor_Server_Command_Request_Object_Definitions);

}

void editor_client_update(struct TessEditor *editor)
{
    PROF_BLOCK();
    AikePlatform *pl = editor->platform;
    AikeTCPConnection *con = editor->tcpCon;
    uint8_t buf[1024];
    uint32_t nBytes;
    while(pl->tcp_recv(pl, con, buf, sizeof(buf), &nBytes))
    {
        if(nBytes == 0)
        {
            printf("Editor client disconnected\n");
            editor_disconnect(editor);
        }
        else
        {
            printf("Editor client received %d bytes\n", nBytes);
            editor_append_cmd_data(&editor->cmdBuf, buf, nBytes);
        }
    }
}

void editor_update(struct TessEditor *editor)
{
    PROF_BLOCK();
    uint32_t w = editor->platform->mainWin.width;
    uint32_t h = editor->platform->mainWin.height;
    editor->normalizedCursorPos = make_v2((float)editor->platform->mouseX/(float)w, (float)editor->platform->mouseY/(float)h);

    if(mouse_left_down(editor->inputSystem) && editor->cursorObjectId != -1)
    {
        khiter_t k = kh_get(uint32, editor->entityMap, editor->cursorObjectId);
        if(k != kh_end(editor->entityMap))
        {
            editor->objectSelected = true;
            editor->selectedObjectId = kh_val(editor->entityMap, k);
            printf("select\n");
        }
    }

    int count = buf_len(editor->edEntities);
    for(int i = 0; i < count; i++)
    {
        struct TessEditorEntity *edEnt = editor->edEntities[i];
        if(edEnt->dirty)
        {
            struct TessEntity *ent = tess_get_entity(editor->world, edEnt->entityId);
            struct Mat4 objectToWorld;
            struct Quat rotation;
            quat_euler_deg(&rotation, edEnt->eulerRotation);
            mat4_trs(&objectToWorld, edEnt->position, rotation, edEnt->scale);
            ent->objectToWorld = objectToWorld;
        }
    }

    if(editor->connected)
        editor_client_update(editor);

    editor_draw_ui(editor);
}



void editor_client_process_command(struct TessEditor *editor, uint16_t cmd, uint8_t *data, uint32_t size)
{
    struct ByteStream stream;
    init_byte_stream(&stream, data, size);
    switch(cmd)
    {
        case Editor_Server_Command_Nop:
            break;
        case Editor_Server_Command_Define_Objects:
            {
                bool res = true;
                uint16_t defCount = 0;
                char buf[257];
                res = stream_read_uint16(&stream, &defCount);
                if(!res)
                    return;
                printf("Received definitions for %d entities\n", defCount);
                for(int i = 0; i < defCount; i++)
                {
                    uint32_t objectId;
                    uint8_t len;
                    if(!stream_read_uint32(&stream, &objectId)) return;
                    if(!stream_read_uint8(&stream, &len)) return;
                    if(!stream_read_str(&stream, buf, len)) return;
                    // TODO: escape string and make sure it has proper length
                    buf[len] = 0;
                    printf(" def %u %s\n", objectId, buf);
                }
            }
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
                printf("Editor client debug msg: %s\n", buf);
            }break;
        default:
            printf("Editor client: unknown command! %d\n", cmd);
            break;
    }

}
