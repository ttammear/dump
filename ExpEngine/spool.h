typedef struct SPoolHdr {
    size_t len;
    size_t cap;
    size_t elemSize;
    void *freeHead;
    size_t listLen;
    size_t listCap;
    char buf[];
} SPoolHdr;

#define spool__hdr(x) ((SPoolHdr *)((char *)(x) - offsetof(SPoolHdr, buf)))

#define spool__list_len(x) ((x) ? spool__hdr(x)->listLen : 0)
#define spool__list_cap(x) ((x) ?  spool__hdr(x)->listCap : 0)
#define spool__list_fits(x, n) (spool__list_len(x) + (n) <= spool__list_cap(x))
#define spool__list_fit(x, n, esize) (spool__list_fits((x), (n)) ? 0 : ((x) = (__typeof__(x))spool__list_grow((void**)(x), spool__list_len(x) + (n), esize)))
#define spool__list_push(x, ...) (spool__list_fit((x), 1, spool__hdr(x)->elemSize), (x)[spool__hdr(x)->listLen++] = (__VA_ARGS__))

#define spool_cap(x) ((x) ? spool__hdr(x)->cap : 0)
#define spool_len(x) ((x) ? spool__hdr(x)->len : 0)
#define spool_allocate(x) ((__typeof(*(x)))spool__allocate((void***)&(x)))
#define spool_free(x, ptr) (*((void**)(ptr)) = spool__hdr(x)->freeHead, spool__hdr(x)->freeHead = ptr)

void** spool__list_grow(void **buf, size_t new_len, size_t poolElemSize)
{
    size_t new_cap = MAX(2*spool__list_cap(buf), new_len);
    new_cap = MAX(new_cap, 16);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(SPoolHdr, buf) + new_cap*sizeof(void*);
    SPoolHdr *new_hdr;
    if(buf) {
        new_hdr = realloc(spool__hdr(buf), new_size);
    } else {
        new_hdr = malloc(new_size);
        new_hdr->listLen = 0;
        new_hdr->len = 0;
        new_hdr->cap = 0;
        new_hdr->freeHead = NULL;
        assert(poolElemSize >= sizeof(void*));
        new_hdr->elemSize = poolElemSize;
    }
    assert(new_hdr);
    new_hdr->listCap = new_cap;
    return (void**)new_hdr->buf;
}

void* spool__allocate(void ***spool)
{
    SPoolHdr *hdr = spool__hdr(*spool);
    void *ret = hdr->freeHead;
    if(__builtin_expect(ret == NULL, 0))
    {
        uint32_t allocCount = MAX(16, 2*hdr->cap);
        uint8_t *mem = malloc(allocCount * hdr->elemSize);
        hdr->freeHead = mem;
        ret = mem;
        for(int i = 0; i < allocCount; i++)
            spool__list_push(*spool, (mem + i*hdr->elemSize));
        hdr = spool__hdr(*spool);
        for(int i = 0; i < allocCount - 1; i++)
        {
            *((void**)mem) = mem + hdr->elemSize;
            mem += hdr->elemSize;
        }
        *((void**)mem) = NULL;
        hdr->cap += allocCount;
    }
    hdr->freeHead = *((void**)ret);
    hdr->len++;
    return ret;
}
