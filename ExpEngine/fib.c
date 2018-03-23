#include <GL/gl.h> // TODO: remove

static struct V3 pyramidVertices[] = 
{
    { -0.5f, -0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f, -0.5f },

    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f },
    {  0.0f,  0.5f,  0.0f },

    { -0.5f, -0.5f,  0.5f },
    { -0.5f, -0.5f, -0.5f },
    {  0.0f,  0.5f,  0.0f },

    {  0.5f, -0.5f,  0.5f },
    { -0.5f, -0.5f,  0.5f },
    {  0.0f,  0.5f,  0.0f },

    {  0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.0f,  0.5f,  0.0f },
};

static unsigned short pyramidIndices[] = 
{
    0, 1, 2, 3, 4, 5, 6, 7, 8,
    9,10,11,12,13,14,15,16,17
};

static struct V4 pyramidColors[] =
{
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},

    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 0.0f, 0.0f, 1.0f},

    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 0.0f, 1.0f, 0.0f, 1.0f},

    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 0.0f, 0.0f, 1.0f, 1.0f},

    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f, 0.3f, 0.0f, 1.0f},
};

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
"attribute vec4 a_position;\n"
"attribute vec2 a_user0;\n"
"attribute vec4 a_user2;\n"
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
   "gl_Position = matrices[id] * a_position;\n"
   "v_texcoord = vec2(a_user0.x, 1.0 - a_user0.y);\n"
   "v_color = /*vec4(idata[id].v1, idata[id].v2)**/a_user2;\n"
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

static const char *fragShaderSolidSrc = "#version 330 core\n"
"varying vec4 v_color;\n"
"void main(void) {\n"
   "gl_FragColor = v_color;\n"
"}";

void drawTriangle(void);

static u32 frameId;
static float redVal = 0.5f;
static float direction = 1.0f;
static float rotval = 0.0f;

static struct Renderer *s_renderer;
static struct Mat4 modelM;
struct Mat4 s_perspective;

struct RenderViewBuilder *builder;
struct SwapBuffer *sbuf;

struct RenderViewBuffer *writeView = NULL;
struct RenderViewBuffer *readView = NULL;

#define MY_MESH_PYRAMID 1
#define MY_MESH_CUBE 2
uint32_t pyramidMeshId;

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

static int s_x, s_y;
static void *s_texdata;

