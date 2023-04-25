#include <stdint.h>
#include <vector>
#include <atomic>
#include <thread>
#include "../Maths/maths.h"
#include "networkedinput.h"

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

#define MAX_SNAPSHOT_ENTITIES   100
#define NUM_SNAPSHOTS 12

struct EntitySnapshot
{
    uint32_t entityId;
    Vec3 position;
    Quaternion rotation;
};

struct WorldStateSnapshot
{
    uint32_t numEntities;
    EntitySnapshot entities[MAX_SNAPSHOT_ENTITIES];
};

class Server
{
public:
    Server(class World *world);

    void serverProc();

    bool init(uint16_t port);
    void deinit();
    void doEvents();
    void takeSnapshot(class World *world);
    int32_t getFreePlayerSlot();

    ENetHost *server;
    bool initialized = false;
    std::vector<ENetPeer*> connectedClients;

    WorldStateSnapshot snapshots[NUM_SNAPSHOTS];
    std::atomic<int32_t> curSnapshot;
    std::atomic<bool> workerRunning;
    std::thread *workThread;

    uint32_t maxPlayers = 32;
    struct ServerPlayer *players;
    class World *world;
};
