static void quad_buffer_init(QuadBuffer *qbuf)
{
    GLuint vao, vbo;
    TRACK_RESOURCE_SET(qbuf->vaouuid, "Quad Buffer VAO");
    TRACK_RESOURCE_SET(qbuf->vbouuid, "Quad Buffer VBO");
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    int maxVertices = ARRAY_COUNT(qbuf->vertices);
    qbuf->vboSize = maxVertices;
    qbuf->vboDataPerVertex = sizeof(QuadVertex);
    qbuf->numQuads = 0;
    qbuf->vao = vao;
    qbuf->vbo = vbo;

    // allocate buffer on GPU
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices*sizeof(QuadVertex), NULL, GL_STREAM_DRAW);

    // enable attributes and define memory layout
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const GLvoid*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const GLvoid*)offsetof(QuadVertex, color));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const GLvoid*)offsetof(QuadVertex, uv));
    glVertexAttribIPointer(3, 1, GL_INT, sizeof(QuadVertex), (const GLvoid*)offsetof(QuadVertex, mode));
    glBindVertexArray(0);

}

static void quad_buffer_free(QuadBuffer *qbuf)
{
    glDeleteVertexArrays(1, &qbuf->vao);
    glDeleteBuffers(1, &qbuf->vbo);
    FREE_RESOURCE(qbuf->vaouuid);
    FREE_RESOURCE(qbuf->vbouuid);
}

static void slow_quad_buffer_init(SlowQuadBuffer *sbuf)
{
    GLuint vbo, vao;
    TRACK_RESOURCE_SET(sbuf->vbouuid, "Slow Quad Buffer VBO");
    TRACK_RESOURCE_SET(sbuf->vaouuid, "Slow Quad Buffer VAO");
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    sbuf->bufSize = 6*(3*4 + 2*4); // 4 * (Vec3 and Vec2)
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sbuf->bufSize, NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sbuf->bufSize / 6, (const GLvoid*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sbuf->bufSize / 6, (const GLvoid*)(3*4));
    glBindVertexArray(0);
    sbuf->vbo = vbo;
    sbuf->vao = vao;
}

static void slow_quad_buffer_free(SlowQuadBuffer *qbuf)
{
    glDeleteBuffers(1, &qbuf->vbo);
    glDeleteVertexArrays(1, &qbuf->vao);
    FREE_RESOURCE(qbuf->vbouuid);
    FREE_RESOURCE(qbuf->vaouuid);
}

void slow_quad_buffer_draw_quad(SlowQuadBuffer *qbuf, Rect rect, GLuint texture)
{
    Vec2 min(rect.x0, rect.y0);
    Vec2 max = min + Vec2(rect.width, rect.height);
    uint32_t idx = qbuf->numQuads++;
    assert(idx < ARRAY_COUNT(qbuf->quads));
    SlowQuad *q = &qbuf->quads[idx];
    q->min = min;
    q->max = max;
    q->texture = texture;
}

static void slow_quad_buffer_render(SlowQuadBuffer *qbuf)
{
    if(qbuf->numQuads == 0)
        return;

    GLuint texLoc = glGetUniformLocation(g_renderer->slowQuadProgram, "tex");
    GLuint projLoc = glGetUniformLocation(g_renderer->slowQuadProgram, "projection");
    glUseProgram(g_renderer->slowQuadProgram);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(texLoc, 0);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, (GLfloat*)&g_renderer->projection);

    glBindBuffer(GL_ARRAY_BUFFER, qbuf->vbo);
    glBindVertexArray(qbuf->vao);

    for(uint32_t i = 0; i < qbuf->numQuads; i++)
    {
        SlowQuad *q = &qbuf->quads[i];
        struct TempData
        {
            Vec3 pos;
            Vec2 uv;
        };
        TempData bufData[6];
        bufData[0] = {{q->min.x, q->min.y, 0.0f}, {0.0f, 0.0f}};
        bufData[1] = {{q->min.x, q->max.y, 0.0f}, {0.0f, 1.0f}};
        bufData[2] = {{q->max.x, q->min.y, 0.0f}, {1.0f, 0.0f}};
        bufData[3] = {{q->min.x, q->max.y, 0.0f}, {0.0f, 1.0f}};
        bufData[4] = {{q->max.x, q->max.y, 0.0f}, {1.0f, 1.0f}};
        bufData[5] = {{q->max.x, q->min.y, 0.0f}, {1.0f, 0.0f}};
        assert(sizeof(bufData) == qbuf->bufSize);

        glBindTexture(GL_TEXTURE_2D, q->texture);
        glBufferSubData(GL_ARRAY_BUFFER, 0, qbuf->bufSize, &bufData);
        glDrawArrays(GL_TRIANGLES, 0 , 6);
    }

    qbuf->numQuads = 0;
}

