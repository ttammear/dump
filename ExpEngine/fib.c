#include <GL/gl.h> // TODO: remove

static struct V3 cubeVertices[] =
{
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f },
    { -0.5f,  0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    {  0.5f,  0.5f,  0.5f },
    {  0.5f, -0.5f, -0.5f },
    { -0.5f, -0.5f, -0.5f },
    { -0.5f,  0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f },
    { -0.5f,  0.5f,  0.5f },
    { -0.5f,  0.5f, -0.5f },
    { -0.5f,  0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f,  0.5f },
    { -0.5f, -0.5f,  0.5f }
};

static struct V3 cubeTexCoords[] =
{
    { 0.0f, 0.0f, 0.0f}, 
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 1.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f}
};

static struct V4 cubeColors[] = 
{
    {1.0f, 0.0f, 0.0f, 1.0f },
    {1.0f, 0.0f, 0.0f, 1.0f },
    {1.0f, 0.0f, 0.0f, 1.0f },
    {1.0f, 0.0f, 0.0f, 1.0f },
    {0.0f, 1.0f, 0.0f, 1.0f },
    {0.0f, 1.0f, 0.0f, 1.0f },
    {0.0f, 1.0f, 0.0f, 1.0f },
    {0.0f, 1.0f, 0.0f, 1.0f },
    {0.0f, 0.0f, 1.0f, 1.0f },
    {0.0f, 0.0f, 1.0f, 1.0f },
    {0.0f, 0.0f, 1.0f, 1.0f },
    {0.0f, 0.0f, 1.0f, 1.0f },
    {1.0f, 1.0f, 0.0f, 1.0f },
    {1.0f, 1.0f, 0.0f, 1.0f },
    {1.0f, 1.0f, 0.0f, 1.0f },
    {1.0f, 1.0f, 0.0f, 1.0f },
    {0.0f, 1.0f, 1.0f, 1.0f },
    {0.0f, 1.0f, 1.0f, 1.0f },
    {0.0f, 1.0f, 1.0f, 1.0f },
    {0.0f, 1.0f, 1.0f, 1.0f },
    {1.0f, 0.0f, 1.0f, 1.0f },
    {1.0f, 0.0f, 1.0f, 1.0f },
    {1.0f, 0.0f, 1.0f, 1.0f },
    {1.0f, 0.0f, 1.0f, 1.0f }
};

static unsigned short s_indices[] =
{
	2,  1,   0,  0,  3,  2,
	6,  5,   4,  4,  7,  6,
    10, 9,   8,  8, 11, 10,
	14, 13, 12, 12, 15, 14,
	18, 17, 16, 16, 19, 18,
	22, 21, 20, 20, 23, 22
};
	 
static const char *vertShaderSrc =  "#version 330 core\n"
"attribute vec3 coordinates;\n"
"attribute vec2 a_texcoord;\n"
"attribute vec4 a_color;\n"
""
"layout (std140) uniform vectors\n"
"{\n" 
    "vec4 v[];\n"
"};\n"
"struct InstanceData { \n"
    "vec2 v1; \n"
    "vec2 v2; \n"
"};\n"
"layout (std140) uniform instanceBlock\n"
"{\n"
    "InstanceData idata[128];\n"
"};\n"
"layout (std140) uniform matrixBlock\n"
"{\n"
    "mat4 matrices[128];\n"
"};\n"
""
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"uniform int instId;\n"
"void main(void) {\n"\
   "int id = gl_InstanceID;"
   "gl_Position = matrices[id] * vec4(coordinates, 1.0);\n"
   "v_texcoord = vec2(a_texcoord.x, 1.0 - a_texcoord.y);\n"
   "v_color = vec4(idata[id].v1, idata[id].v2);\n"
"}";
			
