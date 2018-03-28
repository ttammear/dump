//////////////////////////////////////////////////


void rview_buffer_clear(struct RenderViewBuffer *rbuf)
{
    rbuf->bottomPtr = ((u8*)rbuf) + rbuf->size;
    rbuf->view.space = NULL;
}

struct RenderViewBuffer* rview_buffer_init(void *memory, u32 size)
{
    struct RenderViewBuffer *ret = (struct RenderViewBuffer*)memory;
    ret->size = size;
    rview_buffer_clear(ret);
    ret->swapIndex = 0;
    return ret;
}

void* rview_buffer_destroy(struct RenderViewBuffer *rbuf)
{
    return (void*)rbuf;
}

#define RENDER_VIEW_BUFFER_ALIGNMENT_MASK 0xFF

void* rview_buffer_allocate_from(struct RenderViewBuffer *rbuf, u32 size)
{
    u8* ret = (rbuf->bottomPtr - size);
    // align pointer towards smaller value (memory is growing towards 0)
    ret = ret - ((uintptr)ret & (uintptr)RENDER_VIEW_BUFFER_ALIGNMENT_MASK);
    assert((rbuf->bottomPtr - size) >= ret); // returned memory overlaps current pointer

    if(ret < &rbuf->end[3]) // out of memory, would overlap with header
        return NULL;

    rbuf->bottomPtr = ret;
    return (void*)ret;
}

/////////////////////////////////////////////////////////

void rview_builder_init(struct RenderViewBuilder *builder)
{
    builder->meshBuf = NULL;
    builder->instanceBuf = NULL;
    builder->instanceDataBuf = NULL;
    builder->vertices = NULL;
    builder->indices = NULL;
    builder->batches = NULL;
    builder->indexBase = 0;
    builder->beginBatch = false;
}

void rview_builder_destroy(struct RenderViewBuilder *builder)
{
    buf_free(builder->meshBuf);
    buf_free(builder->instanceBuf);
    buf_free(builder->instanceDataBuf);
    buf_free(builder->vertices);
    buf_free(builder->indices);
}

void rview_builder_reset(struct RenderViewBuilder *builder)
{
    buf_clear(builder->meshBuf);
    buf_clear(builder->instanceBuf);
    buf_clear(builder->instanceDataBuf);
    buf_clear(builder->vertices);
    buf_clear(builder->indices);
    buf_clear(builder->batches);
    builder->indexBase = 0;
    builder->beginBatch = false;
}

int findEntry(struct RenderViewBuilder *builder, uint32_t meshId, uint32_t materialId)
{
    // TODO: NEED constant time lookup here
    int32_t count = buf_len(builder->meshBuf);
    for(int i = 0; i < count; i++)
    {
        if(builder->meshBuf[i].meshId == meshId && builder->meshBuf[i].materialId == materialId)
            return i;
    }
    return -1;
}

void builder_begin_batch(struct RenderViewBuilder *builder, uint32_t tex, uint32_t scX0, uint32_t scY0, uint32_t scX1, uint32_t scY1)
{
    assert(!builder->beginBatch);
    struct UIBatch *b = &builder->curBatch;  
    b->textureId = tex;
    b->scissorX0 = scX0;
    b->scissorY0 = scY0;
    b->scissorX1 = scX1;
    b->scissorY1 = scY1;
    b->indexStart = buf_len(builder->indices);
    builder->beginBatch = true;
}

void builder_end_batch(struct RenderViewBuilder *builder)
{
    assert(builder->beginBatch);
    builder->curBatch.indexCount = buf_len(builder->indices) - builder->curBatch.indexStart;
    if(builder->curBatch.indexCount > 0)
    {
        buf_push(builder->batches, builder->curBatch);
        //printf("new batch %d\n", builder->curBatch.indexCount);
    }
    builder->beginBatch = false;
}

void builder_add_batch(struct RenderViewBuilder *builder, uint32_t tex, uint32_t scX0, uint32_t scY0, uint32_t scX1, uint32_t scY1, uint32_t indexStart, uint32_t indexCount)
{
    assert(!builder->beginBatch);
    struct UIBatch b;  
    b.textureId = tex;
    b.scissorX0 = scX0;
    b.scissorY0 = scY0;
    b.scissorX1 = scX1;
    b.scissorY1 = scY1;
    b.indexStart = indexStart+builder->indexBase;
    b.indexCount = indexCount;
    buf_push(builder->batches, b);
}

void add_vertex(struct RenderViewBuilder *builder, struct UIVertex *vert)
{
    buf_push(builder->vertices, *vert);
}

