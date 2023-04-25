
aike_thread_local struct ProfilerState *t_profState;
atomic_uintptr_t g_profStates[10];

void debug_init(const char *name)
{
    struct ProfilerState *newProfState = aligned_alloc(_Alignof(struct ProfilerState), sizeof(struct ProfilerState));
    memset(newProfState, 0, sizeof(struct ProfilerState));
    assert(newProfState != NULL);

    strncpy(newProfState->name, name, ARRAY_COUNT(newProfState->name)-1);
    newProfState->name[ARRAY_COUNT(newProfState->name)-1] = 0;

    newProfState->profTreeHash = kh_init(64);

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
    if(!atomic_load(&t_profState->pause))
    {
        atomic_exchange(&t_profState->prev, (uintptr_t)t_profState->root);
    }
}

ProfNode* prof_tree_node(ProfTree* tree, ProfNode *parent, int* eventIndex, int depth) {
    ProfNode *newNode = NULL;
    ProfNode *lastSibling = NULL;
    ProfNode *firstChild = NULL;
    uint64_t startCycles;
    uint64_t hash = 0;
    khiter_t k;
    int ret;
    bool open = false;
    while(*eventIndex < t_profState->eventIndex) {
        DebugEvent *e = &t_profState->events[*eventIndex];
        switch(e->type) {
            default: // skip events that have nothing to do with profiler
                (*eventIndex)++;
                continue;
            case Debug_Event_Start_Block:
                (*eventIndex)++;
                assert(!open);
                hash = e->uid^(uint64_t)parent;
                k = kh_put(64, t_profState->profTreeHash, hash, &ret);
                if(ret > 0) {
                    newNode = &tree->nodes[tree->nodeIndex++];
                    kh_val(t_profState->profTreeHash, k) = newNode;
                    assert(newNode - tree->nodes < PROF_TREE_MAX_NODES);
                    if(firstChild == NULL)
                        firstChild = newNode;
                    // TODO: this overwrites children if multiple calls!
                    newNode->firstChild = prof_tree_node(tree, newNode, eventIndex, depth+1);
                    newNode->name = e->name;
                    newNode->nextSibling = NULL;
                    newNode->numInvocations = 1;
                    newNode->cycles = 0;
                    newNode->ms = 0.0;
                    if(lastSibling != NULL) {
                        lastSibling->nextSibling = newNode;
                    }
                } else {
                    prof_tree_node(tree, newNode, eventIndex, depth+1);
                    //assert(firstChild != NULL); // reapeating entry when no entries haven't even been checked
                    newNode = kh_val(t_profState->profTreeHash, k);
                    if(firstChild == NULL)
                        firstChild = newNode;
                    newNode->numInvocations++;
                }
                startCycles = e->clock;
                open = true;
                break;
            case Debug_Event_End_Block:
                if(!open) {
                    // nothing open at this depth, tree doesn't go any deeper here (probably end of parent node)
                    return firstChild;
                }
                newNode->cycles += e->clock - startCycles;
                newNode->ms = (float)newNode->cycles/2400000.f;
                lastSibling = newNode;
                open = false;
                (*eventIndex)++;
                break;
        }
    }
    return firstChild;
}

void debug_end_frame()
{
    // generate profiler tree
    ProfTree *tree = &t_profState->profileTree[t_profState->frameId%PROFILE_TREE_STORED_FRAMES];
    tree->nodeIndex = 0;
    tree->frameId = t_profState->frameId;
    kh_clear(64, t_profState->profTreeHash);
    int idx = 0;
    tree->tree.firstChild = prof_tree_node(tree, &tree->tree, &idx, 0);
    //assert(idx == t_profState->eventIndex); // all nodes checked

    // generate event log
    // NOTE: reusing idx variable
    if(!atomic_load(&t_profState->pause)) {
        idx = t_profState->eventLogHead;
        for(int i = 0; i < t_profState->eventIndex; i++) {
            DebugEvent *e = t_profState->events + i;
            if(e->type < Debug_Event_Queue_Task)
                continue;
            t_profState->eventLog[idx].name = e->name == NULL ? "" : e->name;
            t_profState->eventLog[idx].func = e->taskFunc;
            t_profState->eventLog[idx].frameId = t_profState->frameId;
            idx = (idx+1)%EVENT_LOG_STORED_EVENTS;
            t_profState->eventLogCount = MIN(t_profState->eventLogCount+1, EVENT_LOG_STORED_EVENTS);
            //printf("lc %d\n", t_profState->eventLogCount);
        }
        t_profState->eventLogHead = idx;

        t_profState->frameId++;
    }
    
    t_profState->eventIndex = 0;
}
