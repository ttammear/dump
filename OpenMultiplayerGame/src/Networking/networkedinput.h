#pragma once

#include <stdint.h>

#include <SDL2/SDL_keycode.h>

#define MAX_TRACKED_KEYS 64
#define MAX_TRACKED_ANALOGS 16

struct KeyStates
{
    uint32_t frameId;

    uint8_t keys[MAX_TRACKED_KEYS];
    uint32_t analogs[MAX_TRACKED_ANALOGS];
    uint32_t downCount;
    uint32_t keyDownEvents[256];
    uint32_t upCount;
    uint32_t keyUpEvents[256];
};

struct TrackedKey
{
    SDL_Keycode key;
    uint8_t netId;
    bool tracked = false;
};

struct InputServer
{
    void addEdgeKey(SDL_Keycode key, uint8_t netId);
    void addStateKey(SDL_Keycode key, uint8_t netId);
    void addAnalog(int axis, uint8_t netId);

    void removeEdgeKey(int netId);
    void removeStateKey(int netId);
    void removeAnalog(int netId);

    void getContinuousData(uint16_t currentFrame, uint8_t *buf, uint32_t bufSize, uint32_t *size);
    void getEventData(uint16_t currentFrame, uint8_t *buf, uint32_t bufSize, uint32_t *size);

    uint32_t maxStateKeyId = 0;
    uint32_t maxAnalogId = 0;

    TrackedKey trackedEdgeKeys[MAX_TRACKED_KEYS];
    TrackedKey trackedStateKeys[MAX_TRACKED_KEYS];
    TrackedKey trackedAnalogs[MAX_TRACKED_KEYS];
};

struct KeyEvent
{
    enum EventId
    {
        Down,
        Up
    };

    int key;
    EventId event;
    uint16_t frameId;
};

#define INPUT_HISTORY_SIZE 32

struct InputClient
{
    bool getNextEvent(KeyEvent &event);
    bool getKey(uint16_t frameId, int netId, bool& predicted);
    float getAnalog(uint16_t frameId, int netId, bool& predicted);

    void processContinuousData(uint8_t *data, uint32_t size);
    void processEventData(uint8_t *data, uint32_t size);
    KeyStates keyHistory[INPUT_HISTORY_SIZE];
    int32_t latestData = -1;
};
