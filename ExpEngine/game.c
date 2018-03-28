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

static struct V4 cubeVertices[] =
{
    { -0.5f, -0.5f,  0.5f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f }
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
"attribute vec4 a_user1;\n"
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
   "v_color = /*vec4(idata[id].v1, idata[id].v2)**/a_user1;\n"
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

static const char *uiVertSrc = 
"#version 330 core\n"
"uniform mat4 toClip;\n"
"attribute vec2 a_position;\n"
"attribute vec2 a_user0;\n"
"attribute vec4 a_user1;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"void main(void) {\n"
    "gl_Position = toClip * vec4(a_position, 0.0f, 1.0f);\n"
    "v_texcoord = vec2(a_user0.x, a_user0.y);\n"
    "v_color = a_user1;\n"
"}";

static const char *uiFragSrc =
"#version 330 core\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"uniform sampler2D _MainTex;\n"
"void main(void) {\n"
    "//gl_FragColor = v_color;\n"
    "gl_FragColor = v_color * texture2D(_MainTex, v_texcoord);\n"
"}";

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

struct GameData
{
    struct Renderer *renderer;
    struct SwapBuffer *swapbuf;
    AikePlatform *platform;
};

static int s_x, s_y;
static void *s_texdata;
static float redVal = 0.5f;
static float direction = 1.0f;
static float rotval = 0.0f;
struct Mat4 s_perspective;

struct RenderViewBuilder *s_builder;
struct RenderViewBuffer *readView = NULL;

#define MY_MESH_PYRAMID 1
#define MY_MESH_CUBE 2

struct GameState
{
    struct nk_context nk_ctx;
    struct nk_font_atlas atlas;
    struct nk_font *font; 
    struct nk_draw_null_texture null;
    struct nk_buffer cmds;

    int w;
    int h;
    const void *image;

    bool ui_ready;
};

struct GameState gameState;

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
        memcpy(&bytes[vertSize*i], &cubeVertices[i], 4*4);
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

void process_render_messages(struct Renderer *renderer)
{
    RenderMessage msg;
    RenderMessage response;
    while(renderer_next_message(renderer, &msg))
    {
        switch(msg.type)
        {
            case Render_Message_Mesh_Query_Result:
                if(msg.meshQueryResult.onComplete != NULL)
                {
                    msg.meshQueryResult.onComplete(renderer, &msg.meshQueryResult, msg.meshQueryResult.userData);
                }
                else if(msg.meshQueryResult.userData == (void*)MY_MESH_CUBE)
                {
                    fill_cube_data(msg.meshQueryResult.vertBufPtr, msg.meshQueryResult.idxBufPtr);
                    response = (RenderMessage){};
                    response.type = Render_Message_Mesh_Update;
                    response.meshUpdate.meshId = msg.meshQueryResult.meshId;
                    renderer_queue_message(renderer, &response);
                }
                break;
            case Render_Message_Texture_Query_Response:
                if(msg.texU.userData == &gameState)
                {
                    memcpy(msg.texQR.textureDataPtr, gameState.image, gameState.w*gameState.h*4);
                    nk_font_atlas_end(&gameState.atlas, nk_handle_id((int)msg.texQR.textureId), &gameState.null);
                    nk_init_fixed(&gameState.nk_ctx, calloc(1, 1024*1024), 1024*1024, &gameState.font->handle);
                    gameState.ui_ready = true;
                    response = (RenderMessage){};
                    response.type = Render_Message_Texture_Update;
                    response.texU.textureId = msg.texQR.textureId;
                    renderer_queue_message(renderer, &response);
                }
                else
                {
                    fill_texture_data(msg.texQR.textureDataPtr);
                    response = (RenderMessage){};
                    response.type = Render_Message_Texture_Update;
                    response.texU.textureId = msg.texQR.textureId;
                    renderer_queue_message(renderer, &response);
                }
                break;
            case Render_Message_Mesh_Ready:
                if(msg.meshR.onComplete != NULL)
                {
                    msg.meshR.onComplete(renderer, &msg.meshR, msg.meshR.userData);
                }
                else
                {
                    printf("mesh ready %d\n", msg.meshR.meshId);
                }
                break;
            case Render_Message_Texture_Ready:
                printf("texture ready %d\n", msg.texR.textureId);
                break;
            case Render_Message_Material_Ready:
                printf("material ready %d\n", msg.matR.materialId);
                break;
        }
    }
}

void saveDummyMesh()
{
    void *buf = malloc(1024*1024);
    struct TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(struct TTRHeader);

    struct TTRDescTbl *descTbl = STREAM_PUSH_FLEX(stream, struct TTRDescTbl, entries, 1);
    descTbl->entryCount = 1;

    uint32_t vertBufSize = 24*40;
    struct TTRBuffer *vbuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, vertBufSize);
    vbuf->size = vertBufSize;
    uint8_t *vertStream = vbuf->data;
    for(int i = 0; i < 24; i++)
    {
        struct V4* pos = STREAM_PUSH(vertStream, struct V4);
        struct V2* texC = STREAM_PUSH(vertStream, struct V2);
        struct V4* col = STREAM_PUSH(vertStream, struct V4);
        *pos = cubeVertices[i];
        *texC = *((struct V2*)&cubeTexCoords[i]);
        *col = cubeColors[i];
    }

    uint32_t indexBufSize = ARRAY_COUNT(s_indices)*2;
    struct TTRBuffer *ibuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, indexBufSize);
    ibuf->size = indexBufSize;
    uint16_t *indexStream = (uint16_t*)ibuf->data;
    for(int i = 0; i < ARRAY_COUNT(s_indices); i++)
        indexStream[i] = s_indices[i];

