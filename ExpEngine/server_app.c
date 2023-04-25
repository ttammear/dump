
typedef struct TessServerRoot
{
    TessScheduler scheduler;
    TessServer server;
    AikeMemoryBlock *rootBlock;
    bool loaded; // TODO: remove this hack
    //aike_thread_local struct ProfilerState *t_profState;
    uintptr_t g_profStates[10];
    struct ProfilerState *main_profState;
} TessServerRoot;

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    printf("resize %f %f\n", win->width, win->height);

/*    TessRoot *root = (TessRoot*)platform->userData;
    if(root != NULL)
    {
        // TODO: this is here because we dont have render targets yet
        // when we do, the main window will just be a render target
        // and when the width/height is changed it will be marked dirty
        // and all the cameras rendering to it will recalculate aspectRatio
        int w = root->client.renderSystem.rtW;
        int h = root->client.renderSystem.rtH;
        root->client.gameSystem.defaultCamera.aspectRatio = (float)w/(float)h;
        tess_update_camera_perspective(&root->client.gameSystem.defaultCamera);

        render_system_screen_resize(&root->client.renderSystem, win->width, win->height);
    }*/
}

void aike_init(AikePlatform *platform)
{
    DEBUG_INIT("Main server thread");
    DEBUG_START_FRAME();

    //tess_reload_vtable();

    platform->init_async_io(platform);

    /*platform->create_opengl_context(platform, &platform->mainWin);
    bool sinter = platform->swap_interval(&platform->mainWin, 1);
    if(!sinter)
    {
        fprintf(stderr, "Failed to set swap interval!\n");
        assert(false);
    }*/


    //Renderer *renderer = create_renderer(RENDERER_TYPE_OPENGL, platform);

    AikeMemoryBlock *rootBlock = platform->allocate_memory(platform, sizeof(TessServerRoot), 0);
    TessServerRoot *root = (TessServerRoot*)rootBlock->memory;
    root->rootBlock = rootBlock;
    root->loaded = false;
    platform->userData = root;

    // TODO: scheduler should not use client's arena
    scheduler_init(&root->scheduler, platform, NULL, &root->server);

    tess_server_init(&root->server, platform);


    platform->make_window_current(platform, NULL);
    //renderer->swapBuffer = root->client.renderSystem.viewSwapBuffer;
    //start_renderer(renderer);

    /**tess_refresh_package_list(&root->client.assetSystem);
    int count = buf_len(root->client.assetSystem.packageList);
    for(int i = 0; i < count; i++)
    {
        TStr *packageName = root->client.assetSystem.packageList[i];
        printf("Generating (client) lookup cache for package '%s'\n", packageName->cstr);
        tess_gen_lookup_cache_for_package(&root->client.assetSystem, packageName);
    }*/

    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
        assert(0);
    }

    printf("client as %p server as %p\r\n", NULL, &root->server.assetSystem);

    DEBUG_END_FRAME();
}

void aike_deinit(AikePlatform *platform)
{
    TessServerRoot *root = (TessServerRoot*)platform->userData;

    enet_deinitialize();

    //stop_renderer(root->client.renderSystem.renderer);
    //destroy_renderer(root->client.renderSystem.renderer);

    platform->destroy_async_io(platform);

    tess_server_destroy(&root->server);
    platform->free_memory(platform, root->rootBlock);
    platform->userData = NULL;

    DEBUG_DESTROY();
}

void aike_update(AikePlatform *platform)
{
    DEBUG_START_FRAME();
    PROF_START_STR("aike_update");

    TessServerRoot *root = (TessServerRoot*)platform->userData;
    tess_process_io_events(&root->server.fileSystem);

    //process_render_messages(&root->client.renderSystem);

    if(!root->loaded)
    {
        root->loaded = true;

        //scheduler_set_mode(Tess_Client_Mode_Menu);
    }
    if(root->loaded)
    {
        PROF_BLOCK_STR("Generate frame");

        //tess_update_editor_server(&root->server.editorServer);
        game_server_update(&root->server.gameServer, platform->dt);

        scheduler_yield();
    }
    PROF_END(); // aike_update
    DEBUG_END_FRAME();
}

void aike_begin_hot_reload(AikePlatform *platform)
{
    TessServerRoot *root = (TessServerRoot*)platform->userData;
    //stop_renderer(root->client.renderSystem.renderer);

    for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
        root->g_profStates[i] = g_profStates[i];
    root->main_profState = t_profState;
}

void aike_end_hot_reload(AikePlatform *platform)
{
    TessServerRoot *root = (TessServerRoot*)platform->userData;
    //start_renderer(root->client.renderSystem.renderer);

    for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
        g_profStates[i] = root->g_profStates[i];
    t_profState = root->main_profState;
    //tess_reload_vtable();
}
