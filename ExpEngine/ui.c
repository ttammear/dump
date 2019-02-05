
void ui_atlas_ready(struct Renderer *renderer, struct TextureQueryResponse *tqr, struct TessUISystem *ui)
{
    RenderMessage response = {};
    memcpy(tqr->textureDataPtr, ui->image, ui->nk_w*ui->nk_h*4);
    nk_font_atlas_end(&ui->nk_atlas, nk_handle_id((int)tqr->textureId), &ui->nk_null);
    nk_init_fixed(&ui->nk_ctx, calloc(1, 1024*1024), 1024*1024, &ui->nk_font->handle);
    ui->loaded = true;
    response.type = Render_Message_Texture_Update;
    response.texU.textureId = tqr->textureId;
    renderer_queue_message(ui->renderSystem->renderer, &response);
}

void tess_ui_init(struct TessUISystem *ui)
{
    nk_buffer_init_default(&ui->nk_cmds);
    nk_font_atlas_init_default(&ui->nk_atlas);
    nk_font_atlas_begin(&ui->nk_atlas);
    // ui->nk_font = nk_font_atlas_add_from_file(&ui->nk_atlas, font_path, 13.0f, NULL);
    ui->nk_font = nk_font_atlas_add_default(&ui->nk_atlas, 13.0f, NULL);

    ui->image = nk_font_atlas_bake(&ui->nk_atlas, &ui->nk_w, &ui->nk_h, NK_FONT_ATLAS_RGBA32);

    RenderMessage msg = {};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = ui;
    msg.texQ.onComplete = (TQComplete_A)ui_atlas_ready;
    msg.texQ.width = ui->nk_w;
    msg.texQ.height = ui->nk_h;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Bilinear;
    renderer_queue_message(ui->renderSystem->renderer, &msg);
    ui->loaded = false;
}

void tess_ui_destroy(struct TessUISystem *ui)
{
    if(ui->loaded)
    {
        nk_font_atlas_cleanup(&ui->nk_atlas);
        nk_font_atlas_clear(&ui->nk_atlas);
        nk_buffer_free(&ui->nk_cmds);
        nk_free(&ui->nk_ctx);
        // TODO: destroy atlas renderer texture
    }
    else
    {
        fprintf(stderr, "UI destroyed before full initialization. There will be tangling resources!\n");
    }
}

void tess_ui_begin(struct TessUISystem *ui)
{
    struct nk_context *ctx = &ui->nk_ctx;

    int mx = (int)ui->inputSystem->mousePos.x;
    int my = (int)ui->inputSystem->mousePos.y;
    uint16_t *akeys = ui->platform->keyStates;

    nk_input_begin(ctx);
    nk_input_motion(ctx, mx, my);
    nk_input_button(ctx, NK_BUTTON_LEFT, mx, my, akeys[AIKE_BTN_LEFT] != 0);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, mx, my, akeys[AIKE_BTN_MIDDLE] != 0);
    nk_input_button(ctx, NK_BUTTON_RIGHT, mx, my, akeys[AIKE_BTN_RIGHT] != 0);

    nk_input_key(ctx, NK_KEY_BACKSPACE, akeys[AIKE_KEY_BACKSPACE] != 0);
    nk_input_key(ctx, NK_KEY_TAB, akeys[AIKE_KEY_TAB] != 0);

    uint32_t next = 0;
    while((next = ui->platform->next_character(ui->platform)))
    {
        nk_input_char(ctx, (char)next);
    }

    nk_input_scroll(ctx, (struct nk_vec2){.x = ui->inputSystem->scroll.x, .y = ui->inputSystem->scroll.y});

    nk_input_end(ctx);
}

