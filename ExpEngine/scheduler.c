
struct TessScheduler *g_scheduler;

void scheduler_start_pending_tasks();

void* null_allocator(void *usrData, size_t size) {
    return NULL;
}

// TODO: coro_destroy?
void scheduler_init(TessScheduler *ctx, TessFixedArena *arena, TessClient *client, TessServer *server) {
    coro_create(&ctx->mainCtx, NULL, NULL, NULL, 0);

    void *mem = fixed_arena_push_size(arena, 1024*1024, 64);
    assert(mem);
    ctx->editorStack = mem;
    ctx->editorStackSize = 1024*1024;

    mem = fixed_arena_push_size(arena, 1024*1024, 64);
    assert(mem);
    ctx->gameStack = mem;
    ctx->gameStackSize = 1024*1024;

    mem = fixed_arena_push_size(arena, 1024*1024, 64);
    assert(mem);
    ctx->editorServerStack = mem;
    ctx->editorServerStackSize = 1024*1024;

    ctx->waitingTaskSet = kh_init(64);

    // TODO: this is only so high because the asset system is too dumb to resolve
    // dependency graphs in a resource friendly manner!
    // SO CHANGE THIS ONCE THAT IS NO LONGER TRUE
    const int maxTasks = 300;
    // TODO: use protected stack in debug builds!
    const int taskStackSize = 20*4096;
    ctx->taskQueueHead = NULL;
    ctx->taskQueueTail = NULL;
    ctx->yieldedTasksHead = NULL;
    ctx->yieldedTasksTail = NULL;
    ctx->freeTasks = NULL;
    pool_init_with_memory(ctx->ctxPool, malloc(pool_calc_size(ctx->ctxPool, maxTasks)), maxTasks);
    pool_set_allocator(ctx->ctxPool, null_allocator); // TODO: review
    ctx->taskMemory = malloc(taskStackSize*maxTasks);
    ctx->taskMemorySize = taskStackSize;

    ctx->client = client;
    
    coro_create(&ctx->gameCtx, (void(*)(void*))game_client_coroutine, &client->gameClient, ctx->gameStack, ctx->gameStackSize); 
    coro_create(&ctx->editorCtx, (void(*)(void*))editor_coroutine, &client->editor, ctx->editorStack, ctx->editorStackSize);
    coro_create(&ctx->editorServerCtx, editor_server_coroutine, &server->editorServer, ctx->editorServerStack, ctx->editorServerStackSize);
    g_scheduler = ctx;
}

void scheduler_set_mode(u32 mode) {
   g_scheduler->mode = mode; 
}

void scheduler_assert_task() {
   assert(g_scheduler->state == SCHEDULER_STATE_TASK);
};

static void do_main_task() {
    PROF_BLOCK();
    TessScheduler *s = g_scheduler;
    // do client 
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
    // do editor server
    s->state = SCHEDULER_STATE_EDITOR_SERVER;
    coro_transfer(&s->mainCtx, &s->editorServerCtx);
    // do pending tasks
    scheduler_start_pending_tasks(); 
}

void scheduler_yield() {
    TessScheduler *s = g_scheduler;
    YieldedTask *task;
    switch(s->state) {
        case SCHEDULER_STATE_MAIN:
            do_main_task();
            break;
        case SCHEDULER_STATE_EDITOR:
            s->state = SCHEDULER_STATE_MAIN;
            coro_transfer(&s->editorCtx, &s->mainCtx);
            break;
        case SCHEDULER_STATE_EDITOR_SERVER:
            s->state = SCHEDULER_STATE_MAIN;
            coro_transfer(&s->editorServerCtx, &s->mainCtx);
            break;
        case SCHEDULER_STATE_GAME:
            s->state = SCHEDULER_STATE_MAIN;
            coro_transfer(&s->gameCtx, &s->mainCtx);
            break;
        case SCHEDULER_STATE_TASK:
            {DEBUG_TASK_EVENT(Debug_Event_Suspend_Task, __COUNTER__, "Yield task", s->curTaskCtx->name);}

            // add task to yielded task list, so that we can resume it later
            // TODO: no malloc?
            task = malloc(sizeof(YieldedTask));
            task->ctx = s->curTaskCtx;
            task->next = NULL;
            if(s->yieldedTasksTail == NULL) {
                assert(s->yieldedTasksHead == NULL);
                s->yieldedTasksHead = task;
            } else {
                s->yieldedTasksTail->next = task;
            }
            s->yieldedTasksTail = task;

            s->state = SCHEDULER_STATE_MAIN;
            coro_transfer(&s->curTaskCtx->ctx, &s->mainCtx);
            break;
        default:
            assert(false); // invalid code path, possibly memory corruption?
            break;
    }
}

// TODO: extend this to main thread (even though it should never be used like that!)
void scheduler_wait_for(AsyncTask *task) {
    TessScheduler *s = g_scheduler;
    int ret;
    // TODO: could the event be triggered before this yields? (in case the task is multithreaded)
    if(!atomic_load(&task->done)) {
        {DEBUG_TASK_EVENT(Debug_Event_Suspend_Task, __COUNTER__, "Suspend task (wait event)", s->curTaskCtx->name);}
        khiter_t key = kh_put(64, s->waitingTaskSet, (uint64_t)task, &ret);
        kh_value(s->waitingTaskSet, key) = task;
        assert(ret); // hashmap bucket was empty
        //scheduler_yield();
        s->state = SCHEDULER_STATE_MAIN;
        coro_transfer(&s->curTaskCtx->ctx, &s->mainCtx);
    }
}

