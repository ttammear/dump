
void fixed_arena_init(AikePlatform *platform, struct TessFixedArena *arena, size_t size)
{
    AikeMemoryBlock *block = platform->allocate_memory(platform, size, 0);
    arena->memory = (uint8_t*)block->memory;
    arena->size = size;
    arena->pos = arena->memory;
    arena->block = block;
}

void fixed_arena_init_from_arena(struct TessFixedArena *arena, struct TessFixedArena *fromArena, size_t size)
{
    arena->memory = fixed_arena_push_size(fromArena, size, 8);
    arena->size = size;
    arena->pos = arena->memory;
    arena->block = NULL;
}

void fixed_arena_free(AikePlatform *platform, struct TessFixedArena *arena)
{
    if(arena->block)
        platform->free_memory(platform, arena->block);
}

static void arena_new_block(TessArena *arena)
{
    AikeMemoryBlock *block = arena->platform->allocate_memory(arena->platform, arena->blockSize, 0);
    TessMemoryBlock *tblock = (TessMemoryBlock*)block->memory;
    uint8_t *memory = ALIGN_UP_PTR(((uint8_t*)tblock) + sizeof(TessMemoryBlock), 64);
    assert(memory < block->memory + block->size + 0xFF);
    tblock->block = block;
    tblock->size = block->size - (memory - (uint8_t*)block);
    tblock->memory = memory;
    tblock->pos = tblock->memory;
    tblock->next = arena->first;
    arena->first = tblock;
    arena->allocated += arena->blockSize;
    arena->used += memory - block->memory;
}

void* d_arena_push_size(TessArena *arena, size_t size, uint32_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    TessMemoryBlock *block = arena->first;
    uint8_t *mem = ALIGN_UP_PTR(block->pos, alignment);
    assert(size < arena->blockSize>>2); // allocation should always be smaller than 1/4 block size, if this condition is not met, you should increase block size
    if(mem + size > block->memory + block->size)
    {
        arena_new_block(arena);
        return d_arena_push_size(arena, size, alignment);
    }
    arena->used += mem + size - block->pos;
    block->pos = mem + size;
    return mem;
}

void arena_init(TessArena *arena, AikePlatform *platform, size_t blockSize)
{
    arena->first = NULL;
    arena->blockSize = blockSize;
    arena->platform = platform;
    arena->used = 0;
    arena->allocated = 0;
    arena_new_block(arena);
}

void arena_free(TessArena *arena)
{
    TessMemoryBlock *block = arena->first;
    while(block != NULL)
    {
        TessMemoryBlock *next = block->next;
        arena->platform->free_memory(arena->platform, block->block);
        block = next;
    }
    arena->first = NULL;
    arena->allocated = 0;
    arena->used = 0;
}
