#pragma once
#pragma pack(push, 1)

#define TTR_MAX_NAME_LEN 64
#define TTR_MAX_ASSET_ID_LEN 129

#define TTR_AREF_EXTERN_MASK 0x80000000
#define TTR_IS_EXTERN_AREF(x) (((x.tblIndex) & TTR_AREF_EXTERN_MASK) != 0) 

#define TTR_DELIM '/'

enum TTRVertexAttributeType {
    TTR_Vertex_Attribute_Type_None = 0,
    TTR_Vertex_Attribute_Type_Vec4,
    TTR_Vertex_Attribute_Type_Vec3,
    TTR_Vertex_Attribute_Type_Vec2,
    TTR_Vertex_Attribute_Type_Float,
};

enum TTRShaderType
{
    TTR_Shader_Type_None,
    TTR_Shader_Type_Unlit_Color, // Color
    TTR_Shader_Type_Unlit_Vertex_Color, // nothing
    TTR_Shader_Type_Unlit_Textured, // texture, tint?
    TTR_Shader_Type_Unlit_Fade, // texture RGB and A, tint
    TTR_Shader_Type_Unlit_Textured_Cutout,
    TTR_Shader_Type_Gizmo, // Color
    TTR_Shader_Type_Count,
};

enum TTRTextureFormat
{
    TTR_Texture_Format_None,
    TTR_Texture_Format_RGBA,
};


typedef struct TTRAssetRef
{
    // 0xFFFFFFFF - not assigned (for example objects might not have colliders)
    uint32_t tblIndex; // highest order bit - 0=descTbl 1=importTbl
} TTRAssetRef;

typedef int32_t TTRRef;

typedef struct TTRHeader
{
    uint32_t signature;
    uint16_t majorVersion;
    uint16_t minorVersion;
    //uint8_t globalFPFormat; // IEEE764 single, IEEE764 double etc
    //uint8_t globalOffsetFormat; // offset size 32 bit or 64 bit
    TTRRef descTblRef;
    TTRRef importTblRef;
} TTRHeader;

typedef struct TTRDescTblEntry
{
    uint32_t type;
    TTRRef ref;
    char assetName[TTR_MAX_NAME_LEN];
} TTRDescTblEntry;

typedef struct TTRImportTblEntry
{
    uint32_t type; // 4 character code ie "MESH" "OBJ "
    char packageName[TTR_MAX_NAME_LEN];
    char assetName[TTR_MAX_NAME_LEN];
} TTRImportTblEntry;

typedef struct TTRDescTbl
{
    uint32_t entryCount;
    struct TTRDescTblEntry entries[];
} TTRDescTbl;

typedef struct TTRImportTbl
{
    uint32_t entryCount;
    struct TTRImportTblEntry entries[];
} TTRImportTbl;

// mesh configuration
// only specifies what types attributes are
// what the attributes actually mean is determined by the material/shader
typedef struct TTRMeshDesc
{
    uint32_t indexSize;
    uint32_t vertStride; // TODO: technically redundant because it can be determined fro mattribute sizes?
    uint32_t numAttrs;
    uint8_t attrs[];
} TTRMeshDesc;

typedef struct TTRMeshSection
{
    uint32_t startIndex;
    uint32_t indexCount;
    TTRRef materialRef;
} TTRMeshSection;

typedef struct TTRBuffer
{
    uint32_t size;
    uint8_t data[];
} TTRBuffer;

typedef struct TTRSphereCollider
{
    V3 pos; 
    float radius;
} TTRSphereCollider;

typedef struct TTRBoxCollider
{
    V3 min;
    V3 max;
} TTRBoxCollider;

typedef struct TTRMesh
{
    TTRRef descRef;
    TTRRef vertBufRef;
    TTRRef indexBufRef;
    uint32_t numVertices;
    uint32_t numIndices;
    uint32_t numSections;
    struct TTRMeshSection sections[];
} TTRMesh;

typedef struct TTRCollider
{
    TTRRef vertBufRef;
    TTRRef indexBufRef;
    TTRRef sphereBufRef;
    TTRRef boxBufRef;
    uint32_t numVertices;
    uint32_t numIndices;
    uint32_t numSpheres;
    uint32_t numBoxes;
} TTRCollider;

typedef struct TTRTexture
{
    uint32_t format; // 1 = RGBA
    uint32_t width;
    uint32_t height;
    TTRRef bufRef;
} TTRTexture;

typedef struct TTRMaterial
{
    uint32_t shaderType;
    struct TTRAssetRef albedoTexARef;
    V4 tintColor;
} TTRMaterial;

typedef struct TTRObject
{
    struct TTRAssetRef meshARef;
    struct TTRAssetRef colliderARef;
    // something else?
} TTRObject;

typedef struct TTRMap {
    TTRRef entityTableRef;
    TTRRef objectTableRef;
} TTRMap;

typedef struct TTRMapObjectEntry
{
    uint32_t objectId;
    char assetId[128];
} TTRMapObjectEntry;

typedef struct TTRMapObjectTable
{
    uint32_t numEntries;
    TTRMapObjectEntry entries[];
} TTRMapObjectTable;

typedef struct TTRMapEntityEntry
{
    V3 position;
    Quat rotation;
    V3 scale;
    uint32_t objectId;
} TTRMapEntityEntry;

typedef struct TTRMapEntityTable
{
    uint32_t numEntries;
    TTRMapEntityEntry entries[];
} TTRMapEntityTable;

#pragma pack(pop)

#define TTR_4CHAR(x) ((x[0]) | (x[1]<<8) | (x[2]<<16) | (x[3]<<24))
#define TTR_REF_TO_PTR(type, ref) ((type*)(((uint8_t*)(&(ref))) + ((int32_t)(ref))))
#define TTR_SET_REF_TO_PTR(ref, ptr) (ref) = (((uint8_t*)(ptr)) - ((uint8_t*)(&ref)))

// get size of variable size struct
#define TTR_GET_SIZE(type, arr, count) (sizeof(type) + sizeof(((type*)0)->arr[0])*(count))

#define STREAM_PUSH(stream, type) ({type* rettval = (type*)stream; (stream) += sizeof(type); rettval;})
#define STREAM_PUSH_FLEX(stream, type, arr, count) ({type* rettval = (type*)stream; (stream) += TTR_GET_SIZE(type, arr, (count)); rettval;})
#define STREAM_PUSH_ALIGN(stream, align) (stream = (uint8_t*)ALIGN_UP_PTR(stream, (align)))

#define TTR_AREF_EMPTY(x) ((x).tblIndex == 0xFFFFFFFF)
