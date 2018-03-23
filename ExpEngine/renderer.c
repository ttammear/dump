struct RenderEventHandler *g_renderEventHandler = 0;

static const char *fallbackVert =
"#version 330 core\n"
"attribute vec4 coordinates;\n"
"layout (std140) uniform matrixBlock\n"
"{\n"
    "mat4 matrices[128];\n"
"};\n"
"void main(void) {\n"
    "gl_Position = matrices[gl_InstanceID] * coordinates;\n"
"}\n";
static const char *fallbackFrag = 
"#version 330 core\n"
"void main(void) {\n"
    "gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);\n"
"}\n";

struct Renderer *create_headless_renderer()
{
    struct Renderer *ret = calloc(1, sizeof(struct Renderer));
    return ret;
}

void init_renderer(struct Renderer *renderer)
{
    ring_queue_init(RenderMessage, &renderer->ch.toRenderer);
    ring_queue_init(RenderMessage, &renderer->ch.fromRenderer);
    renderer->meshes = NULL;
    renderer->materials = NULL;
    renderer->textures = NULL;

    struct Texture tex = {}; // TODO: real dummy texture
    struct Mesh mesh = {}; // TODO: real dummy mesh;
    buf_push(renderer->textures, tex);
    buf_push(renderer->meshes, mesh);
}

struct Renderer *create_renderer(u32 rendererType)
{
    struct Renderer *ret = NULL;

    struct Material *mat;

    switch(rendererType)
    {
        case RENDERER_TYPE_OPENGL:
            ret = create_opengl_renderer();
            init_renderer(ret);

            mat = &ret->fallbackMaterial;
            {
                RenderMessage msg = (RenderMessage){};
                msg.type = Render_Message_Material_Query;
                msg.materialQuery.userData = (void*)fallbackVert;
                msg.materialQuery.materialId = 0;
                msg.materialQuery.shaderTypes[0] = ShaderType_GLSL_Vert;
                msg.materialQuery.shaderCodes[0] = fallbackVert;
                msg.materialQuery.shaderLengths[0] = strlen(fallbackVert);
                msg.materialQuery.shaderTypes[1] = ShaderType_GLSL_Frag;
                msg.materialQuery.shaderCodes[1] = fallbackFrag;
                msg.materialQuery.shaderLengths[1] = strlen(fallbackFrag);
                renderer_queue_message(ret, &msg);
            }
            break;
        case RENDERER_TYPE_HEADLESS:
            ret = create_headless_renderer();
            init_renderer(ret);
            break;
        default:
            tt_render_fatal("Unknown/Unsupported renderer");
            break;
    }

    return ret;
}

void destroy_renderer(struct Renderer *renderer)
{
    switch(renderer->type)
    {
        case RENDERER_TYPE_OPENGL:
            destroy_opengl_renderer((struct OpenGLRenderer*)renderer);
            break;
        case RENDERER_TYPE_HEADLESS:
            break;
        default:
            tt_render_fatal("Unknown renderer");
            break;
    }
    free(renderer);
    free(g_renderEventHandler);
}

