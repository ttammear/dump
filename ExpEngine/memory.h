
struct TessFixedArena
{
    AikeMemoryBlock *block;
    uint8_t *memory;
    uint32_t size;
    uint8_t *pos;
};

// allocates memory from platform
void fixed_arena_init(AikePlatform *platform, struct TessFixedArena *arena, size_t size);
// init with memory from another arena
void fixed_arena_init_from_arena(struct TessFixedArena *newarena, struct TessFixedArena *parentArena, size_t size);
void fixed_arena_free(AikePlatform *platform, struct TessFixedArena *arena);

static inline void* arena_push_size(struct TessFixedArena *arena, size_t size)
{
    uint8_t* mem = ALIGN_UP_PTR(arena->pos, 32);
    arena->pos += (mem - arena->pos) + size;
    assert(arena->pos <= arena->memory + arena->size);
    return mem;
}

#define arena_push_struct(arena, type) arena_push_size((arena), sizeof(type))
