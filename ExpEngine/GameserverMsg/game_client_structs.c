#include "game_server_structs.h"

// THIS CODE IS AUTOMATICALLY GENERATED
// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR

void parse_client_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData) {
    uint8_t type;
    stream_read_uint8(s, &type);
    uint32_t success = 1;
    switch(type) {
    case 111: // MsgPlayerReflection
    {
        MsgPlayerReflection* msgplayerreflection = (MsgPlayerReflection*)structBuf;
        success &= stream_read_v3(s, &msgplayerreflection->position);
        if(success) {
            message_player_reflection((MsgPlayerReflection*)structBuf, usrData);
        }
    } break;
    case 112: // MsgServerInputConfig
    {
        MsgServerInputConfig* msgserverinputconfig = (MsgServerInputConfig*)structBuf;
        success &= stream_read_uint8(s, &msgserverinputconfig->count);
        success &= msgserverinputconfig->count <= 255;
        for(int i = 0; success && i < msgserverinputconfig->count; i++) {
            success &= stream_read_uint8(s, &msgserverinputconfig->inputs[i].type);
            success &= stream_read_uint8(s, &msgserverinputconfig->inputs[i].tag);
            success &= stream_read_uint8(s, &msgserverinputconfig->inputs[i].flags);
        }
        if(success) {
            message_server_input_config((MsgServerInputConfig*)structBuf, usrData);
        }
    } break;
    case 113: // MsgCreateDynEntities
    {
        MsgCreateDynEntities* msgcreatedynentities = (MsgCreateDynEntities*)structBuf;
        success &= stream_read_uint16(s, &msgcreatedynentities->count);
        success &= msgcreatedynentities->count <= 65535;
        for(int i = 0; success && i < msgcreatedynentities->count; i++) {
            success &= stream_read_uint32(s, &msgcreatedynentities->entities[i].objectId);
            success &= stream_read_uint32(s, &msgcreatedynentities->entities[i].entityId);
            success &= stream_read_v3(s, &msgcreatedynentities->entities[i].position);
            success &= stream_read_v3(s, &msgcreatedynentities->entities[i].scale);
            success &= stream_read_quat(s, &msgcreatedynentities->entities[i].rotation);
        }
        if(success) {
            message_create_dyn_entities((MsgCreateDynEntities*)structBuf, usrData);
        }
    } break;
    case 114: // MsgUpdateDynEntities
    {
        MsgUpdateDynEntities* msgupdatedynentities = (MsgUpdateDynEntities*)structBuf;
        success &= stream_read_uint16(s, &msgupdatedynentities->count);
        success &= msgupdatedynentities->count <= 65535;
        for(int i = 0; success && i < msgupdatedynentities->count; i++) {
            success &= stream_read_uint32(s, &msgupdatedynentities->entities[i].entityId);
            success &= stream_read_uint16(s, &msgupdatedynentities->entities[i].sequence);
            success &= stream_read_v3(s, &msgupdatedynentities->entities[i].position);
            success &= stream_read_quat(s, &msgupdatedynentities->entities[i].rotation);
        }
        if(success) {
            message_update_dyn_entities((MsgUpdateDynEntities*)structBuf, usrData);
        }
    } break;
    case 115: // MsgDestroyDynEntities
    {
        MsgDestroyDynEntities* msgdestroydynentities = (MsgDestroyDynEntities*)structBuf;
        success &= stream_read_uint16(s, &msgdestroydynentities->count);
        success &= msgdestroydynentities->count <= 65535;
        for(int i = 0; success && i < msgdestroydynentities->count; i++) {
            success &= stream_read_uint32(s, &msgdestroydynentities->entities[i].entityId);
        }
        if(success) {
            message_destroy_dyn_entities((MsgDestroyDynEntities*)structBuf, usrData);
        }
    } break;
    case 116: // MsgPlayerSpawn
    {
        MsgPlayerSpawn* msgplayerspawn = (MsgPlayerSpawn*)structBuf;
        success &= stream_read_v3(s, &msgplayerspawn->position);
        if(success) {
            message_player_spawn((MsgPlayerSpawn*)structBuf, usrData);
        }
    } break;
    case 117: // MsgPlayerDespawn
    {
        MsgPlayerDespawn* msgplayerdespawn = (MsgPlayerDespawn*)structBuf;
        if(success) {
            message_player_despawn((MsgPlayerDespawn*)structBuf, usrData);
        }
    } break;
    }
}
bool message_write_get_server_properties(TessStack *stack, const MsgGetServerProperties* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 118);
    success &= stream_write_uint8(&s, msg->count);
    success &= msg->count <= 255;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint8(&s, msg->properties[i].property);
    }
    client_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_client_world_ready(TessStack *stack, const MsgClientWorldReady* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 119);
    client_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_get_server_inputs(TessStack *stack, const MsgGetServerInputs* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 120);
    client_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_update_reliable(TessStack *stack, const MsgUpdateReliable* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 121);
    success &= stream_write_uint16(&s, msg->tickId);
    success &= stream_write_uint8(&s, msg->count);
    success &= msg->count <= 255;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint8(&s, msg->events[i].event);
    }
    client_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_update_unreliable(TessStack *stack, const MsgUpdateUnreliable* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 122);
    success &= stream_write_uint16(&s, msg->tickId);
    success &= stream_write_uint8(&s, msg->count);
    success &= msg->count <= 255;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint8(&s, msg->input[i].data);
    }
    success &= stream_write_v2(&s, msg->camRot);
    client_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
