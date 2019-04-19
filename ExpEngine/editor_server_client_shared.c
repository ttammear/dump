#pragma pack(push, 1)

typedef struct EditorCommandHeader
{
    uint16_t size;
    uint16_t cmd;
    uint8_t data[];
} EditorCommandHeader;

typedef struct ObjectDefinition
{
    uint32_t objectId;
    uint8_t assetIdLen;
    char assetId[]; // packagename/assetname
} ObjectDefiniton;

typedef struct EditorCommandDefineObjects
{
    uint16_t numEntries;
    struct ObjectDefinition objects[];
} EditorCommandDefineObjects;

typedef struct EditorCommandDebugMessage
{
    uint16_t msgLen;
    char msg[];
} EditorCommandDebugMessage;

typedef struct CreateEntityEntry
{
    uint32_t id;
    uint32_t objectId;
    V3 position;
    V3 eulerRotation;
    V3 scale;
} CreateEntityEntry;

typedef struct ServerCreateEntityEntry
{
    uint32_t objectId;
    V3 position;
    V3 eulerRotation;
    V3 scale;
} ServerCreateEntityEntry;

typedef struct ServerTransformEntityEntry
{
    uint32_t serverId; // server entity id
    V3 position;
    V3 eulerRotation;
    V3 scale;
} ServerTransformEntityEntry;

typedef struct EditorCommandCreateEntities
{
    uint16_t numEntities;
    struct CreateEntityEntry entries[];
} EditorCommandCreateEntities;

typedef struct EditorCommandServerCreateEntities
{
    uint16_t numEntities;
    struct ServerCreateEntityEntry entries[];
} EditorCommandServerCreateEntities;

#pragma pack(pop)

enum EditorServerCommandType
{
    Editor_Server_Command_Nop,
    Editor_Server_Command_Set_Register,
    Editor_Server_Command_Request_Object_Definitions,
    Editor_Server_Command_Define_Objects,
    Editor_Server_Command_Create_Entities, // server->client
    Editor_Server_Command_Server_Create_Entities, // client->server
    Editor_Server_Command_Move_Entity,
    Editor_Server_Command_Transform_Entities,
    Editor_Server_Command_Destroy_Entities, // server<->client
    Editor_Server_Command_Debug_Message,
};

typedef struct ByteStream
{
    uint8_t *start;
    uint8_t *cur;
    uint8_t *end;
} ByteStream;

#define member_size(type, member) sizeof(((type *)0)->member)

/* macros to create a command of structure
 * CommandHeader
 * Data (use stream_write_xxx here and call ARRAY_COMMAND_HEADER_END)
 * uint16_t entryCount
 *   entry1 (use stream_write_xxx here and call ARRAY_COMMAND_CHECK
 *   entry2 (same as ^^^^^)
 *   ...
 * use ARRAY_COMMAND_CHECK with flush=true
*/

#define ARRAY_COMMAND_START(stream) \
    uint32_t entryCountLoc, curLoc;\
    uint8_t buf[TESS_EDITOR_SERVER_MAX_COMMAND_SIZE];\
    struct ByteStream (stream);\
    init_byte_stream(&(stream), buf, sizeof(buf));\
    stream_write_command_header(&(stream), 0, 0);

#define ARRAY_COMMAND_HEADER_END(stream)\
    entryCountLoc = stream_get_offset(&(stream));\
    stream_advance(&(stream), 2);\
    curLoc = stream_get_offset(&(stream));

#define ARRAY_COMMAND_WRITE_HEADER(stream, cmd, numEntries)\
    stream_go_to_offset(&(stream), 0);\
    stream_write_command_header(&(stream), curLoc, cmd);\
    stream_go_to_offset(&(stream), entryCountLoc);\
    stream_write_uint16(&(stream), (numEntries));\
    stream_go_to_offset(&(stream), curLoc);\

#define ARRAY_COMMAND_INC(stream) \
    curLoc = stream_get_offset(&(stream));

#define ARRAY_COMMAND_RESET(stream)\
    stream_go_to_offset(&(stream), entryCountLoc + 2);\
    curLoc = stream_get_offset(&stream);

void editor_server_process_client_command(struct TessEditorServer *server, struct TessEditorServerClient *client, uint32_t cmd, uint8_t *data, uint32_t dataSize);
void editor_client_process_command(struct TessEditor *editor, uint16_t cmd, uint8_t *data, uint32_t size);

void init_byte_stream(struct ByteStream *stream, void *buf, size_t len)
{
    stream->start = (uint8_t*)buf;
    stream->cur = stream->start;
    stream->end = stream->start + len;
}

void stream_go_to_offset(struct ByteStream *stream, uint32_t offset)
{
    assert(stream->start + offset <= stream->end);
    stream->cur = stream->start + offset;
}

