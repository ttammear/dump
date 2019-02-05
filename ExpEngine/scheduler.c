
struct TessScheduler *g_scheduler;

void scheduler_start_pending_tasks();

// TODO: coro_destroy?
void scheduler_init(TessScheduler *ctx, TessFixedArena *arena, TessClient *client) {
    coro_create(&ctx->mainCtx, NULL, NULL, NULL, 0);

    void *mem = fixed_arena_push_size(arena, 1024*1024, 64);
    assert(mem);
    ctx->editorStack = mem;
    ctx->editorStackSize = 1024*1024;

    mem = fixed_arena_push_size(arena, 1024*1024, 64);
    assert(mem);
    ctx->gameStack = mem;
    ctx->gameStackSize = 1024*1024;

    ctx->waitingTaskSet = kh_init(64);

    // TODO: this is only so high because the asset system is too dumb to resolve
    // dependency graphs in a resource friendly manner!
    // SO CHANGE THIS ONCE THAT IS NO LONGER TRUE
    const int maxTasks = 444;
    // TODO: use protected stack in debug builds!
    const int taskStackSize = 4096;
    ctx->taskQueueHead = NULL;
    ctx->taskQueueTail = NULL;
    ctx->freeTasks = NULL;
    pool_init_with_memory(ctx->ctxPool, malloc(pool_calc_size(ctx->ctxPool, maxTasks)), maxTasks);
    ctx->taskMemory = malloc(taskStackSize*maxTasks);
    ctx->taskMemorySize = taskStackSize;

    ctx->client = client;
    
    coro_create(&ctx->gameCtx, (void(*)(void*))game_client_coroutine, &client->gameClient, ctx->gameStack, ctx->gameStackSize); 
    coro_create(&ctx->editorCtx, (void(*)(void*))editor_coroutine, &client->editor, ctx->editorStack, ctx->editorStackSize);
    g_scheduler = ctx;
}

void scheduler_set_mode(u32 mode) {
   g_scheduler->mode = mode; 
}

static void do_main_task() {
    TessScheduler *s = g_scheduler;
    switch(s->mode) {
        case Tess_Client_Mode_Game:
            s->state = SCHEDULER_STATE_GAME;
            coro_transfer(&s->mainCtx, &s->gameCtx);
            break;
        case Tess_Client_Mode_Menu:
            tess_main_menu_update(&s->client->mainMenu);
            break;
        case Tess_Client_Mode_Editor:
            s->state = SCHEDULER_STATE_EDITOR;
            coro_transfer(&s->mainCtx, &s->editorCtx);
            break;
        case Tess_Client_Mode_CrazyTown:
            assert(0); // no crazytown yet
            break;
        default:
            assert(0); // invalid/unknown state
            break;
    }
    scheduler_start_pending_tasks(); 
}

void scheduler_yield() {
    TessScheduler *s = g_scheduler;
    switch(s->state) {
        case SCHEDULER_STATE_MAIN:
             do_main_task();
             break;
        case SCHEDULER_STATE_EDITOR:
             s->state = SCHEDULER_STATE_MAIN;
             coro_transfer(&s->editorCtx, &s->mainCtx);
             break;
        case SCHEDULER_STATE_GAME:
             s->state = SCHEDULER_STATE_MAIN;
             coro_transfer(&s->gameCtx, &s->mainCtx);
             break;
        case SCHEDULER_STATE_TASK:
             {DEBUG_TASK_EVENT(Debug_Event_Suspend_Task, __COUNTER__, "Suspend task", s->curTaskCtx->name);}
             s->state = SCHEDULER_STATE_MAIN;
             coro_transfer(&s->curTaskCtx->ctx, &s->mainCtx);
             break;
        default:
             assert(false); // invalid code path, possibly memory corruption?
             break;
    }
}

void scheduler_wait_for(AsyncTask *task) {
    TessScheduler *s = g_scheduler;
    int ret;
    khiter_t key = kh_put(64, s->waitingTaskSet, (uint64_t)task, &ret);
    kh_value(s->waitingTaskSet, key) = task;
    assert(ret); // hashmap bucket was empty
    scheduler_yield();
}

