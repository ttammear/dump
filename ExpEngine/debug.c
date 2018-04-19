
aike_thread_local struct ProfilerState *t_profState;
atomic_uintptr_t g_profStates[10];

void debug_init(const char *name)
{
    struct ProfilerState *newProfState = aligned_alloc(_Alignof(struct ProfilerState), sizeof(struct ProfilerState));
    memset(newProfState, 0, sizeof(struct ProfilerState));
    assert(newProfState != NULL);

    strncpy(newProfState->name, name, ARRAY_COUNT(newProfState->name)-1);
    newProfState->name[ARRAY_COUNT(newProfState->name)-1] = 0;

    _Bool success = false;
    while(!success)
    {
        uint32_t freeIdx = -1;
        for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
        {
            if(atomic_load(&g_profStates[i]) == (uintptr_t)NULL)
            {
                freeIdx = i;
                break;
            }
        }
        assert(freeIdx != -1); // out of profile entries!
        uintptr_t nullPtr = (uintptr_t)NULL;
        success = atomic_compare_exchange_strong(&g_profStates[freeIdx], &nullPtr, (uintptr_t)newProfState);
    }

    t_profState = newProfState;
}

void entry_recursive(struct ProfileEntry *entry, int depth)
{
    while(entry != NULL)
    {
        uint64_t cycles = entry->sum;
        double milliseconds = (double)cycles / 2800000.0;
        for(int i = 0; i < depth; i++)
            printf(" "); 
        printf("%s %lucycles %fms\n", entry->locationStr, cycles, milliseconds);
        entry_recursive(entry->firstChild, depth+1);
        entry = entry->nextSibling;
    }
}

void debug_frame_report()
{
    entry_recursive(t_profState->root->firstChild, 0);
}

void debug_destroy()
{
    for(int i = 0; i < ARRAY_COUNT(g_profStates); i++)
    {
        if(atomic_load(&g_profStates[i]) == (uintptr_t)t_profState)
            atomic_exchange(&g_profStates[i], (uintptr_t)NULL);
    }
    // technically someone could still be reading it, but again, we dont care
    free(t_profState);
}

void debug_start_frame()
{
    for(int i = 0; i < PROFILER_MAX_ENTRIES_PER_FRAME; i++)
    {
        struct ProfileEntry *ent = &t_profState->entries[t_profState->curFrame][i];
        ent->sum = 0;
        ent->init = false;
        ent->firstChild = NULL;
        ent->nextSibling = NULL;
    }
    t_profState->freeEntryId = 0;

    struct ProfileEntry *root = PROF__GET_ENTRY_GUID();
    root->firstChild = NULL;
    root->nextSibling = NULL;
    root->locationStr = "Root";
    root->init = true;
    t_profState->openEntries[0].entry = root;
    t_profState->openEntries[0].lastChild = NULL;
    t_profState->curSEntry = 1;

    if(!atomic_load(&t_profState->pause))
    {
        atomic_exchange(&t_profState->prev, (uintptr_t)t_profState->root);
    }
    t_profState->root = root;
}

void debug_end_frame()
{
    if(!atomic_load(&t_profState->pause))
    {
        t_profState->curFrame = (t_profState->curFrame+1) % PROFILER_FRAME_HISTORY;
        assert(t_profState->curSEntry == 1); // more PROF_START than PROF_END
        assert(t_profState->curFrame < PROFILER_FRAME_HISTORY);
    }
}
