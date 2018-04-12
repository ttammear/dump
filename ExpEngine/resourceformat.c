#pragma push(pack, 1)

#define TTR_MAX_NAME_LEN 64

#define TTR_AREF_EXTERN_MASK 0x80000000
#define TTR_IS_EXTERN_AREF(x) (((x.tblIndex) & TTR_AREF_EXTERN_MASK) != 0) 

struct TTRAssetRef
{
    uint32_t tblIndex; // highest order bit - 0=descTbl 1=importTbl
};

typedef int32_t TTRRef;

struct TTRHeader
{
    uint32_t signature;
    uint16_t majorVersion;
    uint16_t minorVersion;
    //uint8_t globalFPFormat; // IEEE764 single, IEEE764 double etc
    //uint8_t globalOffsetFormat; // offset size 32 bit or 64 bit
    TTRRef descTblRef;
    TTRRef importTblRef;
};

struct TTRDescTblEntry
{
    uint32_t type;
    TTRRef ref;
    char assetName[TTR_MAX_NAME_LEN];
};

struct TTRImportTblEntry
{
    uint32_t type;
    char packageName[TTR_MAX_NAME_LEN];
    char assetName[TTR_MAX_NAME_LEN];
};

struct TTRDescTbl
{
    uint32_t entryCount;
    struct TTRDescTblEntry entries[];
};

struct TTRImportTbl
{
    uint32_t entryCount;
    struct TTRImportTblEntry entries[];
};

struct TTRMeshDesc
{
    uint32_t indexSize;
    uint32_t vertStride;
    uint32_t numAttrs;
    uint8_t attrs[];
};

struct TTRMeshSection
{
    uint32_t startIndex;
    uint32_t indexCount;
};

struct TTRBuffer
{
    uint32_t size;
    uint8_t data[];
};

struct TTRMesh
{
    TTRRef descRef;
    TTRRef vertBufRef;
    TTRRef indexBufRef;
    uint32_t numVertices;
    uint32_t numIndices;
    uint32_t numSections;
    struct TTRMeshSection sections[];
};

struct TTRObject
{
    struct TTRAssetRef meshARef;
    // TODO: material?
    // TODO: collider?
    // something else?
};

#pragma pop(pack)

#define TTR_4CHAR(x) ((x[0]) | (x[1]<<8) | (x[2]<<16) | (x[3]<<24))
#define TTR_REF_TO_PTR(type, ref) ((type*)(((uint8_t*)(&(ref))) + ((int32_t)(ref))))
#define TTR_SET_REF_TO_PTR(ref, ptr) (ref) = (((uint8_t*)(ptr)) - ((uint8_t*)(&ref)))

// get size of variable size struct
#define TTR_GET_SIZE(type, arr, count) (sizeof(type) + sizeof(((type*)0)->arr[0])*(count))

#define STREAM_PUSH(stream, type) ({type* rettval = (type*)stream; (stream) += sizeof(type); rettval;})
#define STREAM_PUSH_FLEX(stream, type, arr, count) ({type* rettval = (type*)stream; (stream) += TTR_GET_SIZE(type, arr, count); rettval;})

struct LoadMeshData
{
    // stage 0 - get data layout
    struct Renderer *renderer;
    void *fileMem;
    size_t dataSize;
    // stage 1 - copy data
    struct TTRMesh *ttrMesh;
    struct MeshQueryResult mqr;
    // stage 2 - loaded
    uint32_t meshId;
};

void ttr_load_first_mesh(struct LoadMeshData *data, uint32_t stage);

void ttr_mesh_query_result(struct Renderer *renderer, struct MeshQueryResult *mqr, void *userData)
{
    struct LoadMeshData *lmdata = (struct LoadMeshData*)userData;
    lmdata->mqr = *mqr;
    lmdata->renderer = renderer;
    ttr_load_first_mesh(lmdata, 1);
}

void ttr_mesh_ready(struct Renderer *renderer, struct MeshReady *mr, void *userData)
{
    struct LoadMeshData *lmdata = (struct LoadMeshData*)userData;
    lmdata->meshId = mr->meshId;
    ttr_load_first_mesh(lmdata, 2);
}

static uint32_t pyramid;

void ttr_load_first_mesh(struct LoadMeshData *data, uint32_t stage)
{
    switch(stage)
    {
        case 0:
        {
        struct TTRHeader *header = (struct TTRHeader*)data->fileMem;
        struct TTRDescTbl *tbl = TTR_REF_TO_PTR(struct TTRDescTbl, header->descTblRef);
        for(int i = 0; i < tbl->entryCount; i++)
        {
            if(tbl->entries[i].type == TTR_4CHAR("MESH"))
            {
                struct TTRMesh *ttrMesh = TTR_REF_TO_PTR(struct TTRMesh, tbl->entries[i].ref);
                struct TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(struct TTRMeshDesc, ttrMesh->descRef);
                data->ttrMesh = ttrMesh;
                RenderMessage msg = {};
                msg.type = Render_Message_Mesh_Query;
                msg.meshQuery.userData = data;
                msg.meshQuery.onComplete = ttr_mesh_query_result;
                msg.meshQuery.meshId = 0;
                msg.meshQuery.vertexCount = ttrMesh->numVertices;
                msg.meshQuery.indexCount = ttrMesh->numIndices;
                for(int j = 0; j < ttrMeshDesc->numAttrs; j++)
                    msg.meshQuery.attributeTypes[j] = ttrMeshDesc->attrs[j];
                renderer_queue_message(data->renderer, &msg);
            }
            else
            {
                fprintf(stderr, "No mesh found in file!\n");
                return;
            }
        }
        }
        break;
        case 1:
        {
            struct TTRMesh *ttrMesh = data->ttrMesh;
            struct TTRBuffer *vbuf = TTR_REF_TO_PTR(struct TTRBuffer, ttrMesh->vertBufRef);
            struct TTRBuffer *ibuf = TTR_REF_TO_PTR(struct TTRBuffer, ttrMesh->indexBufRef);
            struct TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(struct TTRMeshDesc, ttrMesh->descRef);
            uint32_t stride = ttrMeshDesc->vertStride;
            uint32_t vcount = ttrMesh->numVertices;
            uint32_t icount = ibuf->size / 2;
            memcpy(data->mqr.vertBufPtr, vbuf->data, stride*vcount);
            memcpy(data->mqr.idxBufPtr, ibuf->data, 2*icount);
            RenderMessage msg = {};
            msg.type = Render_Message_Mesh_Update;
            msg.meshUpdate.meshId = data->mqr.meshId;
            msg.meshUpdate.onComplete = ttr_mesh_ready;
            msg.meshUpdate.userData = data;
            renderer_queue_message(data->renderer, &msg);
        }
        break;
        case 2:
        printf("Mesh load complete! %d\n", data->meshId);
        free(data);
        break;
    }
}
