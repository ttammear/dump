#include <stdint.h>

#ifndef CONTAINERS_HEADER
#define CONTAINERS_HEADER
struct StructArray
{
    uint32_t elementSize;
    uint32_t capacity;
    uint32_t count;

    uint8_t *data;
};

struct StructPool
{
    // element size in bytes
    uint32_t elementSize;
    // maximum number of elements
    uint32_t capacity;
    // current count of allocated elements
    uint32_t count;
    // count of elements available for allocation (same as capacity if struct_pool_mark_all_free was called on startup)
    uint32_t initialized;
    // bit map where each bit represents an element being allocated or not
    uint8_t *allocMap;
    // pointer to allocated data
    uint8_t *data;
};


void struct_array_init(struct StructArray *arr, uint32_t elementSize, uint32_t capacity);
void* struct_array_add(struct StructArray *arr);
void struct_array_remove(struct StructArray *arr, uint32_t index);
void* struct_array_get(struct StructArray *arr, uint32_t index);


// STRUCT POOL (pointers to elements remain valid)

// initialize pool (all elements will be allocated, call struct_pool_free_all if you only use the structs for storage!)
void struct_pool_init(struct StructPool *pool, uint32_t elementSize, uint32_t capacity, bool freeByDefault);
// allocate a struct and return pointer to it
void* struct_pool_alloc(struct StructPool *pool);
// return struct to the pool
void struct_pool_return(struct StructPool *pool, void *instance);
// free all (internal) memory, struct pool will no longer be valid to use
void struct_pool_free_resources(struct StructPool *pool);
// mark all elements as free
void struct_pool_mark_all_free(struct StructPool *pool);
// add struct to pool (make it available for allocation)
void* struct_pool_push_struct(struct StructPool *pool);

#endif

#ifdef CONTAINERS_IMPLEMENTATION

void struct_array_init(StructArray *arr, uint32_t elementSize, uint32_t capacity)
{
    arr->elementSize = elementSize;
    arr->capacity = capacity;
    arr->count = 0;

    uint32_t storeSize = capacity * elementSize;
    arr->data = (uint8_t*)aike_alloc(storeSize);
}

void struct_array_free_resources(StructArray *arr)
{
    aike_free(arr->data);
}

void* struct_array_add(StructArray *arr)
{
    if(arr->count >= arr->capacity)
    {
        assert(false); // TODO: implement resize
    }
    int index = arr->count++;
    return (void*)(arr->data + (arr->elementSize * index));
}

void struct_array_remove(StructArray *arr, uint32_t index)
{
    assert(index < arr->count);
    if(index < arr->count-1)
    {
        uint32_t esize = arr->elementSize;
        uint8_t *moveBegin = arr->data + (index+1)*esize;
        uint32_t moveSize = (arr->count - index - 1)*esize;
        memmove((void*)(moveBegin - esize), moveBegin, moveSize);
    }
    arr->count--;
}

void* struct_array_get(StructArray *arr, uint32_t index)
{
    assert(index < arr->count);
    return (void*)(arr->data + index * arr->elementSize);
}

///////////////////// STRUCT POOL //////////////////////////

void struct_pool_init(struct StructPool *pool, uint32_t elementSize, uint32_t capacity, bool freeByDefault)
{
    pool->elementSize = elementSize;
    pool->capacity = capacity;
    pool->count = 0;
    pool->initialized = freeByDefault ? capacity : 0;

    uint32_t allocMapS = (capacity + 7) / 8;
    uint32_t storeS = elementSize * capacity;
    uint8_t *data = (uint8_t*)aike_alloc(allocMapS + storeS);
    pool->allocMap = data;
    pool->data = data + allocMapS;

    memset(pool->allocMap, freeByDefault ? 0 : 0xFF, allocMapS); 
}

// TODO: use freelist maybe?
void* struct_pool_alloc(struct StructPool *pool)
{
    void *ret = NULL;

    uint32_t byteCount = (pool->initialized+7) / 8;
    bool found = false;

    for(int i = 0; i < byteCount; i++)
    {
        // TODO: 32-bit or even SIMD comparison?
        if(pool->allocMap[i] != 0xFF)
        {
            uint8_t mapbyte = pool->allocMap[i];
            uint32_t baseIndex = i<<3;
            for(int j = 0; j < 8; j++)
            {
                if(baseIndex + j >= pool->initialized)
                    goto return_pool_element; // return NULL
                if((mapbyte & (1 << j)) == 0)
                {
                    found = true;
                    pool->allocMap[i] |= 1 << j;
                    ret = (void*) (pool->data + ((i<<3) + j) * pool->elementSize);
                    goto return_pool_element;
                }
            }
        }
    }

return_pool_element:
    pool->count++;
    return ret;
}

void struct_pool_return(struct StructPool *pool, void *instance)
{
    assert((uint8_t*)instance >= pool->data);
    int index = ((uintptr_t)instance - (uintptr_t)pool->data) / pool->elementSize;
    assert(index < pool->capacity);
    int byte = index >> 3; // index / 8
    uint8_t bit = index & 0xF; // index % 8
    pool->allocMap[byte] &= ~(1 << bit);
    pool->count--;
}

void struct_pool_free_resources(struct StructPool *pool)
{
    aike_free(pool->allocMap);
}

void struct_pool_mark_all_free(struct StructPool *pool)
{
    uint32_t byteCount = (pool->capacity+7) / 8;
    for(int i = 0; i < byteCount; i++)
    {
        pool->allocMap[i] = 0;
    }
    pool->initialized = pool->capacity;
    pool->count = 0;
}

void* struct_pool_push_struct(struct StructPool *pool)
{
    // TODO: resize?
    assert(pool->initialized < pool->capacity); // all elements already available
    void *ret = (void*)(pool->data + pool->elementSize * pool->initialized);
    pool->initialized++;
    return ret;
}

#endif