void tess_ui_end(struct TessUISystem *ui)
{
    struct nk_context *ctx = &ui->nk_ctx;

    //float width = gdata->platform->mainWin.width;
    //float height = gdata->platform->mainWin.height;
    // NOTE: currently backbuffer isn't resized with window
    float width = ui->renderSystem->rtW;
    float height = ui->renderSystem->rtH;
    ui->width = width;
    ui->height = height;

    struct Mat4 orthoM = {};
    orthoM.m11 =  2.0f / width;
    orthoM.m22 = -2.0f / height;
    orthoM.m33 = 1.0f;
    orthoM.m44 =  1.0f;
    orthoM.m14 = -1.0f;
    orthoM.m24 =  1.0f;
    ui->renderSystem->viewBuilder->orthoMatrix = orthoM;
    ui->renderSystem->viewBuilder->materialId = 3;

    builder_new_vertex_stream(ui->renderSystem->viewBuilder);

    uint32_t maxVerts = 20000;
    uint32_t maxIndices = 40000;

    // TODO: don't allocate so much on stack!
    struct UIVertex vertBuf[maxVerts];
    uint16_t indexBuf[maxIndices];

    const struct nk_draw_command *cmd;
    void *vertices, *elements;
    const nk_draw_index *offset = NULL;

    {
        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, 0},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, 16},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, 24},
            {NK_VERTEX_LAYOUT_END}
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct UIVertex);
        config.vertex_alignment = _Alignof(struct UIVertex);
        config.null = ui->nk_null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        /* setup buffers to load vertices and elements */
        {
            struct nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertBuf, sizeof(vertBuf[0])*maxVerts);
            nk_buffer_init_fixed(&ebuf, indexBuf, sizeof(indexBuf[0])*maxIndices);
            nk_convert(ctx, &ui->nk_cmds, &vbuf, &ebuf, &config);

            uint32_t vertCount = vbuf.allocated / sizeof(struct UIVertex);
            assert(vertCount < maxVerts); // vertex pool full!
            add_vertices(ui->renderSystem->viewBuilder, vertBuf, vertCount);
        }

        float sx = 1.0f;
        float sy = 1.0f;

        uint32_t curIdxBase = 0;
        nk_draw_foreach(cmd, ctx, &ui->nk_cmds) {
            if (!cmd->elem_count) continue;
            int scX0 = (cmd->clip_rect.x * sx);
            int scY0 = ((height - (int32_t)(cmd->clip_rect.y + cmd->clip_rect.h)) * sy);
            int scX1 = (cmd->clip_rect.w * sx);
            int scY1 = (cmd->clip_rect.h * sy);

            builder_begin_batch(ui->renderSystem->viewBuilder, cmd->texture.id, scX0, scY0, scX1, scY1);
            for(int j = 0; j < cmd->elem_count; j+=3)
            {
                add_index(ui->renderSystem->viewBuilder, indexBuf[j+curIdxBase]);
                add_index(ui->renderSystem->viewBuilder, indexBuf[j+2+curIdxBase]);
                add_index(ui->renderSystem->viewBuilder, indexBuf[j+1+curIdxBase]);
            }
            builder_end_batch(ui->renderSystem->viewBuilder);
            curIdxBase += cmd->elem_count;
        }
        if(curIdxBase + 10 >= maxIndices)
            printf("UI index pool exceeded!\n");
    }

    nk_clear(ctx);
}

// PROFILER

void profile_entry_recursive(struct nk_context *ctx, struct ProfNode *node, int depth) {
    while(node != NULL) {
        uint64_t cycles = node->cycles;
        double ms = node->ms;
        char buf[1024];
        stbsp_sprintf(buf, "%s %fms", node->name, ms);
        if(nk_tree_push_id(ctx, NK_TREE_TAB, buf, NK_MINIMIZED, (uint32_t)(uintptr_t)node->name)) {
            nk_layout_row_dynamic(ctx, 15, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "Cycles: %llu", (unsigned long long)cycles); 
            nk_layout_row_dynamic(ctx, 15, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "Time: %fms", ms);
            nk_layout_row_dynamic(ctx, 15, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "Calls: %d", node->numInvocations);
            profile_entry_recursive(ctx, node->firstChild, depth+1);
            nk_tree_pop(ctx);
        }
        node = node->nextSibling;
    }
}

