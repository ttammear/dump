
typedef struct GameData
{
    struct RenderViewBuilder *viewBuilder;
    AikePlatform *platform;

    struct nk_context nk_ctx;
    struct nk_font_atlas atlas;
    struct nk_font *font; 
    struct nk_draw_null_texture null;
    struct nk_buffer cmds;

    int w;
    int h;
    const void *image;

    bool ui_ready;

    struct V3 camPos;

    int s_x, s_y;
    void *s_texdata;
    float rotval;

} GameData;

void atlas_ready(struct Renderer *renderer, struct TextureQueryResponse *tqr, void *userData)
{
    struct GameData *gdata = (struct GameData*)userData;
    RenderMessage response = {};
    memcpy(tqr->textureDataPtr, gdata->image, gdata->w*gdata->h*4);
    nk_font_atlas_end(&gdata->atlas, nk_handle_id((int)tqr->textureId), &gdata->null);
    nk_init_fixed(&gdata->nk_ctx, calloc(1, 1024*1024), 1024*1024, &gdata->font->handle);
    gdata->ui_ready = true;
    response.type = Render_Message_Texture_Update;
    response.texU.textureId = tqr->textureId;
    renderer_queue_message(renderer, &response);
}

void fill_cat_texture(struct Renderer *renderer, struct TextureQueryResponse *tqr, void *userData)
{
    struct GameData *gdata = (struct GameData*)userData;
    uint32_t texsize = gdata->s_x*gdata->s_y*4;
    memcpy(tqr->textureDataPtr, gdata->s_texdata, texsize);
    stbi_image_free(gdata->s_texdata);

    RenderMessage response = {};
    response.type = Render_Message_Texture_Update;
    response.texU.textureId = tqr->textureId;
    renderer_queue_message(renderer, &response);
}

/*void loadDummyMesh(const char *path, struct Renderer *renderer)
{
    static bool loaded;
    if(loaded)
        return;

    FILE *file = fopen(path, "rb");
    if(file)
    {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        void *buf = malloc(size);
        assert(buf);
        fread(buf, 1, size, file);

        struct LoadMeshData *mdata = malloc(sizeof(struct LoadMeshData));
        mdata->renderer = renderer;
        mdata->fileMem = buf;
        mdata->dataSize = size;
        ttr_load_first_mesh(mdata, 0);
        loaded = true;
    }
    else
        fprintf(stderr, "Loading %s failed!\n", path);
}*/

void init_game(struct Renderer *renderer, struct GameData *gdata, struct TessClient *tess)
{
    int comp;
    gdata->s_texdata = stbi_load("./cat.png", &gdata->s_x, &gdata->s_y, &comp, 4);
    assert(gdata->s_texdata != NULL);
    
    RenderMessage msg = {};

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = NULL;
    msg.matQ.materialId = 0;
    msg.matQ.shaderId = Shader_Type_Unlit_Textured;
    msg.matQ.iData.unlitTextured.textureId = 4;
    msg.matQ.iData.unlitTextured.color = make_v4(1.0f, 1.0f, 1.0f, 1.0f);
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = NULL;
    msg.matQ.materialId = 0;
    msg.matQ.shaderId = Shader_Type_Unlit_Vertex_Color;
    msg.matQ.iData.unlitVertexColor.color = make_v4(1.0f, 1.0f, 1.0f, 1.0f);
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = gdata;
    msg.texQ.onComplete = fill_cat_texture;
    msg.texQ.textureId = 0;
    msg.texQ.width = gdata->s_x;
    msg.texQ.height = gdata->s_y;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Trilinear;
    renderer_queue_message(renderer, &msg);

    // GUI
    //const char *font_path = "FreeMono.ttf";
    const char *font_path = NULL;
    nk_buffer_init_default(&gdata->cmds);
    nk_font_atlas_init_default(&gdata->atlas);
    nk_font_atlas_begin(&gdata->atlas);
    if (font_path) gdata->font = nk_font_atlas_add_from_file(&gdata->atlas, font_path, 13.0f, NULL);
    else gdata->font = nk_font_atlas_add_default(&gdata->atlas, 13.0f, NULL);

    gdata->image = nk_font_atlas_bake(&gdata->atlas, &gdata->w, &gdata->h, NK_FONT_ATLAS_RGBA32);

    msg = (RenderMessage){};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = gdata;
    msg.texQ.onComplete = atlas_ready;
    msg.texQ.width = gdata->w;
    msg.texQ.height = gdata->h;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Bilinear;
    renderer_queue_message(renderer, &msg);

    //nk_init_default(&ctx, &font->handle);}
    gdata->ui_ready = false;
}