void add_index(struct RenderViewBuilder *builder, uint16_t index)
{
    buf_push(builder->indices, index+builder->indexBase);
}

void add_vertices(struct RenderViewBuilder *builder, struct UIVertex *vertices, uint32_t count)
{
    uint32_t startIdx = buf_len(builder->vertices);
    buf_push_count(builder->vertices, count);
    memcpy(&builder->vertices[startIdx], vertices, count*sizeof(vertices[0]));
}

void builder_new_vertex_stream(struct RenderViewBuilder *builder)
{
    assert(!builder->beginBatch); // can't start new stream inside batch!
    builder->indexBase = buf_len(builder->vertices);
}

void add_mesh_instance(struct RenderViewBuilder *builder, uint32_t meshId, uint32_t materialId, struct Mat4 *modelM, void *instanceData, u32 instanceDataSize)
{
    int entryIdx = findEntry(builder, meshId, materialId);
    if(entryIdx < 0)
    {
        buf_push(builder->meshBuf, (struct BuilderMeshEntry){ 
                    .meshId = meshId,
                    .materialId = materialId
            });
        entryIdx = buf_len(builder->meshBuf)-1;
    }
    u32 dbufIdx = 0;
    if(instanceData != NULL)
    {
        dbufIdx = buf_push_count(builder->instanceDataBuf, ALIGN16(instanceDataSize));
        memcpy(&builder->instanceDataBuf[dbufIdx], instanceData, instanceDataSize);
    }
    buf_push(builder->instanceBuf, (struct BuilderMeshInstance) {
                .modelM = *modelM,
                .mentryIdx = entryIdx,
                .instanceDataIdx = dbufIdx,
                .instanceDataSize = instanceDataSize
        });
    builder->meshBuf[entryIdx].instanceCount++;
}


void build_view(struct RenderViewBuilder *builder, struct RenderViewBuffer *buf)
{
    PROF_BLOCK();
    struct RenderView *view = &buf->view;
    view->numSpaces = 1;
    view->numPostProcs = 0;

    u32 renderSpaceSize = sizeof(struct RenderSpace) + sizeof(void*) * buf_len(builder->meshBuf);

    rview_buffer_clear(buf);
    struct RenderSpace *space = rview_buffer_allocate_from(buf, renderSpaceSize);
    assert(space != NULL);
    view->space = space;
    
    int32_t count = buf_len(builder->meshBuf);
    space->numEntries = count;
    for(u32 i = 0; i < count; i++)
    {
        u32 instanceCount = builder->meshBuf[i].instanceCount;
        u32 renderMeshEntrySize = sizeof(struct RenderMeshEntry) 
            + sizeof(struct RenderMeshInstance)*instanceCount;

        struct RenderMeshEntry *entry = rview_buffer_allocate_from(buf, renderMeshEntrySize);
        assert(entry != NULL);
        entry->meshId = builder->meshBuf[i].meshId;
        entry->materialId = builder->meshBuf[i].materialId;
        entry->numInstances = 0;

        space->meshEntries[i] = entry;
    }

    count = buf_len(builder->instanceBuf);
    u32 matCount = ALIGN_UP(count, TT_SIMD_32_WIDTH);
    struct Mat4_sse2 *tmatBuf = rview_buffer_allocate_from(buf, sizeof(struct Mat4) * matCount);
    view->tmatrixBuf = tmatBuf;

    for(u32 i = 0; i < count; i++)
    {
        struct BuilderMeshInstance *binstance = &builder->instanceBuf[i];
        struct RenderMeshEntry *mentry = view->space->meshEntries[binstance->mentryIdx];
        struct RenderMeshInstance *rinstance = &mentry->instances[mentry->numInstances++];
        rinstance->matrixIndex = i;
        rinstance->instanceDataPtr = rview_buffer_allocate_from(buf, binstance->instanceDataSize);
        assert(rinstance->instanceDataPtr != NULL);
        memcpy(rinstance->instanceDataPtr, &builder->instanceDataBuf[binstance->instanceDataIdx], binstance->instanceDataSize);
        rinstance->instanceDataSize = binstance->instanceDataSize;
        // TODO: uniforms
    }

    PROF_START_STR("Matrix multiplication");

    mat4_load_sse2(&view->worldToClip, &builder->viewProjection, &builder->viewProjection, &builder->viewProjection, &builder->viewProjection);

    for(u32 i = 0; i < matCount/4; i++)
    {
        struct Mat4 *m1 = &builder->instanceBuf[i*4].modelM;
        struct Mat4 *m2 = &builder->instanceBuf[i*4+1].modelM;
        struct Mat4 *m3 = &builder->instanceBuf[i*4+2].modelM;
        struct Mat4 *m4 = &builder->instanceBuf[i*4+3].modelM;

        struct Mat4_sse2 modelMatrices;
        mat4_load_sse2(&modelMatrices, m1, m2, m3, m4);
        mat4_mul_sse2(&tmatBuf[i], &view->worldToClip, &modelMatrices);
    }
    PROF_END();

    uint32_t verticesSize = sizeof(builder->vertices[0]) * buf_len(builder->vertices);
    uint32_t indicesSize = sizeof(builder->indices[0]) * buf_len(builder->indices);
    if(verticesSize > 0 && indicesSize > 0)
    {
        void *vertBuf = rview_buffer_allocate_from(buf, verticesSize);
        void *idxBuf = rview_buffer_allocate_from(buf, indicesSize);
        assert(vertBuf != NULL && idxBuf != NULL);
        memcpy(vertBuf, builder->vertices, verticesSize);
        memcpy(idxBuf, builder->indices, indicesSize);
        view->vertices = (struct UIVertex*)vertBuf;
        view->numVertices = buf_len(builder->vertices);
        view->indices = (uint16_t*)idxBuf;
        view->numIndices = buf_len(builder->indices);
    }
    view->materialId = builder->materialId;
    view->orthoMatrix = builder->orthoMatrix;

    view->numUIBatches = buf_len(builder->batches);
    if(view->numUIBatches > 0)
    {
        uint32_t bsize = sizeof(struct UIBatch)*view->numUIBatches;
        view->uiBatches = rview_buffer_allocate_from(buf, bsize);
        memcpy(view->uiBatches, builder->batches, bsize);
    }

#ifdef _DEBUG
    // NOTE: just a sanity check, functionally this does nothing!
    count = buf_len(builder->meshBuf);
    for(u32 i = 0; i < count; i++)
    {
        assert(view->space->meshEntries[i]->numInstances == builder->meshBuf[i].instanceCount);
    }
#endif
}

