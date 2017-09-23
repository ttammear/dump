#include <stdint.h>
#include "../transform.h"

#define NT_HISTORY_SIZE 16

struct NetworkedTransform
{
    Transform *transform;
    
    double playCursor;
    double playSpeed;
    bool playing;
    int32_t latestIndex = -1;
    uint8_t latestSeq;
    Vec3 positions[NT_HISTORY_SIZE];
    Quaternion rotations[NT_HISTORY_SIZE];
};

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

#include <chrono>

class Client
{
public:
    bool init();
    void deinit();
    bool connect(const char* hostName, unsigned short port);
    void doEvents();

    void readPositionUpdate(uint8_t *data, uint32_t length);
    void updateTransforms();

    ENetHost *client;
    ENetPeer *peer;
    struct Entity *entities;
    NetworkedTransform transforms[100];
    std::chrono::steady_clock::time_point lastUpdate;
    bool initialized = false;
};
