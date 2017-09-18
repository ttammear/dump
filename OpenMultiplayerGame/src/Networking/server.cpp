#include "server.h"

#include <enet/enet.h>
#include <cstdio>

bool Server::Init(uint16_t port)
{
    const char *errorstr = "Error occurred while initializeing ENet. /n";

    if(enet_initialize() != 0)
    {
        fprintf(stderr, errorstr);
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    server = enet_host_create(&address, 32, 2, 0, 0);
    if(server == NULL)
    {
        fprintf(stderr, errorstr);
        enet_deinitialize();
        return false;
    }

    initialized = true;
}

void Server::Deinit()
{
    if(!initialized)
        return;
    enet_deinitialize();
    enet_host_destroy(server);
}
