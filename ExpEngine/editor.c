void objectid_callback(Renderer *renderer, ObjectIDSamplesReady *tdr, void *userData);
void editor_update(struct TessEditor *editor);
void editor_destroy(struct TessEditor *editor);
bool editor_connect(TessEditor *editor, const char *ipStr, uint16_t port);
void editor_disconnect(TessEditor *editor);
void editor_connected(TessEditor *editor);
void editor_client_send_command(TessEditor *editor, uint8_t *data, uint32_t size);

static inline void editor_client_send_stream(TessEditor *editor, ByteStream *stream)
{
    editor_client_send_command(editor, stream->start, stream_get_offset(stream));
}

void query_objectid(TessEditor *editor)
{
    RenderMessage rmsg = {};
    rmsg.type = Render_Message_Sample_Object_Id;
    rmsg.sampleO.normalizedSampleCoords = &editor->normalizedCursorPos;
    rmsg.sampleO.sampleCount = 1;
    rmsg.sampleO.buffer = (uint32_t*)&editor->cursorObjectIdBuf;
    rmsg.sampleO.onComplete = (void**)&g_tessVtbl->editorObjectId;
    rmsg.sampleO.userData = editor;
    renderer_queue_message(editor->renderSystem->renderer, &rmsg);
}

void objectid_callback(Renderer *renderer, ObjectIDSamplesReady *tdr, void *userData)
{
    TessEditor *editor = (TessEditor *)userData;
    editor->cursorObjectId = editor->cursorObjectIdBuf;
    if(editor->connected)
        query_objectid(editor);
}

void editor_init(TessEditor *editor)
{
    fixed_arena_init(editor->platform, &editor->arena, 256 * 1024);
    POOL_FROM_ARENA(editor->edEntityPool, &editor->arena, 1000);

    editor->cursorObjectIdBuf = 0xFFFFFFFF;

    editor->nk_ctx = &editor->uiSystem->nk_ctx;

    editor->entityMap = kh_init(uint32);
    editor->serverEntityMap = kh_init(uint32);
    editor->edEntities = NULL;
    editor->tcpCon = NULL;

    editor->objectSelected = false;
    editor->moving = false;
    editor->init = true;

    editor->cam = &editor->world->defaultCamera;
    editor->camRot = (V2){0};
    editor->camLocked = false;
}

void editor_coroutine(TessEditor *editor)
{
    editor_init(editor);

    bool success = editor_connect(editor, editor->ipStr, editor->port);
    assert(success);

    while(1)
    {
        editor_update(editor);
        coro_transfer(&editor->coroCtx, editor->client->mainctx);
    }

    editor_destroy(editor);
}

void editor_destroy(TessEditor *editor)
{
    if(editor->connected)
        editor_disconnect(editor);

    kh_destroy(uint32, editor->entityMap);
    kh_destroy(uint32, editor->serverEntityMap);
    buf_free(editor->edEntities);

    fixed_arena_free(editor->platform, &editor->arena);
}

void editor_reset(TessEditor *editor)
{
    kh_clear(uint32, editor->entityMap);
    kh_clear(uint32, editor->serverEntityMap);
    pool_clear(editor->edEntityPool);
    buf_clear(editor->edEntities);
    // clear world
    tess_reset_world(editor->world);

    editor_command_buf_reset(&editor->cmdBuf, editor, NULL, false);

    editor->objectSelected = false;
}

bool editor_connect(TessEditor *editor, const char *ipStr, uint16_t port)
{
    assert(editor->init);

    // TODO: terminate old connection and connect to new
    assert(editor->tcpCon == NULL);

    query_objectid(editor);

    AikeTCPConnection *connection;
    connection = editor->platform->tcp_connect(editor->platform, ipStr, port);
    editor->tcpCon = connection;

    if(NULL != connection)
    {
        printf("Editor connected to %s:%d\n", ipStr, port); 
        editor->connected = true;
        editor_connected(editor);
        editor_reset(editor);
        return true;
    }
    return false;
}

void editor_disconnect(TessEditor *editor)
{
    assert(editor->init && editor->connected);
    editor->platform->tcp_close_connection(editor->platform, editor->tcpCon);
    editor->tcpCon = NULL;
    editor->connected = false;
    editor->client->mode = Tess_Client_Mode_Menu;
}

