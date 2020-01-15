typedef struct TessFixedArena
{
    AikeMemoryBlock *block;
    uint8_t *memory;
    uint32_t size;
    uint8_t *pos;
} TessFixedArena;

typedef struct TessMemoryBlock
{
    AikeMemoryBlock *block;
    struct TessMemoryBlock *next;
    size_t size;
    uint8_t *memory;
    uint8_t *pos;
} TessMemoryBlock;

typedef struct TessArena
{
    TessMemoryBlock *first;
    size_t blockSize;
    AikePlatform *platform;
    // just for statistics
    size_t used;
    size_t allocated;
} TessArena;

typedef struct TessStack
{
    uint8_t *start;
    uint8_t *cur;
    size_t size;
} TessStack;

// allocates memory from platform
void fixed_arena_init(AikePlatform *platform, TessFixedArena *arena, size_t size);
// init with memory from another arena
void fixed_arena_init_from_arena(TessFixedArena *newarena, TessFixedArena *parentArena, size_t size);
void fixed_arena_free(AikePlatform *platform, TessFixedArena *arena);

static inline void* fixed_arena_push_size(TessFixedArena *arena, size_t size, uint32_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    uint8_t* mem = ALIGN_UP_PTR(arena->pos, alignment);
    arena->pos += (mem - arena->pos) + size;
    assert(arena->pos <= arena->memory + arena->size);
    return mem;
}

void arena_init(TessArena *arena, AikePlatform *platform, size_t blockSize);
void arena_free(TessArena *arena);
void* d_arena_push_size(TessArena *arena, size_t size, uint32_t alignment);

#define arena_push_size(arena, size, alignment) _Generic((arena), TessFixedArena*: fixed_arena_push_size, TessArena*: d_arena_push_size)((arena), (size), alignment)
#define arena_push_struct(arena, type) arena_push_size(arena, sizeof(type), _Alignof(type))


static inline void stack_init(TessStack *stack, void *mem, size_t size)
{
    stack->start = (uint8_t*)mem;
    stack->cur = stack->start;
    stack->size = size;
}

static inline void* stack_push(TessStack *stack, size_t size, uint32_t alignment)
{
    uint8_t *aligned = ALIGN_UP_PTR(stack->cur, alignment);
    uint8_t **ptr = (uint8_t**)(stack->cur + (aligned - stack->cur) + size);
    *ptr = stack->cur;
    stack->cur = ((uint8_t*)ptr) + sizeof(uint8_t**);
    assert(stack->cur >= stack->start && stack->cur < stack->start + stack->size);
    return aligned;
}

static inline void stack_pop(TessStack *stack)
{
    uint8_t **popPtr = (uint8_t**)(stack->cur - sizeof(uint8_t**));
    stack->cur = *popPtr;
    assert(stack->cur >= stack->start && stack->cur < stack->start + stack->size);
}