    struct TTRMeshDesc *mdesc = STREAM_PUSH_FLEX(stream, struct TTRMeshDesc, attrs, 3);
    mdesc->indexSize = 2;
    mdesc->vertStride = 40;
    mdesc->numAttrs = 3;
    //mdesc->attrs[0] = Vertex_Attribute_Type_Vec4;
    mdesc->attrs[0] = Vertex_Attribute_Type_Vec2;
    mdesc->attrs[1] = Vertex_Attribute_Type_Vec4;


    struct TTRMesh* mesh = STREAM_PUSH_FLEX(stream, struct TTRMesh, sections, 1);
    TTR_SET_REF_TO_PTR(mesh->descRef, mdesc);
    TTR_SET_REF_TO_PTR(mesh->vertBufRef, vbuf);
    TTR_SET_REF_TO_PTR(mesh->indexBufRef, ibuf);
    mesh->numVertices = 24;
    mesh->numIndices = ARRAY_COUNT(s_indices);
    mesh->numSections = 1;
    mesh->sections[0].startIndex = 0;
    mesh->sections[0].indexCount = ARRAY_COUNT(s_indices);
//    printf("%p %p %p %p\n", buf, mesh, mesh, mesh3);

    TTR_SET_REF_TO_PTR(header->descTblRef, descTbl);
    TTR_SET_REF_TO_PTR(descTbl->entries[0].ref, mesh);
    descTbl->entries[0].type = TTR_4CHAR("MESH");

    FILE *file = fopen("Packages/First/first.ttr", "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing first.ttr failed!\n");
    free(buf);
}

void loadDummyMesh(const char *path, struct Renderer *renderer)
{
    static bool loaded;
    if(loaded)
        return;

    FILE *file = fopen(path, "rb");
    if(file)
    {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        void *buf = malloc(size);
        assert(buf);
        fread(buf, 1, size, file);

        struct LoadMeshData *mdata = malloc(sizeof(struct LoadMeshData));
        mdata->renderer = renderer;
        mdata->fileMem = buf;
        mdata->dataSize = size;
        ttr_load_first_mesh(mdata, 0);
        loaded = true;
    }
    else
        fprintf(stderr, "Loading %s failed!\n", path);
}

void loadpyramid(struct Renderer* renderer, struct MeshQueryResult *res, void *userData)
{
    fill_pyramid_data(res->vertBufPtr, res->idxBufPtr);
    RenderMessage response = (RenderMessage){};
    response.type = Render_Message_Mesh_Update;
    response.meshUpdate.meshId = res->meshId;
    renderer_queue_message(renderer, &response);
}

