//////////////////////////////////////////////////


void rview_buffer_clear(struct RenderViewBuffer *rbuf)
{
    rbuf->bottomPtr = ((u8*)rbuf) + rbuf->size;
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
}

void rview_builder_destroy(struct RenderViewBuilder *builder)
{
    buf_free(builder->meshBuf);
    buf_free(builder->instanceBuf);
    buf_free(builder->instanceDataBuf);
}

void rview_builder_reset(struct RenderViewBuilder *builder)
{
    buf_clear(builder->meshBuf);
    buf_clear(builder->instanceBuf);
    buf_clear(builder->instanceDataBuf);
}

int findEntry(struct RenderViewBuilder *builder, struct Mesh *mesh, struct Material *material)
{
    // TODO: NEED constant time lookup here
    int32_t count = buf_len(builder->meshBuf);
    for(int i = 0; i < count; i++)
    {
        if(builder->meshBuf[i].mesh == mesh && builder->meshBuf[i].material == material)
            return i;
    }
    return -1;
}

void add_mesh_instance(struct RenderViewBuilder *builder, struct Mesh *mesh, struct Material *material, struct Mat4 *modelM, void *instanceData, u32 instanceDataSize)
{
    int entryIdx = findEntry(builder, mesh, material);
    if(entryIdx < 0)
    {
        buf_push(builder->meshBuf, (struct BuilderMeshEntry){ 
                    .mesh = mesh,
                    .material = material
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
    PROF_START();
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
        entry->mesh = builder->meshBuf[i].mesh;
        entry->material = builder->meshBuf[i].material;
        entry->numInstances = 0;

        space->meshEntries[i] = entry;
    }

    count = buf_len(builder->instanceBuf);
    for(u32 i = 0; i < count; i++)
    {
        struct BuilderMeshInstance *binstance = &builder->instanceBuf[i];
        struct RenderMeshEntry *mentry = view->space->meshEntries[binstance->mentryIdx];
        struct RenderMeshInstance *rinstance = &mentry->instances[mentry->numInstances++];
        rinstance->modelM = binstance->modelM;
        rinstance->instanceDataPtr = rview_buffer_allocate_from(buf, binstance->instanceDataSize);
        assert(rinstance->instanceDataPtr != NULL);
        memcpy(rinstance->instanceDataPtr, &builder->instanceDataBuf[binstance->instanceDataIdx], binstance->instanceDataSize);
        rinstance->instanceDataSize = binstance->instanceDataSize;
        // TODO: uniforms
    }

#ifdef _DEBUG
    // NOTE: just a sanity check, functionally this does nothing!
    count = buf_len(builder->meshBuf);
    for(u32 i = 0; i < count; i++)
    {
        assert(view->space->meshEntries[i]->numInstances == builder->meshBuf[i].instanceCount);
    }
#endif
    PROF_END();
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

