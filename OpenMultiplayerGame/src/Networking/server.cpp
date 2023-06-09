#include "server.h"

#include <enet/enet.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <mutex>
#include "../Maths/maths.h"
#include "../GameWorld/world.h"
#include "serverplayer.h"
#include "../macros.h"

#include "../Physics/character.h" // TODO: remove

std::mutex enet_mutex;

Server::Server(World *world)
{
    curSnapshot = -1;
    workerRunning = false;
    this->world = world;
}

void writeu16(uint8_t *data, uint16_t value)
{
    data[0] = value;
    data[1] = value >> 16;
}

void writeVec3(uint8_t *data, Vec3 value)
{
    memcpy(data, &value, sizeof(Vec3));
}

void writeQuat(uint8_t *data, Quaternion value)
{
    memcpy(data, &value, sizeof(Quaternion));
}

void Server::serverProc()
{
    ENetPacket *packet;

    double lag = 0.0f;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point cur;

    uint8_t seq = 0;

    while(workerRunning)
    {
        usleep(100000 - (int)(lag * 1000000.0));
        std::lock_guard<std::mutex> guard(enet_mutex);
        if(curSnapshot > 0 && snapshots[curSnapshot].numEntities > 0)
        {
            WorldStateSnapshot *wss = &snapshots[curSnapshot];

            uint8_t buf[2048];
            int curLoc = 1;
            buf[0] = wss->numEntities;
            for(int i = 0; i < wss->numEntities; i++)
            {
                //if(wss->entities[i].entityId == 43)
                    //printf("here %d\n", i);
                writeu16(&buf[curLoc], wss->entities[i].entityId); // entity id
                buf[curLoc+2] = seq; // TODO
                writeVec3(&buf[curLoc+3], wss->entities[i].position);
                writeQuat(&buf[curLoc+15], wss->entities[i].rotation);
                curLoc += 31;
            }

            for(auto cpeer : connectedClients)
            {
                packet = enet_packet_create(buf, curLoc, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_peer_send(cpeer, 1, packet);
                enet_host_flush(server);
            }
        }

        cur = std::chrono::steady_clock::now();
        std::chrono::duration<double> timeDur = cur - startTime;
        double timePassed = timeDur.count();
        lag += timePassed - 0.1;
        startTime = cur;
        seq++;
    }
}

bool Server::init(uint16_t port)
{
    const char *errorstr = "Error occurred while initializeing ENet.";

    if(enet_initialize() != 0)
    {
        fprintf(stderr, "%s\n", errorstr);
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    server = enet_host_create(&address, 32, 2, 0, 0);
    if(server == NULL)
    {
        fprintf(stderr, "%s\n", errorstr);
        enet_deinitialize();
        return false;
    }

    players = new ServerPlayer[this->maxPlayers];
    for(int i = 0; i < this->maxPlayers; i++)
    {
        players[i].init(world);
    }

    workThread = new std::thread(&Server::serverProc, this);
    workerRunning = true;

    initialized = true;
}

void Server::deinit()
{
    if(!initialized)
        return;
    enet_deinitialize();
    enet_host_destroy(server);
    workerRunning = false;
    workThread->join();
    delete workThread;
}

int32_t Server::getFreePlayerSlot()
{
    for(int i = 0; i < maxPlayers; i++)
    {
        if(!FLAGSET(players[i].flags, ServerPlayer::Flags::InUse))
        {
            return i;
        }
    }
    return -1;
}

void Server::doEvents()
{
    ENetEvent event;
    const char *msg;
    ENetPacket *packet;
    std::lock_guard<std::mutex> guard(enet_mutex);
    while (enet_host_service (server, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            printf("A new client connected from %x:%u.\n", 
                    event.peer->address.host,
                    event.peer->address.port);
            msg = "Hello there client!";
            packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
            connectedClients.push_back(event.peer);

            int slot = getFreePlayerSlot();
            if(slot < 0) // out of player slots
            {
                event.peer->data = NULL;
                enet_peer_disconnect(event.peer, 0);
            }
            else
            {
                players[slot].markInUse(true);
                players[slot].spawn(Vec3(0.0f, 50.0f, 0.0f), Quaternion::Identity());
                printf("pent %d\n", players[slot].player.character->entity->id);
                players[slot].player.character->entity->transform.rotation = Quaternion::AngleAxis(45.0f, Vec3(0.0f, 1.0f, 0.0f));
                event.peer->data = &players[slot];
            }
        }
        break;

        case ENET_EVENT_TYPE_RECEIVE:

            if(event.packet->dataLength >= 2)
            {
                if(event.packet->data[0] == 0x03 && event.packet->data[1] == 0x00)
                {
                    InputClient *iclient = &((ServerPlayer*)event.peer->data)->inputClient;
                    uint8_t *data = (uint8_t*)event.packet->data;
                    printf("received input packet\n");
                    iclient->processContinuousData(&data[2], event.packet->dataLength - 2);
                    printf("Input: ");
                    bool p;
                    for(int i = 0; i < 4; i++)
                        printf("%d", iclient->getKey(0, i, p));
                    printf("\n");
                }
            }

            enet_packet_destroy(event.packet);
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
            auto iter = std::find(connectedClients.begin(), connectedClients.end(), event.peer);
            if(iter != connectedClients.end())
                connectedClients.erase(iter);
            printf("%x:%u disconnected.\n", event.peer->address.host, event.peer->address.port);
            if(event.peer->data != NULL)
            {
                auto player = (ServerPlayer*)event.peer->data;   
                player->markInUse(false);
            }
            event.peer->data = NULL;
            break;
        }
    }
}

void Server::takeSnapshot(World *world)
{
    int32_t next = (curSnapshot + 1) % NUM_SNAPSHOTS;
    WorldStateSnapshot *wss = &snapshots[next];

    int numCaptured = 0;
    for(int i = 0; i < MAX_ENTITIES; i++)
    {
        assert(numCaptured < MAX_SNAPSHOT_ENTITIES);
        Entity *e = &world->entities[i];
        if(e->inUse && e->active)
        {
            EntitySnapshot *ess = &wss->entities[numCaptured];
            ess->position = e->transform.position;
            ess->rotation = e->transform.rotation;
            ess->entityId = i;
            numCaptured++;
        }
    }
    wss->numEntities = numCaptured;

    curSnapshot = next;
}