void init_game(struct Renderer *renderer, struct GameData *gdata)
{
    int comp;
    s_texdata = stbi_load("./cat.png", &s_x, &s_y, &comp, 4);
    assert(s_texdata != NULL);

    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.userData = renderer;
    msg.meshQuery.onComplete = loadpyramid;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = ARRAY_COUNT(pyramidVertices);
    msg.meshQuery.indexCount = ARRAY_COUNT(pyramidIndices);
    msg.meshQuery.attributeTypes[1] = Vertex_Attribute_Type_Vec4;
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.userData = (void*)MY_MESH_CUBE;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = ARRAY_COUNT(cubeVertices);
    msg.meshQuery.indexCount = ARRAY_COUNT(s_indices);
    msg.meshQuery.attributeTypes[0] = Vertex_Attribute_Type_Vec3;
    msg.meshQuery.attributeTypes[1] = Vertex_Attribute_Type_Vec4;
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = (void*)vertShaderSrc;
    msg.matQ.materialId = 0;
    msg.matQ.shaderTypes[0] = ShaderType_GLSL_Vert;
    msg.matQ.shaderCodes[0] = vertShaderSrc;
    msg.matQ.shaderLengths[0] = strlen(vertShaderSrc);
    msg.matQ.shaderTypes[1] = ShaderType_GLSL_Frag;
    msg.matQ.shaderCodes[1] = fragShaderSrc;
    msg.matQ.shaderLengths[1] = strlen(fragShaderSrc);
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = (void*)fragShaderSolidSrc;
    msg.matQ.materialId = 0;
    msg.matQ.shaderTypes[0] = ShaderType_GLSL_Vert;
    msg.matQ.shaderCodes[0] = vertShaderSrc;
    msg.matQ.shaderLengths[0] = strlen(vertShaderSrc);
    msg.matQ.shaderTypes[1] = ShaderType_GLSL_Frag;
    msg.matQ.shaderCodes[1] = fragShaderSolidSrc;
    msg.matQ.shaderLengths[1] = strlen(fragShaderSrc);
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = NULL;
    msg.matQ.materialId = 0;
    msg.matQ.shaderTypes[0] = ShaderType_GLSL_Vert;
    msg.matQ.shaderCodes[0] = uiVertSrc;
    msg.matQ.shaderLengths[0] = strlen(uiVertSrc);
    msg.matQ.shaderTypes[1] = ShaderType_GLSL_Frag;
    msg.matQ.shaderCodes[1] = uiFragSrc;
    msg.matQ.shaderLengths[1] = strlen(uiFragSrc);
    renderer_queue_message(renderer, &msg);

    msg = (RenderMessage){};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = NULL;
    msg.texQ.textureId = 0;
    msg.texQ.width = s_x;
    msg.texQ.height = s_y;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Trilinear;
    renderer_queue_message(renderer, &msg);

    s_builder = malloc(sizeof(struct RenderViewBuilder));
    rview_builder_init(s_builder);

    readView = take_view_buffer(gdata->swapbuf);

    // GUI
    //const char *font_path = "FreeMono.ttf";
    const char *font_path = NULL;
    nk_buffer_init_default(&gameState.cmds);
    nk_font_atlas_init_default(&gameState.atlas);
    nk_font_atlas_begin(&gameState.atlas);
    if (font_path) gameState.font = nk_font_atlas_add_from_file(&gameState.atlas, font_path, 13.0f, NULL);
    else gameState.font = nk_font_atlas_add_default(&gameState.atlas, 13.0f, NULL);

    gameState.image = nk_font_atlas_bake(&gameState.atlas, &gameState.w, &gameState.h, NK_FONT_ATLAS_RGBA32);

    msg = (RenderMessage){};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = &gameState;
    msg.texQ.width = gameState.w;
    msg.texQ.height = gameState.h;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Bilinear;
    renderer_queue_message(renderer, &msg);

    //nk_init_default(&ctx, &font->handle);}
    gameState.ui_ready = false;

    pyramid = 1;
}

void deinit_game()
{
    rview_builder_destroy(s_builder);
    free(s_builder);
}

