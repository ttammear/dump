

static struct Renderer *s_renderer;

struct SwapBuffer *sbuf;
struct GameData gdata;

void initRender()
{
    sbuf = malloc(sizeof(struct SwapBuffer));
    swap_buffer_init(sbuf);
}

void deinitRender()
{
    swap_buffer_destroy(sbuf);
    free(sbuf);
}

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    mat4_perspective(&s_perspective, 90.0f, win->width/win->height, 0.1f, 1000.0f);
    printf("resize %f %f\n", win->width, win->height);

    RenderMessage msg = {};
    msg.type = Render_Message_Screen_Resize;
    msg.screenR.width = win->width;
    msg.screenR.height = win->height;
    if(s_renderer != NULL)
        renderer_queue_message(s_renderer, &msg);
}

void aike_init(AikePlatform *platform)
{
    DEBUG_INIT();

    initRender();

    platform->create_opengl_context(&platform->mainWin);
    bool sinter = platform->swap_interval(&platform->mainWin, 0);
    if(!sinter)
    {
        fprintf(stderr, "Failed to set swap interval!\n");
    }

    s_renderer = create_renderer(RENDERER_TYPE_OPENGL, platform, sbuf);

    gdata.renderer = s_renderer;
    gdata.swapbuf = sbuf;
    gdata.platform = platform;
    //AikeThread *thread = platform->create_thread(data, game_loop);
    //platform->detach_thread(thread);

    platform->make_window_current(platform, NULL);
    s_renderer->renderThread = platform->create_thread(s_renderer, s_renderer->threadProc);

    init_game(s_renderer, &gdata);
}

void aike_deinit(AikePlatform *platform)
{
    deinit_game();

    destroy_renderer(s_renderer);
    deinitRender();
    DEBUG_DESTROY();
}

void aike_update(AikePlatform *platform)
{
    // opengl_update_proc(s_renderer);
    update_game(&gdata);
}
