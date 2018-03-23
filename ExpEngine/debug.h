#pragma once

#ifdef _DEBUG

struct ProfileEntry
{
    uint64_t start;
    uint64_t sum;
    uint32_t init;
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
#define PROFILER_MAX_ENTRIES_PER_FRAME      100
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

#define PROF__GET_ENTRY_GUID() (&g_profState->entries[g_profState->curFrame][__COUNTER__ % PROFILER_MAX_ENTRIES_PER_FRAME])

// TODO: this will not work with recursive entries
// TODO: if __COUNTER__ gets used anywhere else then
// we're either wasting entries or colliding

#define PROF_START_STR(str) \
{\
    struct ProfileEntry *entry = PROF__GET_ENTRY_GUID(); \
    struct OpenProfileEntry *oe = &g_profState->openEntries[g_profState->curSEntry++]; \
    if(!entry->init) \
    { \
        struct OpenProfileEntry *parentOE = &g_profState->openEntries[g_profState->curSEntry-2]; \
        oe->entry = entry; \
        entry->start = __rdtsc(); \
        entry->locationStr = str; \
        if(parentOE->entry->firstChild == NULL) \
           parentOE->entry->firstChild = entry; \
        if(parentOE->lastChild != NULL) \
            parentOE->lastChild->nextSibling = entry; \
        parentOE->lastChild = entry; \
        entry->init = true; \
    } \
    else \
    { \
        entry->start = __rdtsc(); \
        oe->lastChild = NULL; \
        oe->entry = entry; \
    } \
}

static inline void prof__end(void *dummy)
{
    struct ProfileEntry *entry = g_profState->openEntries[--g_profState->curSEntry].entry;
    dummy = 0;
    entry->sum += __rdtsc() - entry->start;
}

#define S1(x) #x
#define S2(x) S1(x)

#define PROF_START() PROF_START_STR(__func__)

#define MERGE_(a, b) a##b
#define LABEL_(a) MERGE_(dummy_, a)
#define UNIQUE_N_ LABEL_(__LINE__)
#define PROF_BLOCK() void __attribute__((cleanup(prof__end))) *UNIQUE_N_; PROF_START()

#define PROF_END() \
{ \
    struct ProfileEntry *entry = g_profState->openEntries[--g_profState->curSEntry].entry; \
    entry->sum += __rdtsc() - entry->start; \
}

#define DEBUG_START_FRAME() debug_start_frame()
#define DEBUG_END_FRAME() debug_end_frame()
#define DEBUG_DESTROY() debug_destroy()
#define DEBUG_FRAME_REPORT() debug_frame_report()
#define DEBUG_INIT() debug_init()

#else

#define PROF_END()
#define PROF_START()
#define PROF_START_STR(x)
#define PROF_BLOCK()

#define DEBUG_START_FRAME()
#define DEBUG_END_FRAME()
#define DEBUG_DESTROY()
#define DEBUG_FRAME_REPORT()
#define DEBUG_INIT()


#endif