void update_game(struct GameData *gdata)
{
    process_render_messages(gdata->renderer);

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

    rview_builder_reset(s_builder);
#pragma push(pack, 1)
    struct TestInstanceData
    {
        struct V4 v1;
        //struct V2 v2;
    };
#pragma pop(pack)

    struct TestInstanceData data;
    data.v1 = (struct V4){1.0f, 1.0f, 0.0f, 0.0f};

    struct Mat4 model;
    srand(1);

    struct V3 one = (struct V3){1.0f, 1.0f, 1.0f};

    static uint32_t pyMat = 0;

    for(int i = 0; i < 100; i++)
    for(int j = 0; j < 100; j++)
    {
        struct Quat rot;
        struct Quat rot2;
        quat_angle_axis(&rot, rotval*70.0f * ((float)j / 10.0f), make_v3(0.0f, 1.0f, 0.0f));
        quat_angle_axis(&rot2, -rotval*140.0f * ((float)i / 10.0f), make_v3(0.0f, 1.0f, 0.0f));
        bool cube = j%2==0;
        data.v1 = cols[rand() % ARRAY_COUNT(cols)];
        mat4_trs(&model, make_v3((float)i*2 - 100.0f, /*-5.0f*redVal*/ - 2.0f, 10.0f + (float)j*2 - 7.0f), cube?rot:rot2, one);
        add_mesh_instance(s_builder, cube?2:pyramid, cube?1:pyMat, &model, &data, sizeof(data));
    }

    struct UIVertex v1 = {{0.0f, 0.0f, 0.1f, 1.0f}, {}, 0xFFFF0000};
    struct UIVertex v2 = {{100.0f, 100.0f, 0.1f, 1.0f}, {}, 0xFFFF0000};
    struct UIVertex v3 = {{100.0f, 0.0f, 0.1f, 1.0f}, {}, 0xFFFF0000};
    struct UIVertex v4 = {{  0.0f, 100.0f, 0.1f, 1.0f}, {}, 0xFFFF0000};

    struct UIVertex v5 = {{0.0f, 0.0f, 0.1f, 1.0f}, {}, 0xAA0000FF};
    struct UIVertex v6 = {{200.0f, 200.0f, 0.1f, 1.0f}, {}, 0xAA0000FF};
    struct UIVertex v7 = {{200.0f, 0.0f, 0.1f, 1.0f}, {}, 0xAA0000FF};

    float width = gdata->platform->mainWin.width;
    float height = gdata->platform->mainWin.height;

    /*builder_begin_batch(s_builder, 0, 0, 0, width, height);
    add_vertex(s_builder, &v1);
    add_vertex(s_builder, &v2);
    add_vertex(s_builder, &v3);
    add_vertex(s_builder, &v4);
    add_index(s_builder, 0);
    add_index(s_builder, 1);
    add_index(s_builder, 2);
    add_index(s_builder, 0);
    add_index(s_builder, 3);
    add_index(s_builder, 1);
    builder_end_batch(s_builder);

    builder_new_vertex_stream(s_builder);

    builder_begin_batch(s_builder, 0, 0, 0, width, height);
    add_vertex(s_builder, &v5);
    add_vertex(s_builder, &v6);
    add_vertex(s_builder, &v7);
    add_index(s_builder, 0);
    add_index(s_builder, 1);
    add_index(s_builder, 2);
    builder_end_batch(s_builder);*/

    s_builder->viewProjection = s_perspective;
    s_builder->materialId = 3;

    struct Mat4 orthoM = {};
    orthoM.m11 =  2.0f / width;
    orthoM.m22 = -2.0f / height;
    orthoM.m33 = 1.0f;
    orthoM.m44 =  1.0f;
    orthoM.m41 = -1.0f;
    orthoM.m42 =  1.0f;
    s_builder->orthoMatrix = orthoM;

    // ------------------------------------------------

    struct nk_context *ctx = &gameState.nk_ctx;

    int mx = (int)gdata->platform->mouseX;
    int my = (int)gdata->platform->mouseY;
    uint16_t *akeys = gdata->platform->keyStates;

    nk_input_begin(ctx);
    nk_input_motion(ctx, mx, my);
    nk_input_button(ctx, NK_BUTTON_LEFT, mx, my, akeys[AIKE_BTN_LEFT] != 0);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, mx, my, akeys[AIKE_BTN_MIDDLE] != 0);
    nk_input_button(ctx, NK_BUTTON_RIGHT, mx, my, akeys[AIKE_BTN_RIGHT] != 0);
    nk_input_end(ctx);

    enum {EASY, HARD};
    static int op = EASY;
    static float value = 0.6f;
    static int i =  20;



    if(gameState.ui_ready)
    {
        if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 400),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
            /* fixed widget pixel width */
            nk_layout_row_static(ctx, 30, 100, 1);
            if (nk_button_label(ctx, "Write dummy")) {
                saveDummyMesh();
            }
            nk_layout_row_static(ctx, 30, 100, 1);
            if (nk_button_label(ctx, "Load dummy")) {
                loadDummyMesh("Packages/First/first.ttr",gdata->renderer); 
            }

            /* fixed widget window ratio width */
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            /* custom widget pixel width */
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
            nk_layout_row_end(ctx);

            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Mesh:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                pyramid = nk_slide_int(ctx, 0, pyramid, 3, 1);
            nk_layout_row_end(ctx);

            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
                nk_label(ctx, "Material:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                pyMat = nk_slide_int(ctx, 0, pyMat, 2, 1);
            nk_layout_row_end(ctx);

            nk_layout_row_dynamic(ctx, 30, 1);
            nk_property_int(ctx, "#Material:", 0, (int*)&pyMat, 2, 1, 0.1f);
        }
        nk_end(ctx);
    }

    // draw commands
    builder_new_vertex_stream(s_builder);

