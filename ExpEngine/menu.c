enum TessMainMenuMode
{
    Tess_Main_Menu_Mode_Menu,
    Tess_Main_Menu_Mode_Connect_Editor,
};

void tess_main_menu_init(struct TessMainMenu *menu)
{
    menu->mode = Tess_Main_Menu_Mode_Menu;
    menu->nk_ctx = &menu->uiSystem->nk_ctx; 
    strcpy(menu->ipStrBuf, "127.0.0.1");
    strcpy(menu->portStrBuf, "7879");
    menu->statusStr = "";
}

void load_map(TessClient *client)
{
    tess_reset_world(&client->gameSystem);
    void *buf = malloc(1024*1024);
    FILE *file = fopen("Packages/Sponza/map.ttm", "rb");
    if(file != NULL)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
        fread(buf, 1, size, file);
        fclose(file);

        MapReader reader;
        bool res = map_reader_begin(&reader, buf, size);
        if(!res)
        {
            fprintf(stderr, "Loading map failed!\n");
            goto load_done;
        }
        MapObject obj;
        while(map_next_object(&reader, &obj) == Map_Error_Code_Success)
        {
            TStr *assetId = tess_intern_string_s(&client->strings, obj.assetId, sizeof(obj.assetId));
            tess_register_object(&client->gameSystem, obj.objectId, assetId);
        }
        MapEntity ent;
        while(map_next_entity(&reader, &ent) == Map_Error_Code_Success)
        {
            Mat4 modelToWorld;
            mat4_trs(&modelToWorld, ent.pos, ent.rot, ent.scale);
            tess_create_entity(&client->gameSystem, ent.objectId, &modelToWorld);
        }
    }
load_done:
    free(buf);
}

void draw_main_menu(struct TessMainMenu *menu, struct nk_context *ctx)
{
    int32_t width = 400;
    int32_t height = 500;
    int32_t winW = menu->uiSystem->width; // TODO: real size
    int32_t winH = menu->uiSystem->height;
    float x = (winW-width)/2;
    float y = (winH-height)/2;
    if(nk_begin(ctx, "Main menu", nk_rect(x, y, width, height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, 60, 1);
        if(nk_button_label(ctx, "Play"))
        {
            load_map(menu->client);
            menu->client->gameSystem.defaultCamera.position = make_v3(10.0f, 0.0f, -30.0f);
            menu->client->mode = Tess_Client_Mode_Game;
        }
        if(nk_button_label(ctx, "Editor"))
        {
            menu->mode = Tess_Main_Menu_Mode_Connect_Editor;
            //menu->client->mode = Tess_Client_Mode_Editor;
        }
        if(nk_button_label(ctx, "Exit"))
        {
            tess_client_exit(menu->client);
        }
    }
    nk_end(ctx);
}

void draw_editor_connect_menu(struct TessMainMenu *menu, struct nk_context *ctx)
{
    int32_t width = 400;
    int32_t height = 500;
    int32_t winW = menu->uiSystem->width; // TODO: real size
    int32_t winH = menu->uiSystem->height;
    float x = winW > width ? (winW-width)/2 : 0;
    float y = winH > height ? (winH-height)/2 : 0;
    if(nk_begin(ctx, "Connect Editor", nk_rect(x, y, width, height), NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER))
    {
        nk_layout_row_static(ctx, 15, 100, 1);
        nk_layout_row_static(ctx, 30, 200, 1);
        nk_label(ctx, "Connect to editor server", NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 15, 100, 1);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 4);
            nk_layout_row_push(ctx, 0.08f);
            nk_label(ctx, "Ip:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 0.5f);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, menu->ipStrBuf, sizeof(menu->ipStrBuf), 0);
            nk_layout_row_push(ctx, 0.03f);
            nk_label(ctx, "", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 0.16f);
            nk_label(ctx, "Port:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 0.2f);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE|NK_EDIT_NO_CURSOR, menu->portStrBuf, sizeof(menu->portStrBuf)-1, nk_filter_decimal);
            nk_layout_row_push(ctx, 0.03f);
        nk_layout_row_end(ctx);

        strcpy(menu->client->editor.ipStr, menu->ipStrBuf);
        menu->client->editor.port = atoi(menu->portStrBuf);
        
        nk_layout_row_static(ctx, 20, 100, 1);
            nk_layout_row_dynamic(ctx, 30, 2);
            if(nk_button_label(ctx, "Back"))
            {
                menu->mode = Tess_Main_Menu_Mode_Menu;
            }
            if(nk_button_label(ctx, "Connect"))
            {
                menu->client->mode = Tess_Client_Mode_Editor;
                menu->mode = Tess_Main_Menu_Mode_Menu;
            }

        nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, menu->statusStr, NK_TEXT_LEFT);
    }
    nk_end(ctx);

}

void tess_main_menu_update(struct TessMainMenu *menu)
{
    struct nk_context *ctx = menu->nk_ctx;
    switch(menu->mode)
    {
        case Tess_Main_Menu_Mode_Connect_Editor:
            draw_editor_connect_menu(menu, ctx);
            break;
        default:
        case Tess_Main_Menu_Mode_Menu:
            draw_main_menu(menu, ctx);
            break;
    }
}
