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

struct Mesh *create_mesh()
{
    // TODO: use pool / stack allocator
    struct Mesh *ret = malloc(sizeof(struct Mesh));
    ret->flags = 0;
    ret->numVertices = 0;
    ret->numIndices = 0;
    ret->vertices = NULL;
    ret->texCoords = NULL;
    ret->normals = NULL;
    ret->colors = NULL;
    ret->indices = NULL;

    // TODO:temp
    ret->tex = NULL;

    return ret;
}

struct Texture *create_texture()
{
    struct Texture *ret = malloc(sizeof(struct Texture));
    ret->flags = 0;
    ret->format = TextureFormat_None;
    ret->width = 0;
    ret->height = 0;
    ret->data = NULL;
    return ret;
}

struct Renderer *create_headless_renderer()
{
    struct Renderer *ret = calloc(1, sizeof(struct Renderer));
    return ret;
}

struct Renderer *create_renderer(u32 rendererType)
{
    struct Renderer *ret = NULL;
    g_renderEventHandler = malloc(sizeof(struct RenderEventHandler));
    init_render_event_handler();

    struct Material *mat;

    switch(rendererType)
    {
        case RENDERER_TYPE_OPENGL:
            ret = create_opengl_renderer();
            mat = &ret->fallbackMaterial;
            material_init(mat);
            material_add_shader(mat, ShaderType_GLSL_Vert, fallbackVert);
            material_add_shader(mat, ShaderType_GLSL_Frag, fallbackFrag);
            material_queue_for_update(mat);
            break;
        case RENDERER_TYPE_HEADLESS:
            ret = create_headless_renderer();
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

void destroy_mesh(struct Mesh *mesh)
{
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasVertices))
    { 
        free(mesh->vertices);
        CLEAR_BIT(mesh->flags, MeshFlag_HasVertices);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasIndices))
    { 
        free(mesh->indices);
        CLEAR_BIT(mesh->flags, MeshFlag_HasIndices);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasTexcoords))
    { 
        free(mesh->texCoords);
        CLEAR_BIT(mesh->flags, MeshFlag_HasTexcoords);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasNormals))
    { 
        free(mesh->normals);
        CLEAR_BIT(mesh->flags, MeshFlag_HasNormals);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasColors))
    { 
        free(mesh->colors);
        CLEAR_BIT(mesh->flags, MeshFlag_HasColors);
    }
    trigger_render_event(RenderEvent_MeshDestroy, mesh);
    free(mesh);
}

void destroy_texture(struct Texture *tex)
{
    // TODO: trigger render event
    if(BIT_IS_SET(tex->flags, TextureFlag_HasData))
    {
        free(tex->data);
        CLEAR_BIT(tex->flags, TextureFlag_HasData);
    }
    trigger_render_event(RenderEvent_TextureDestroy, tex);
    free(tex);
}

void mesh_invalidate_vertices(struct Mesh *mesh)
{
    free(mesh->vertices);
    CLEAR_BIT(mesh->flags, MeshFlag_HasVertices);
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasTexcoords))
    {
        free(mesh->texCoords);
        CLEAR_BIT(mesh->flags, MeshFlag_HasTexcoords);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasNormals))
    {
        free(mesh->normals);
        CLEAR_BIT(mesh->flags, MeshFlag_HasNormals);
    }
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasColors))
    {
        free(mesh->colors);
        CLEAR_BIT(mesh->flags, MeshFlag_HasColors);
    }
}

void mesh_copy_vertices(struct Mesh *mesh, struct V3 *verts, u32 count)
{
    // delete old vertices, if larger buffer is required
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasVertices) && count > mesh->numVertices)
    {
        mesh_invalidate_vertices(mesh);
    }
    // allocate new memory if not allocated yet or size does not match
    if(!BIT_IS_SET(mesh->flags, MeshFlag_HasVertices))
    {
        mesh->vertices = malloc(sizeof(struct V3) * count);
    }
    memcpy(mesh->vertices, verts, sizeof(struct V3) * count);
    SET_BIT(mesh->flags, MeshFlag_HasVertices);
    SET_BIT(mesh->flags, MeshFlag_Dirty);
    mesh->numVertices = count;
}