void deinit_game()
{
}

struct CreateObjectsWindowState
{
    char fileName[64];
    char packageNames[5][64];
    char assetNames[5][64];
};

void write_image()
{
    int w, h, comp;
    void *data = stbi_load("./image.png", &w, &h, &comp, 4);

    void *buf = malloc(10 * 1024 * 1024);
    TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(TTRHeader);
    TTRDescTbl *dtbl = STREAM_PUSH_FLEX(stream, TTRDescTbl, entries, 1);
    dtbl->entryCount = 1;
    TTRImportTbl *itbl = STREAM_PUSH_FLEX(stream, TTRImportTbl, entries, 0);
    itbl->entryCount = 0;

    TTRTexture *ttex = STREAM_PUSH(stream, TTRTexture);
    ttex->format = 1;
    ttex->width = w;
    ttex->height = h;

    TTRBuffer *tbuf = STREAM_PUSH_FLEX(stream, TTRBuffer, data, w*h*4);
    tbuf->size = w*h*4;
    memcpy(tbuf->data, data, w*h*4);
    TTR_SET_REF_TO_PTR(ttex->bufRef, tbuf);

    header->majorVersion = 0;
    header->minorVersion = 1;
    TTR_SET_REF_TO_PTR(header->descTblRef, dtbl);
    TTR_SET_REF_TO_PTR(header->importTblRef, itbl);

    dtbl->entries[0].type = TTR_4CHAR("TEX ");
    strcpy(dtbl->entries[0].assetName, "Screenshot");
    TTR_SET_REF_TO_PTR(dtbl->entries[0].ref, ttex);

    FILE *file = fopen("./image.ttr", "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing texture file failed!\n");


    free(buf);
    stbi_image_free(data);
}

void write_object(struct CreateObjectsWindowState *cws)
{
    int count = 0;
    for(int i = 0; i < 5; i++)
    {
        if(cws->packageNames[i][0] != 0 && cws->assetNames[i][0] != 0)
            count++;
    }

    void *buf = malloc(1024*1024);
    struct TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(struct TTRHeader);

    struct TTRDescTbl *dtbl = STREAM_PUSH_FLEX(stream, struct TTRDescTbl, entries, count);
    dtbl->entryCount = count;

    struct TTRImportTbl *itbl = STREAM_PUSH_FLEX(stream, struct TTRImportTbl, entries, count);
    itbl->entryCount = count;

    for(int i = 0; i < count; i++)
    {
        struct TTRObject *obj = STREAM_PUSH(stream, struct TTRObject);
        struct TTRMaterial *mat = STREAM_PUSH(stream, struct TTRMaterial);
        TTR_SET_REF_TO_PTR(obj->materialRef, mat);
        mat->shaderType = Shader_Type_Unlit_Vertex_Color;
        mat->tintColor = make_v4(1.0f, 0.0f, 0.0f, 1.0f);
        obj->meshARef.tblIndex = i | TTR_AREF_EXTERN_MASK;
        dtbl->entries[i].type = TTR_4CHAR("OBJ ");
        TTR_SET_REF_TO_PTR(dtbl->entries[i].ref, obj);
        char strBuf[64];
        strcpy(strBuf, "object");
        switch(i)
        {
            case 0: strcat(strBuf, "0"); break;
            case 1: strcat(strBuf, "1"); break;
            case 2: strcat(strBuf, "2"); break;
            case 3: strcat(strBuf, "3"); break;
            case 4: strcat(strBuf, "4"); break;
        };
        strncpy(dtbl->entries[i].assetName, strBuf, TTR_MAX_NAME_LEN);

        itbl->entries[i].type = TTR_4CHAR("MESH");
        strncpy(itbl->entries[i].assetName, cws->assetNames[i], TTR_MAX_NAME_LEN);
        strncpy(itbl->entries[i].packageName, cws->packageNames[i], TTR_MAX_NAME_LEN);
    }

    header->majorVersion = 0;
    header->minorVersion = 1;
    TTR_SET_REF_TO_PTR(header->descTblRef, dtbl);
    TTR_SET_REF_TO_PTR(header->importTblRef, itbl);

    char filePath[AIKE_MAX_PATH];
    strcpy(filePath, "Packages/First/");
    strcat(filePath, cws->fileName);
    FILE *file = fopen(filePath, "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing object file failed!\n");

    free(buf);
}


