
const char* vertex_shader_str = 
R"foo(#version 330 core
layout(location = 0) in vec4 position; 
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 uv;
layout(location = 3) in int drawMode;

uniform mat4 projection;

out vec4 fragColor;
out vec3 fragUv;
flat out int fragMode;

void main() {
	gl_Position = projection*position;
    fragColor = color;
    fragUv = uv;
    fragMode = drawMode;
})foo";

const char* frag_shader_str = 
R"foo(#version 330 core
layout(location = 0) out vec4 outColor;

in vec4 fragColor;
in vec3 fragUv;
flat in int fragMode;

uniform sampler2DArray tex;
uniform sampler2DArray tileTex;

vec4 textFragProc()
{
	float a = texture(tex, fragUv).r;
    vec3 col = a * fragColor.rgb;
    return vec4(col, a);
}

vec4 texTileFragProc()
{
    return texture(tileTex, fragUv);
}

vec4 solidFragProc()
{
    float a = fragColor.a;
    vec3 col = a * fragColor.rgb;
    return vec4(col, a);
}

void main() {
    switch(fragMode)
    {
        case 0:
            outColor = solidFragProc();
            break;
        case 1:
            outColor = textFragProc();
            break;
        case 2:
            outColor = texTileFragProc();
            break;
        default:
            outColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
})foo";

const char *slow_quad_vertex = 
R"foo(#version 330 core
layout(location = 0) in vec4 position; 
layout(location = 1) in vec2 uv;

uniform mat4 projection;

out vec2 fragUv;

void main() {
	gl_Position = projection*position;
    fragUv = uv;
}
)foo";

const char *slow_quad_frag = 
R"foo(#version 330 core
layout(location = 0) out vec4 outColor;

in vec2 fragUv;

uniform sampler2D tex;

void main() {
    outColor = texture(tex, fragUv);
}
)foo";

// TODO WHY?
static void glShowError(const char *title, const char *text)
{
    fprintf(stderr, "%s %s\n", title, text);
}

static bool CheckShaderCompileStatus(GLint shader)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[1024];
		glGetShaderInfoLog(shader, 1024, NULL, errorBuf);
        glShowError("Error", errorBuf);
	}
	return status == GL_TRUE;
}

static bool compile_shader(GLuint *compiledProgram, const char *vertexShader, const char *fragmentShader)
{
	bool success = true;
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	int vert_shader_len = strlen(vertexShader);
	int frag_shader_len = strlen(fragmentShader);
	glShaderSource(vert_shader, 1, &vertexShader, &vert_shader_len);
	glShaderSource(frag_shader, 1, &fragmentShader, &frag_shader_len);
	glCompileShader(vert_shader);
	glCompileShader(frag_shader);
	if (!CheckShaderCompileStatus(vert_shader) || !CheckShaderCompileStatus(frag_shader))
	{
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
		return false;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[1024];
		glGetProgramInfoLog(program, 1024, NULL, errorBuf);
        glShowError("Error", errorBuf);
		success = false;
	}
	else
	{
		*compiledProgram = program;
	}
	glDetachShader(program, vert_shader);
	glDetachShader(program, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return success;
}


MemoryManager *g_memoryManager;
Renderer *g_renderer;
UserInterface *g_ui;
AikeInput *g_input;
EventManager *g_emgr;
Aike *g_aike;

static AikeInput input;
static EventManager eventManager;
static Aike aike;

// TODO: get rid of this
GLuint program;

extern "C"
{
    DLL_PUBLIC void aike_update_window(AikePlatform *platform, AikeWindow *win);
    DLL_PUBLIC void aike_init(AikePlatform *platform);
    DLL_PUBLIC void aike_update(AikePlatform *platform);
    DLL_PUBLIC void aike_deinit(AikePlatform *platform);
}

DLL_PUBLIC void aike_update_window(AikePlatform *platform, AikeWindow *win)
{
    layout_tree_resize(&g_ui->layoutTree, win->width, win->height);
    g_renderer->projection = Mat4::proj2d((float)win->width, (float)win->height);
    GLint projMLoc = glGetUniformLocation(g_renderer->mainProgram, "projection");
    if(projMLoc == -1)
        printf("projection matrix not found!\n");
    // TODO: this should not be needed!
    glUseProgram(g_renderer->mainProgram);
    glUniformMatrix4fv(projMLoc, 1, GL_FALSE, (GLfloat*)&g_renderer->projection);
    glViewport(0, 0, win->width, win->height);
}

void aike_make_window_current(AikeWindow *window)
{
    g_platform->make_window_current(g_platform, window);
    g_renderer->curWindow = window;
}

DLL_PUBLIC void aike_init(AikePlatform *platform)
{
    g_memoryManager = memory_manager_initialize();
    g_platform = platform;
    platform->numMenus = 0;
    platform->frameCounter = 0;
    platform->rootMenu = NULL;

    for(int i = 0; i < ARRAY_COUNT(aike.images.imagePresent); i++)
        aike.images.imagePresent[i] = false;
    g_aike = &aike;
    aike_init_tile_pool(&g_aike->tilePool);

    g_input = &input;
    input_init(g_input);

    g_emgr = &eventManager;
    event_manager_init(g_emgr);

    g_renderer = (Renderer*)aike_alloc(sizeof(Renderer));
    memset(g_renderer, 0, sizeof(Renderer));
    renderer_init(g_renderer);

    g_ui = (UserInterface*)aike_alloc(sizeof(UserInterface));
    user_interface_init(g_ui, 1024, 768);

    compile_shader(&program, vertex_shader_str, frag_shader_str);
    compile_shader(&g_renderer->slowQuadProgram, slow_quad_vertex, slow_quad_frag);
    g_renderer->mainProgram = program;
    CachedFont* timesFont = font_manager_load_font(&g_renderer->fontManager, "./fonts/times.ttf", 32);
    font_manager_load_font(&g_renderer->fontManager, "./fonts/times.ttf", 18);

    const char *filename = "./cat.png";
    const char *patternfile = "./pattern.png";
    int x,y,n;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 4);
    if(data != NULL)
        aike_open_image(&aike, x, y, 4, data, false);
    else
        fprintf(stderr, "Failed to load image %s\n", filename);
    stbi_image_free(data);

    // checkerboard pattern
    uint32_t psz = AIKE_IMG_CHUNK_SIZE;
    uint32_t freq = AIKE_IMG_CHUNK_SIZE / 8;
    uint32_t step = psz / freq;
    assert(psz%freq == 0);
    data = (unsigned char*)aike_alloc(psz * psz * 4);
    for(uint32_t i = 0; i < psz; i++)
    for(uint32_t j = 0; j < psz; j++)
    {
        *((uint32_t*)&data[i*psz*4 + j*4]) = (((i/step)&1) ^ ((j/step)&1)) ? 0xff8c8c8c : 0xff6d6d6d;
    }
    x = psz;
    y = psz;
    n = 4;

    if(data != NULL)
        aike_open_image(&aike, x, y, 4, data, true);
    else
        fprintf(stderr, "Failed to load image %s\n", patternfile);
    aike_free(data);

    // GOOO
    aike_make_window_current(&platform->mainWin);
    layout_tree_tests(&g_ui->layoutTree);
}