void render_debug_log(struct nk_context *ctx, bool *wasClosed) {
    if(nk_begin(ctx, "Debug log", nk_rect(400, 350, 600, 600),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE)) {
        struct ProfilerState *ps = t_profState;

        nk_layout_row_dynamic(ctx, 25, 2);
        if(!atomic_load(&ps->pause) && nk_button_label(ctx, "Pause capture")) {
            atomic_store(&ps->pause, true);
        } else if(atomic_load(&ps->pause) && nk_button_label(ctx, "Resume capture")) {
            atomic_store(&ps->pause, false);
        }

        int start = ps->eventLogHead - ps->eventLogCount;
        start = (start < 0) ? EVENT_LOG_STORED_EVENTS + start : start;
        for(int i = start; i < ps->eventLogCount; i = (i+1) % EVENT_LOG_STORED_EVENTS) {
            EventLogEntry *e = ps->eventLog + i;
            nk_layout_row_dynamic(ctx, 15, 3);
            nk_label(ctx, e->name, NK_TEXT_ALIGN_LEFT);
            if(e->func != NULL) {
                nk_label(ctx, e->func, NK_TEXT_ALIGN_LEFT);
            }
            nk_labelf(ctx, NK_TEXT_LEFT, "%d", e->frameId);
        }
    }
    nk_end(ctx);
}

void render_profiler(struct nk_context *ctx, bool *wasClosed)
{
    if(nk_begin(ctx, "Profiler", nk_rect(400, 350, 600, 600),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE))
    {
        for(int j = 0; j < ARRAY_COUNT(g_profStates); j++) {
            uintptr_t pptr = atomic_load(&g_profStates[j]);
            if(pptr == (uintptr_t)NULL) {
                continue;
            }
            struct ProfilerState *ps = (struct ProfilerState*)pptr;
            if(nk_tree_push_id(ctx, NK_TREE_TAB, ps->name, NK_MINIMIZED, 1)) {
                // current profiler frameIdx
                const int idx2 = (ps->frameId-1)%PROFILE_TREE_STORED_FRAMES;
                // id of potentially dirty frame that's being generated
                const int dirtyIdx = ps->frameId%PROFILE_TREE_STORED_FRAMES;
                int idx;
                bool paused = atomic_load(&ps->pause);
                if(!paused) {
                    ps->pauseIdx = idx = idx2;
                } else {
                    idx = ps->pauseIdx;
                }

                nk_layout_row_dynamic(ctx, 25, PROFILE_TREE_STORED_FRAMES);

                for(int i = 0; i < PROFILE_TREE_STORED_FRAMES; i++) {
                    //nk_button_color(ctx, (struct nk_color){.r = 255, .g = 0, .b = 0, .a = 255});
                    struct nk_color col;
                    if(i == idx) {
                        col = (struct nk_color){.r = 255, .g = 0, .b = 0, .a = 255};
                    } else if(i == dirtyIdx) {
                        col = (struct nk_color){.r = 255, .g = 255, .b = 0, .a = 255};
                    } else {
                        col = (struct nk_color){.r = 0, .g = 255, .b = 0, .a = 255};
                    }
                    struct nk_window *win = ctx->current;
                    struct nk_panel *layout = ctx->current->layout;
                    struct nk_rect bounds;
                    struct nk_input *in = &ctx->input;
                    enum nk_widget_layout_states state = nk_widget(&bounds, ctx);
                    nk_fill_rect(&win->buffer, bounds, 0, col); // 0 rounding
                    if(nk_button_behavior(&ctx->last_widget_state, bounds, in, ctx->button_behavior) && i != dirtyIdx) {
                        ps->pauseIdx = i;
                    }
                }

                nk_layout_row_dynamic(ctx, 25, 2);
                // TODO: this atomic stuff doesn't really do anything?
                if(!atomic_load(&ps->pause) && nk_button_label(ctx, "Pause capture")) {
                    atomic_store(&ps->pause, true);
                } else if(atomic_load(&ps->pause) && nk_button_label(ctx, "Resume capture")) {
                    atomic_store(&ps->pause, false);
                }
                ProfNode *tree = ps->profileTree[idx].tree.firstChild;
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_labelf(ctx, NK_TEXT_LEFT, "Frame ID: %d", ps->profileTree[idx].frameId);
                if(tree != NULL) {
                    profile_entry_recursive(ctx, tree, 0);
                }
                nk_tree_pop(ctx);
            }
        }
        *wasClosed = false;
    }
    else 
    { 
        *wasClosed = true;
        nk_window_show(ctx, "Profiler", NK_SHOWN);
    }
    nk_end(ctx);
}