void drawTriangle(void)
{
    int comp, req_comp = 4;
    s_texdata = stbi_load("./cat.png", &s_x, &s_y, &comp, 4);
    assert(s_texdata != NULL);
	
    s_renderer = create_renderer(RENDERER_TYPE_OPENGL);

    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.userData = (void*)MY_MESH_PYRAMID;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = ARRAY_COUNT(pyramidVertices);
    msg.meshQuery.indexCount = ARRAY_COUNT(pyramidIndices);
    msg.meshQuery.attributeTypes[2] = Vertex_Attribute_Type_Vec4;
    renderer_queue_message(s_renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.userData = (void*)MY_MESH_CUBE;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = ARRAY_COUNT(cubeVertices);
    msg.meshQuery.indexCount = ARRAY_COUNT(s_indices);
    msg.meshQuery.attributeTypes[0] = Vertex_Attribute_Type_Vec3;
    msg.meshQuery.attributeTypes[2] = Vertex_Attribute_Type_Vec4;
    renderer_queue_message(s_renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.materialQuery.userData = (void*)vertShaderSrc;
    msg.materialQuery.materialId = 0;
    msg.materialQuery.shaderTypes[0] = ShaderType_GLSL_Vert;
    msg.materialQuery.shaderCodes[0] = vertShaderSrc;
    msg.materialQuery.shaderLengths[0] = strlen(vertShaderSrc);
    msg.materialQuery.shaderTypes[1] = ShaderType_GLSL_Frag;
    msg.materialQuery.shaderCodes[1] = fragShaderSrc;
    msg.materialQuery.shaderLengths[1] = strlen(fragShaderSrc);
    renderer_queue_message(s_renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = NULL;
    msg.texQ.textureId = 0;
    msg.texQ.width = s_x;
    msg.texQ.height = s_y;
    msg.texQ.format = Texture_Format_RGBA;
    renderer_queue_message(s_renderer, &msg);

    /*struct Texture *tex = create_texture();
    texture_copy_data(tex, x, y, TextureFormat_RGBA, data);
    texture_queue_for_update(tex);
    stbi_image_free(data);*/

    initRender();
}

void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    mat4_perspective(&s_perspective, 90.0f, win->width/win->height, 0.1f, 1000.0f);
    glViewport(0, 0, win->width, win->height);
    printf("resize %f %f\n", win->width, win->height);
}

void aike_init(AikePlatform *platform)
{
    DEBUG_INIT();
	drawTriangle();

    bool sinter = platform->swap_interval(&platform->mainWin, 0);
    if(!sinter)
    {
        fprintf(stderr, "Failed to set swap interval!\n");
    }
}

void aike_deinit(AikePlatform *platform)
{
    destroy_renderer(s_renderer);
    deinitRender();
    DEBUG_DESTROY();
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

void fill_pyramid_data(void *vdata, void *idata)
{
    printf("fill pyramid data\n");
    uint32_t count = ARRAY_COUNT(pyramidVertices);
    uint8_t *bytes = (uint8_t*)vdata;

    uint32_t vertSize = 8*4;
    float one = 1.0f;

    for(int i = 0; i < count; i++)
    {
        memcpy(&bytes[vertSize*i], &pyramidVertices[i], 3*4);
        memcpy(&bytes[vertSize*i+12], &one, 4);
        memcpy(&bytes[vertSize*i+16], &pyramidColors[i], 4*4);
    }
    memcpy(idata, pyramidIndices, sizeof(pyramidIndices));
}

void fill_cube_data(void *vdata, void *idata)
{
    printf("fill cube data\n");
    uint32_t count = ARRAY_COUNT(cubeVertices);
    uint8_t *bytes = (uint8_t*)vdata;

    uint32_t vertSize = 11*4;
    float one = 1.0f;

    for(int i = 0; i < count; i++)
    {
        memcpy(&bytes[vertSize*i], &cubeVertices[i], 3*4);
        memcpy(&bytes[vertSize*i+12], &one, 4);
        memcpy(&bytes[vertSize*i+16], &cubeTexCoords[i], 4*3);
        memcpy(&bytes[vertSize*i+28], &cubeColors[i], 4*4);
    }
    memcpy(idata, s_indices, sizeof(s_indices));
}

void fill_texture_data(void *tdata)
{
    printf("fill texture data\n");
    uint32_t texsize = s_x*s_y*4;
    memcpy(tdata, s_texdata, texsize);
    stbi_image_free(s_texdata);
}

void process_render_messages(AikePlatform *platform)
{
    PROF_BLOCK();
    RenderMessage msg;
    RenderMessage response;
    while(renderer_next_message(s_renderer, &msg))
    {
        switch(msg.type)
        {
            case Render_Message_Mesh_Query_Result:
                if(msg.meshQueryResult.userData == (void*)MY_MESH_PYRAMID)
                {
                    fill_pyramid_data(msg.meshQueryResult.vertBufPtr, msg.meshQueryResult.idxBufPtr);
                    pyramidMeshId = msg.meshQueryResult.meshId;
                    response = (RenderMessage){};
                    response.type = Render_Message_Mesh_Update;
                    response.meshUpdate.meshId = pyramidMeshId;
                    renderer_queue_message(s_renderer, &response);
                }
                else if(msg.meshQueryResult.userData == (void*)MY_MESH_CUBE)
                {
                    fill_cube_data(msg.meshQueryResult.vertBufPtr, msg.meshQueryResult.idxBufPtr);
                    response = (RenderMessage){};
                    response.type = Render_Message_Mesh_Update;
                    response.meshUpdate.meshId = msg.meshQueryResult.meshId;
                    renderer_queue_message(s_renderer, &response);
                }
                break;
            case Render_Message_Texture_Query_Response:
                fill_texture_data(msg.texQR.textureDataPtr);
                response = (RenderMessage){};
                response.type = Render_Message_Texture_Update;
                response.texU.textureId = msg.texQR.textureId;
                renderer_queue_message(s_renderer, &response);
                break;
        }
    }
}

void aike_update(AikePlatform *platform)
{
    uint64_t startRT = __rdtsc();
    DEBUG_START_FRAME();
    


    process_render_messages(platform);

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
    struct Mat4 model;
    srand(1);

    struct V3 one = (struct V3){1.0f, 1.0f, 1.0f};


    PROF_START_STR("Render instances");
    for(int i = 0; i < 100; i++)
    for(int j = 0; j < 100; j++)
    {
        struct Quat rot;
        struct Quat rot2;
        quat_angle_axis(&rot, rotval*70.0f * ((float)j / 10.0f), make_v3(0.0f, 1.0f, 0.0f));
        quat_angle_axis(&rot2, -rotval*140.0f * ((float)i / 10.0f), make_v3(0.0f, 1.0f, 0.0f));

        bool cube = j%2==0;
        data.v1 = cols[rand() % ARRAY_COUNT(cols)];
        PROF_START_STR("Calc model matrix");
        mat4_trs(&model, make_v3((float)i*2 - 100.0f, /*-5.0f*redVal*/ - 10.0f, 10.0f + (float)j*2 - 5.0f), cube?rot:rot2, one);
        PROF_END();
        add_mesh_instance(builder, cube?2:1, 1, &model, &data, sizeof(data));
    }
    PROF_END();

    // TODO: this should be part of view builder?
    mat4_load_sse2(&readView->view.worldToClip, &s_perspective, &s_perspective, &s_perspective, &s_perspective);
    build_view(builder, readView);

    swap_view_for_newer(sbuf, readView);

    writeView = swap_view_if_newer(sbuf, writeView);
    opengl_render_view((struct OpenGLRenderer*)s_renderer, readView);

    platform->present_frame(platform, &platform->mainWin);
    PROF_END();
    //platform->sleep(100000);

    DEBUG_FRAME_REPORT();

    frameId++;

    DEBUG_END_FRAME();

    uint64_t cycles = __rdtsc() - startRT;
    //printf("%lu cycles %fms\n", cycles, (double)(cycles/1000) / 2800.0);
}
