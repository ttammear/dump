#include <stdint.h>

typedef struct _ENetHost ENetHost;

class Client
{
public:
    bool Init();
    void Deinit();

    ENetHost *client;
    bool initialized = false;
};
