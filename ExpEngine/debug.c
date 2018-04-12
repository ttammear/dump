
aike_thread_local struct ProfilerState *t_profState;
struct ProfilerState *g_profStates[10];
atomic_uint g_profCount;

void debug_init(const char *name)
{
    struct ProfilerState *newProfState = aligned_alloc(_Alignof(struct ProfilerState), sizeof(struct ProfilerState));
    memset(newProfState, 0, sizeof(struct ProfilerState));
    assert(newProfState != NULL);
    newProfState->name = name;

    uint32_t entry = atomic_fetch_add(&g_profCount, 1);
    assert(entry >= 0 && entry < ARRAY_COUNT(g_profStates));
    g_profStates[entry] = newProfState;

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
        // TODO: this should be atomic, but theres almost no chance that
        // we need to care, and its for debug anyway...
        if(g_profStates[i] == t_profState)
            g_profStates[i] = NULL;
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
