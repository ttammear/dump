
typedef struct TessRoot
{
    TessScheduler scheduler;
    struct TessClient client;
    TessServer server;
    AikeMemoryBlock *rootBlock;
    bool loaded; // TODO: remove this hack
    //aike_thread_local struct ProfilerState *t_profState;
    uintptr_t g_profStates[10];
    struct ProfilerState *main_profState;
} TessRoot;

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    printf("resize %f %f\n", win->width, win->height);

    TessRoot *root = (TessRoot*)platform->userData;
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
    }
}

// TODO: delete?
void test_task(void *data) {
    printf("Task %p\n", data);
    scheduler_task_end();
}

void aike_init(AikePlatform *platform)
{
    DEBUG_INIT("Main thread");

    tess_reload_vtable();

    platform->init_async_io(platform);

    platform->create_opengl_context(platform, &platform->mainWin);
    bool sinter = platform->swap_interval(&platform->mainWin, 1);
    if(!sinter)
    {
        fprintf(stderr, "Failed to set swap interval!\n");
        assert(false);
    }


    Renderer *renderer = create_renderer(RENDERER_TYPE_OPENGL, platform);

    AikeMemoryBlock *rootBlock = platform->allocate_memory(platform, sizeof(TessRoot), 0);
    TessRoot *root = (TessRoot*)rootBlock->memory;
    root->rootBlock = rootBlock;
    root->loaded = false;
    platform->userData = root;

    tess_client_init(&root->client, platform, renderer);

    // TODO: ... no
    root->client.renderSystem.rtW = platform->mainWin.width;
    root->client.renderSystem.rtH = platform->mainWin.height;

    tess_server_init(&root->server, platform);

    scheduler_init(&root->scheduler, &root->client.arena, &root->client);

    root->client.gameSystem.defaultCamera.aspectRatio = platform->mainWin.width/platform->mainWin.height;
    root->client.gameSystem.defaultCamera.FOV = 75.0f;
    root->client.gameSystem.defaultCamera.nearPlane = 0.1f;
    root->client.gameSystem.defaultCamera.farPlane = 1000.0f;
    tess_update_camera_perspective(&root->client.gameSystem.defaultCamera);

    platform->make_window_current(platform, NULL);
    renderer->swapBuffer = root->client.renderSystem.viewSwapBuffer;
    start_renderer(renderer);

    tess_refresh_package_list(&root->client.assetSystem);
    int count = buf_len(root->client.assetSystem.packageList);
    for(int i = 0; i < count; i++)
    {
        TStr *packageName = root->client.assetSystem.packageList[i];
        printf("Generating lookup cache for package '%s'\n", packageName->cstr);
        tess_gen_lookup_cache_for_package(&root->client.assetSystem, packageName);
    }

    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
        assert(0);
    }

    /*for(int i = 0; i < 1000; i++) {
        scheduler_queue_task(test_task, (void*)i);
    }*/
}

void aike_deinit(AikePlatform *platform)
{
    TessRoot *root = (TessRoot*)platform->userData;

    enet_deinitialize();

    stop_renderer(root->client.renderSystem.renderer);
    destroy_renderer(root->client.renderSystem.renderer);

    platform->destroy_async_io(platform);

    tess_server_destroy(&root->server);
    tess_client_destroy(&root->client);
    platform->free_memory(platform, root->rootBlock);
    platform->userData = NULL;

    DEBUG_DESTROY();
}

void aike_update(AikePlatform *platform)
{
    DEBUG_START_FRAME();

    TessRoot *root = (TessRoot*)platform->userData;
    tess_process_io_events(&root->client.fileSystem);

    process_render_messages(&root->client.renderSystem);

    if(!root->loaded && root->client.uiSystem.loaded && tess_are_all_loads_complete(&root->client.assetSystem))
    {
        root->loaded = true;

        Mat4 objectToWorld;
        Quat q;
        quat_angle_axis(&q, 180.0f, make_v3(0.0f, 1.0f, 0.0f));
        mat4_trs(&objectToWorld, make_v3(0.0f, 0.0f, 10.0f), q, make_v3(1.0f, 1.0f, 1.0f));

        scheduler_set_mode(Tess_Client_Mode_Menu);
    }
    if(root->loaded)
    {
        PROF_BLOCK_STR("Generate frame");
        tess_client_begin_frame(&root->client);

        tess_update_editor_server(&root->server.editorServer);
        game_server_update(&root->server.gameServer, platform->dt);

        scheduler_yield();
        tess_client_end_frame(&root->client);
    }
    DEBUG_END_FRAME();
}

void aike_begin_hot_reload(AikePlatform *platform)
{
    TessRoot *root = (TessRoot*)platform->userData;
    stop_renderer(root->client.renderSystem.renderer);

    for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
        root->g_profStates[i] = g_profStates[i];
    root->main_profState = t_profState;
}

void aike_end_hot_reload(AikePlatform *platform)
{
    TessRoot *root = (TessRoot*)platform->userData;
    start_renderer(root->client.renderSystem.renderer);

    for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
        g_profStates[i] = root->g_profStates[i];
    t_profState = root->main_profState;
    tess_reload_vtable();
}
