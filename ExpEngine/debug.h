#pragma once

struct ProfileEntry
{
    uint64_t start;
    uint64_t end;
    struct ProfileEntry *firstChild;
    struct ProfileEntry *nextSibling;
    const char *locationStr;
};

struct OpenProfileEntry
{
    struct ProfileEntry *entry;
    struct ProfileEntry *lastChild;
};

#define PROFILER_FRAME_HISTORY              600
#define PROFILER_MAX_ENTRIES_PER_FRAME     1000
#define PROFILER_MAX_NESTING                 50

struct ProfilerState
{
     struct ProfileEntry entries[PROFILER_FRAME_HISTORY][PROFILER_MAX_ENTRIES_PER_FRAME];
     uint32_t curFrame;
     uint32_t freeEntryId;

     struct ProfileEntry *root;

     uint32_t curSEntry;
     struct OpenProfileEntry openEntries[PROFILER_MAX_NESTING];

};

extern struct ProfilerState *g_profState;

#define PROF__GET_ENTRY() (&g_profState->entries[g_profState->curFrame][g_profState->freeEntryId++])

/*void testst(const char *str)
{
    struct ProfileEntry *entry = PROF__GET_ENTRY(); \
    struct OpenProfileEntry *parentOE = &g_profState->openEntries[g_profState->curSEntry-1]; 
    struct ProfileEntry *parent = parentOE->entry; 
    g_profState->openEntries[g_profState->curSEntry].lastChild = NULL; 
    g_profState->openEntries[g_profState->curSEntry++].entry = entry; 
    entry->start = __rdtsc(); 
    entry->locationStr = str;
    entry->firstChild = NULL;
    entry->nextSibling = NULL;
    if(parent->firstChild == NULL)
        parent->firstChild = entry; 
    if(parentOE->lastChild != NULL) 
        parentOE->lastChild->nextSibling = entry; 
    parentOE->lastChild = entry; 

}

#define PROF_START_STR(str) (testst(str))
*/


#define PROF_START_STR(str) \
{\
    struct ProfileEntry *entry = PROF__GET_ENTRY(); \
    struct OpenProfileEntry *parentOE = &g_profState->openEntries[g_profState->curSEntry-1]; \
    struct ProfileEntry *parent = parentOE->entry; \
    g_profState->openEntries[g_profState->curSEntry].lastChild = NULL; \
    g_profState->openEntries[g_profState->curSEntry++].entry = entry; \
    entry->start = __rdtsc(); \
    entry->locationStr = str; \
    entry->firstChild = NULL; \
    entry->nextSibling = NULL; \
    if(parent->firstChild == NULL) \
        parent->firstChild = entry; \
    if(parentOE->lastChild != NULL) \
        parentOE->lastChild->nextSibling = entry; \
    parentOE->lastChild = entry; \
} 



#define PROF_START() PROF_START_STR(__func__)

#define PROF_END() \
{ \
    struct ProfileEntry *entry = g_profState->openEntries[--g_profState->curSEntry].entry; \
    entry->end = __rdtsc(); \
}

