void saveDummyMesh()
{
    void *buf = malloc(1024*1024);
    struct TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(struct TTRHeader);

    struct TTRDescTbl *descTbl = STREAM_PUSH_FLEX(stream, struct TTRDescTbl, entries, 1);
    descTbl->entryCount = 1;

    uint32_t vertBufSize = 24*40;
    struct TTRBuffer *vbuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, vertBufSize);
    vbuf->size = vertBufSize;
    uint8_t *vertStream = vbuf->data;
    for(int i = 0; i < 24; i++)
    {
        struct V4* pos = STREAM_PUSH(vertStream, struct V4);
        struct V2* texC = STREAM_PUSH(vertStream, struct V2);
        struct V4* col = STREAM_PUSH(vertStream, struct V4);
        *pos = cubeVertices[i];
        *texC = *((struct V2*)&cubeTexCoords[i]);
        *col = cubeColors[i];
    }

    uint32_t indexBufSize = ARRAY_COUNT(s_indices)*2;
    struct TTRBuffer *ibuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, indexBufSize);
    ibuf->size = indexBufSize;
    uint16_t *indexStream = (uint16_t*)ibuf->data;
    for(int i = 0; i < ARRAY_COUNT(s_indices); i++)
        indexStream[i] = s_indices[i];

    struct TTRMeshDesc *mdesc = STREAM_PUSH_FLEX(stream, struct TTRMeshDesc, attrs, 3);
    mdesc->indexSize = 2;
    mdesc->vertStride = 40;
    mdesc->numAttrs = 3;
    //mdesc->attrs[0] = Vertex_Attribute_Type_Vec4;
    mdesc->attrs[0] = Vertex_Attribute_Type_Vec2;
    mdesc->attrs[1] = Vertex_Attribute_Type_Vec4;


    struct TTRMesh* mesh = STREAM_PUSH_FLEX(stream, struct TTRMesh, sections, 1);
    TTR_SET_REF_TO_PTR(mesh->descRef, mdesc);
    TTR_SET_REF_TO_PTR(mesh->vertBufRef, vbuf);
    TTR_SET_REF_TO_PTR(mesh->indexBufRef, ibuf);
    mesh->numVertices = 24;
    mesh->numIndices = ARRAY_COUNT(s_indices);
    mesh->numSections = 1;
    mesh->sections[0].startIndex = 0;
    mesh->sections[0].indexCount = ARRAY_COUNT(s_indices);
//    printf("%p %p %p %p\n", buf, mesh, mesh, mesh3);

    TTR_SET_REF_TO_PTR(header->descTblRef, descTbl);
    TTR_SET_REF_TO_PTR(descTbl->entries[0].ref, mesh);
    descTbl->entries[0].type = TTR_4CHAR("MESH");

    FILE *file = fopen("Packages/First/first.ttr", "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing first.ttr failed!\n");
    free(buf);
}


struct SomeData
{
    int test;
    DELEGATE_NO_TYPEDEF(onComplete, void*, (void*,void*), 4);
    int whatever;
};

struct SomeData someData;

void* testFunction(void *data1, void *data2)
{
    printf("testFunction %p %p\n", data1, data2);
    return NULL;
}

void randomfun()
{
    DELEGATE_INIT(someData.onComplete);
    DELEGATE_LISTEN(someData.onComplete, testFunction);

    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEED, (void*)0xCAFEBABE);

    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEED, (void*)0xCAFEBABE);

    // no malloc yet
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE2, (void*)0xCAFEBABE);

    // now should malloc 2
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE3, (void*)0xCAFEBABE);

    // free at least 1 and 1 left
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE4, (void*)0xCAFEBABE);
    
    // nothing left
    DELEGATE_REMOVE(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE5, (void*)0xCAFEBABE);

    // no malloc!
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE6, (void*)0xCAFEBABE);

    // nothing left
    DELEGATE_CLEAR(someData.onComplete);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE7, (void*)0xCAFEBABE);

    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_LISTEN(someData.onComplete, testFunction);
    DELEGATE_INVOKE(someData.onComplete, (void*)0xFACEFEE8, (void*)0xCAFEBABE);
    DELEGATE_CLEAR(someData.onComplete);
}