void mesh_copy_texcoords(struct Mesh *mesh, struct V3 *texCoords, u32 count)
{
    if(count == mesh->numVertices)
    {
        if(!BIT_IS_SET(mesh->flags, MeshFlag_HasTexcoords))
        {
            mesh->texCoords = malloc(sizeof(struct V3) * count);
        }
        memcpy(mesh->texCoords, texCoords, sizeof(struct V3) * count);
        SET_BIT(mesh->flags, MeshFlag_HasTexcoords);
        SET_BIT(mesh->flags, MeshFlag_Dirty);
    }
    else
        tt_render_warning("mesh_copy_texcoords() count didn't match mesh vertex count, ignoring!");
}

void mesh_copy_normals(struct Mesh *mesh, struct V3 *normals, u32 count)
{
    if(count == mesh->numVertices)
    {
        if(!BIT_IS_SET(mesh->flags, MeshFlag_HasNormals))
        {
            mesh->normals = malloc(sizeof(struct V3) * count);
        }
        memcpy(mesh->normals, normals, sizeof(struct V3) * count);
        SET_BIT(mesh->flags, MeshFlag_HasNormals);
        SET_BIT(mesh->flags, MeshFlag_Dirty);
    }
    else
        tt_render_warning("mesh_copy_texcoords() count didn't match mesh vertex count, ignoring!");
}

void mesh_copy_colors(struct Mesh *mesh, struct V4 *colors, u32 count)
{
    if(count == mesh->numVertices)
    {
        if(!BIT_IS_SET(mesh->flags, MeshFlag_HasColors))
        {
            mesh->colors = malloc(sizeof(struct V4) * count);
        }
        memcpy(mesh->colors, colors, sizeof(struct V4) * count);
        SET_BIT(mesh->flags, MeshFlag_HasColors);
        SET_BIT(mesh->flags, MeshFlag_Dirty);
    }
    else
        tt_render_warning("mesh_copy_texcoords() count didn't match mesh vertex count, ignoring!");
}


void mesh_copy_indices(struct Mesh *mesh, u16 *indices, u32 count)
{
    // delete old indices, if larger buffer is required
    if(BIT_IS_SET(mesh->flags, MeshFlag_HasIndices) && count > mesh->numIndices)
    {
        free(mesh->indices);
        CLEAR_BIT(mesh->flags, MeshFlag_HasIndices);
    }
    // allocate new memory if not allocated yet or size does not match
    if(!BIT_IS_SET(mesh->flags, MeshFlag_HasIndices))
    {
        mesh->indices = malloc(sizeof(u16) * count);
    }
    memcpy(mesh->indices, indices, sizeof(u16) * count);
    SET_BIT(mesh->flags, MeshFlag_HasIndices);
    SET_BIT(mesh->flags, MeshFlag_Dirty);
    mesh->numIndices = count;
}

void mesh_queue_for_update(struct Mesh *mesh)
{
    trigger_render_event(RenderEvent_MeshUpdate, mesh);
}

u32 tex_format_get_comps(u16 format)
{
    switch(format)
    {
        case TextureFormat_None:
            return 0;
        case TextureFormat_RGB:
            return 3;
        case TextureFormat_RGBA:
            return 4;
        case TextureFormat_R8:
            return 1;
        default:
            tt_render_fatal("Invalid texture format");
            break;
    }
    return (u32)-1;
}

void texture_copy_data(struct Texture *tex, u32 width, u32 height, u16 format, void *data)
{
    u32 oldComps = tex_format_get_comps(tex->format);
    u32 oldSize = oldComps * tex->width * tex->height;

    u32 comps = tex_format_get_comps(format);
    u32 size = comps * width * height;

    if(oldSize < size)
    {
        free(tex->data);
        CLEAR_BIT(tex->flags, TextureFlag_HasData);
    }
    
    void *buf;
    if(!BIT_IS_SET(tex->flags, TextureFlag_HasData))
    {
        buf = malloc(size);
        SET_BIT(tex->flags, TextureFlag_HasData);
    }
    else
        buf = tex->data;
    if(buf == NULL)
        tt_render_fatal("Out of memory");
    memcpy(buf, data, size);
    SET_BIT(tex->flags, TextureFlag_Dirty);
    tex->width = width;
    tex->height = height;
    tex->data = buf;
    tex->format = format;
}