static void font_manager_initialize(FontManager *fmgr)
{
    fmgr->numCachedFonts = 0;
}

static void font_manager_free_resources(FontManager* fmgr)
{
    int count = fmgr->numCachedFonts;
    for(int i = 0; i < count; i++)
    {
        cached_font_unload(fmgr->cachedFonts[i]);
        aike_free(fmgr->cachedFonts[i]);
    }
    fmgr->numCachedFonts = 0;
}

static void renderer_init(Renderer *renderer)
{
    font_manager_initialize(&renderer->fontManager);

    renderer->currentLayer = 0;

    for(int i = 0; i < ARRAY_COUNT(renderer->layerBuffers); i++)
    {
        quad_buffer_init(&renderer->layerBuffers[i]);
    }
    for(int i = 0; i < ARRAY_COUNT(renderer->slowLayerBuffers); i++)
    {
        slow_quad_buffer_init(&renderer->slowLayerBuffers[i]);
    }

    TRACK_RESOURCE_SET(renderer->atlasdebug, "Renderer main texture atlas");
    glGenTextures(1, &renderer->textureAtlasArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->textureAtlasArray);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 512, 512, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    renderer->textureAtlasNumLayers = 8;
    renderer->textureAtlasUsedLayers = 0;
}

static void renderer_free_resources(Renderer *renderer)
{
    font_manager_free_resources(&renderer->fontManager);
    for(int i = 0; i < ARRAY_COUNT(renderer->layerBuffers); i++)
    {
        quad_buffer_free(&renderer->layerBuffers[i]);
    }
    for(int i = 0; i < ARRAY_COUNT(renderer->slowLayerBuffers); i++)
    {
        slow_quad_buffer_free(&renderer->slowLayerBuffers[i]);
    }
    FREE_RESOURCE(renderer->atlasdebug);
    glDeleteTextures(1, &renderer->textureAtlasArray);
}

// TODO: ability to return layers back to a pool?
// TODO: instead of this add a method to add texture to the atlas and get the UVs + layer back
static int renderer_get_atlas_layer(Renderer *renderer)
{
    assert(renderer->textureAtlasUsedLayers < renderer->textureAtlasNumLayers); // out of layers
    return renderer->textureAtlasUsedLayers++;
}

static void quad_buffer_draw_quad(QuadBuffer *qbuf, QuadVertex vertices[], uint32_t mode = 0)
{
    int index = qbuf->numQuads*6;
    QuadVertex *qvertices = &qbuf->vertices[index];
    int drawOrder[] = {0, 3, 1, 3, 2, 1};
    if(index+5 >= qbuf->vboSize)
        aike_fatal_error("Too many quads in quad buffer, increase buffer size!");
    else
    {
        for(int i = 0; i < 6; i++)
        {
            qvertices[i].position = vertices[drawOrder[i]].position;
            qvertices[i].color = vertices[drawOrder[i]].color;
            qvertices[i].uv = vertices[drawOrder[i]].uv;
            qvertices[i].mode = mode;
            //printf("set vertex %d %f %f %f\n", i, vertices[i].position.x, vertices[i].position.y, vertices[i].position.z);
        }
    }
    qbuf->numQuads++;
}

/*static void quad_buffer_render(QuadBuffer *qbuf, GLint program)
{
    if(qbuf->numQuads == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program);
    
    glBindVertexArray(qbuf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, qbuf->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, qbuf->numQuads*6*sizeof(QuadVertex), qbuf->vertices);
    // TODO: try to stall this draw, otherwise we'd be waiting for the upload to complete
    glDrawArrays(GL_TRIANGLES, 0, qbuf->numQuads*6);
    glUseProgram(0);
    qbuf->numQuads = 0;
}*/

