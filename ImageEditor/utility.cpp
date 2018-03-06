struct AEFreeListEntry
{
    struct AEFreeListEntry* next;
    uint32_t index;
};

struct AllocationEntry
{
    uint32_t uuid;
    void *memory;
    const char *description;
    const char *fileName;
    int lineNumber;
    AEFreeListEntry flEntry;
    bool isAlloc;
    uint32_t count;
};

struct AllocatedListEntry
{
    void *ptr;
    AllocationEntry *statEntry;
    struct AllocatedListEntry *next;
    struct AllocatedListEntry* prev;
};

static void aike_fatal_error(const char* error)
{
    fprintf(stderr, "Fatal error: %s\n", error);
    __builtin_trap();
}

void* memory_manager_temp_alloc(MemoryManager *memMgr, uint32_t size)
{
    if(size == 0)
        aike_fatal_error("Zero size allocations not allowed!");
    uint8_t *ret = memMgr->stackPointer - size;
    if(ret-4 < memMgr->stackStart)
    {
        aike_fatal_error("Temporary memory overflow");
        return (void*)0;
    }
    else
    {
        memMgr->stackPointer -= size + 4;
        uint32_t *popAmount = (uint32_t*)memMgr->stackPointer;
        *popAmount = size + 4;
    }
    return (void*)ret;
}

void memory_manager_temp_pop(MemoryManager *memMgr)
{
    uint32_t popAmount = *((uint32_t*)memMgr->stackPointer);
    uint8_t *posAfterPop = memMgr->stackPointer + popAmount;
    if(popAmount == 0 || posAfterPop > memMgr->stackStart + memMgr->stackSize)
    {
        aike_fatal_error("Temporary memory underflow. Too many pops or corrupted stack!");
    }
    else
    {
        memMgr->stackPointer += popAmount;
    }
}

void* temp_alloc(uint32_t size)
{
    return memory_manager_temp_alloc(g_memoryManager, size);
}

void temp_pop()
{
    memory_manager_temp_pop(g_memoryManager);
}

MemoryManager* memory_manager_initialize()
{
    MemoryManager *memMgr = (MemoryManager*)malloc(sizeof(MemoryManager));

    const int initialSize = 1000;

    memMgr->num_current_allocations = 0;
    //memMgr->entries = new AllocationEntry[1000]
    memMgr->entries = (AllocationEntry*)malloc(initialSize*sizeof(AllocationEntry));
    if(memMgr->entries == NULL)
    {
        aike_fatal_error("Out of memory");
    }
    memMgr->capacity = initialSize;
    memMgr->maxIndex = 0;

    for(int i = 0; i < initialSize-1; i++)
    {
        memMgr->entries[i].flEntry.next = &memMgr->entries[i+1].flEntry;
        memMgr->entries[i].flEntry.index = i;
    }
    memMgr->entries[initialSize-1].flEntry.next = NULL;
    memMgr->entries[initialSize-1].flEntry.index = initialSize-1;
    memMgr->freeListHead = &memMgr->entries[0].flEntry;
    memMgr->allocationListHead = NULL;

    // stack allocator
    uint32_t stackSize = 4*1024*1024; // 4MB
    memMgr->stackStart = (uint8_t*)malloc(stackSize);
    memMgr->stackPointer = memMgr->stackStart+stackSize;
    memMgr->stackSize = stackSize;

    return memMgr;
}

void memory_manager_free_resources(MemoryManager *memMgr)
{
    free(memMgr->entries);
    memMgr->entries = NULL;
    free(memMgr);
}

void memory_manager_grow(MemoryManager *memMgr)
{
    uint32_t oldSize = memMgr->capacity;
    uint32_t newSize = oldSize*2;
    void* result = realloc(memMgr->entries, newSize*sizeof(AllocationEntry));
    if(result == NULL)
        aike_fatal_error("Out of memory");

    int start = oldSize;
    int end = newSize-1;
    for(int i = start; i < end-1; i++)
    {   
        memMgr->entries[i].flEntry.next = &memMgr->entries[i+1].flEntry;
        memMgr->entries[i].flEntry.index = i;
    }

    memMgr->entries[end-1].flEntry.next = memMgr->freeListHead;
    memMgr->entries[end-1].flEntry.index = end-1;
    memMgr->freeListHead = &memMgr->entries[start].flEntry;
    memMgr->capacity = newSize;
}

