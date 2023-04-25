#include "networkedinput.h"

#include <cstring>
#include <cstdio>
#include <assert.h>

#include <SDL2/SDL.h>

void InputServer::getContinuousData(uint16_t curFrame, uint8_t *buf, uint32_t bufSize, uint32_t *size)
{
    uint32_t bitFieldSize = (uint32_t)ceilf((maxStateKeyId + 1.0f) / 8.0f);
    uint32_t dataSize = bitFieldSize + (maxAnalogId * 2) + 3;

    assert(bitFieldSize <= 32);

    if(dataSize > bufSize)
    {
        fprintf(stderr, "Input buffer too small!\n");
        return;
    }

    memset(buf, 0, dataSize);
    memcpy(buf, &curFrame, sizeof(curFrame));
    buf[2] = (uint8_t)bitFieldSize;
    int curByte = 3;
    bool anyData = false;

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    for(int i = 0; i <= maxStateKeyId; i++)
    {
        bool keyState = false;
        if(trackedStateKeys[i].tracked)
        {
            uint32_t scancode = SDL_GetScancodeFromKey(trackedStateKeys[i].key);         
            keyState = state[scancode] != 0;
        }
        uint32_t byteIdx = i / 8;
        assert(byteIdx < bitFieldSize);
        uint32_t bit = i % 8;
        buf[curByte] |= keyState << bit;
        curByte = byteIdx + 3;
        anyData = true;
    }

    for(int i = 0; i < maxAnalogId; i++)
    {
        int16_t analogState = 0;
        if(trackedAnalogs[i].tracked)
            analogState = 0; // TODO
        memcpy(&buf[curByte], &analogState, sizeof(analogState));
        curByte += sizeof(analogState);
        anyData = true;
    }

    if(anyData)
        *size = dataSize;
    else
        *size = 0;
}

void InputServer::getEventData(uint16_t curFrame, uint8_t *buf, uint32_t bufSize, uint32_t *size)
{
    *size = 0;
}

void InputServer::addEdgeKey(SDL_Keycode key, uint8_t netId)
{
    if(netId >= MAX_TRACKED_KEYS)
    {
        fprintf(stderr, "addEdgeKey netId out of range! Not tracking %d/%d\n", key, netId);
        return;
    }
    TrackedKey *ckey = &trackedEdgeKeys[netId];
    ckey->key = key;
    ckey->netId = netId;
    ckey->tracked = true;
}

void InputServer::addStateKey(SDL_Keycode key, uint8_t netId)
{
    if(netId >= MAX_TRACKED_KEYS)
    {
        fprintf(stderr, "addStateKey netId out of range! Not tracking %d/%d\n", key, netId);
        return;
    }
    TrackedKey *ckey = &trackedStateKeys[netId];
    ckey->key = key;
    ckey->netId = netId;
    ckey->tracked = true;
    if(netId > maxStateKeyId)
        maxStateKeyId = netId;
}

void InputServer::addAnalog(SDL_Keycode axis, uint8_t netId)
{
    if(netId >= MAX_TRACKED_ANALOGS)
    {
        fprintf(stderr, "addAnalog netId out of range! Not tracking %d\%d\n", axis, netId);
        return;
    }
    TrackedKey *ckey = &trackedAnalogs[netId];
    //ckey->key = axis; // TODO
    ckey->netId = netId;
    ckey->tracked = true;
    if(netId > maxAnalogId)
        maxAnalogId = netId;
}

void InputClient::processContinuousData(uint8_t *data, uint32_t size)
{
    if(size < 4) // must contain frameId, numKeyBytes and 1 key or analog state
        return;

    uint16_t frameId = data[0] | (data[1] << 8);
    uint32_t historyIndex = frameId % INPUT_HISTORY_SIZE;
    uint8_t keyBytes = data[2];

    if(size < keyBytes + 3) // corrupted
    {
        fprintf(stderr, "Input packet: too small\n");
        return;
    }
    if(keyBytes * 8 > MAX_TRACKED_KEYS) // too many keys
    {
        fprintf(stderr, "Input packet: too many keys\n");
        return;
    }

    uint32_t numAnalogBytes = size - keyBytes - 3;
    uint32_t numAnalogs = numAnalogBytes / 2;

    if(numAnalogBytes & 1) // analogs are 2 bytes - can't be odd
    {
        fprintf(stderr, "Input packet: odd number of analog bytes\n");
        return;
    }
    if(numAnalogs > MAX_TRACKED_ANALOGS) // too many analogs
    {
        fprintf(stderr, "Input packet: too many analogs\n");
        return;
    }
    
    uint32_t curLocation = 3;
    for(int i = 0; i < keyBytes; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            uint32_t id = i*8 + j;
            if(id >= MAX_TRACKED_KEYS)
            {
                fprintf(stderr, "Input packet: key id out of range\n");
                continue;
            }
            bool value = ((data[curLocation] >> j) & 1) != 0;
            /*if(value)
                printf("key %d down\n", id);*/
            
            keyHistory[historyIndex].keys[id] = value;
        }
        curLocation++;
    }    

    for(int i = 0; i < numAnalogs; i++)
    {
        uint16_t value = data[curLocation] | (data[curLocation+1] << 8);
        // TODO: set in history
        curLocation += 2;
    }

    keyHistory[historyIndex].frameId = frameId;
    assert(curLocation == size); // if everything was read then current location must be at end
}

bool InputClient::getKey(uint16_t frameId, int netId, bool& predicted)
{
    uint32_t historyIndex = frameId % INPUT_HISTORY_SIZE;
    auto kEntry = &keyHistory[frameId];
    if(kEntry->frameId == frameId)
    {
        predicted = false;
        return kEntry->keys[netId];
    }
    else
    {
        predicted = true;
        fprintf(stderr, "InputClient::getKey - input not present for frame %d", frameId);
        return false; // TODO: predict value based on past of future
    }
}


