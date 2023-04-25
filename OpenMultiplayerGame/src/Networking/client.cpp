#include "client.h"

#include <enet/enet.h>
#include <cstdio>
#include <cstring>
#include "../GameWorld/world.h"

bool Client::init()
{
    const char *errorstr = "Error occurred while initializeing ENet.";

    if(enet_initialize() != 0)
    {
        fprintf(stderr, "%s\n", errorstr);
        return false;
    }

    client = enet_host_create(NULL, 1, 2, 0, 0);
    if(client == NULL)
    {
        fprintf(stderr, "%s\n", errorstr);
        enet_deinitialize();
        return false;
    }

    for(int i = 0; i < 100; i++)
    {
        transforms[i].transform = &entities[i].transform;
    }

    initialized = true;
    lastUpdate = std::chrono::steady_clock::now();
}

void Client::deinit()
{
    if(!initialized)
        return;
    enet_deinitialize();
    enet_host_destroy(client);
    client = NULL;
}

bool Client::connect(const char* hostName, unsigned short port)
{
    ENetAddress address;
    enet_address_set_host(&address, hostName);
    address.port = port;
    peer = enet_host_connect (client, &address, 2, 0);  
    if(peer == NULL)
    {
       fprintf(stderr, "No available peers for initiating an ENet connection.\n");
       exit(EXIT_FAILURE);
    }
    
    ENetEvent event;
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        printf("Connection to %s:%d succeeded.", hostName, port);
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset (peer);
        printf("Connection to %s:%d failed.", hostName, port);
        return false;
    }

    inputServer.addStateKey(SDLK_w, 0);
    inputServer.addStateKey(SDLK_a, 1);
    inputServer.addStateKey(SDLK_s, 2);
    inputServer.addStateKey(SDLK_d, 3);

    return true;
}

uint8_t readu8(uint8_t *data)
{
    return data[0];
}

uint16_t readu16(uint8_t *data)
{
    return data[0] | (data[1] << 8);
}

Vec3 readVec3(uint8_t *data)
{
    Vec3 ret;
    memcpy(&ret, data, sizeof(Vec3));
    return ret;
}

Quaternion readQuat(uint8_t *data)
{
    Quaternion ret;
    memcpy(&ret, data, sizeof(Quaternion));
    return ret;
}

#pragma pack(push, 1)
struct PosUpdate
{
    uint16_t entityId;
    uint8_t seq;
    Vec3 position;
    Quaternion rotation;
};
#pragma pack(pop)

void Client::updateTransforms()
{
    std::chrono::steady_clock::time_point curT = std::chrono::steady_clock::now();
    std::chrono::duration<double> difD = curT - lastUpdate;
    lastUpdate = curT;
    double dt = difD.count();

    for(int i = 0; i < 100; i++)
    {
        if(transforms[i].playing)
        {
            double pc = fmod(transforms[i].playCursor * 10.0, NT_HISTORY_SIZE);
            double t = fmod(pc, 1);
            uint32_t starti = (int)pc;
            assert(starti < NT_HISTORY_SIZE);
            uint32_t endi = (starti + 1) % NT_HISTORY_SIZE;

            Vec3 pos1 = transforms[i].positions[starti];
            Vec3 pos2 = transforms[i].positions[endi];
            Quaternion rot1 = transforms[i].rotations[starti];
            Quaternion rot2 = transforms[i].rotations[endi];

            transforms[i].transform->position = pos1 + t*(pos2-pos1);
            transforms[i].transform->rotation = Quaternion::NLerp(rot1, rot2, t);
            Quaternion w = transforms[i].transform->rotation;

            transforms[i].playCursor += dt;
        }
    }
}

