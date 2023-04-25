#pragma once

// THIS CODE IS AUTOMATICALLY GENERATED
// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR

typedef struct MsgPlayerReflection {
    uint8_t type;
    V3 position;
} MsgPlayerReflection;

typedef struct MsgServerInputConfig {
    uint8_t type;
    uint8_t count;
    struct {
        uint8_t type;
        uint8_t tag;
        uint8_t flags;
    } inputs[255];
} MsgServerInputConfig;

typedef struct MsgCreateDynEntities {
    uint8_t type;
    uint16_t count;
    struct {
        uint32_t objectId;
        uint32_t entityId;
        V3 position;
        V3 scale;
        Quat rotation;
    } entities[65535];
} MsgCreateDynEntities;

typedef struct MsgUpdateDynEntities {
    uint8_t type;
    uint16_t count;
    struct {
        uint32_t entityId;
        uint16_t sequence;
        V3 position;
        Quat rotation;
    } entities[65535];
} MsgUpdateDynEntities;

typedef struct MsgDestroyDynEntities {
    uint8_t type;
    uint16_t count;
    struct {
        uint32_t entityId;
    } entities[65535];
} MsgDestroyDynEntities;

typedef struct MsgPlayerSpawn {
    uint8_t type;
    V3 position;
} MsgPlayerSpawn;

typedef struct MsgPlayerDespawn {
    uint8_t type;
} MsgPlayerDespawn;

typedef struct MsgGetServerProperties {
    uint8_t type;
    uint8_t count;
    struct {
        uint8_t property;
    } properties[255];
} MsgGetServerProperties;

typedef struct MsgClientWorldReady {
    uint8_t type;
} MsgClientWorldReady;

typedef struct MsgGetServerInputs {
    uint8_t type;
} MsgGetServerInputs;

typedef struct MsgUpdateReliable {
    uint8_t type;
    uint16_t tickId;
    uint8_t count;
    struct {
        uint8_t event;
    } events[255];
} MsgUpdateReliable;

typedef struct MsgUpdateUnreliable {
    uint8_t type;
    uint16_t tickId;
    uint8_t count;
    struct {
        uint8_t data;
    } input[255];
    V2 camRot;
} MsgUpdateUnreliable;

void parse_client_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData);
void parse_server(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData);
static void message_player_reflection(MsgPlayerReflection *msg, void *usrPtr);
static void message_server_input_config(MsgServerInputConfig *msg, void *usrPtr);
static void message_create_dyn_entities(MsgCreateDynEntities *msg, void *usrPtr);
static void message_update_dyn_entities(MsgUpdateDynEntities *msg, void *usrPtr);
static void message_destroy_dyn_entities(MsgDestroyDynEntities *msg, void *usrPtr);
static void message_player_spawn(MsgPlayerSpawn *msg, void *usrPtr);
static void message_player_despawn(MsgPlayerDespawn *msg, void *usrPtr);
static void message_get_server_properties(MsgGetServerProperties *msg, void *usrPtr);
static void message_client_world_ready(MsgClientWorldReady *msg, void *usrPtr);
static void message_get_server_inputs(MsgGetServerInputs *msg, void *usrPtr);
static void message_update_reliable(MsgUpdateReliable *msg, void *usrPtr);
static void message_update_unreliable(MsgUpdateUnreliable *msg, void *usrPtr);

bool message_write_player_reflection(TessStack *stack, const MsgPlayerReflection* msg, uint8_t isserver, void *usrData);

bool message_write_server_input_config(TessStack *stack, const MsgServerInputConfig* msg, uint8_t isserver, void *usrData);

bool message_write_create_dyn_entities(TessStack *stack, const MsgCreateDynEntities* msg, uint8_t isserver, void *usrData);

bool message_write_update_dyn_entities(TessStack *stack, const MsgUpdateDynEntities* msg, uint8_t isserver, void *usrData);

bool message_write_destroy_dyn_entities(TessStack *stack, const MsgDestroyDynEntities* msg, uint8_t isserver, void *usrData);

bool message_write_player_spawn(TessStack *stack, const MsgPlayerSpawn* msg, uint8_t isserver, void *usrData);

bool message_write_player_despawn(TessStack *stack, const MsgPlayerDespawn* msg, uint8_t isserver, void *usrData);

bool message_write_get_server_properties(TessStack *stack, const MsgGetServerProperties* msg, uint8_t isserver, void *usrData);

bool message_write_client_world_ready(TessStack *stack, const MsgClientWorldReady* msg, uint8_t isserver, void *usrData);

bool message_write_get_server_inputs(TessStack *stack, const MsgGetServerInputs* msg, uint8_t isserver, void *usrData);

bool message_write_update_reliable(TessStack *stack, const MsgUpdateReliable* msg, uint8_t isserver, void *usrData);

bool message_write_update_unreliable(TessStack *stack, const MsgUpdateUnreliable* msg, uint8_t isserver, void *usrData);

static void server_send_data(uint8_t *data, uint32_t len, void *usrPtr);
static void client_send_data(uint8_t *data, uint32_t len, void *usrPtr);

//NOTE: this is basically function overloading for c11
#define server_message_send(Stream, X, UsrData) _Generic((X), \
MsgPlayerReflection*: message_write_player_reflection, \
MsgServerInputConfig*: message_write_server_input_config, \
MsgCreateDynEntities*: message_write_create_dyn_entities, \
MsgUpdateDynEntities*: message_write_update_dyn_entities, \
MsgDestroyDynEntities*: message_write_destroy_dyn_entities, \
MsgPlayerSpawn*: message_write_player_spawn, \
MsgPlayerDespawn*: message_write_player_despawn \
)(Stream, X, 1, UsrData)
#define client_message_send(Stream, X, UsrData) _Generic((X), \
MsgGetServerProperties*: message_write_get_server_properties, \
MsgClientWorldReady*: message_write_client_world_ready, \
MsgGetServerInputs*: message_write_get_server_inputs, \
MsgUpdateReliable*: message_write_update_reliable, \
MsgUpdateUnreliable*: message_write_update_unreliable \
)(Stream, X, 0,UsrData)
