#pragma once

#define SERVER_PLAYER_INPUT_MAX_EVENTS 256
#define SERVER_INPUT_DESCRIPTION_LENGTH 128
#define SERVER_MAX_INPUTS 16

typedef enum ServerInputType{
    SERVER_INPUT_TYPE_NONE,
    SERVER_INPUT_TYPE_KEY,
    SERVER_INPUT_TYPE_ANALOG,
    SERVER_INPUT_TYPE_SPHERICAL,
    SERVER_INPUT_TYPE_COUNT
} ServerInputType;

typedef enum ServerInputTag {
    SERVER_INPUT_TAG_NONE,
    SERVER_INPUT_TAG_WALK_FORWARD,
    SERVER_INPUT_TAG_WALK_BACKWARD,
    SERVER_INPUT_TAG_WALK_RIGHT,
    SERVER_INPUT_TAG_WALK_LEFT,
    SERVER_INPUT_TAG_WALK_JUMP,
    SERVER_INPUT_TAG_WALK_ATTACK,
    SERVER_INPUT_TAG_WALK_ACTION,
    SERVER_INPUT_TAG_WALK_MOUSELOOK,
    SERVER_INPUT_TAG_COUNT,
} ServerInputTag;

typedef enum ServerInputKeyFlag {
    SERVER_INPUT_KEY_FLAG_NONE,
    SERVER_INPUT_KEY_FLAG_TRACK_STATE = 1<<0,
    SERVER_INPUT_KEY_FLAG_TRACK_DOWN_EVENT = 1<<1,
    SERVER_INPUT_KEY_FLAG_TRACK_UP_EVENT  = 1<<2
} ServerInputKeyFlag;

typedef enum ServerInputAnalogFlag {
    SERVER_INPUT_ANALOG_FLAG_NONE,
    SERVER_INPUT_ANALOG_FLAG_TRACK_STATE = 1<<0
} ServerInputAnalogFlag;

typedef enum ServerInputSphereFlag {
    SERVER_INPUT_SPHERE_FLAG_NONE,
    SERVER_INPUT_SPHERE_FLAG_TRACK_STATE = 1<<0
} ServerInputSphereFlag;

// input config entry
typedef struct ServerInput {
    uint8_t type;
    uint8_t tag;
    uint8_t flags;
    //char description[SERVER_INPUT_DESCRIPTION_LENGTH];
} ServerInput;

typedef struct ServerInputConfig {
    uint8_t inputCount;
    ServerInput inputs[SERVER_MAX_INPUTS]; 
    uint8_t trackedKeyCount;
    uint8_t trackedKeyToInputId[SERVER_MAX_INPUTS];
} ServerInputConfig;

typedef struct ClientTrackedKey {
    uint8_t inputId; // server input id
    uint16_t platformKeycode;
} ClientTrackedKey;

typedef struct ClientEventKey {
    uint8_t inputId; // server input id
    uint16_t platformKeycode;
} ClientEventKey;

typedef struct ClientInputConfig {
    uint8_t inputCount;
    uint8_t trackedKeyCount;
    uint8_t eventKeyCount;
    ClientTrackedKey trackedKeys[SERVER_MAX_INPUTS];
    ClientEventKey eventKeys[SERVER_MAX_INPUTS];
    ServerInput serverInputs[SERVER_MAX_INPUTS];
} ClientInputConfig;

enum ServerPlayerInputEventType{
    Server_Player_Input_Event_Key_Up,
    Server_Player_Input_Event_Key_Down,
    Server_Player_Input_Event_Count,
};

typedef struct ServerPlayerInputEvent {
    uint8_t type;
    union {
        struct {
            uint16_t serverInputId;
        } keyEdge;
    };
} ServerPlayerInputEvent;

typedef struct ServerPlayerInput {
    uint16_t keyStates[SERVER_MAX_INPUTS];
    // TODO: support more than 1?
    V2 sphericalState;
    uint16_t pendingEventCount;
    ServerPlayerInputEvent pendingEvents[SERVER_PLAYER_INPUT_MAX_EVENTS];
} ServerPlayerInput;
