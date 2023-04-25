#include "serverplayer.h"

#include "algorithm"

#include "../macros.h"
#include "../GameWorld/world.h"
#include "../Player/player.h"

void setPlayerInput(void *ptr, PlayerInput *pinput)
{
    ServerPlayer *sp = (ServerPlayer*)ptr;
    bool pred;
    pinput->moveForward = sp->inputClient.getKey(0, 0, pred);
    pinput->moveBackward = sp->inputClient.getKey(0, 2, pred);
    pinput->moveRight = sp->inputClient.getKey(0, 3, pred);
    pinput->moveLeft = sp->inputClient.getKey(0, 1, pred);
    pinput->jump = true;
    pinput->rotation = Quaternion::Identity();
}

void ServerPlayer::init(class World *world)
{
    this->world = world;
    player.init(world);
    player.setActive(false);
}

void ServerPlayer::spawn(Vec3 position, Quaternion rotation)
{
    SETFLAG(flags, Flags::Spawned);
    player.setActive(true);
    player.setPosition(&position);

    WorldPlayer wp;
    wp.usrPtr = this;
    wp.getInputFunc = setPlayerInput;
    wp.player = &this->player;
    world->players.push_back(wp);
}

void ServerPlayer::despawn()
{
    CLEARFLAG(flags, Flags::Spawned);
    player.setActive(false);
    auto fplayer = std::find_if(
            world->players.begin(), 
            world->players.end(), 
            [this] (WorldPlayer& p) { return p.usrPtr == this; } );
    world->players.erase(fplayer);
}

void ServerPlayer::markInUse(bool inUse)
{
    if(inUse)
        SETFLAG(flags, Flags::InUse);
    else
    {
        CLEARFLAG(flags, Flags::InUse);
        if(FLAGSET(flags, Flags::Spawned))
        {
            despawn();
        }
    }
}