static inline uint32_t stream_get_bytes_left(struct ByteStream *stream)
{
    return stream->end - stream->cur;
}

static inline uint32_t stream_get_offset(struct ByteStream *stream)
{
    return stream->cur - stream->start;
}

static inline bool stream_advance(struct ByteStream *stream, uint32_t count)
{
    if(stream->cur + count > stream->end)
        return false;
    stream->cur += count;
    return true;
}

static inline bool stream_write_v3(struct ByteStream *stream, struct V3 vec)
{
    if(stream->cur + 12 > stream->end)
        return false;
    // TODO: not sure if this is safe on all platforms
    uint8_t *vdata = (uint8_t*)&vec;
    for(int i = 0; i < 12; i++)
        stream->cur[i] = vdata[i];
    stream->cur += 12;
    return true;
}

static inline bool stream_write_quat(struct ByteStream *stream, struct Quat vec)
{
    if(stream->cur + 16 > stream->end)
        return false;
    uint8_t *vdata = (uint8_t*)&vec;
    for(int i = 0; i < 16; i++)
        stream->cur[i] = vdata[i];
    stream->cur += 16;
    return true;
}

static inline bool stream_write_uint32(struct ByteStream *stream, uint32_t value)
{
    if(stream->cur + 4 > stream->end)
        return false;
    stream->cur[0] = (uint8_t)value;
    stream->cur[1] = (uint8_t)(value>>8);
    stream->cur[2] = (uint8_t)(value>>16);
    stream->cur[3] = (uint8_t)(value>>24);
    stream->cur += 4;
    return true;
}

static inline bool stream_write_uint16(struct ByteStream *stream, uint16_t value)
{
    if(stream->cur + 2 > stream->end)
        return false;
    stream->cur[0] = (uint8_t)value;
    stream->cur[1] = (uint8_t)(value>>8);
    stream->cur += 2;
    return true;
}

static inline bool stream_write_uint8(struct ByteStream *stream, uint8_t value)
{
    if(stream->cur + 1 > stream->end)
        return false;
    stream->cur[0] = value;
    stream->cur++;
    return true;
}

static inline bool stream_write_str(struct ByteStream *stream, const char *str, uint32_t len)
{
    if(stream->cur + len > stream->end)
        return false;
    for(int i = 0; i < len; i++)
        stream->cur[i] = str[i];
    stream->cur += len;
    return true;
}

static inline bool stream_read_v3(struct ByteStream *stream, struct V3 *value)
{
    static_assert(sizeof(struct V3) == 12, "V3 assumed to have size 12!");
    if(stream->cur + 12 > stream->end)
        return false;
    // TODO: is this safe on all platforms?
    uint8_t *valB = (uint8_t*)value;
    for(int i = 0; i < 12; i++)
        valB[i] = stream->cur[i];
    stream->cur += 12;
    return true;
}

static inline bool stream_read_quat(struct ByteStream *stream, struct Quat *value)
{
    static_assert(sizeof(struct Quat) == 16, "Quat assumed to have size 16!");
    if(stream->cur + 16 > stream->end)
        return false;
    // TODO: is this safe on all platforms?
    uint8_t *valB = (uint8_t*)value;
    for(int i = 0; i < 16; i++)
        valB[i] = stream->cur[i];
    stream->cur += 16;
    return true;
}

static inline bool stream_read_uint32(struct ByteStream *stream, uint32_t *value)
{
    if(stream->cur + 4 > stream->end)
        return false;
    *value = (uint32_t)stream->cur[0] | (uint32_t)(stream->cur[1]<<8)
        | (uint32_t)(stream->cur[2]<<16) | (uint32_t)(stream->cur[3]<<24);
    stream->cur += 4;
    return true;
}

static inline bool stream_read_uint16(struct ByteStream *stream, uint16_t *value)
{
    if(stream->cur + 2 > stream->end)
        return false;
    *value = (uint16_t)stream->cur[0] | (uint16_t)(stream->cur[1]<<8);
    stream->cur += 2;
    return true;
}

static inline bool stream_read_uint8(struct ByteStream *stream, uint8_t *value)
{
    if(stream->cur + 1 > stream->end)
        return false;
    *value = (uint8_t)stream->cur[0];
    stream->cur++;
    return true;
}

static inline bool stream_read_str(struct ByteStream *stream, char *buf, uint32_t len)
{
    if(stream->cur + len > stream->end)
        return false;
    for(int i = 0; i < len; i++)
        buf[i] = stream->cur[i];
    stream->cur += len;
    return true;
}

static inline bool stream_read_command_header(struct ByteStream *stream, uint32_t *cmdSize, uint32_t *cmdId)
{
    if(stream->cur + sizeof(struct EditorCommandHeader) > stream->end)
        return false;
    uint16_t size, cmd;
    stream_read_uint16(stream, &size);
    stream_read_uint16(stream, &cmd);
    *cmdSize = size;
    *cmdId = cmd;
    return true;
}

