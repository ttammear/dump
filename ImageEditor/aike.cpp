
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

vec4 textFragProc()
{
	float a = texture(tex, fragUv).r;
    vec3 col = a * fragColor.rgb;
    return vec4(col, a);
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
        default:
            outColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
})foo";

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

static AikeInput input;
static EventManager eventManager;

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
    layout_tree_resize(&g_ui->layoutTree, win->screenRect.width, win->screenRect.height);
    g_renderer->projection = Mat4::proj2d((float)win->screenRect.width, (float)win->screenRect.height);
    GLint projMLoc = glGetUniformLocation(g_renderer->mainProgram, "projection");
    if(projMLoc == -1)
        printf("projection matrix not found!\n");
    // TODO: this should not be needed!
    glUseProgram(g_renderer->mainProgram);
    glUniformMatrix4fv(projMLoc, 1, GL_FALSE, (GLfloat*)&g_renderer->projection);
    glViewport(0, 0, win->screenRect.width, win->screenRect.height);
}

DLL_PUBLIC void aike_init(AikePlatform *platform)
{
    g_memoryManager = memory_manager_initialize();
    g_platform = platform;
    platform->numMenus = 0;
    platform->frameCounter = 0;
    platform->rootMenu = NULL;

    g_input = &input;
    input_init(g_input);

    g_emgr = &eventManager;
    event_manager_init(g_emgr);

    g_renderer = new Renderer();
    renderer_init(g_renderer);

    g_ui = (UserInterface*)aike_alloc(sizeof(UserInterface));
    user_interface_init(g_ui, 1024, 768);
    layout_tree_tests(&g_ui->layoutTree);

    compile_shader(&program, vertex_shader_str, frag_shader_str);
    g_renderer->mainProgram = program;
    CachedFont* timesFont = font_manager_load_font(&g_renderer->fontManager, "./fonts/times.ttf", 32);
    font_manager_load_font(&g_renderer->fontManager, "./fonts/times.ttf", 18);

}

DLL_PUBLIC void aike_update(AikePlatform *platform)
{
    g_input->mousePos = Vec2(platform->mouseX, platform->mouseY);
    g_input->mouseScreenPos = Vec2(platform->mouseScreenX, platform->mouseScreenY);
    g_input->inputStatesPrev = g_input->inputStates;
    g_input->inputStates = platform->mouseButtons;

    platform->make_window_current(platform, &platform->mainWin);
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

    delete g_renderer;
    memory_manager_print_entries(g_memoryManager, true);
    memory_manager_free_resources(g_memoryManager);
}
