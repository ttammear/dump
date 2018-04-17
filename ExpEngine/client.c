void tess_client_init(struct TessClient *tess, AikePlatform *platform, struct Renderer *renderer)
{
    memset(tess, 0, sizeof(struct TessClient));
    tess->platform = platform;

    fixed_arena_init(platform, &tess->arena, 512 * 1024); 

    // Init strings
    //
    tess_strings_init(&tess->strings, &tess->arena);

    // Init file system
    //
    tess->fileSystem.platform = platform;
    // Virtual table tells file system where to pass the data once it's done loading
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_None] = NULL;
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_TTR] = (FilePipelineProc)tess_process_ttr_file;
    tess->fileSystem.pipeline_ptrs[Tess_File_Pipeline_TTR] = &tess->assetSystem;
    tess_file_system_init(&tess->fileSystem, &tess->arena);

    // Init asset system
    //
    tess->assetSystem.fileSystem = &tess->fileSystem;
    tess->assetSystem.tstrings = &tess->strings;
    tess->assetSystem.renderer = renderer;
    tess_asset_system_init(&tess->assetSystem, &tess->arena);

    // Init game system
    //
    for(int i = 0; i < ARRAY_COUNT(tess->gameSystem.objectTable); i++)
    {
        tess->gameSystem.objectTable[i] = (struct TessObject){0};
        tess->gameSystem.objectTable[i].id = i;
    }
    tess->gameSystem.assetSystem = &tess->assetSystem;
    POOL_FROM_ARENA(tess->gameSystem.entityPool, &tess->arena, TESS_MAX_ENTITIES);
    tess->gameSystem.activeEntities = NULL;
    tess->gameSystem.renderSystem = &tess->renderSystem;
    tess->gameSystem.activeCamera = &tess->gameSystem.defaultCamera;

    // Init input
    //
    tess->inputSystem.platform = platform;
    tess_input_init(&tess->inputSystem);

    // Init render system
    //
    render_system_init(&tess->renderSystem);
    tess->renderSystem.renderer = renderer;

    // Init UI
    //
    tess->uiSystem.renderSystem = &tess->renderSystem;
    tess->uiSystem.platform = platform;
    tess_ui_init(&tess->uiSystem);

    // Init menu
    tess->mainMenu.uiSystem = &tess->uiSystem;
    tess->mainMenu.inputSystem = &tess->inputSystem;
    tess->mainMenu.client = tess;
    tess_main_menu_init(&tess->mainMenu);

    // Init editor
    tess->editor.init = false;
    tess->editor.renderSystem = &tess->renderSystem;
    tess->editor.platform = platform;
    tess->editor.uiSystem = &tess->uiSystem;
    tess->editor.client = tess;
    tess->editor.inputSystem = &tess->inputSystem;
    tess->editor.world = &tess->gameSystem;
    editor_init(&tess->editor);
}

void tess_client_destroy(struct TessClient *tess)
{
    // Destroy file system
    
    // Destroy asset system
    tess_asset_system_destroy(&tess->assetSystem);

    // Destroy strings
    tess_strings_destroy(&tess->strings);

    // Destroy game system
    buf_free(tess->gameSystem.activeEntities);

    // Destroy render system
    render_system_destroy(&tess->renderSystem);

    // Destroy UI
    tess_ui_destroy(&tess->uiSystem);

    // Destroy tess 
    fixed_arena_free(tess->platform, &tess->arena);

    // Destroy editor
    editor_destroy(&tess->editor);
}

void tess_client_begin_frame(struct TessClient *client)
{
    tess_input_begin(&client->inputSystem);
    render_system_begin_update(&client->renderSystem);
    tess_ui_begin(&client->uiSystem);

    tess_render_entities(&client->gameSystem);
}

void tess_client_end_frame(struct TessClient *client)
{
    tess_ui_end(&client->uiSystem);
    render_system_end_update(&client->renderSystem);
    tess_input_end(&client->inputSystem);
}

void tess_client_exit(struct TessClient *client)
{
    client->platform->exit(client->platform);
}