static inline bool stream_write_command_header(struct ByteStream *stream, uint16_t cmdSize, uint16_t cmdId)
{
    if(stream->cur + sizeof(struct EditorCommandHeader) > stream->end)
        return false;
    stream_write_uint16(stream, cmdSize);
    stream_write_uint16(stream, cmdId);
    return true;
}

static inline bool stream_write_empty_command(struct ByteStream *stream, uint16_t cmdId)
{
    return stream_write_command_header(stream, sizeof(struct EditorCommandHeader), cmdId);
}

static inline void editor_command_buf_reset(struct TessEditorCommandBuf *cmdBuf, void *usrPtr, void *usrClientPtr, bool isServer)
{
    cmdBuf->currentCommandBytes = 0;
    cmdBuf->currentCommandSize = 0;
    cmdBuf->usrPtr = usrPtr;
    cmdBuf->usrClientPtr = usrClientPtr;
    cmdBuf->isServer = isServer;
}

void editor_flush_command(struct TessEditorCommandBuf *cmdbuf)
{
    uint32_t cmdSize = cmdbuf->currentCommandBytes;
    assert(cmdSize < TESS_EDITOR_SERVER_MAX_COMMAND_SIZE);
    struct ByteStream stream;
    init_byte_stream(&stream, cmdbuf->currentCommand, cmdSize);
    static_assert(member_size(struct EditorCommandHeader, size) == 2, "use uintXX_t");
    bool res = stream_advance(&stream, 2);
    assert(res); // only commands with valid headers should reach this point
    static_assert(member_size(struct EditorCommandHeader, cmd) == 2, "use uintXX_t");
    uint16_t cmd;
    res = stream_read_uint16(&stream, &cmd);
    assert(res); // only commands with valid headers should reach this point

    uint32_t nLeft = stream_get_bytes_left(&stream);

    if(cmdbuf->isServer)
        editor_server_process_client_command((struct TessEditorServer*)cmdbuf->usrPtr, (struct TessEditorServerClient *)cmdbuf->usrClientPtr, cmd, stream.cur, nLeft);
    else
        editor_client_process_command((struct TessEditor*)cmdbuf->usrPtr, cmd, stream.cur, nLeft);
    cmdbuf->currentCommandBytes = 0;
}

void editor_append_cmd_data(struct TessEditorCommandBuf *cmdbuf, uint8_t *data, uint32_t nBytes)
{
    PROF_BLOCK();
    assert(nBytes != 0);
    uint32_t curCmdSize;
    if(cmdbuf->currentCommandBytes > sizeof(struct EditorCommandHeader))
        curCmdSize = cmdbuf->currentCommandSize;
    else // new command
    {
        struct ByteStream stream;
        uint16_t cmdSize;
        init_byte_stream(&stream, data, nBytes);
        static_assert(member_size(struct EditorCommandHeader, size) == 2, "change to read_uintXX");
        if(stream_read_uint16(&stream, &cmdSize))
        {
            curCmdSize = cmdSize;
            cmdbuf->currentCommandSize = curCmdSize;
        }
        else // even the command size didn't fit in fragment. Unlikely but we are ready for anything!
        {
            // why am I doing this
            static_assert(TESS_EDITOR_SERVER_MAX_COMMAND_SIZE > 2, "you are dumb!");
            memcpy(cmdbuf->currentCommand + cmdbuf->currentCommandBytes, data, nBytes);
            cmdbuf->currentCommandBytes += nBytes;
            return;
        }
    }
    // command was bigger or smaller than allowed!
    // the best thing we know to do is ignore and reset
    if(curCmdSize > TESS_EDITOR_SERVER_MAX_COMMAND_SIZE || curCmdSize < sizeof(struct EditorCommandHeader))
    {
        cmdbuf->currentCommandBytes = 0;
        return;
    }

    uint32_t bytesLeft = curCmdSize - cmdbuf->currentCommandBytes;
    assert(bytesLeft != 0);
    if(bytesLeft > nBytes) // still not the whole command
    {
        memcpy(cmdbuf->currentCommand + cmdbuf->currentCommandBytes, data, nBytes);
        cmdbuf->currentCommandBytes += nBytes;
    }
    else
    {
        memcpy(cmdbuf->currentCommand + cmdbuf->currentCommandBytes, data, bytesLeft);
        cmdbuf->currentCommandBytes += bytesLeft;
        // as side effect this will reset currentCommandBytes
        editor_flush_command(cmdbuf);
        // recurse with remaining data
        if(bytesLeft != nBytes)
            editor_append_cmd_data(cmdbuf, data + bytesLeft, nBytes - bytesLeft);
    }
}

