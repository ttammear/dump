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

void tess_load_map(TessGameSystem *world, TessMapHeader *map, uint32_t maxOffset)
{
    tess_reset_world(world);

    struct ByteRange
    {
        uint8_t *start;
        uint8_t *end;;
    } mapRange = {(uint8_t*)map, ((uint8_t*)map) + maxOffset};

    bool loadFailed = false;

#define EXIT_IF_FAIL(x) if(!(x)) { loadFailed = true; goto map_load_failed;}

    // check if header is safe to read and correct
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(map, mapRange));
    EXIT_IF_FAIL(TTR_4CHAR("TESM") == map->signature);
    // TODO: check version

    // check if object table is safe to read
    TessObjectTable *otbl = TTR_REF_TO_PTR(TessObjectTable, map->objectTableRef);
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(otbl, mapRange));
    uint32_t numObjEntries = otbl->numEntries;
    EXIT_IF_FAIL(T_ARRAY_IN_RANGE(otbl->entries, numObjEntries, mapRange));

    // check if entity table is safe to read
    TessEntityTable *etbl = TTR_REF_TO_PTR(TessEntityTable, map->entityTableRef);
    EXIT_IF_FAIL(T_STRUCT_IN_RANGE(etbl, mapRange));
    uint32_t numEntEntries = etbl->numEntries;
    EXIT_IF_FAIL(T_ARRAY_IN_RANGE(etbl->entries, numEntEntries, mapRange));

    for(int i = 0; i < numObjEntries; i++)
    {
       EXIT_IF_FAIL(tess_map_is_valid_asset_id(otbl->entries[i].assetId, sizeof(otbl->entries[0].assetId)));
       TStr *assetId = tess_intern_string_s(world->tstrings, otbl->entries[i].assetId, sizeof(otbl->entries[0].assetId));
       tess_register_object(world, otbl->entries[i].objectId, assetId);
    }

    for(int i = 0; i < numEntEntries; i++)
    {
        V3 pos, scale;
        Quat rot;
        uint32_t objectId;
        pos = etbl->entries[i].position;
        rot = etbl->entries[i].rotation;
        scale = etbl->entries[i].scale;
        objectId = etbl->entries[i].objectId;

        Mat4 modelToWorld;
        mat4_trs(&modelToWorld, pos, rot, scale);

        tess_create_entity(world, objectId, &modelToWorld);
    }

map_load_failed: 
    if(loadFailed)
    {
        fprintf(stderr, "Failed to load map\n");
        return;
    }
#undef EXIT_IF_FAIL
}
#endif