void Client::sendFrameInputs(uint16_t frameId)
{
    uint8_t buf[2048];

    buf[0] = 0x03;
    buf[1] = 0;

    uint32_t curLocation = 2;

    uint32_t inputDataSize;
    inputServer.getContinuousData(frameId, &buf[curLocation], sizeof(buf) - curLocation, &inputDataSize);
    curLocation += inputDataSize;

    if(curLocation > 2)
    {
        //printf("send %d bytes of input data\n", curLocation);
        ENetPacket *packet;
        packet = enet_packet_create(buf, curLocation, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(peer, 1, packet);
        enet_host_flush(client);
    }
}

bool is_newer(uint8_t a, uint8_t b)
{
    // TODO: branchless solution maybe?
    if(a > b && a - b < 128)
        return true;
    else if(a < b && b - a > 128)
        return true;
    return false;
}

uint8_t smallestDif(uint8_t a, uint8_t b)
{
    if(a > b && a - b < 128)
        return a - b;
    else if(a > b && a - b >= 128)
        return 256 - a + b;
    else if(b > a && b - a < 128)
        return b - a;
    else if(b > a && b - a >= 128)
        return 256 - b + a;
    else
        return 0;
}

void Client::readPositionUpdate(uint8_t *data, uint32_t length)
{
    int count = readu8(&data[0]);
    int expectedSize = sizeof(PosUpdate) * count + 1;
    if(length != expectedSize)
    {
        fprintf(stderr, "Corrupted position update message! Expected size %d was %d\n", expectedSize, length);
        return;
    }
    uint32_t curLoc = 1;
    for(int i = 0; i < count; i++)
    {
        assert(curLoc + sizeof(PosUpdate) - 1 < length);
        PosUpdate posu;
        posu.entityId = readu16(&data[curLoc]);
        posu.seq = readu8(&data[curLoc + 2]);
        posu.position = readVec3(&data[curLoc + 3]);
        posu.rotation = readQuat(&data[curLoc + 15]);
        curLoc += sizeof(PosUpdate);

        if(i == 12)
            printf("??? %f %f %f %f\n", posu.position.x, posu.position.y, posu.position.z, posu.rotation.w);

        // play transform
        NetworkedTransform *ntrans = &transforms[posu.entityId];

        uint32_t index;
        uint32_t dif;

        if(ntrans->latestIndex < 0)
        {
            ntrans->playCursor = 0.0f;
            ntrans->playing = false;
            ntrans->playSpeed = 10.0f;

            for(int i = 0; i < NT_HISTORY_SIZE; i++)
            {
                ntrans->positions[i] = posu.position;
                ntrans->rotations[i] = posu.rotation;
            }
            dif = 1;
        }
        else
        {
            dif = smallestDif(posu.seq, ntrans->latestSeq);
            if(!is_newer(posu.seq, ntrans->latestSeq))
                continue;
        }

        index = (ntrans->latestIndex + dif) % NT_HISTORY_SIZE;

        ntrans->latestSeq = posu.seq;
        ntrans->positions[index] = posu.position;
        ntrans->rotations[index] = posu.rotation;
        
        float interpStep = 1.0f / dif;
        for(int i = 0; i < dif - 1; i++)
        {
            float t = (i+1) * interpStep;
            int uidx = ((int)ntrans->latestIndex + i + 1) % NT_HISTORY_SIZE;
            Vec3 oldPos = ntrans->positions[ntrans->latestIndex];
            Quaternion oldRot = ntrans->rotations[ntrans->latestIndex];
            ntrans->positions[uidx] = oldPos + t*(posu.position-oldPos);
            ntrans->rotations[uidx] = Quaternion::NLerp(oldRot, posu.rotation, t);
        }

        ntrans->latestIndex = index;
        if(!ntrans->playing && ntrans->latestIndex - ntrans->playCursor >= 2.0f)
        {
            ntrans->playing = true;
        }
    }
}

void Client::doEvents()
{
    ENetEvent event;
    while (enet_host_service (client, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n", 
                    event.peer->address.host,
                    event.peer->address.port);
            /* Store any relevant client information here. */
            //event.peer->data = "Client information";
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            /*printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);*/

            if(event.channelID == 1)
                readPositionUpdate((uint8_t*)event.packet->data, event.packet->dataLength);

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);
            
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
            printf ("%x:%u disconnected.\n", event.peer->address.host, event.peer->address.port);
            event.peer->data = NULL;
            break;
        }
    }
}
