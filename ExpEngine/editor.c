void objectid_callback(struct Renderer *renderer, struct ObjectIDSamplesReady *tdr, void *userData);
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
    query_objectid((struct TessEditor*)userData);
}


void editor_init(struct TessEditor *editor)
{
    query_objectid(editor);
    editor->init = true;
    editor->nk_ctx = &editor->uiSystem->nk_ctx;

    editor->entityMap = kh_init(uint32);
    editor->edEntities = NULL;
}

void editor_destroy(struct TessEditor *editor)
{
    kh_destroy(uint32, editor->entityMap);
    buf_free(editor->edEntities);
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

void editor_draw_ui(struct TessEditor *editor)
{
    struct nk_context *ctx = editor->nk_ctx;

    if(nk_begin(ctx, "Editor", nk_rect(400, 50, 350, 300),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        if(nk_button_label(ctx, "Create object"))
        {
            editor_create_object(editor, 1);
        }
        nk_property_int(ctx, "Mode", 0, (int*)&editor->tess->mode, INT_MAX, 1, 1);
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
}

void editor_update(struct TessEditor *editor)
{
    if(!editor->init)
        editor_init(editor);

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

    editor_draw_ui(editor);
}


