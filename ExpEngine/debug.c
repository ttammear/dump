
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
        uint64_t cycles = entry->end - entry->start;
        double milliseconds = (double)cycles / 2800000.0;
        for(int i = 0; i < depth; i++)
            printf(" "); 
        printf("%s %lucycles %fms\n", entry->locationStr, cycles, milliseconds);
        entry_recursive(entry->firstChild, depth+1);
        entry = entry->nextSibling;
    }
}

double debug_frame_report()
{
    /*double hack;
    for(int i = 0; i < g_profState->freeEntryId; i++)
    {
        struct ProfileEntry *entry = &g_profState->entries[g_profState->curFrame][i];
        uint64_t cycles = entry->end - entry->start;
        double milliseconds = (double)cycles / 2800000.0;
        //printf("%s %lucycles %fms\n", entry->locationStr, cycles, milliseconds);
        hack = milliseconds;
    }*/

    printf("%d entries\n", g_profState->freeEntryId);

    entry_recursive(g_profState->root, 0);

    return 0.0;
}

void debug_destroy()
{
    free(g_profState);
}

void debug_start_frame()
{
    g_profState->freeEntryId = 0;

    struct ProfileEntry *root = PROF__GET_ENTRY();
    root->firstChild = NULL;
    root->nextSibling = NULL;
    root->locationStr = "Root";
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