void editor_send_create_entity_command(TessEditor *editor, uint32_t objectId)
{
    ARRAY_COMMAND_START(stream);
    ARRAY_COMMAND_HEADER_END(stream);
    bool written = true;
    written &= stream_write_uint32(&stream, objectId);
    written &= stream_write_v3(&stream, make_v3(0.0f, 0.0f, 0.0f)); // pos
    written &= stream_write_v3(&stream, make_v3(0.0f, 0.0f, 0.0f)); // rot
    written &= stream_write_v3(&stream, make_v3(1.0f, 1.0f, 1.0f)); // scale
    ARRAY_COMMAND_INC(stream);
    ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Server_Create_Entities, 1);
    editor_client_send_stream(editor, &stream);
} 

TessEditorEntity* editor_create_object(TessEditor *editor, uint32_t objectId)
{
    Mat4 identity;
    mat4_identity(&identity);
    uint32_t entityId = tess_create_entity(editor->world, objectId, &identity);
    TessEditorEntity *edEnt = pool_allocate(editor->edEntityPool);
    assert(edEnt); // TODO: deal with this properly!
    edEnt->entityId = entityId;
    edEnt->position = make_v3(0.0f, 0.0f, 0.0f);
    edEnt->scale = make_v3(1.0f, 1.0f, 1.0f);
    edEnt->eulerRotation = make_v3(0.0f, 0.0f, 0.0f);
    edEnt->id = edEnt - editor->edEntityPool;
    edEnt->localDirty = false;
    edEnt->remoteDirty = false;
    
    int dummy;
    khiter_t k = kh_put(uint32, editor->entityMap, entityId, &dummy);
    assert(dummy > 0); // key already present in table
    kh_val(editor->entityMap, k) = edEnt->id;
    buf_push(editor->edEntities, edEnt);
    return edEnt;
}

TessEditorEntity* editor_get_entity(TessEditor *editor, uint32_t entityId)
{
    if(entityId >= pool_cap(editor->edEntityPool))
        return NULL;
    return editor->edEntityPool + entityId;
}

void editor_send_debug_message(TessEditor *editor, char *msg);

void editor_export_map(TessEditor *editor, const char *path)
{
    void *mem = malloc(1024 * 1024);

    uint8_t *stream = (uint8_t*)mem;
    TessMapHeader *map = STREAM_PUSH(stream, TessMapHeader);
    map->signature = TTR_4CHAR("TESM");
    map->majVer = 0;
    map->minVer = 0;

    STREAM_PUSH_ALIGN(stream, 8);
    TessObjectTable *otbl = STREAM_PUSH_FLEX(stream, TessObjectTable, entries, 1);
    otbl->numEntries = 1;
    otbl->entries[0].objectId = 1;
    strcpy(otbl->entries[0].assetId, "First/object0");

    STREAM_PUSH_ALIGN(stream, 8);
    uint32_t entityCount = buf_len(editor->edEntities);
    TessEntityTable *etbl = STREAM_PUSH_FLEX(stream, TessEntityTable, entries, entityCount);
    etbl->numEntries = entityCount;
    for(int i = 0; i < entityCount; i++)
    {
        TessEditorEntity *edEnt = editor->edEntities[i];
        etbl->entries[i].position = edEnt->position;
        etbl->entries[i].scale = edEnt->scale;
        TessEntity *ent = tess_get_entity(editor->world, edEnt->entityId);
        assert(ent);
        etbl->entries[i].objectId = ent->objectId;
    }

    TTR_SET_REF_TO_PTR(map->objectTableRef, otbl);
    TTR_SET_REF_TO_PTR(map->entityTableRef, etbl);

    FILE *file = fopen(path, "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)mem;
        fwrite(mem, 1, size, file);
        fclose(file);
        printf("Exported map file with %d objects and %d entities\n", 1, entityCount);
    }
    else
        fprintf(stderr, "Writing map file failed!\n");

    free(mem);
}

