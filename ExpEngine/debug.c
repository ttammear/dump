
struct ProfilerState *g_profState;

void debug_init()
{
    g_profState = malloc(sizeof(struct ProfilerState));
    assert(g_profState != NULL);
    g_profState->curFrame = 0;
    g_profState->freeEntryId = 0;
    g_profState->curSEntry = 0;
}

void entry_recursive(struct ProfileEntry *entry, int depth)
{
    while(entry != NULL)
    {
        uint64_t cycles = entry->sum;
        double milliseconds = (double)cycles / 2800000.0;
        /*for(int i = 0; i < depth; i++)
            printf(" "); 
        printf("%s %lucycles %fms\n", entry->locationStr, cycles, milliseconds);*/
        entry_recursive(entry->firstChild, depth+1);
        entry = entry->nextSibling;
    }
}

void debug_frame_report()
{
    entry_recursive(g_profState->root->firstChild, 0);
}

void debug_destroy()
{
    free(g_profState);
}

void debug_start_frame()
{
    for(int i = 0; i < PROFILER_MAX_ENTRIES_PER_FRAME; i++)
    {
        struct ProfileEntry *ent = &g_profState->entries[g_profState->curFrame][i];
        ent->sum = 0;
        ent->init = false;
        ent->firstChild = NULL;
        ent->nextSibling = NULL;
    }
    for(int i = 0; i < PROFILER_MAX_NESTING; i++)
    {
        struct OpenProfileEntry *oent = &g_profState->openEntries[i];
        oent->lastChild = NULL;
    }

    g_profState->freeEntryId = 0;

    struct ProfileEntry *root = PROF__GET_ENTRY_GUID();
    root->firstChild = NULL;
    root->nextSibling = NULL;
    root->locationStr = "Root";
    root->init = true;
    g_profState->openEntries[0].entry = root;
    g_profState->openEntries[0].lastChild = NULL;
    g_profState->curSEntry = 1;
    g_profState->root = root;
}

void debug_end_frame()
{
    g_profState->curFrame = (g_profState->curFrame+1) % PROFILER_FRAME_HISTORY;
    assert(g_profState->curSEntry == 1); // more PROF_START than PROF_END
    assert(g_profState->curFrame < PROFILER_FRAME_HISTORY);
}