void scheduler_event(u32 type, void *data, AsyncTask *task) {
    TessScheduler *s = g_scheduler;
    khiter_t k = kh_get(64, s->waitingTaskSet, (uint64_t)task);
    if(k != kh_end(s->waitingTaskSet)) {
       kh_del(64, s->waitingTaskSet, k);
       AsyncTask *t = (AsyncTask*)task;
       switch(type) {
           case SCHEDULER_EVENT_RENDER_MESSAGE:
               // TODO: avoid so many copies somehow?
               // TODO: this is unsafe, referencing to mainCtx stack from task
               t->renderMsg = ((RenderMessage*)data);
               // TODO: wrap this into function?
               assert(s->state == SCHEDULER_STATE_MAIN);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = t->ctx;
               DEBUG_TASK_EVENT(Debug_Event_Resume_Task, __COUNTER__, "Resume task (waited on render message)", t->ctx->name);
               coro_transfer(&s->mainCtx, &t->ctx->ctx);
               break;
           case SCHEDULER_EVENT_ASSET_DEPS_LOADED:
               assert(s->state == SCHEDULER_STATE_MAIN);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = t->ctx;
               {DEBUG_TASK_EVENT(Debug_Event_Resume_Task, __COUNTER__, "Resume task (waited on asset dependencies)", t->ctx->name);}
               coro_transfer(&s->mainCtx, &t->ctx->ctx);
               break;
           case SCHEDULER_EVENT_FILE_LOADED:
               assert(s->state == SCHEDULER_STATE_MAIN);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = t->ctx;
               {DEBUG_TASK_EVENT(Debug_Event_Resume_Task, __COUNTER__, "Resume task (waited on file read)", t->ctx->name);}
               coro_transfer(&s->mainCtx, &t->ctx->ctx);
               break;
           default:
               assert(0);
               break;
       }
    }
}

void scheduler_queue_task_(TaskFunc func, void *usrData, const char* name) {
    TessScheduler *s = g_scheduler;
    SchedulerTask* task;
    if(s->freeTasks != NULL) { // try to reuse previously allocated task
        task = s->freeTasks;
        s->freeTasks = task->next;
    }
    else {
        // TODO: no malloc!
        task = malloc(sizeof(SchedulerTask));
    }
    task->func = func;
    task->data = usrData;
    task->next = NULL;
    task->name = name;
    if(s->taskQueueTail == NULL) {
        assert(s->taskQueueHead == NULL);
        s->taskQueueHead = task;
    } else {
        s->taskQueueTail->next = task;
    }
    s->taskQueueTail = task;
    DEBUG_TASK_EVENT(Debug_Event_Queue_Task, __COUNTER__, "Queue task", name);
}

// TODO: find an "automated" way of doing this, like seeding a return address to task stack
void scheduler_task_end() {
    TessScheduler *s = g_scheduler;
    TaskContext *ctx = s->curTaskCtx;
    assert(s->state == SCHEDULER_STATE_TASK);
    // copy because we need it for coro_transfer and it's more convenient to free it here
    TaskContext ctxCopy = *ctx;
    pool_free(s->ctxPool, ctx); 
#if _DEBUG
    ctx = NULL;
#endif
    // TODO: not really necessary, for debug purposes only
    s->curTaskCtx = NULL;
    s->state = SCHEDULER_STATE_MAIN;
    DEBUG_TASK_EVENT(Debug_Event_End_Task, __COUNTER__, "End task", ctxCopy.name);
    coro_transfer(&ctxCopy.ctx, &s->mainCtx);
}

void scheduler_start_pending_tasks() {
    TessScheduler *s = g_scheduler;
    assert(s->state == SCHEDULER_STATE_MAIN);
        
    while(true) {
        if(s->taskQueueTail == NULL) // no tasks
            return;
        TaskContext *ctx = pool_allocate(s->ctxPool);
        if(!ctx) // no more tasks
            return;
        SchedulerTask *task = s->taskQueueHead;

        DEBUG_TASK_EVENT(Debug_Event_Queue_Task, __COUNTER__, "Start task", task->name);
        coro_create(&ctx->ctx, task->func, task->data, s->taskMemory+s->taskMemorySize*(ctx - s->ctxPool), s->taskMemorySize);
        s->state = SCHEDULER_STATE_TASK;
        s->curTaskCtx = ctx;
        ctx->name = task->name;
        coro_transfer(&s->mainCtx, &ctx->ctx);

        s->taskQueueHead = s->taskQueueHead->next;
        if(s->taskQueueHead == NULL) {
            s->taskQueueTail = NULL;
        }
        task->next = s->freeTasks;
        s->freeTasks = task;
    }
}
