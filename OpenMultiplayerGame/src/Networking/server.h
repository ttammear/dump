#include <stdint.h>

typedef struct _ENetHost ENetHost;

class Server
{
public:
    bool Init(uint16_t port);
    void Deinit();

    ENetHost *server;
    bool initialized = false;
};
