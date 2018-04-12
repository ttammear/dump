
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
    arena->memory = arena_push_size(fromArena, size);
    arena->size = size;
    arena->pos = arena->memory;
    arena->block = NULL;
}


void fixed_arena_free(AikePlatform *platform, struct TessFixedArena *arena)
{
    if(arena->block)
        platform->free_memory(platform, arena->block);
}