void editor_draw_ui(TessEditor *editor)
{
    PROF_BLOCK();
    struct nk_context *ctx = editor->nk_ctx;

    if(nk_begin(ctx, "Editor", nk_rect(400, 50, 350, 300),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        if(nk_button_label(ctx, "Create object"))
        {
            editor_send_create_entity_command(editor, 1);
        }
        if(nk_button_label(ctx, "Export map"))
        {
            editor_export_map(editor, "map.ttm");
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
            TessEditorEntity *edEnt = editor_get_entity(editor, editor->selectedEditorEntityId);
            if(edEnt == NULL)
            {
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Error", NK_TEXT_ALIGN_CENTERED);
            }
            TessEditorEntity edEntCpy = *edEnt;

            assert(edEnt != NULL);
            TessEntity *ent = tess_get_entity(editor->world, edEnt->entityId);
            TessObject *obj = tess_get_object(editor->world, ent->objectId);
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
            if(memcmp(edEnt, &edEntCpy, sizeof(edEntCpy)) != 0)
            {
                edEnt->localDirty = true;
                edEnt->remoteDirty = true;
            }

            uint32_t selObj = editor->selectedObjectId;

            V3 translate, scale;
            Quat rotate;
            Mat4 trs;
            translate = edEnt->position;
            quat_euler_deg(&rotate, edEnt->eulerRotation);
            scale = make_v3(0.2f, 0.2f, 5.0f);
            mat4_trs(&trs, translate, rotate, scale);
            render_system_render_mesh(editor->renderSystem, 0, 2, selObj|0x80000000, &trs);

            Quat alignY;
            Quat rotate2 = rotate;
            quat_angle_axis(&rotate, 90.0f, make_v3(1.0f, 0.0f, 0.0f));
            quat_mul(&alignY, rotate2, rotate);
            mat4_trs(&trs, translate, alignY, scale);
            render_system_render_mesh(editor->renderSystem, 0, 2, selObj|0x81000000, &trs);

            Quat alignX;
            quat_angle_axis(&rotate, 90.0f, make_v3(0.0f, 1.0f, 0.0f));
            quat_mul(&alignX, rotate2, rotate);
            mat4_trs(&trs, translate, alignX, scale);
            render_system_render_mesh(editor->renderSystem, 0, 2, selObj|0x82000000, &trs);

            if(editor->cursorObjectId != 0xFFFFFFFF 
                    && (editor->cursorObjectId & 0x80000000) != 0)
            {
                if(mouse_left_down(editor->inputSystem) && !editor->moving)
                {
                    editor->moving = true;
                    if((editor->cursorObjectId & 0x0F000000) == 0)
                        editor->moveDirection = make_v3(0.0f, 0.0f, 1.0f);
                    if((editor->cursorObjectId & 0x0F000000) == 0x01000000)
                        editor->moveDirection = make_v3(0.0f, 1.0f, 0.0f);
                    if((editor->cursorObjectId & 0x0F000000) == 0x02000000)
                        editor->moveDirection = make_v3(1.0f, 0.0f, 0.0f);
                }
            }
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

void editor_client_send_command(TessEditor *editor, uint8_t *data, uint32_t size)
{
    if(!editor->connected)
    {
        fprintf(stderr, "Tried to send editor command when not connected, ignoring\n");
        return;
    }
    assert(size > 0 && size <= TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    editor->platform->tcp_send(editor->platform, editor->tcpCon, data, size);
}
void editor_client_send_empty_command(TessEditor *editor, uint16_t cmd)
{
    uint8_t buf[64];
    ByteStream stream;
    init_byte_stream(&stream, buf, sizeof(buf));
    int res = stream_write_empty_command(&stream, cmd);
    assert(res);
    editor_client_send_command(editor, buf, stream_get_offset(&stream));
}

void editor_send_debug_message(TessEditor *editor, char *msg)
{
    uint32_t msgLen = strlen(msg);
    uint8_t buf[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];
    ByteStream stream;
    init_byte_stream(&stream, buf, TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    bool success = stream_write_uint16(&stream, 2 + 2 + 2 + msgLen);
    success &= stream_write_uint16(&stream, Editor_Server_Command_Debug_Message);
    success &= stream_write_uint16(&stream, msgLen);
    success &= stream_write_str(&stream, msg, msgLen);
    if(success)
        editor_client_send_command(editor, buf, stream_get_offset(&stream));
}

void editor_connected(TessEditor *editor)
{
    editor_client_send_empty_command(editor, Editor_Server_Command_Request_Object_Definitions);

}

void editor_client_update(TessEditor *editor)
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
            break;
        }
        else
        {
            printf("Editor client received %d bytes\n", nBytes);
            editor_append_cmd_data(&editor->cmdBuf, buf, nBytes);
        }
    }
}

void editor_camera_update(TessEditor *editor)
{
    V3 forward = make_v3(0.0f, 0.0f, 0.01f);
    V3 right;
    TessCamera *cam = editor->cam;
    quat_v3_mul_dir(&forward, cam->rotation, make_v3(0.0f, 0.0f, 0.01f));
    quat_v3_mul_dir(&right, cam->rotation, make_v3(0.01f, 0.0f, 0.0f));

    if(key_down(editor->inputSystem, AIKE_KEY_SPACE))
        editor->camLocked = !editor->camLocked;

    if(!editor->camLocked)
    {
        if(key(editor->inputSystem, AIKE_KEY_W))
            v3_add(&cam->position, cam->position, forward);
        if(key(editor->inputSystem, AIKE_KEY_S))
            v3_sub(&cam->position, cam->position, forward);
        if(key(editor->inputSystem, AIKE_KEY_D))
            v3_add(&cam->position, cam->position, right);
        if(key(editor->inputSystem, AIKE_KEY_A))
            v3_sub(&cam->position, cam->position, right);

        v2_add(&editor->camRot, editor->camRot, editor->inputSystem->mouseDelta);
        editor->camRot = make_v2(editor->camRot.x, MAX(editor->camRot.y, -90.0f));
        editor->camRot = make_v2(editor->camRot.x, MIN(editor->camRot.y, 90.0f));
    }

    Quat xRot, yRot;
    quat_euler_deg(&xRot, make_v3(0.0f, editor->camRot.y, editor->camRot.x));
    cam->rotation = xRot;
}

void editor_update(TessEditor *editor)
{
    PROF_BLOCK();
    uint32_t w = editor->platform->mainWin.width;
    uint32_t h = editor->platform->mainWin.height;
    editor->normalizedCursorPos = editor->client->inputSystem.normMousePos;

    if(mouse_left_down(editor->inputSystem) && editor->cursorObjectId != -1)
    {
        khiter_t k = kh_get(uint32, editor->entityMap, editor->cursorObjectId);
        if(k != kh_end(editor->entityMap))
        {
            editor->objectSelected = true;
            editor->selectedEditorEntityId = kh_val(editor->entityMap, k);
            editor->selectedObjectId = editor->cursorObjectId;
            printf("select\n");
        }
    }

    editor_camera_update(editor);

    int count = buf_len(editor->edEntities);

    { // send changed transforms to server
        ARRAY_COMMAND_START(stream);
        ARRAY_COMMAND_HEADER_END(stream);
        uint32_t entCount = 0;
        for(int i = 0; i < count; i++)
        {
            TessEditorEntity *edEnt = editor->edEntities[i];
            if(edEnt->remoteDirty)
            {
                bool written = true;
                written &= stream_write_uint32(&stream, edEnt->serverId);
                written &= stream_write_v3(&stream, edEnt->position); // pos
                written &= stream_write_v3(&stream, edEnt->eulerRotation); // rot
                written &= stream_write_v3(&stream, edEnt->scale); // scale
                if(!written)
                {
                    ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Transform_Entities, entCount);
                    editor_client_send_stream(editor, &stream);
                    ARRAY_COMMAND_RESET(stream);
                    entCount = 0;
                    i--; // force retry current
                }
                else
                {
                    edEnt->remoteDirty = false;
                    ARRAY_COMMAND_INC(stream);
                    entCount++;
                }
            }
        }
        if(entCount > 0)
        {
            ARRAY_COMMAND_WRITE_HEADER(stream, Editor_Server_Command_Transform_Entities, entCount);
            editor_client_send_stream(editor, &stream);
        }
    }

    for(int i = 0; i < count; i++)
    {
        TessEditorEntity *edEnt = editor->edEntities[i];
        if(edEnt->localDirty)
        {
            TessEntity *ent = tess_get_entity(editor->world, edEnt->entityId);
            Mat4 objectToWorld;
            Quat rotation;
            quat_euler_deg(&rotation, edEnt->eulerRotation);
            mat4_trs(&objectToWorld, edEnt->position, rotation, edEnt->scale);
            ent->objectToWorld = objectToWorld;
            edEnt->localDirty = false;
        }
    }

    if(editor->connected)
        editor_client_update(editor);


    editor_draw_ui(editor);

    // object moving
    if(editor->moving)
    {
        if(mouse_left_up(editor->inputSystem))
        {
            editor->moving = false;
        }
        else
        {
            TessEditorEntity *edEnt = editor_get_entity(editor, editor->selectedEditorEntityId);
            if(edEnt != NULL)
            {
                float mouseDeltaLen = v2_len(editor->inputSystem->normMouseDelta);
                if(mouseDeltaLen > 0.000001f)
                {
                    V3 startClip, endClip, end;
                    V3 dir = editor->moveDirection;
                    startClip = tess_world_to_clip_pos(editor->cam, edEnt->position);
                    v3_add(&end, edEnt->position, dir);
                    endClip = tess_world_to_clip_pos(editor->cam, end);

                    V3 clipDir;
                    v3_sub(&clipDir, endClip, startClip);
                    V2 screenDir = make_v2(clipDir.x, clipDir.y);
                    float screenDirLen = v2_len(screenDir);
                    
                    if(!v3_hasnan(startClip) && !v3_hasnan(endClip) && screenDirLen > 0.0000001f)
                    {
                        v2_normalize(&screenDir);

                        V2 mouseDeltaNorm = editor->inputSystem->normMouseDelta;
                        mouseDeltaNorm.y *= -1.0f; // TODO: directx wouldnt have this..
                        v2_normalize(&mouseDeltaNorm);

                        float amount = v2_dot(screenDir, mouseDeltaNorm);
                        amount *= mouseDeltaLen * (2.0f/screenDirLen);

                        v3_add(&edEnt->position, edEnt->position, make_v3(amount*dir.x, amount*dir.y, amount*dir.z));
                        edEnt->localDirty = true;
                        edEnt->remoteDirty = true;
                    }
                }
            }
        }
    }
}



void editor_client_process_command(TessEditor *editor, uint16_t cmd, uint8_t *data, uint32_t size)
{
    ByteStream stream;
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
        case Editor_Server_Command_Create_Entities:
            {
                uint16_t entryCount;
                uint32_t serverId;
                uint32_t objectId;
                V3 pos, rot, scale;
                if(!stream_read_uint16(&stream, &entryCount)) return;
                for(int i = 0; i < entryCount; i++)
                {
                    if(!stream_read_uint32(&stream, &serverId)) return;
                    if(!stream_read_uint32(&stream, &objectId)) return;
                    if(!stream_read_v3(&stream, &pos)) return;
                    if(!stream_read_v3(&stream, &rot)) return;
                    if(!stream_read_v3(&stream, &scale)) return;
                    TessEditorEntity *edEnt;
                    edEnt = editor_create_object(editor, 1);
                    if(edEnt == NULL)
                    {
                        fprintf(stderr, "Editor could not create entity: editorEntity pool full!\n");
                        return;
                    }

                    edEnt->serverId = serverId;
                    int dummy;
                    khiter_t k = kh_put(uint32, editor->serverEntityMap, serverId, &dummy);
                    assert(dummy > 0); // key should not be already present in table
                    kh_val(editor->serverEntityMap, k) = edEnt->id;

                    edEnt->position = pos;
                    edEnt->eulerRotation = rot;
                    edEnt->scale = scale;
                    edEnt->localDirty = true;
                }
            } break;
        case Editor_Server_Command_Transform_Entities:
            {
                uint16_t entryCount;
                uint32_t serverId;
                V3 pos, rot, scale;
                if(!stream_read_uint16(&stream, &entryCount)) return;
                {
                    if(!stream_read_uint32(&stream, &serverId)) return;
                    if(!stream_read_v3(&stream, &pos)) return;
                    if(!stream_read_v3(&stream, &rot)) return;
                    if(!stream_read_v3(&stream, &scale)) return;
                    TessEditorEntity *edEnt;
                    khiter_t k = kh_get(uint32, editor->serverEntityMap, serverId);
                    if(k == kh_end(editor->serverEntityMap))
                        return;
                    uint32_t edEntityId = kh_val(editor->serverEntityMap, k);
                    edEnt = editor_get_entity(editor, edEntityId);
                    if(edEnt == NULL)
                        return;
                    edEnt->position = pos;
                    edEnt->eulerRotation = rot;
                    edEnt->scale = scale;
                    edEnt->localDirty = true;
                }
            } break;
        default:
            printf("Editor client: unknown command! %d\n", cmd);
            break;
    }

}