DLL_PUBLIC void aike_update(AikePlatform *platform)
{
    g_input->mousePos = Vec2(platform->mouseX, platform->mouseY);
    g_input->mouseScreenPos = Vec2(platform->mouseScreenX, platform->mouseScreenY);
    g_input->inputStatesPrev = g_input->inputStates;
    g_input->inputStates = platform->mouseButtons;

    // copy current keystate to prev slot
    memcpy(g_input->keyStatesPrev, g_input->keyStates, sizeof(g_input->keyStates));
    // get new state
    assert(sizeof(g_input->keyStates) == sizeof(platform->keyStates));
    memcpy(g_input->keyStates, platform->keyStates, sizeof(g_input->keyStates));

    input_update(g_input, platform);

    aike_make_window_current(&platform->mainWin);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    user_interface_pre_frame(g_ui);
    user_interface_draw(g_ui);

    platform->present_frame(platform, &platform->mainWin);

    for(int i = 0; i < platform->numMenus; i++)
    {
        context_menu_render(g_renderer, platform->menus[i]);
    }

    if(MOUSE1_DOWN())
        event_manager_event(g_emgr, Event_Mouse1Down);
    if(MOUSE2_DOWN())
        event_manager_event(g_emgr, Event_Mouse2Down);
    if(MOUSE1_UP())
        event_manager_event(g_emgr, Event_Mouse1Up);
    if(MOUSE2_UP())
        event_manager_event(g_emgr, Event_Mouse2Up);

    g_input->inputStatesPrev = g_input->inputStates;

    platform->sleep(16666); // 16.666 ms
    platform->frameCounter++;
}


DLL_PUBLIC void aike_deinit(AikePlatform *platform)
{
    for(int i = 0; i < ARRAY_COUNT(aike.images.images); i++)
    {
        if(aike.images.imagePresent[i])
        {
            aike_close_image(&aike, &aike.images.images[i]);
        }
    }

    user_interface_free_resources(g_ui);

    aike_free(g_ui);
    renderer_free_resources(g_renderer);

    event_manager_free_resources(g_emgr);

    glDeleteProgram(program);

    uint32_t er = glGetError();
    if(er != 0)
    {
        printf("GLERROR: %04x\n", er);
    }

    aike_destroy_tile_pool(&g_aike->tilePool);

    aike_free(g_renderer);
    memory_manager_print_entries(g_memoryManager, true);
    memory_manager_free_resources(g_memoryManager);
}

bool aike_mouse_in_window_rect(AikePlatform *platform, AikeWindow *win, Rect rect)
{
    Vec2 lmPos;
    Vec2 smPos = g_input->mouseScreenPos;
    // TODO: calculate this per window every frame and reuse the result
    platform->screen_to_window_coord(platform, win, smPos.x, smPos.y, &lmPos.x, &lmPos.y);
    return IN_RECT(rect, lmPos) && platform->mouse_coord_valid(platform, win);
}
