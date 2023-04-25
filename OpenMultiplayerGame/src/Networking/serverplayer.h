#include "../Maths/maths.h"

#include "../Player/player.h"
#include "networkedinput.h"

struct ServerPlayer
{
    enum Flags
    {
        Spawned = 1 << 0,
        InUse   = 2 << 1
    };

    void init(World *world);
    void markInUse(bool inUse);
    void spawn(Vec3 position, Quaternion rotation);
    void despawn();

    uint32_t flags;
    uint32_t playerId;
    Player player;
    InputClient inputClient;
    class World *world;
};