int memory_manager_find_entry(MemoryManager *memMgr, uint32_t uuid)
{
    // TODO: binary search or something smarter
    for(int i = 0; i <= memMgr->maxIndex; i++)
    {
        if(memMgr->entries[i].isAlloc && memMgr->entries[i].uuid == uuid)
            return i;
    }
    return -1;
}

void memory_manager_remove_entry(MemoryManager *memMgr, void *ptr)
{
    AllocatedListEntry *entry = memMgr->allocationListHead;
    while(entry != NULL && entry->ptr != ptr)
    {
        entry = entry->next;        
    }
    if(entry != NULL) // found
    {
        entry->statEntry->count--;
        if(entry->prev != NULL)
            entry->prev->next = entry->next;
        if(entry->next != NULL)
            entry->next->prev = entry->prev;
        if(entry == memMgr->allocationListHead)
        {
            assert(entry->prev == NULL);
            memMgr->allocationListHead = entry->next;
        }
        delete entry;
    }
    else
    {
        aike_fatal_error("Trying to free unallocated resource");
    }
}

void memory_manager_add_entry(MemoryManager *memMgr, void *ptr, uint32_t uuid, const char* desc, const char* file, int lineNr)
{
    // TODO: this MUST be thread safe
    // probably need a lock for reallocation
    if(memMgr->freeListHead == NULL)
    {
        memory_manager_grow(memMgr);
    }
    // TODO: atomic compare swap / increment??
    int index = memory_manager_find_entry(memMgr, uuid);
    if(index == -1)
    {
        index = memMgr->freeListHead->index;
        memMgr->freeListHead = memMgr->freeListHead->next;
        if(index > memMgr->maxIndex)
            memMgr->maxIndex = index;
        AllocationEntry *entry = &memMgr->entries[index];
        entry->memory = ptr;
        entry->description = desc;
        entry->fileName = file;
        entry->lineNumber = lineNr;
        entry->uuid = uuid;
        entry->isAlloc = true;
        entry->count = 0;
    }

    AllocatedListEntry *allocEntry = new AllocatedListEntry;
    allocEntry->next = memMgr->allocationListHead;
    allocEntry->prev = NULL;
    allocEntry->statEntry = &memMgr->entries[index];
    allocEntry->ptr = ptr;
    if(memMgr->allocationListHead != NULL)
        memMgr->allocationListHead->prev = allocEntry;
    memMgr->allocationListHead = allocEntry;

    memMgr->entries[index].count++;
}

uint32_t debug_track_resource(uint32_t uuid, const char *description, const char *fileName, int lineNumber)
{
    memory_manager_add_entry(g_memoryManager, (void*)(uint64_t)uuid, uuid, description, fileName, lineNumber); 
    return uuid;
}

void debug_untrack_resource(uint32_t uuid)
{
    memory_manager_remove_entry(g_memoryManager, (void*)(uint64_t)uuid);
}

void* debug_alloc(size_t size, uint32_t uuid, const char *fileName, int lineNumber)
{
    void *ret = malloc(size);
    memory_manager_add_entry(g_memoryManager, ret, uuid, "Allocation", fileName, lineNumber);
    return ret;
}

void debug_free(void *ptr)
{
    free(ptr);
    memory_manager_remove_entry(g_memoryManager, ptr);
}

void memory_manager_print_entries(MemoryManager *memMgr, bool breakIfAnyEntries)
{
    printf("\n##### RESOURCE REPORT START #####\n");
    int max = memMgr->maxIndex;
    int count = 0;
    for(int i = 0; i <= max; i++)
    {
        if(memMgr->entries[i].isAlloc && memMgr->entries[i].count > 0)
        {
            printf("%d resources from %s:%d Description: %s\n", memMgr->entries[i].count, memMgr->entries[i].fileName, memMgr->entries[i].lineNumber, memMgr->entries[i].description);
            count++;
        }
        if(i == max && count == 0)
        {
            printf("No tangling resources!\n");
        }
    }
    printf("##### RESOURCE REPORT END #####\n\n");
    if(count > 0 && breakIfAnyEntries)
        aike_fatal_error("Not all aquired resources were freed!");
}

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}