void asset_picker_init(AssetPickerState *assetPicker)
{
    assetPicker->state = 0;
    assetPicker->packageName = NULL;
    assetPicker->assetName = NULL;
    assetPicker->result = NULL;
    assetPicker->show = false;
}

bool render_asset_picker(struct nk_context *ctx, TessAssetSystem *as, uint32_t assetType, AssetPickerState *state)
{
    bool ret = false;
    if(!state->show)
        return ret;
    nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Asset picker", NK_WINDOW_BORDER|NK_WINDOW_TITLE, nk_rect(10, 10, 400, 600));
    {
        struct nk_list_view view;

        nk_menubar_begin(ctx);
        nk_layout_row_dynamic(ctx, 25, 4);
        if(nk_button_label(ctx, "Packages"))
            state->state = 0;
        if(state->state >= 1)
            nk_button_label(ctx, state->packageName->cstr);
        if(state->assetName != NULL)
            nk_button_label(ctx, state->assetName->cstr);
        nk_menubar_end(ctx);

        nk_layout_row_dynamic(ctx, 400, 1);
        if(state->state == 0)
        {
            if(nk_group_begin(ctx, "ppickerlist", NK_WINDOW_BORDER))
            {
                int count = buf_len(as->packageList);
                for (int i = 0; i < count; i++) 
                {
                    nk_layout_row_dynamic(ctx, 25, 1);
                    if(nk_button_label(ctx, as->packageList[i]->cstr))
                    {
                        state->state = 1;
                        state->packageName = as->packageList[i];
                    }
                }
                nk_group_end(ctx);
            }
        }
        else
        {
            if(nk_group_begin(ctx, "apickerlist", NK_WINDOW_BORDER))
            {
                khiter_t key = kh_get(str, as->packageAssetMap, state->packageName->cstr);
                if(key != kh_end(as->packageAssetMap))
                {
                    AssetLookupCache *cache = kh_val(as->packageAssetMap, key);
                    int count = buf_len(cache->entries); 
                    for(int i = 0; i < count; i++)
                    {
                        if(cache->entries[i]->assetType != assetType)
                            continue;
                        nk_layout_row_dynamic(ctx, 20, 1);
                        TStr *assetName = cache->entries[i]->assetName;
                        if(nk_button_label(ctx, assetName->cstr))
                        {
                            state->assetName = assetName;
                            char buf[128];
                            strcpy(buf, state->packageName->cstr);
                            int clen = strlen(buf);
                            buf[clen] = TTR_DELIM;
                            buf[clen+1] = '\0';
                            assert(strlen(buf) + strlen(state->assetName->cstr) < ARRAY_COUNT(buf));
                            strcat(buf, state->assetName->cstr);
                            state->result = tess_intern_string_s(as->tstrings, buf, ARRAY_COUNT(buf));
                            ret = true;
                        }
                    }
                }
                nk_group_end(ctx);
            }
        }
    }
    if(ret) 
        nk_popup_close(ctx);
    nk_popup_end(ctx);

    return ret;
}

void init_create_asset_state(struct CreateAssetState *cas)
{
    cas->showAddWindow = false;
    cas->createType = 0;
    cas->assetNameBuf[0] = 0;
    cas->objectAssetNameBuf[0] = 0;
    asset_picker_init(&cas->pickerState);
}