void swap_buffer_init(struct SwapBuffer *sb)
{
    sb->viewBuffersTaken = 1;
    for(u32 i = 0; i < 3; i++)
    {
        // TODO: not cool
        u32 size = 10 * 1024 * 1024;
        sb->viewBuffers[i] = rview_buffer_init(malloc(size), size);
    }
    sb->freeViewBuffer = sb->viewBuffers[0];
    sb->numSwaps = 0;
}

void swap_buffer_destroy(struct SwapBuffer *sb)
{
    for(u32 i = 0; i < 3; i++)
    {
        free(rview_buffer_destroy(sb->viewBuffers[i]));
        sb->viewBuffers[i] = NULL;
    }
}

struct RenderViewBuffer* take_view_buffer(struct SwapBuffer *buf)
{
    // TODO: atomic increment buf->viewsTaken
    i32 result = __sync_fetch_and_add(&buf->viewBuffersTaken, 1);
    if(result > 2)
    {
        tt_render_fatal("Only 2 view buffers can be aquired: 1 by renderer and 1 by rendering side, the 3rd is reserved for swapping.");
    }
    return buf->viewBuffers[result];
}

// generally the producer aka the game should use this function
struct RenderViewBuffer* swap_view_for_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf)
{
    while(true)
    {
        __sync_synchronize();
        struct RenderViewBuffer *oldVal = sbuf->freeViewBuffer;
        vbuf->swapIndex = sbuf->numSwaps;
        b32 suc = __sync_bool_compare_and_swap(&sbuf->freeViewBuffer, oldVal, vbuf);
        if(suc)
        {
            __sync_fetch_and_add(&sbuf->numSwaps, 1);
            return oldVal;
        }
        else // TODO REMOVE
            printf("fetch failed\n");
    }
}

// generally the consumer aka the renderer should use this function
struct RenderViewBuffer* swap_view_if_newer(struct SwapBuffer *sbuf, struct RenderViewBuffer *vbuf)
{
    while(true)
    {
        __sync_synchronize();
        struct RenderViewBuffer *oldVal = sbuf->freeViewBuffer;
        // TODO: theoretically 4 billion swaps would wrap, but that's 2.269875326 years at 60fps
        if(oldVal->swapIndex < vbuf->swapIndex) // game has not updated view, we already have the latest view
        {
            return vbuf;
        }

        // take the view or try again if we failed
        b32 suc = __sync_bool_compare_and_swap(&sbuf->freeViewBuffer, oldVal, vbuf);
        if(suc)
        {
            __sync_fetch_and_add(&sbuf->numSwaps, 1);
            return oldVal;
        }
        else // TODO REMOVE
            printf("fetch failed\n");
    }
}

