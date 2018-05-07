#pragma pack(push, 1)
typedef struct TessEntityEntry
{
    V3 position;
    Quat rotation;
    V3 scale;
    uint32_t objectId;
} TessEntityEntry;

typedef struct TessObjectEntry
{
    uint32_t objectId;
    char assetId[128];
} TessObjectEntry;

typedef struct TessObjectTable
{
    uint32_t numEntries;
    TessObjectEntry entries[];
} TessObjectTable;

typedef struct TessEntityTable
{
    uint32_t numEntries;
    TessEntityEntry entries[];
} TessEntityTable;

typedef struct TessMapHeader
{
    uint32_t signature;
    uint16_t majVer;
    uint16_t minVer;
    TTRRef entityTableRef;
    TTRRef objectTableRef;
} TessMapHeader;
#pragma pack(pop)

#define CHECK_RANGE(x, size, range) ((uint8_t*)(x) >= (range).start && ((uint8_t*)(x))+(size) <= (range).end)
#define MEMBER_IN_RANGE(x, member, range) CHECK_RANGE(x, sizeof((x)->member), range)
#define T_STRUCT_IN_RANGE(x, range) CHECK_RANGE((x), sizeof(*(x)), range)
#define T_ARRAY_IN_RANGE(x, count, range) CHECK_RANGE((x), sizeof(*(x)) * (count), range)

// TODO: get rid of
#ifndef MAP_NO_IMPLEMENTATION

bool tess_map_check_name(char *name, uint32_t maxLen)
{
    for(int i = 0; i < maxLen; i++)
    {
        char c = name[i];
        if((c >= 'A' && c <= 'z') || (c >= '0' && c <= '9') || (c == '/'))
            continue;
        else if(c == 0)
            break;
        else
            return false;
    }
    return true;
}

bool tess_map_is_valid_asset_id(char *name, uint32_t maxLen)
{
    int runCount = 0;
    int curRun = 0;

    bool validName = tess_map_check_name(name, maxLen);
    if(!validName)
        return false;

    for(int i = 0; i < maxLen; i++)
    {
        if(name[i] == '/')
        {
            if(curRun > TTR_MAX_NAME_LEN || curRun == 0)
                return false;
            curRun = 0;
            runCount++;
        }
        else if(name[i] == 0)
            break;
        else
            curRun++;
    }
    return runCount == 1;
}

typedef struct MapReader
{
    TessObjectTable *otbl;
    TessEntityTable *etbl;
    uint32_t numObjEntries;
    uint32_t numEntEntries;

    uint32_t curObject;
    uint32_t curEntity;
} MapReader;

typedef struct MapObject
{
    uint32_t objectId;
    char assetId[128];        
} MapObject;

typedef struct MapEntity
{
    uint32_t objectId;
    V3 pos;
    Quat rot;
    V3 scale;
} MapEntity;

enum MapErrorCode
{
    Map_Error_Code_Success,
    Map_Error_Code_End,
    Map_Error_Code_Parse_Failed,
};


bool map_reader_begin(MapReader *reader, void *data, size_t size)
{
#define EXIT_IF_FAIL(x) if(!(x)) { loadFailed = true; goto map_load_failed;}
    TessMapHeader *map = (TessMapHeader*)data;
    struct ByteRange
    {
        uint8_t *start;
        uint8_t *end;
    } mapRange = {(uint8_t*)data, ((uint8_t*)data) + size};
    bool loadFailed = false;
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(map, mapRange));
    EXIT_IF_FAIL(TTR_4CHAR("TESM") == map->signature);

    reader->otbl = TTR_REF_TO_PTR(TessObjectTable, map->objectTableRef);
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(reader->otbl, mapRange));
    reader->numObjEntries = reader->otbl->numEntries;
    EXIT_IF_FAIL(T_ARRAY_IN_RANGE(reader->otbl->entries, reader->numObjEntries, mapRange));

    reader->etbl = TTR_REF_TO_PTR(TessEntityTable, map->entityTableRef);
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(reader->etbl, mapRange));
    reader->numEntEntries = reader->etbl->numEntries;
    EXIT_IF_FAIL(T_ARRAY_IN_RANGE(reader->etbl->entries, reader->numEntEntries, mapRange));

    reader->curObject = 0;
    reader->curEntity = 0;
map_load_failed: 
    if(loadFailed)
    {
        fprintf(stderr, "Failed to load map\n");
        return false;
    }
    return true;
#undef EXIT_IF_FAIL
}

uint32_t map_next_object(MapReader *reader, MapObject *entity)
{
    auto otbl = reader->otbl; 
    auto i = reader->curObject;
    if(i >= reader->numObjEntries)
        return Map_Error_Code_End;
    if(!tess_map_is_valid_asset_id(otbl->entries[i].assetId, 
               sizeof(otbl->entries[0].assetId)))
        return Map_Error_Code_Parse_Failed;
    strncpy(entity->assetId, otbl->entries[i].assetId, sizeof(entity->assetId));
    entity->objectId = otbl->entries[i].objectId;
    reader->curObject++;
    return Map_Error_Code_Success;
}

uint32_t map_next_entity(MapReader *reader, MapEntity *entity)
{
    auto etbl = reader->etbl;
    auto i = reader->curEntity;
    if(i >= reader->numEntEntries)
        return Map_Error_Code_End;
    entity->pos = etbl->entries[i].position;
    entity->rot = etbl->entries[i].rotation;
    entity->scale = etbl->entries[i].scale;
    entity->objectId = etbl->entries[i].objectId;
    reader->curEntity++;
    return Map_Error_Code_Success;
}

#endif