static void quad_buffer_render(QuadBuffer *qbuf, GLint program, GLuint texture)
{
    if(qbuf->numQuads == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint texLoc = glGetUniformLocation(program, "tex");
    glUseProgram(program);
    
    glUniform1i(texLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glBindVertexArray(qbuf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, qbuf->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, qbuf->numQuads*6*sizeof(QuadVertex), qbuf->vertices);
    // TODO: try to stall this draw, otherwise we'd be waiting for the upload to complete
    glDrawArrays(GL_TRIANGLES, 0, qbuf->numQuads*6);
    glUseProgram(0);

    qbuf->numQuads = 0;
}

static void cached_font_set_name(CachedFont *font, const char* name)
{
    strncpy(font->name, name, ARRAY_COUNT(font->name)); 
}

GLuint opengl_load_texture(uint32_t width, uint32_t height, uint32_t numcomps, void *data)
{
    GLuint ret;
    glGenTextures(1, &ret);
    GLenum format;
    if(numcomps == 3)
        format = GL_RGB;
    if(numcomps == 4)
        format = GL_RGBA;
    else
    {
        // TODO: some centralized way of bailing out?
        fprintf(stderr, "comps other than 3 or 4 not implemented");
        abort();
    }
    glBindTexture(GL_TEXTURE_2D, ret);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return ret;
}

static void cached_font_initialize(CachedFont *font)
{
    cached_font_set_name(font, "Default");
    font->size = 0;
}

static void cached_font_unload(CachedFont *font)
{
}

static void font_manager_unload_font(FontManager* fmgr, CachedFont *font)
{
    cached_font_unload(font);

    bool found = false;
    const int count = fmgr->numCachedFonts;
    for(int i = 0; i < count; i++)
    {
        if(fmgr->cachedFonts[i] == font)
        {
            for(int j = i+1; i < count; i++)
            {
                fmgr->cachedFonts[j-1] = fmgr->cachedFonts[j];
            }
            fmgr->numCachedFonts--;
            found = true;
            break;
        }
    }
    assert(found); // tried to unload font that's not even loaded
    aike_free(font);
}


static CachedFont* font_manager_load_font(FontManager* fmgr, const char *path, uint32_t size)
{
    // TODO: font pool?
    CachedFont* font = (CachedFont*)aike_alloc(sizeof(CachedFont));
    cached_font_initialize(font);

    AutoAlloc ta1(1024*1024);
    AutoAlloc ta2(512*512);

    FILE *fp = fopen(path, "rb");
    if(fp == NULL)
    {
        fprintf(stderr, "Font file not found!\n");
        abort();
    }


    fread(ta1.ptr, 1, 1024*1024, fp);
    stbtt_pack_context pc;
    stbtt_PackBegin(&pc, (unsigned char*)ta2.ptr, 512, 512, 0, 1, NULL);
    stbtt_PackSetOversampling(&pc, 1, 1);
    stbtt_PackFontRange(&pc, (unsigned char*)ta1.ptr, 0, (float)size, 32, 95, font->charData+32);
    stbtt_PackEnd(&pc);

    // TODO: make sure it fits
    //stbtt_BakeFontBitmap((const unsigned char*)ta1.ptr, 0, (float)size, (unsigned char*)ta2.ptr, 512, 512, 32, 96, font->charData);

    uint32_t layer = renderer_get_atlas_layer(g_renderer);



    glBindTexture(GL_TEXTURE_2D_ARRAY, g_renderer->textureAtlasArray);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 512, 512, 1, GL_RED, GL_UNSIGNED_BYTE, ta2.ptr); 
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    font->atlasLayer = layer;
    printf("loaded layer %d\n", layer);
    // TODO: actual name from ttf
    cached_font_set_name(font, path);
    font->size = size;

    if(fp != NULL)
        fclose(fp);

    assert(fmgr->numCachedFonts < ARRAY_COUNT(fmgr->cachedFonts));
    fmgr->cachedFonts[fmgr->numCachedFonts++] = font;

    return font;
}

static void text_width(CachedFont *font, Vec2 position, const char *text)
{
    while(*text)
    {
        if(*text >= 32)
        {
            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(font->charData, 512, 512, *text, &position.x, &position.y, &q, 1);
        }
        text++;
    }
}

static void text_width_s(CachedFont *font, Vec2 position, const char *text, uint32_t charCount)
{
    for(int i = 0; i < charCount && *text; i++)
    {
        if(*text >= 32)
        {
            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(font->charData, 512, 512, *text, &position.x, &position.y, &q, 1);
        }
        text++;
    }
}

static int text_per_line(CachedFont *font, float width, const char *text, float *linewidth)
{
    float x = 0.0f;
    float y = 0.0f;
    int numChars = 0;
    float lastx = 0.0f;
    while(*text)
    {
        if(*text >= 32)
        {
            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(font->charData, 512, 512, *text, &x, &y, &q, 1);
            if(x > width)
            {
                *linewidth = lastx;
                return numChars == 0 ? 1 : numChars;
            }
            else
            {
                lastx = q.x1;
            }
            numChars++;
        }
        text++;
    }
    *linewidth = lastx;
    return numChars;
}

static void renderer_render_text(Renderer *renderer, CachedFont *font, Rect rect, const char *text, TextAlignment alignment)
{
    FontManager *fmgr = &renderer->fontManager;
    QuadVertex vertices[4];

    float minx = rect.x0;
    float y = rect.y0 + font->size;

    while(*text) 
    {
        float lineWidth;
        int characters = text_per_line(font, rect.width, text, &lineWidth);
        assert(characters > 0);
        float x = minx;
        if(alignment == TextAlignment_Center)
        {
            x += (rect.width-lineWidth) / 2.0f;
        }
        for(int i = 0; i < characters; i++)
        {
            if(*text >= 32)
            {
                stbtt_aligned_quad q;
                // TODO: align_to_integer?
                stbtt_GetPackedQuad(font->charData, 512, 512, *text, &x, &y, &q, 1);
                vertices[0] = {{q.x0, q.y0, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {q.s0, q.t0, (float)font->atlasLayer}};
                vertices[1] = {{q.x1, q.y0, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {q.s1, q.t0, (float)font->atlasLayer}};
                vertices[2] = {{q.x1, q.y1, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {q.s1, q.t1, (float)font->atlasLayer}};
                vertices[3] = {{q.x0, q.y1, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {q.s0, q.t1, (float)font->atlasLayer}};

                quad_buffer_draw_quad(&g_renderer->layerBuffers[g_renderer->currentLayer], vertices, 1);
            }
            text++;
        }
        y += font->size;
    }
}

static void renderer_draw_quad(Renderer *renderer, Rect rect, Vec4 color)
{
    QuadVertex vertices[4];
    vertices[0] = {{rect.x0, rect.y0, 0.0f}, {color.r, color.g, color.b, color.a}, {0.0f, 0.0f, 0.0f}};
    vertices[1] = {{rect.x0+rect.width, rect.y0, 0.0f}, {color.r, color.g, color.b, color.a}, {1.0f, 0.0f, 0.0f}};
    vertices[2] = {{rect.x0+rect.width, rect.y0+rect.height, 0.0f}, {color.r, color.g, color.b, color.a}, {1.0f, 1.0f, 0.0f}};
    vertices[3] = {{rect.x0, rect.y0+rect.height, 0.0f}, {color.r, color.g, color.b, color.a}, {0.0f, 1.0f, 0.0f}};
    assert(renderer->currentLayer < ARRAY_COUNT(renderer->layerBuffers));
    quad_buffer_draw_quad(&renderer->layerBuffers[renderer->currentLayer], vertices);
}

static void renderer_draw_texture(Renderer *renderer, Rect rect, GLuint tex)
{
    slow_quad_buffer_draw_quad(&renderer->slowLayerBuffers[renderer->currentLayer], rect, tex);
}


static void renderer_reset(Renderer *renderer)
{
    renderer->currentLayer = 0;
    for(int i = 0; i < ARRAY_COUNT(renderer->layerBuffers); i++)
    {
        renderer->layerBuffers[i].numQuads = 0;
    }
}

static void renderer_render(Renderer *renderer)
{
    int count = ARRAY_COUNT(renderer->layerBuffers);
    for(int i = 0; i < count; i++)
    {
        quad_buffer_render(&renderer->layerBuffers[i], renderer->mainProgram, renderer->textureAtlasArray);
        slow_quad_buffer_render(&renderer->slowLayerBuffers[i]);
    }
}