void texture_queue_for_update(struct Texture *tex)
{
    trigger_render_event(RenderEvent_TextureUpdate, tex);
}

void material_init(struct Material *mat)
{
    mat->rendererHandle = 0;
    mat->flags = 0;
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        mat->shaders[i].flags = 0;
        mat->shaders[i].source = NULL;
        mat->shaders[i].rendererHandle = 0;
    }
}

void material_destroy(struct Material *mat)
{
    trigger_render_event(RenderEvent_MaterialDestroy, mat);
}

void material_add_shader(struct Material *mat, u32 type, const char *source)
{
    i32 idx = -1;
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        if(!BIT_IS_SET(mat->shaders[i].flags, ShaderFlag_Active))
        {
            idx = i;
            break;
        }
    }
    assert(idx >= 0);
    assert(type < ShaderType_Count);
    SET_BIT(mat->flags, MaterialFlag_Dirty);
    SET_BIT(mat->shaders[idx].flags, ShaderFlag_Dirty);
    SET_BIT(mat->shaders[idx].flags, ShaderFlag_Active);
    mat->shaders[idx].type = type;
    mat->shaders[idx].source = source;
}

void material_remove_shader(struct Material *mat, u32 index)
{
    assert(index < MATERIAL_MAX_SHADERS);
    assert(BIT_IS_SET(mat->shaders[index].flags, ShaderFlag_Active));
    SET_BIT(mat->flags, MaterialFlag_Dirty);
    CLEAR_BIT(mat->shaders[index].flags, ShaderFlag_Active);
}

void material_queue_for_update(struct Material *mat)
{
    trigger_render_event(RenderEvent_MaterialUpdate, mat);
}

b32 material_find_shader_of_type(struct Material *mat, u32 type, u32 *ret)
{
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        struct Shader *shader = &mat->shaders[i];
        if(BIT_IS_SET(shader->flags, ShaderFlag_Active) && shader->type == type)
        {
            *ret = i;
            return true;
        }
    }
    return false;
}

void init_render_event_handler()
{
    for(int i = 0; i < RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
       g_renderEventHandler->freelist[i] = i;
    }
    for(int i = 0; i < RenderEvent_Count; i++)
    {
        g_renderEventHandler->entries[i] = NULL;
    }
    g_renderEventHandler->firstFree = 0;
}

void *subscribe_to_render_event(u32 eventId, RenderEventCallback_F callback, void *usrPtr)
{
    assert(g_renderEventHandler->firstFree != RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS);
    assert(eventId < RenderEvent_Count);
    if(g_renderEventHandler->firstFree < RENDER_EVENT_HANDLER_MAX_SUBSCRIBERS)
    {
        struct RenderEventSubEntry *entry 
            = &g_renderEventHandler->entryPool[g_renderEventHandler->firstFree++];
        entry->usrPtr = usrPtr;
        entry->callback = callback;
        entry->prev = NULL;
        entry->next = g_renderEventHandler->entries[eventId];
        g_renderEventHandler->entries[eventId] = entry;
        return entry;
    }
    else
    {
        tt_render_fatal("subscribe_to_render_event() pool ran out of entries!");
        return NULL;
    }
}

void unsubscribe_from_render_event(u32 eventId, void *ptr)
{
    assert(eventId < RenderEvent_Count);
    struct RenderEventSubEntry *entry = g_renderEventHandler->entries[eventId];
    while(entry != NULL)
    {
        if(ptr == entry)
        {
            if(entry->prev != NULL)
                entry->prev->next = entry->next;
            if(entry->next != NULL)
                entry->next->prev = entry->prev;
            return;
        }
        entry = entry->next;
    }
    tt_render_warning("unsubscribe_from_render_event() called, but not subscribed");
}

void trigger_render_event(u32 eventId, void *eventData)
{
    assert(eventId < RenderEvent_Count);
    struct RenderEventSubEntry *entry = g_renderEventHandler->entries[eventId];
    while(entry != NULL)
    {
        entry->callback(eventId, eventData, entry->usrPtr);
        entry = entry->next;
    }
}