void scheduler_event(u32 type, void *data, AsyncTask *task) {
    TessScheduler *s = g_scheduler;
    khiter_t k = kh_get(64, s->waitingTaskSet, (uint64_t)task);
    if(k != kh_end(s->waitingTaskSet)) {
       kh_del(64, s->waitingTaskSet, k);
       switch(type) {
           case SCHEDULER_EVENT_RENDER_MESSAGE:
               // TODO: avoid so many copies somehow?
               // TODO: this is unsafe, referencing to mainCtx stack from task
               task->renderMsg = ((RenderMessage*)data);
               atomic_store(&task->done, true);
               // TODO: wrap this into function?
               assert(s->state == SCHEDULER_STATE_MAIN);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = task->ctx;
               DEBUG_TASK_EVENT(Debug_Event_Resume_Task, __COUNTER__, "Resume task (waited on render message)", task->ctx->name);
               coro_transfer(&s->mainCtx, &task->ctx->ctx);
               break;
           case SCHEDULER_EVENT_FILE_LOADED:
               assert(s->state == SCHEDULER_STATE_MAIN);
               atomic_store(&task->done, true);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = task->ctx;
               {DEBUG_TASK_EVENT(Debug_Event_Resume_Task, __COUNTER__, "Resume task (waited on file read)", task->ctx->name);}
               coro_transfer(&s->mainCtx, &task->ctx->ctx);
               break;
           default:
               assert(0);
               break;
       }
    }
}

// TODO: when a task is started from within a task, just do it inside the task?
SchedulerTask* scheduler_queue_task_(TaskFunc func, void *usrData, const char* name) {
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
    task->name = name;
    task->flags = 0;

    task->next = NULL;
    if(s->taskQueueTail == NULL) {
        assert(s->taskQueueHead == NULL);
        s->taskQueueHead = task;
    } else {
        s->taskQueueTail->next = task;
    }
    s->taskQueueTail = task;

    DEBUG_TASK_EVENT(Debug_Event_Queue_Task, __COUNTER__, "Queue task", name);
    return task;
}

void scheduler_cancel_task(SchedulerTask *task) {
    assert((task->flags & Scheduler_Task_Flag_Started) == 0); // can only cancel tasks that have not been started yet (the ones still in queue)
    task->flags |= Scheduler_Task_Flag_Cancelled;
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

// TODO: put started tasks in a list and keep giving them
// time every frame until they end (scheduler_yield doesn't work because
// the control isn't retured to the task after calling it)
void scheduler_start_pending_tasks() {
    PROF_BLOCK();
    TessScheduler *s = g_scheduler;
    assert(s->state == SCHEDULER_STATE_MAIN);
        
    // start as many queued tasks as possible
    while(  true) {
        if(s->taskQueueTail == NULL) // no tasks
        {
            break;
        }
        TaskContext *ctx = pool_allocate(s->ctxPool);
        if(ctx == NULL) // no more tasks
        {
            break;
        }
        SchedulerTask *task = s->taskQueueHead;

        // only start job if it has not been cancelled
        if((task->flags & Scheduler_Task_Flag_Cancelled) == 0) {
           
            DEBUG_TASK_EVENT(Debug_Event_Queue_Task, __COUNTER__, "Start task", task->name);
            coro_create(&ctx->ctx, task->func, task->data, s->taskMemory+s->taskMemorySize*(ctx - s->ctxPool), s->taskMemorySize);
            s->state = SCHEDULER_STATE_TASK;
            s->curTaskCtx = ctx;
            ctx->name = task->name;
            coro_transfer(&s->mainCtx, &ctx->ctx);
        } else {
            pool_free(s->ctxPool, ctx);
        }

        // update linked list and put task memory back to freelist
        s->taskQueueHead = s->taskQueueHead->next;
        if(s->taskQueueHead == NULL) {
            s->taskQueueTail = NULL;
        }
        task->flags |= Scheduler_Task_Flag_Started;
        task->next = s->freeTasks;
        s->freeTasks = task;
    }

    // if there are any yielded tasks, resume them
    // we throw away the entire list and iterate it
    // tasks that yield again will readd themselves in scheduler_yield()
    struct YieldedTask *yieldedTasksHead = s->yieldedTasksHead;
    struct YieldedTask *cur = yieldedTasksHead;
    struct YieldedTask *next;
    s->yieldedTasksHead = NULL;
    s->yieldedTasksTail = NULL;
    if(cur == NULL) {
        return;
    }
    do {
        TaskContext *ctx = cur->ctx;
        next = cur->next; // do this because we free before post condition
        s->state = SCHEDULER_STATE_TASK;
        s->curTaskCtx = ctx;
        DEBUG_TASK_EVENT(Debug_Event_Queue_Task, __COUNTER__, "Resume yielded task", ctx->name);
        coro_transfer(&s->mainCtx, &ctx->ctx);
        //TODO: no malloc, no free
        free(cur);
    } while((cur = next) != NULL);
}