void update_game(struct GameData *gdata)
{
    PROF_START();
    gdata->rotval += 0.01f;

#pragma pack(push, 1)
    struct TestInstanceData
    {
        struct V4 v1;
        //struct V2 v2;
    };
#pragma pack(pop)

    struct TestInstanceData data;
    data.v1 = (struct V4){1.0f, 1.0f, 0.0f, 0.0f};

    struct Mat4 model;
    srand(1);

    struct V3 one = (struct V3){1.0f, 1.0f, 1.0f};

    static uint32_t pyMat = 0;

    PROF_START_STR("Add mesh instances");


    struct Quat rot;
    struct Quat rot2;
    for(int i = 0; i < 100; i++)
    for(int j = 0; j < 100; j++)
    {
        quat_angle_axis(&rot, gdata->rotval*70.0f * ((float)j / 10.0f), make_v3(0.0f, 1.0f, 0.0f));
        quat_angle_axis(&rot2, -gdata->rotval*140.0f * ((float)i / 10.0f), make_v3(0.0f, 1.0f, 0.0f));
        bool cube = j%2==0;
        //data.v1 = cols[rand() % ARRAY_COUNT(cols)];
        mat4_trs(&model, make_v3((float)i*2 - 100.0f, -5.0f, 10.0f + (float)j*2 - 17.0f), cube?rot:rot2, one);
        add_mesh_instance(gdata->viewBuilder, cube?1:0, cube?pyMat:1, &model, -1);
    }

    PROF_END();

    //float width = gdata->platform->mainWin.width;
    //float height = gdata->platform->mainWin.height;
    // NOTE: currently backbuffer isn't resized with window
    float width = 1024;
    float height = 768;

    gdata->viewBuilder->materialId = 3;

    struct Mat4 orthoM = {};
    orthoM.m11 =  2.0f / width;
    orthoM.m22 = -2.0f / height;
    orthoM.m33 = 1.0f;
    orthoM.m44 =  1.0f;
    orthoM.m14 = -1.0f;
    orthoM.m24 =  1.0f;
    gdata->viewBuilder->orthoMatrix = orthoM;

    // ------------------------------------------------

    PROF_START_STR("Nuklear GUI");

    struct nk_context *ctx = &gdata->nk_ctx;

    int mx = (int)gdata->platform->mouseX;
    int my = (int)gdata->platform->mouseY;
    uint16_t *akeys = gdata->platform->keyStates;

    nk_input_begin(ctx);
    nk_input_motion(ctx, mx, my);
    nk_input_button(ctx, NK_BUTTON_LEFT, mx, my, akeys[AIKE_BTN_LEFT] != 0);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, mx, my, akeys[AIKE_BTN_MIDDLE] != 0);
    nk_input_button(ctx, NK_BUTTON_RIGHT, mx, my, akeys[AIKE_BTN_RIGHT] != 0);

    nk_input_key(ctx, NK_KEY_BACKSPACE, akeys[AIKE_KEY_BACKSPACE] != 0);
    nk_input_key(ctx, NK_KEY_TAB, akeys[AIKE_KEY_TAB] != 0);

    if(akeys[AIKE_KEY_W])
        gdata->camPos.z += 0.1f;
    if(akeys[AIKE_KEY_S])
        gdata->camPos.z -= 0.1f;
    if(akeys[AIKE_KEY_A])
        gdata->camPos.x -= 0.1f;
    if(akeys[AIKE_KEY_D])
        gdata->camPos.x += 0.1f;

    uint32_t next = 0;
    while((next = gdata->platform->next_character(gdata->platform)))
    {
        nk_input_char(ctx, (char)next);
    }
    nk_input_end(ctx);

    enum {EASY, HARD};
    static int op = EASY;
    static float value = 0.6f;
    static int i =  20;

    static struct CreateObjectsWindowState cwState;

    if(gdata->ui_ready)
    {
        if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 400),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {

            nk_layout_row_static(ctx, 30, 100, 1);
            if (nk_button_label(ctx, "Load dummy")) {
            }

            /* fixed widget window ratio width */
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            /* custom widget pixel width */
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
            nk_layout_row_end(ctx);

            nk_layout_row_dynamic(ctx, 30, 1);
            //nk_property_int(ctx, "#Mesh:", 0, (int*)&pyramid, 20, 1, 0.1f);

            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_layout_row_push(ctx, 60);
                nk_label(ctx, "Material:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                pyMat = nk_slide_int(ctx, 0, pyMat, 2, 1);
            nk_layout_row_end(ctx);

            nk_layout_row_dynamic(ctx, 30, 1);
            nk_property_int(ctx, "#Material:", 0, (int*)&pyMat, 2, 1, 0.1f);
        }
        nk_end(ctx);

        if(nk_begin(ctx, "Create object", nk_rect(400, 50, 350, 500),
                    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            if(nk_button_label(ctx, "Gen img"))
            {
                write_image();
            }
            nk_layout_row_dynamic(ctx, 30, 1);
            if(nk_button_label(ctx, "Generate"))
            {
                printf("Generate\n");
                write_object(&cwState);
            }
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_layout_row_push(ctx, 70);
                nk_label(ctx, "Filename:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 150);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, cwState.fileName, ARRAY_COUNT(cwState.fileName), 0);
            nk_layout_row_end(ctx);
            for(int j = 0; j < 5; j++)
            {
                nk_layout_row_dynamic(ctx, 25, 1);
                nk_label(ctx, "Entry:", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 25, 2);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, cwState.packageNames[j], ARRAY_COUNT(cwState.packageNames[j]), 0);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, cwState.assetNames[j], ARRAY_COUNT(cwState.assetNames[j]), 0);
            }
        }
        nk_end(ctx);

        render_profiler(ctx);
    }

    // draw commands
    builder_new_vertex_stream(gdata->viewBuilder);

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
        config.null = gdata->null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        /* setup buffers to load vertices and elements */
        {struct nk_buffer vbuf, ebuf;
        nk_buffer_init_fixed(&vbuf, vertBuf, sizeof(vertBuf[0])*maxVerts);
        nk_buffer_init_fixed(&ebuf, indexBuf, sizeof(indexBuf[0])*maxIndices);
        nk_convert(ctx, &gdata->cmds, &vbuf, &ebuf, &config);}

        add_vertices(gdata->viewBuilder, vertBuf, maxVerts);

        float sx = 1.0f;
        float sy = 1.0f;

        uint32_t curIdxBase = 0;
        nk_draw_foreach(cmd, ctx, &gdata->cmds) {
            if (!cmd->elem_count) continue;
            int scX0 = (cmd->clip_rect.x * sx);
            int scY0 = ((height - (int32_t)(cmd->clip_rect.y + cmd->clip_rect.h)) * sy);
            int scX1 = (cmd->clip_rect.w * sx);
            int scY1 = (cmd->clip_rect.h * sy);

            builder_begin_batch(gdata->viewBuilder, cmd->texture.id, scX0, scY0, scX1, scY1);
            for(int j = 0; j < cmd->elem_count; j+=3)
            {
                add_index(gdata->viewBuilder, indexBuf[j+curIdxBase]);
                add_index(gdata->viewBuilder, indexBuf[j+2+curIdxBase]);
                add_index(gdata->viewBuilder, indexBuf[j+1+curIdxBase]);
            }
            builder_end_batch(gdata->viewBuilder);
            curIdxBase += cmd->elem_count;
        }
    }

    nk_clear(ctx);
    PROF_END();
    PROF_END();
}
