#pragma once

#define PROF_TREE_MAX_NODES 1024
#define PROFILE_TREE_STORED_FRAMES 60
#define EVENT_LOG_STORED_EVENTS 65536

typedef struct DebugEvent {
    uint64_t clock;
    uint16_t threadId;
    uint32_t uid;
    uint8_t type;
    const char *name;
    union {
        void *data;
        const char *taskFunc;
    };
} DebugEvent;

typedef struct ProfNode { // node of profiler tree (generated from debug events)
    struct ProfNode *firstChild;
    struct ProfNode *nextSibling;
    const char *name;
    uint32_t numInvocations;
    uint64_t cycles;
    float ms;
} ProfNode;

typedef struct ProfTree {
   ProfNode tree; // sentinel node
   uint32_t nodeIndex;
   uint32_t frameId;
   ProfNode nodes[PROF_TREE_MAX_NODES];
} ProfTree;

typedef struct EventLogEntry {
    const char *name;
    uint32_t frameId;
    const char *func;
} EventLogEntry;

// NOTE: allocate aligned!
struct ProfilerState
{
     uint32_t frameId;

     struct ProfileEntry *root;
     atomic_uintptr_t prev;

     char name[128];
     atomic_bool pause;
     int pauseIdx;

     khash_t(64) *profTreeHash; // used to generate profile tree (removes duplicate entries)

     _Alignas(8) uint32_t eventIndex;
     DebugEvent events[65536];
    
     uint32_t eventLogCount;
     uint32_t eventLogHead;
     EventLogEntry eventLog[EVENT_LOG_STORED_EVENTS]; // NOTE: ringbuffer oldest -> newest

     ProfTree profileTree[PROFILE_TREE_STORED_FRAMES];
};

extern aike_thread_local struct ProfilerState *t_profState;

extern atomic_uintptr_t g_profStates[10];

enum DebugEventType {
    Debug_Event_None,

    // profiler events
    Debug_Event_End_Frame,
    Debug_Event_Start_Block,
    Debug_Event_End_Block,

    // other events
    Debug_Event_Queue_Task, // keep this first in thos block
    Debug_Event_Start_Task,
    Debug_Event_Suspend_Task,
    Debug_Event_Resume_Task,
    Debug_Event_End_Task,
    Debug_Event_Count,
} DebugEventType;

static_assert(Debug_Event_Count < 256, "With that many events, make sure you use 16bit integer and delete this message!");

#ifdef _DEBUG

#define DEBUG_EVENT(etype, guid, ename) \
    uint32_t _eIdx = __atomic_fetch_add(&t_profState->eventIndex, 1, __ATOMIC_ACQ_REL);\
    assert(_eIdx < ARRAY_COUNT(t_profState->events)); \
    DebugEvent *_evnt = t_profState->events + _eIdx; \
    _evnt->threadId = 0; \
    _evnt->clock = __rdtsc(); \
    _evnt->type = etype; \
    _evnt->uid = guid; \
    _evnt->name = ename; \
    _evnt->data = NULL

#define DEBUG_TASK_EVENT(etype, guid, ename, name) \
    DEBUG_EVENT(etype, guid, ename); \
    _evnt->taskFunc = name

#define PROF__GET_ENTRY_GUID() (&t_profState->entries[t_profState->curFrame][__COUNTER__ % PROFILER_MAX_ENTRIES_PER_FRAME])

static inline void block__end(void *dummy) {
    if(t_profState) {
        DEBUG_EVENT(Debug_Event_End_Block, __COUNTER__, __func__);
    }
}

#define S1(x) #x
#define S2(x) S1(x)

#define PROF_START() PROF_START_STR(__func__)

#define MERGE_(a, b) a##b
#define LABEL_(a) MERGE_(dummy_, a)
#define UNIQUE_N_ LABEL_(__LINE__) // unique line based on line number

#define PROF_BLOCK() void __attribute__((cleanup(block__end))) *UNIQUE_N_; DEBUG_EVENT(Debug_Event_Start_Block, __COUNTER__, __func__); 
#define PROF_BLOCK_STR(x) void __attribute__((cleanup(block__end))) *UNIQUE_N_; DEBUG_EVENT(Debug_Event_Start_Block, __COUNTER__, x); 
#define PROF_START_STR(x) {DEBUG_EVENT(Debug_Event_Start_Block, __COUNTER__, x);}
#define PROF_END() {DEBUG_EVENT(Debug_Event_End_Block, __COUNTER__, __func__);}

#define DEBUG_START_FRAME() debug_start_frame()
#define DEBUG_END_FRAME() debug_end_frame()
#define DEBUG_DESTROY() debug_destroy()
#define DEBUG_FRAME_REPORT() debug_frame_report()
#define DEBUG_INIT(x) debug_init(x)

#else

#define PROF_END()
#define PROF_START()
#define PROF_START_STR(x)
#define PROF_BLOCK()
#define PROF_BLOCK_STR(x)

#define DEBUG_START_FRAME()
#define DEBUG_END_FRAME()
#define DEBUG_DESTROY()
#define DEBUG_FRAME_REPORT()
#define DEBUG_INIT(x)


#endif

