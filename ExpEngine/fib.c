struct GameData gdata;

struct TessRoot
{
    struct TessState state;
    AikeMemoryBlock *rootBlock;
    bool loaded; // TODO: remove this hack
};

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    printf("resize %f %f\n", win->width, win->height);

    struct TessRoot *root = (struct TessRoot*)platform->userData;
    if(root != NULL)
    {
        // TODO: this is here because we dont have render targets yet
        // when we do, the main window will just be a render target
        // and when the width/height is changed it will be marked dirty
        // and all the cameras rendering to it will recalculate aspectRatio
        root->state.gameSystem.defaultCamera.aspectRatio = platform->mainWin.width/platform->mainWin.height;
        tess_update_camera_perspective(&root->state.gameSystem.defaultCamera);
    

        render_system_screen_resize(&root->state.renderSystem, win->width, win->height);
    }
}

void aike_init(AikePlatform *platform)
{
    DEBUG_INIT("Main thread");

    platform->init_async_io(platform);

    platform->create_opengl_context(&platform->mainWin);
    bool sinter = platform->swap_interval(&platform->mainWin, 0);
    if(!sinter)
    {
        fprintf(stderr, "Failed to set swap interval!\n");
        assert(false);
    }

    struct Renderer *renderer = create_renderer(RENDERER_TYPE_OPENGL, platform);

    AikeMemoryBlock *rootBlock = platform->allocate_memory(platform, sizeof(struct TessRoot), 0);
    struct TessRoot *root = (struct TessRoot*)rootBlock->memory;
    root->rootBlock = rootBlock;
    root->loaded = false;
    platform->userData = root;
    tess_init(&root->state, platform, renderer);

    root->state.gameSystem.defaultCamera.aspectRatio = platform->mainWin.width/platform->mainWin.height;
    root->state.gameSystem.defaultCamera.FOV = 90;
    root->state.gameSystem.defaultCamera.nearPlane = 0.1f;
    root->state.gameSystem.defaultCamera.farPlane = 1000.0f;
    tess_update_camera_perspective(&root->state.gameSystem.defaultCamera);

    gdata.viewBuilder = root->state.renderSystem.viewBuilder;
    gdata.platform = platform;

    platform->make_window_current(platform, NULL);
    renderer->swapBuffer = root->state.renderSystem.viewSwapBuffer;
    renderer->renderThread = platform->create_thread(renderer, renderer->threadProc);

    init_game(renderer, &gdata, &root->state);

    TStr *firstInterned = tess_intern_string(&root->state.strings, "First");

    tess_gen_lookup_cache_for_package(&root->state.assetSystem, firstInterned);
    
    TStr *obj0I = tess_intern_string(&root->state.strings, "First/object0");
    tess_register_object(&root->state.gameSystem, 1, obj0I);
}

void aike_deinit(AikePlatform *platform)
{
    struct TessRoot *root = (struct TessRoot*)platform->userData;

    deinit_game();

    destroy_renderer(root->state.renderSystem.renderer);

    platform->destroy_async_io(platform);

    tess_destroy(&root->state);
    platform->free_memory(platform, root->rootBlock);
    platform->userData = NULL;

    DEBUG_DESTROY();
}

void aike_update(AikePlatform *platform)
{
    DEBUG_START_FRAME();

    struct TessRoot *root = (struct TessRoot*)platform->userData;
    tess_process_io_events(&root->state.fileSystem);
    tess_check_complete(&root->state.assetSystem);

    process_render_messages(&root->state.renderSystem);

    if(!root->loaded && tess_are_all_loads_complete(&root->state.assetSystem))
    {
        tess_temp_assign_all(&root->state.gameSystem);
        root->loaded = true;

        struct Mat4 objectToWorld;
        struct Quat q;
        quat_angle_axis(&q, 180.0f, make_v3(0.0f, 1.0f, 0.0f));
        mat4_trs(&objectToWorld, make_v3(0.0f, 0.0f, 10.0f), q, make_v3(1.0f, 1.0f, 1.0f));
        //tess_create_entity(&root->state.gameSystem, 1, &objectToWorld);

        root->state.mode = Tess_Mode_Editor;
    }
    if(root->loaded)
    {
        PROF_BLOCK_STR("Generate frame");
        tess_input_begin(&root->state.inputSystem);
        render_system_begin_update(&root->state.renderSystem);
        tess_ui_begin(&root->state.uiSystem);

        switch(root->state.mode)
        {
            case Tess_Mode_Menu:
                break;
            case Tess_Mode_Editor:
                editor_update(&root->state.editor);
                break;
            case Tess_Mode_CrazyTown:
                update_game(&gdata);
                break;
            default:
                fprintf(stderr, "Invalid mode!\n");
                break;
        };
        tess_render_entities(&root->state.gameSystem);

        tess_ui_end(&root->state.uiSystem);
        render_system_end_update(&root->state.renderSystem);
        tess_input_end(&root->state.inputSystem);
    }
    DEBUG_END_FRAME();
    //DEBUG_FRAME_REPORT();
}
