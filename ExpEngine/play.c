void play_update(TessClient *client);

void game_client_coroutine(GameClient *gclient)
{
    gclient->init = true;
    gclient->connected = false;

    gclient->eClient = enet_host_create(NULL, 1, 2, 0, 0);
    if(NULL == gclient->eClient)
    {
        fprintf(stderr, "Failed to create eNet client for gameClient!\n");
        goto game_client_done;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;
    peer = enet_host_connect(gclient->eClient, &address, 2, 0);
    assert(NULL != peer);

    bool connecting = true;
    while(connecting)
    {
        int res = enet_host_service(gclient->eClient, &event, 0);
        if(res > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            printf("Game client connected to localhost:7777\n");
            gclient->connected = true;
            connecting = false;
        }
        else if(res > 0)
        {
            printf("Connection to localhost:7777 failed... Retrying\n");
            connecting = false;
        }
        coro_transfer(&gclient->coroCtx, gclient->client->mainctx);
    }

    while(gclient->connected)
    {
        while(enet_host_service(gclient->eClient, &event, 0) > 0)
        {
            switch(event.type)
            {
                case ENET_EVENT_TYPE_DISCONNECT:
                    gclient->connected = false;
                    printf("Disconnected!\n");
                    break;
                default:
                    break;
            }
        }

        if(gclient->connected)
            play_update(gclient->client);
        coro_transfer(&gclient->coroCtx, gclient->client->mainctx);
    }

    enet_host_destroy(gclient->eClient);
game_client_done:
    gclient->init = false;
    coro_transfer(&gclient->coroCtx, gclient->client->mainctx);
}


void play_update(TessClient *client)
{
    static V2 camRot;

    TessCamera *cam = &client->gameSystem.defaultCamera;
    TessInputSystem *input = &client->inputSystem;

    V3 forward = make_v3(0.0f, 0.0f, 0.1f);
    V3 right;
    quat_v3_mul_dir(&forward, cam->rotation, make_v3(0.0f, 0.0f, 0.01f));
    quat_v3_mul_dir(&right, cam->rotation, make_v3(0.01f, 0.0f, 0.0f));

        if(key(input, AIKE_KEY_W))
            v3_add(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_S))
            v3_sub(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_D))
            v3_add(&cam->position, cam->position, right);
        if(key(input, AIKE_KEY_A))
            v3_sub(&cam->position, cam->position, right);

        v2_add(&camRot, camRot, input->mouseDelta);
        camRot = make_v2(camRot.x, MAX(camRot.y, -90.0f));
        camRot = make_v2(camRot.x, MIN(camRot.y, 90.0f));

    Quat xRot, yRot;
    quat_euler_deg(&xRot, make_v3(0.0f, camRot.y, camRot.x));
    cam->rotation = xRot;

    if(key(input, AIKE_KEY_ESC))
    {
        client->mode = Tess_Client_Mode_Menu;
    }
}

