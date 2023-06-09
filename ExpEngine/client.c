void tess_client_init(TessClient *tess, AikePlatform *platform, Renderer *renderer)
{
    memset(tess, 0, sizeof(TessClient));
    tess->platform = platform;

    fixed_arena_init(platform, &tess->arena, 30 * 1024 * 1024); 

    // Init strings
    //
    tess_strings_init(&tess->strings, &tess->arena);

    // Init file system
    //
    tess->fileSystem.platform = platform;
    // Virtual table tells file system where to pass the data once it's done loading
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_None] = NULL;
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_Task] = (FilePipelineProc)tess_task_file_loaded;
    tess->fileSystem.pipeline_ptrs[Tess_File_Pipeline_Task] = NULL;
    tess_file_system_init(&tess->fileSystem, &tess->arena);

    // Init asset system
    //
    tess->assetSystem.fileSystem = &tess->fileSystem;
    tess->assetSystem.tstrings = &tess->strings;
    tess->assetSystem.renderer = renderer;
    tess->assetSystem.physics = NULL; 
    tess->assetSystem.isServer = false;
    tess_asset_system_init(&tess->assetSystem, &tess->arena);

    // Init game system
    //
    for(int i = 0; i < ARRAY_COUNT(tess->gameSystem.objectTable); i++)
    {
        tess->gameSystem.objectTable[i] = (TessObject){0};
        tess->gameSystem.objectTable[i].id = i;
    }
    tess->gameSystem.assetSystem = &tess->assetSystem;
    POOL_FROM_ARENA(tess->gameSystem.entityPool, &tess->arena, TESS_MAX_ENTITIES);
    tess->gameSystem.activeEntities = NULL;
    tess->gameSystem.renderSystem = &tess->renderSystem;
    tess->gameSystem.activeCamera = &tess->gameSystem.defaultCamera;
    tess->gameSystem.tstrings = &tess->strings;
    uint32_t size = create_physx_physics(NULL);
    tess->gameSystem.physics = malloc(size);
    create_physx_physics(tess->gameSystem.physics);
    tess->gameSystem.physics->init(tess->gameSystem.physics, false);
    tess_world_init(&tess->gameSystem);
    tess->assetSystem.physics = tess->gameSystem.physics; 

    // Init input
    //
    tess->inputSystem.platform = platform;
    tess->inputSystem.renderSystem = &tess->renderSystem;
    tess_input_init(&tess->inputSystem);

    // Init render system
    //
    render_system_init(&tess->renderSystem);
    tess->renderSystem.renderer = renderer;
    tess->renderSystem.platform = platform;

    // Init UI
    //
    tess->uiSystem.renderSystem = &tess->renderSystem;
    tess->uiSystem.inputSystem = &tess->inputSystem;
    tess->uiSystem.platform = platform;
    tess_ui_init(&tess->uiSystem);

    // Init menu
    tess->mainMenu.uiSystem = &tess->uiSystem;
    tess->mainMenu.inputSystem = &tess->inputSystem;
    tess->mainMenu.client = tess;
    tess_main_menu_init(&tess->mainMenu);

    // Init editor
    void *mem = fixed_arena_push_size(&tess->arena, 1024 * 1024, 64);
    tess->editor.init = false;
    tess->editor.renderSystem = &tess->renderSystem;
    tess->editor.platform = platform;
    tess->editor.uiSystem = &tess->uiSystem;
    tess->editor.client = tess;
    tess->editor.inputSystem = &tess->inputSystem;
    tess->editor.world = &tess->gameSystem;
    tess->editor.tstrings = &tess->strings;

    // Init game client
    mem = fixed_arena_push_size(&tess->arena, 1024 * 1024, 64);
    memset(&tess->gameClient, 0, sizeof(GameClient));
    tess->gameClient.init = false;
    tess->gameClient.client = tess;
    tess->gameClient.world = &tess->gameSystem;
    tess->gameClient.platform = platform;
    tess->gameClient.strings = &tess->strings;
    tess->gameClient.assetSystem = &tess->assetSystem;
    POOL_FROM_ARENA(tess->gameClient.dynEntityPool, &tess->arena, TESS_CLIENT_MAX_DYN_ENTITIES);
}

void tess_client_destroy(TessClient *tess)
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
}

void tess_client_begin_frame(TessClient *client)
{
    PROF_BLOCK();
    tess_input_begin(&client->inputSystem);
    render_system_begin_update(&client->renderSystem);
    tess_ui_begin(&client->uiSystem);

    tess_render_entities(&client->gameSystem);
}

void tess_client_end_frame(TessClient *client)
{
    PROF_BLOCK();
    tess_ui_end(&client->uiSystem);
    render_system_end_update(&client->renderSystem);
    tess_input_end(&client->inputSystem);
}

void tess_client_exit(TessClient *client)
{
    client->platform->exit(client->platform);
}
