#include "game_server_structs.h"

// THIS CODE IS AUTOMATICALLY GENERATED
// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR

void parse_server_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData) {
    uint8_t type;
    stream_read_uint8(s, &type);
    uint32_t success = 1;
    switch(type) {
    case 118: // MsgGetServerProperties
    {
        MsgGetServerProperties* msggetserverproperties = (MsgGetServerProperties*)structBuf;
        success &= stream_read_uint8(s, &msggetserverproperties->count);
        success &= msggetserverproperties->count <= 255;
        for(int i = 0; success && i < msggetserverproperties->count; i++) {
            success &= stream_read_uint8(s, &msggetserverproperties->properties[i].property);
        }
        if(success) {
            message_get_server_properties((MsgGetServerProperties*)structBuf, usrData);
        }
    } break;
    case 119: // MsgClientWorldReady
    {
        MsgClientWorldReady* msgclientworldready = (MsgClientWorldReady*)structBuf;
        if(success) {
            message_client_world_ready((MsgClientWorldReady*)structBuf, usrData);
        }
    } break;
    case 120: // MsgGetServerInputs
    {
        MsgGetServerInputs* msggetserverinputs = (MsgGetServerInputs*)structBuf;
        if(success) {
            message_get_server_inputs((MsgGetServerInputs*)structBuf, usrData);
        }
    } break;
    case 121: // MsgUpdateReliable
    {
        MsgUpdateReliable* msgupdatereliable = (MsgUpdateReliable*)structBuf;
        success &= stream_read_uint16(s, &msgupdatereliable->tickId);
        success &= stream_read_uint8(s, &msgupdatereliable->count);
        success &= msgupdatereliable->count <= 255;
        for(int i = 0; success && i < msgupdatereliable->count; i++) {
            success &= stream_read_uint8(s, &msgupdatereliable->events[i].event);
        }
        if(success) {
            message_update_reliable((MsgUpdateReliable*)structBuf, usrData);
        }
    } break;
    case 122: // MsgUpdateUnreliable
    {
        MsgUpdateUnreliable* msgupdateunreliable = (MsgUpdateUnreliable*)structBuf;
        success &= stream_read_uint16(s, &msgupdateunreliable->tickId);
        success &= stream_read_uint8(s, &msgupdateunreliable->count);
        success &= msgupdateunreliable->count <= 255;
        for(int i = 0; success && i < msgupdateunreliable->count; i++) {
            success &= stream_read_uint8(s, &msgupdateunreliable->input[i].data);
        }
        success &= stream_read_v2(s, &msgupdateunreliable->camRot);
        if(success) {
            message_update_unreliable((MsgUpdateUnreliable*)structBuf, usrData);
        }
    } break;
    }
}
bool message_write_player_reflection(TessStack *stack, const MsgPlayerReflection* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 111);
    success &= stream_write_v3(&s, msg->position);
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_server_input_config(TessStack *stack, const MsgServerInputConfig* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 112);
    success &= stream_write_uint8(&s, msg->count);
    success &= msg->count <= 255;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint8(&s, msg->inputs[i].type);
        success &= stream_write_uint8(&s, msg->inputs[i].tag);
        success &= stream_write_uint8(&s, msg->inputs[i].flags);
    }
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_create_dyn_entities(TessStack *stack, const MsgCreateDynEntities* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 113);
    success &= stream_write_uint16(&s, msg->count);
    success &= msg->count <= 65535;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint32(&s, msg->entities[i].objectId);
        success &= stream_write_uint32(&s, msg->entities[i].entityId);
        success &= stream_write_v3(&s, msg->entities[i].position);
        success &= stream_write_v3(&s, msg->entities[i].scale);
        success &= stream_write_quat(&s, msg->entities[i].rotation);
    }
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_update_dyn_entities(TessStack *stack, const MsgUpdateDynEntities* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 114);
    success &= stream_write_uint16(&s, msg->count);
    success &= msg->count <= 65535;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint32(&s, msg->entities[i].entityId);
        success &= stream_write_uint16(&s, msg->entities[i].sequence);
        success &= stream_write_v3(&s, msg->entities[i].position);
        success &= stream_write_quat(&s, msg->entities[i].rotation);
    }
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_destroy_dyn_entities(TessStack *stack, const MsgDestroyDynEntities* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 115);
    success &= stream_write_uint16(&s, msg->count);
    success &= msg->count <= 65535;
    for(int i = 0; success && i < msg->count; i++) {
        success &= stream_write_uint32(&s, msg->entities[i].entityId);
    }
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_player_spawn(TessStack *stack, const MsgPlayerSpawn* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 116);
    success &= stream_write_v3(&s, msg->position);
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
bool message_write_player_despawn(TessStack *stack, const MsgPlayerDespawn* msg, uint8_t isserver, void *usrData) {
    uint8_t *mem = stack_push(stack, 65536, 4);
    ByteStream s;
init_byte_stream(&s, mem, 65536);
    uint8_t success = 1;
    success &= stream_write_uint8(&s, 117);
    server_send_data(s.start, stream_get_offset(&s), usrData);
    stack_pop(stack);
    return success;
}
