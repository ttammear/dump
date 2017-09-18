#include "client.h"

#include <enet/enet.h>
#include <cstdio>

bool Client::Init()
{
    const char *errorstr = "Error occurred while initializeing ENet. /n";

    if(enet_initialize() != 0)
    {
        fprintf(stderr, errorstr);
        return false;
    }

    client = enet_host_create(NULL, 1, 2, 0, 0);
    if(client == NULL)
    {
        fprintf(stderr, errorstr);
        enet_deinitialize();
        return false;
    }

    initialized = true;
}

void Client::Deinit()
{
    if(!initialized)
        return;
    enet_deinitialize();
    enet_host_destroy(client);
    client = NULL;
}
