struct Renderer *create_headless_renderer()
{
    struct Renderer *ret = calloc(1, sizeof(struct Renderer));
    return ret;
}

void init_renderer(struct Renderer *renderer, AikePlatform *platform)
{
    ring_queue_init(RenderMessage, &renderer->ch.toRenderer);
    ring_queue_init(RenderMessage, &renderer->ch.fromRenderer);
    renderer->platform = platform;
    renderer->renderThread = NULL;
}

struct Renderer *create_renderer(u32 rendererType, AikePlatform *platform)
{
    struct Renderer *ret = NULL;

    switch(rendererType)
    {
        case RENDERER_TYPE_OPENGL:
            ret = create_opengl_renderer(platform);
            init_renderer(ret, platform);
            break;
        case RENDERER_TYPE_HEADLESS:
            ret = create_headless_renderer();
            init_renderer(ret, platform);
            break;
        default:
            tt_render_fatal("Unknown/Unsupported renderer");
            break;
    }
    ret->type = rendererType;

    return ret;
}

void destroy_renderer(struct Renderer *renderer)
{
    switch(renderer->type)
    {
        case RENDERER_TYPE_OPENGL:
            destroy_opengl_renderer(renderer);
            break;
        case RENDERER_TYPE_HEADLESS:
            break;
        default:
            tt_render_fatal("Unknown renderer");
            break;
    }
    free(renderer);
}

void start_renderer(struct Renderer *renderer)
{
    switch(renderer->type)
    {
        case RENDERER_TYPE_OPENGL:
            start_opengl_renderer(renderer);
            break;
        default:
            assert(false);
            break;
    }
}

void stop_renderer(struct Renderer *renderer)
{
    switch(renderer->type)
    {
        case RENDERER_TYPE_OPENGL:
            stop_opengl_renderer(renderer);
            break;
        default:
            assert(false);
            break;
    }
}