static const char *fragShaderSrc = "#version 330 core\n"
"uniform sampler2D _MainTex;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"void main(void) {\n"
   "//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
   "gl_FragColor = texture2D(_MainTex, v_texcoord) * v_color;\n"
   "//gl_FragColor = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);\n"
"}";

void drawTriangle(void);

static u32 frameId;
static double msCombined;
static float redVal = 0.5f;
static float direction = 1.0f;
static float rotval = 0.0f;

static struct Renderer *s_renderer;
static struct Mat4 modelM;
static struct Mesh *s_mesh;

struct RenderViewBuilder *builder;
struct SwapBuffer *sbuf;

struct RenderViewBuffer *writeView = NULL;
struct RenderViewBuffer *readView = NULL;

struct Material *s_mat;

void initRender()
{
    builder = malloc(sizeof(struct RenderViewBuilder));
    rview_builder_init(builder);
    sbuf = malloc(sizeof(struct SwapBuffer));
    swap_buffer_init(sbuf);
    writeView = take_view_buffer(sbuf);
    readView = take_view_buffer(sbuf);
}

void deinitRender()
{
    rview_builder_destroy(builder);
    free(builder);
    swap_buffer_destroy(sbuf);
    free(sbuf);
}

void drawTriangle(void)
{
    int x, y, comp, req_comp = 4;
    unsigned char * data = stbi_load("./cat.png", &x, &y, &comp, 4);
    assert(data != NULL);
	
    s_renderer = create_renderer(RENDERER_TYPE_OPENGL);

    s_mat = malloc(sizeof(struct Material));
    material_init(s_mat);
    material_add_shader(s_mat, ShaderType_GLSL_Vert, vertShaderSrc);
    material_add_shader(s_mat, ShaderType_GLSL_Frag, fragShaderSrc);
    material_queue_for_update(s_mat);

    s_mesh = create_mesh();
    mesh_copy_vertices(s_mesh, cubeVertices, sizeof(cubeVertices) / sizeof(struct V3));
    mesh_copy_texcoords(s_mesh, cubeTexCoords, sizeof(cubeTexCoords) / sizeof(struct V3));
    mesh_copy_colors(s_mesh, cubeColors, sizeof(cubeColors) / sizeof(struct V4));
    mesh_copy_indices(s_mesh, s_indices, sizeof(s_indices) / sizeof(uint16_t));
    mesh_queue_for_update(s_mesh);

    struct Texture *tex = create_texture();
    texture_copy_data(tex, x, y, TextureFormat_RGBA, data);
    texture_queue_for_update(tex);
    s_mesh->tex = tex;
    stbi_image_free(data);

    initRender();
}

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    glViewport(0, 0, win->width, win->height);
    printf("resize %f %f\n", win->width, win->height);
}

void aike_init(AikePlatform *platform)
{
    debug_init();
	drawTriangle();
}

void aike_deinit(AikePlatform *platform)
{
    material_destroy(s_mat);
    free(s_mat);
    destroy_renderer(s_renderer);
    deinitRender();
    debug_destroy();
}

struct V4 cols[] = 
{
    {1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.7f, 1.0f},
    {0.5f, 1.0f, 0.0f, 1.0f},
    {0.4f, 0.4f, 0.4f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.6f, 1.0f},
    {1.0f, 0.3f, 0.4f, 1.0f},
};

void aike_update(AikePlatform *platform)
{
    debug_start_frame();

    PROF_START();

    redVal += 0.01f * direction;
    if(redVal > 1.0f)
    {
        direction = -1.0f;
        redVal = 1.0f;
    }
    else if(redVal < 0.0f)
    {
        direction = 1.0f;
        redVal = 0.0f;
    }
    rotval += 0.01f;


    rview_builder_reset(builder);

#pragma push(pack, 1)
    struct TestInstanceData
    {
        struct V4 v1;
        //struct V2 v2;
    };
#pragma pop(pack)

    struct TestInstanceData data;
    data.v1 = (struct V4){1.0f, 1.0f, 0.0f, 0.0f};

    struct Mat4 mvp;
    struct Mat4 perspective;
    mat4_perspective(&perspective, 90.0f, 640.0f/480.0f, 0.1f, 1000.0f);
    struct Mat4 model;
    struct Quat rot;
    quat_angle_axis(&rot, rotval*360.0f, make_v3(0.0f, 1.0f, 0.0f));
    srand(1);

    struct V3 one = (struct V3){1.0f, 1.0f, 1.0f};

    PROF_START_STR("Render instances");
    for(int i = 0; i < 100; i++)
    for(int j = 0; j < 100; j++)
    {
        data.v1 = cols[rand() % ARRAY_COUNT(cols)];
        mat4_trs(&model, make_v3((float)i*2 - 4.0f, -5.0f*redVal, 10.0f + (float)j*2 - 5.0f), rot, one);
        mat4_mul(&mvp, &perspective, &model);
        add_mesh_instance(builder, s_mesh, s_mat, &mvp, &data, sizeof(data));
    }
    PROF_END();

    build_view(builder, readView); // for 15k meshes, this takes about 4ms...
    swap_view_for_newer(sbuf, readView);

    writeView = swap_view_if_newer(sbuf, writeView);
    opengl_render_view((struct OpenGLRenderer*)s_renderer, readView);

    platform->present_frame(platform, &platform->mainWin);
    PROF_END();
    //platform->sleep(100000);

    double hack = debug_frame_report();

    msCombined += hack;
    frameId++;

    if(frameId%1000 == 0)
    {
        printf("last 1000 %f ms\n", msCombined / 1000.0);
        msCombined = 0.0;
    }

    debug_end_frame();
}
