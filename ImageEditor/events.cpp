

static void event_manager_init(EventManager *emgr)
{
    for(int i = 0; i < ARRAY_COUNT(emgr->listeners); i++)
    {
        emgr->listeners[i].userData = NULL;
        emgr->listeners[i].onTrigger = NULL;
        emgr->listeners[i].next = NULL;
    }
}

static void event_manager_free_resources(EventManager *emgr)
{
    for(int i = 0; i < ARRAY_COUNT(emgr->listeners); i++)
    {
        ListenEvent *e = emgr->listeners[i].next;
        int count = 0;

        if(emgr->listeners[i].onTrigger != NULL)
            count++;

        while(e != NULL)
        {
            ListenEvent *next = e->next;
            aike_free(e);
            count++;
            e = next;
        }
        if(count != 0)
        {
            aike_log_warning("%d Tangling event listener for event %d\n", count, i);
        }
    }
}

static void event_manager_event(EventManager *emgr, Event event)
{
    ListenEvent *le = &emgr->listeners[event];
    if(le->onTrigger != NULL)
    {
        while(le != NULL)
        {
            if(le->onTrigger(le->userData))
                break;
            le = le->next;
        }
    }
}

static void event_manager_add_listener(EventManager *emgr, Event event, EventAction onTrigger, void *userData, int priority)
{
    ListenEvent *le = &emgr->listeners[event];
    if(le->onTrigger == NULL) // no listeners for this event, add front
    {
        emgr->listeners[event].next = NULL;
        emgr->listeners[event].onTrigger = onTrigger;
        emgr->listeners[event].userData = userData;
        emgr->listeners[event].priority = priority;
    }
    else if(le->priority < priority) // add front
    {
        ListenEvent *next = (ListenEvent*)aike_alloc(sizeof(ListenEvent));
        memcpy(next, le, sizeof(ListenEvent));
        
        le->next = next;
        le->onTrigger = onTrigger;
        le->userData = userData;
        le->priority = priority;
    }
    else // add middle (or tail)
    {
        while(le->next != NULL && le->next->priority > priority)
            le = le->next;

        ListenEvent *next = (ListenEvent*)aike_alloc(sizeof(ListenEvent));
        next->next = le->next;
        next->onTrigger = onTrigger;
        next->userData = userData;
        next->priority = priority;
        le->next = next;
    }
}

static void event_manager_remove_listener(EventManager *emgr, Event event, EventAction onTrigger, void *userData)
{
    ListenEvent *le = &emgr->listeners[event];
    ListenEvent *prev = NULL;
    bool found = false;
    while(le != NULL)
    {
        if(le->onTrigger == onTrigger && le->userData == userData)
        {
            if(le == &emgr->listeners[event])
            {
                if(le->next != NULL)
                {
                    // make copy because we will overwrite it
                    ListenEvent *n = le->next;
                    memcpy(le, n, sizeof(ListenEvent));
                    aike_free(n);
                }
                else
                {
                    emgr->listeners[event].onTrigger = NULL;
                    emgr->listeners[event].userData = NULL;
                }
            }
            else
            {
                assert(prev != NULL); // can't be NULL because first element is different case
                prev->next = le->next;
                aike_free(le);
            }
            found = true;
            break;
        }
        prev = le;
        le = le->next;
    }
    assert(found); // listener not found
}

