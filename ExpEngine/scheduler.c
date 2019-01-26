
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

    const int maxTasks = 10;
    const int taskStackSize = 4096;
    ctx->tasks = NULL;
    pool_init_with_memory(ctx->ctxPool, malloc(pool_calc_size(ctx->ctxPool, maxTasks)), maxTasks);
    void * test = pool_allocate(ctx->ctxPool);
    void * test2 = pool_allocate(ctx->ctxPool);
    pool_print(ctx->ctxPool);
    pool_print(ctx->ctxPool);
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
             s->state = SCHEDULER_STATE_MAIN;
             coro_transfer(s->curTaskCtx, &s->mainCtx);
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

void scheduler_event(u32 type, void *data, void *usrPtr) {
    TessScheduler *s = g_scheduler;
    khiter_t k = kh_get(64, s->waitingTaskSet, (uint64_t)usrPtr);
    if(k != kh_end(s->waitingTaskSet)) {
       kh_del(64, s->waitingTaskSet, k);
       AsyncTask *t = (AsyncTask*)usrPtr;
       switch(type) {
           case SCHEDULER_EVENT_RENDER_MESSAGE:
               // TODO: avoid so many copies somehow?
               // TODO: this is unsafe, referencing to mainCtx stack from task
               t->renderMsg = ((RenderMessage*)data);
               assert(s->state == SCHEDULER_STATE_MAIN);
               s->state = SCHEDULER_STATE_TASK;
               s->curTaskCtx = t->ctx;
               coro_transfer(&s->mainCtx, t->ctx);
               break;
           default:
               assert(0);
               break;
       }
    }
}

void scheduler_queue_task(TaskFunc func, void *usrData) {
    TessScheduler *s = g_scheduler;
    SchedulerTask task = {.func = func, .data = usrData};
    buf_push(s->tasks, task);
}

// TODO: find an "automated" way of doing this, like seeding a return address to task stack
void scheduler_task_end() {
    TessScheduler *s = g_scheduler;
    coro_context *ctx = s->curTaskCtx;
    assert(s->state == SCHEDULER_STATE_TASK);
    // copy because we need it for coro_transfer and it's more convenient to free it here
    coro_context ctxCopy = *ctx;
    pool_free(s->ctxPool, ctx); 
#if _DEBUG
    ctx = NULL;
#endif
    // TODO: not really necessary, for debug purposes only
    s->curTaskCtx = NULL;
    s->state = SCHEDULER_STATE_MAIN;
    coro_transfer(&ctxCopy, &s->mainCtx);
}

void scheduler_start_pending_tasks() {
    TessScheduler *s = g_scheduler;
    assert(s->state == SCHEDULER_STATE_MAIN);
    while(true) {
        if(buf_len(s->tasks) <= 0) // no tasks
            return;
        coro_context *ctx = pool_allocate(s->ctxPool);
        if(!ctx) // no more tasks
            return;
        // TODO: this is FILO not FIFO!!
        SchedulerTask task = s->tasks[buf_len(s->tasks)-1];

       coro_create(ctx, task.func, task.data, s->taskMemory+s->taskMemorySize*(ctx - s->ctxPool), s->taskMemorySize);
       s->state = SCHEDULER_STATE_TASK;
       s->curTaskCtx = ctx;
       coro_transfer(&s->mainCtx, ctx);
       buf_pop_back(s->tasks);
    }
}
