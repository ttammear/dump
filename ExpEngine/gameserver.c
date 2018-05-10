
void game_server_init(GameServer *gs)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 7777;

    gs->eServer = enet_host_create(&address, 32, 2, 0, 0);
    if(NULL == gs->eServer)
    {
        fprintf(stderr, "Failed to create ENet host for server!\n");
        assert(0); // TODO: proper error handling!
    }
    else
    {
        printf("Game server listening at %s:%d\n", "localhost", 7777);
    }
}

void game_server_destroy(GameServer *gs)
{
    if(NULL != gs->eServer)
    {
        enet_host_destroy(gs->eServer);
        printf("Game server closed!\n");
    }
}

void game_server_update(GameServer *gs)
{
    ENetEvent event;
    while(enet_host_service(gs->eServer, &event, 0) > 0)
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                printf("New game client from %x:%u\n", event.peer->address.host, event.peer->address.port);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Received packet. Size: %zu\n", event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Game client disconnected: %x:%u\n", event.peer->address.host, event.peer->address.port);
                event.peer->data = NULL;
                break;
            default: 
                break;
        }
    }
}