//    struct UIVertex vertBuf[10000];
//    uint16_t indexBuf[6000];

    uint32_t maxVerts = 10000;
    uint32_t maxIndices = 6000;

    //struct UIVertex *vertBuf = malloc(sizeof(struct UIVertex) * maxVerts);
    //uint16_t *indexBuf = malloc(sizeof(uint16_t) * maxIndices);
    struct UIVertex vertBuf[maxVerts];
    uint16_t indexBuf[maxIndices];

    const struct nk_draw_command *cmd;
    void *vertices, *elements;
    const nk_draw_index *offset = NULL;

    {
        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, 0},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, 16},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, 24},
            {NK_VERTEX_LAYOUT_END}
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct UIVertex);
        config.vertex_alignment = NK_ALIGNOF(struct UIVertex);
        config.null = gameState.null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        /* setup buffers to load vertices and elements */
        {struct nk_buffer vbuf, ebuf;
        nk_buffer_init_fixed(&vbuf, vertBuf, sizeof(vertBuf[0])*maxVerts);
        nk_buffer_init_fixed(&ebuf, indexBuf, sizeof(indexBuf[0])*maxIndices);
        nk_convert(ctx, &gameState.cmds, &vbuf, &ebuf, &config);}

        add_vertices(s_builder, vertBuf, maxVerts);

        float sx = 1.0f;
        float sy = 1.0f;

        uint32_t curIdxBase = 0;
        nk_draw_foreach(cmd, ctx, &gameState.cmds) {
            if (!cmd->elem_count) continue;
            int scX0 = (cmd->clip_rect.x * sx);
            int scY0 = ((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * sy);
            int scX1 = (cmd->clip_rect.w * sx);
            int scY1 = (cmd->clip_rect.h * sy);

            builder_begin_batch(s_builder, cmd->texture.id, scX0, scY0, scX1, scY1);
            for(int j = 0; j < cmd->elem_count; j++)
            {
                add_index(s_builder, indexBuf[j+curIdxBase]);
            }
            builder_end_batch(s_builder);
            curIdxBase += cmd->elem_count;
        }
    }

    nk_clear(ctx);

    // 0000000000000000000000000000000000000000000000000

    build_view(s_builder, readView);

    readView = swap_view_for_newer(gdata->swapbuf, readView);
}

/*void* game_loop(void *data)
{
    struct GameData *gdata = (struct GameData *)data;

    init_game(gdata->renderer, gdata);

    while(true)
    {
        update_game(gdata);
    }

    deinit_game();

    free(gdata);
    return NULL;
}*/