void render_asset_window(struct nk_context *ctx, TessAssetSystem *as, bool *wasClosed, CreateAssetState *cas)
{
    if(cas->showAddWindow)
    {
        if(nk_begin(ctx, "Add asset", nk_rect(10, 10, 600, 400),
                    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE))
        {
            nk_layout_row_dynamic(ctx, 30, 3);
            if(nk_option_label(ctx, "texture", cas->createType == 0)) cas->createType = 0;
            if(nk_option_label(ctx, "mesh", cas->createType == 1)) cas->createType = 1;
            if(nk_option_label(ctx, "object", cas->createType == 2)) cas->createType = 2;

            nk_layout_row_dynamic(ctx, 10, 1);
            //nk_spacing(ctx, 1);

            switch(cas->createType)
            {
                case 0: // texture
                    break;
                case 1: // mesh
                    break;
                case 2: // object
                    nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                    nk_layout_row_push(ctx, 120);
                    nk_label(ctx, "Name:", NK_TEXT_LEFT);
                    nk_layout_row_push(ctx, 200);
                    static_assert(TTR_MAX_NAME_LEN <= ARRAY_COUNT(cas->assetNameBuf), "..");
                    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, cas->assetNameBuf, TTR_MAX_NAME_LEN, 0);
                    nk_layout_row_end(ctx);

                    nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
                    nk_layout_row_push(ctx, 120);
                    nk_label(ctx, "Mesh asset:", NK_TEXT_LEFT);
                    nk_layout_row_push(ctx, 200);
                    nk_label(ctx, cas->objectAssetNameBuf, NK_TEXT_LEFT);
                    nk_layout_row_push(ctx, 80);
                    if(nk_button_label(ctx, "Pick"))
                    {
                        asset_picker_init(&cas->pickerState);
                        cas->pickerState.show = true;
                    }
                    if(render_asset_picker(ctx, as, TTR_4CHAR("MESH"), &cas->pickerState))
                    {
                        strcpy(cas->objectAssetNameBuf, cas->pickerState.result->cstr);
                        cas->pickerState.show = false;
                    }
                    nk_layout_row_end(ctx);

                    nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                    nk_layout_row_push(ctx, 120);
                    nk_label(ctx, "Material:", NK_TEXT_LEFT);
                    nk_layout_row_push(ctx, 200);
                    static int current_material;
                    static const char *materials[] = {"None","Unlit color","Unlit Vertex color","Unlit textured","Unlit fade","Gizmo"};
                    current_material = nk_combo(ctx, materials, ARRAY_COUNT(materials), current_material, 25, nk_vec2(nk_widget_width(ctx),200));
                    nk_layout_row_end(ctx);
                    break;
            };

            nk_layout_row_dynamic(ctx, 30, 1);

            nk_spacing(ctx, 1);

            if(nk_button_label(ctx, "Create"))
            {
                cas->showAddWindow = false;
            }
        }
        else
        {
            cas->showAddWindow = false;
            nk_window_show(ctx, "Add asset", NK_SHOWN);
        }
        nk_end(ctx);
    }

    if(nk_begin(ctx, "Create asset", nk_rect(10, 10, 800, 600),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE))
    {
        struct nk_list_view view;
        nk_layout_row_static(ctx, 30, 200, 1);
        if(nk_button_label(ctx, "Add asset"))
        {
            init_create_asset_state(cas);
            cas->showAddWindow = true;
        }
        if(nk_button_label(ctx, "Import asset"))
        {
            // TODO: gui to import asset from existing package
        }
        nk_layout_row_dynamic(ctx, 400, 1);
        if(nk_group_begin(ctx, "cassets", NK_WINDOW_BORDER))
        {
            for (int i = 0; i < 100; i++) 
            {
                char buf[512];
                stbsp_sprintf(buf, "testings %d", i);
                nk_layout_row_dynamic(ctx, 20, 3);
                nk_label(ctx, buf, NK_TEXT_LEFT);
                nk_label(ctx, "image", NK_TEXT_LEFT);
                nk_label(ctx, "reference", NK_TEXT_LEFT);
            }
            nk_group_end(ctx);
        }

        *wasClosed = false;
    }
    else
    {
        *wasClosed = true;
        nk_window_show(ctx, "Create asset", NK_SHOWN);
    }
    nk_end(ctx);
}

void render_command_box(struct TessUISystem *ui, struct nk_context *ctx, char *buf, uint32_t bufSize)
{
    if(nk_begin(ctx, "CommandBox", nk_rect(0, 0, ui->width, 40), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_edit_focus(ctx, NK_EDIT_FIELD|NK_EDIT_GOTO_END_ON_ACTIVATE);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, bufSize, 0);
    }
    nk_end(ctx);
    nk_window_set_focus(ctx, "CommandBox");
}



