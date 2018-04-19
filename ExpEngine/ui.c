
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
        ui->loaded = false;
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

    int mx = (int)ui->platform->mouseX;
    int my = (int)ui->platform->mouseY;
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
    nk_input_end(ctx);
}

void tess_ui_end(struct TessUISystem *ui)
{
    struct nk_context *ctx = &ui->nk_ctx;

    //float width = gdata->platform->mainWin.width;
    //float height = gdata->platform->mainWin.height;
    // NOTE: currently backbuffer isn't resized with window
    float width = 1024;
    float height = 768;

    struct Mat4 orthoM = {};
    orthoM.m11 =  2.0f / width;
    orthoM.m22 = -2.0f / height;
    orthoM.m33 = 1.0f;
    orthoM.m44 =  1.0f;
    orthoM.m41 = -1.0f;
    orthoM.m42 =  1.0f;
    ui->renderSystem->viewBuilder->orthoMatrix = orthoM;
    ui->renderSystem->viewBuilder->materialId = 3;

    builder_new_vertex_stream(ui->renderSystem->viewBuilder);

    uint32_t maxVerts = 20000;
    uint32_t maxIndices = 12000;

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
            assert(vertCount < maxVerts);
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
    }

    nk_clear(ctx);
}

// PROFILER

void profile_entry_recursive(struct nk_context *ctx, struct ProfileEntry *entry, int depth)
{
    while(entry != NULL)
    {
        uint64_t cycles = entry->sum;
        double milliseconds = (double)cycles / 2800000.0;
        /*for(int i = 0; i < depth; i++)
            printf(" "); */
        char buf[1024];
        stbsp_sprintf(buf, "%s %fms", entry->locationStr, milliseconds);
        if(nk_tree_push_id(ctx, NK_TREE_TAB, buf, NK_MINIMIZED, (uint32_t)entry->locationStr))
        {
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "%llucy %fms", (unsigned long long)cycles, milliseconds);
            profile_entry_recursive(ctx, entry->firstChild, depth+1);
            nk_tree_pop(ctx);
        }
        entry = entry->nextSibling;
    }
}

void render_profiler(struct nk_context *ctx)
{
    if(nk_begin(ctx, "Profile", nk_rect(400, 350, 300, 400),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE))
    {
        for(int j = 0; j < ARRAY_COUNT(g_profStates); j++)
        {
            uintptr_t pptr = atomic_load(&g_profStates[j]);
            if(pptr != (uintptr_t)NULL)
            {
                struct ProfilerState *profState = (struct ProfilerState*)pptr;
                uintptr_t rootUptr = atomic_load(&profState->prev);
                struct ProfileEntry *root = (struct ProfileEntry*)rootUptr;
                if(nk_tree_push_id(ctx, NK_TREE_TAB, profState->name, NK_MINIMIZED, j))
                {
                    nk_layout_row_dynamic(ctx, 25, 2);
                    if (!atomic_load(&profState->pause) && nk_button_label(ctx, "Pause")) 
                    {
                        atomic_store(&profState->pause, true);
                    }
                    else if (atomic_load(&profState->pause) && nk_button_label(ctx, "Resume")) 
                    {
                        atomic_store(&profState->pause, false);
                    }
                    if(root)
                        profile_entry_recursive(ctx, root->firstChild, 0);
                    nk_tree_pop(ctx);
                }
            }
        }
    }
    nk_end(ctx);
}

