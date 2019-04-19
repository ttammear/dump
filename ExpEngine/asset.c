
internal void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen);
TStr* tess_asset_id_from_aref(TessAssetSystem *as, TTRDescTbl *tbl, TTRImportTbl *imTbl, TTRAssetRef aref, TStr *currentPackage);
void tess_fill_mesh_data(Renderer *renderer, MeshQueryResult *mqr, void *userData);
void tess_finalize_mesh(Renderer *renderer, MeshReady *mr, void *userData);
void tess_fill_texture_data(Renderer *renderer, TextureQueryResponse *tqr, void *userData);
void tess_finalize_texture(Renderer *renderer, TextureReady *tr, void *userData);
void tess_finalize_object(Renderer *renderer, MaterialReady *mr, void *userData);
internal void tess_async_wait_deps(struct TessLoadingAsset *lasset, AsyncTask *task);
TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId);
TessAsset* tess_force_load_asset(TessAssetSystem *as, TStr *assetId); 
internal void loading_asset_init(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId);
internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, TessAssetStatus newStatus);
internal void asset_loaded(TessAssetSystem *as, TessAsset *asset);
ASYNC void tess_unlist_asset(TessAssetSystem *as, TessAsset *asset);
ASYNC void fill_material_query(RenderMessage *msg, TTRMaterial *ttrMat, TessTTRContext *ttrCtx);

internal inline void tess_set_asset_status(TessAssetSystem *as, TStr *assetId, uint32_t status)
{
    int dummy;
    khiter_t k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
    kh_value(as->assetStatusMap, k) = status;
}

internal inline bool tess_is_asset_loading(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadingAssetMap);
}

void tess_get_asset_metrics(struct TessAssetSystem *as, struct TessAssetSystemMetrics *tasm) {
    tasm->numLoadingAssets = buf_len(as->loadingAssets);
    tasm->numLoadedAssets = as->numLoadedAssets;
    tasm->numOpenedAssetFiles = as->numOpenAssetFiles;
    tasm->totalFileLoads = as->totalFileLoads;
}

void tess_finalize_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TessAsset *asset, TStr *assetId, enum TessAssetType type) {
    asset->assetId = assetId;
    asset->type = type;
    printf("Asset loaded! %s\n", lasset->assetId->cstr);
    tess_loading_asset_done(as, lasset, Tess_Asset_Status_Loaded);
    asset_loaded(as, asset);
    as->numLoadedAssets++;
}

ASYNC TessTextureAsset* tess_load_texture(TessAssetSystem *as, struct TessLoadingAsset *lasset, TTRTexture *ttex) {
    scheduler_assert_task(); // this is an async function
    AsyncTask task;
    
    RenderMessage msg = {};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.textureId = 0;
    msg.texQ.width = ttex->width;
    msg.texQ.height = ttex->height;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Trilinear;
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TTRBuffer *tbuf = TTR_REF_TO_PTR(TTRBuffer, ttex->bufRef);
    assert(tbuf->size == ttex->width*ttex->height*4);
    memcpy(task.renderMsg->texQR.textureDataPtr, tbuf->data, tbuf->size);
    msg = (const struct RenderMessage){0};
    msg.type = Render_Message_Texture_Update;
    msg.texU.textureId = task.renderMsg->texQR.textureId;
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);

    TessTextureAsset *texture = pool_allocate(as->texturePool);
    assert(texture);
    texture->textureId = task.renderMsg->texR.textureId;

    tess_finalize_asset(as, lasset, &texture->asset, lasset->assetId, Tess_Asset_Texture);

    return texture;
}

ASYNC void tess_unload_texture(TessAssetSystem *as, TessTextureAsset *tex) {
    scheduler_assert_task(); // this is an async function 
    RenderMessage msg = {0};
    msg.type = Render_Message_Texture_Destroy;
    msg.texD.textureId = tex->textureId;
    tex->textureId = 0; // reduntant
    renderer_queue_message(as->renderer, &msg); // TODO: this should be async?
    
    tess_unlist_asset(as, &tex->asset);
    
    pool_free(as->texturePool, tex);
}

ASYNC TessMeshAsset* tess_load_mesh(TessTTRContext *tctx, TessLoadingAsset *lasset, TTRMesh *tmesh) {
    AsyncTask task;
    TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(TTRMeshDesc, tmesh->descRef);
    RenderMessage msg = {};
    RenderMessage mmsg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = tmesh->numVertices;
    msg.meshQuery.indexCount = tmesh->numIndices;
    for(int j = 0; j < ttrMeshDesc->numAttrs; j++) {
        msg.meshQuery.attributeTypes[j] = ttrMeshDesc->attrs[j];
    }
    renderer_async_message(tctx->as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TTRBuffer *vbuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->vertBufRef);
    TTRBuffer *ibuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->indexBufRef);
    // TODO: check if buffer sizes match
    memcpy(task.renderMsg->meshQueryResult.vertBufPtr, vbuf->data, vbuf->size);
    memcpy(task.renderMsg->meshQueryResult.idxBufPtr, ibuf->data, ibuf->size);

    TessMeshAsset *mesh = pool_allocate(tctx->as->meshPool);

    // update mesh
    msg = (const RenderMessage){};
    msg.type = Render_Message_Mesh_Update;
    msg.meshUpdate.meshId = task.renderMsg->meshQueryResult.meshId;
    assert(tmesh->numSections <= MAX_MESH_SECTIONS);
    msg.meshUpdate.numSections = tmesh->numSections;
    mesh->numSections = tmesh->numSections;
    for(int i = 0; i < tmesh->numSections; i++) {
        msg.meshUpdate.sections[i].offset = tmesh->sections[i].startIndex;
        msg.meshUpdate.sections[i].count = tmesh->sections[i].indexCount;

        TTRMaterial* ttrMat = TTR_REF_TO_PTR(TTRMaterial, tmesh->sections[i].materialRef);
        mmsg = (const RenderMessage){};
        fill_material_query(&mmsg, ttrMat, tctx);
        renderer_async_message(tctx->as->renderer, &task, &mmsg);
        scheduler_wait_for(&task); // TODO: submit all then wait?
        msg.meshUpdate.sections[i].materialId = task.renderMsg->matR.materialId;
        mesh->materials[i] = task.renderMsg->matR.materialId;
    }

    renderer_async_message(tctx->as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    assert(mesh);
    mesh->meshId = task.renderMsg->meshR.meshId;
    mesh->asset.refCount = 0;

    tess_finalize_asset(tctx->as, lasset, &mesh->asset, lasset->assetId, Tess_Asset_Mesh);

    return mesh;
}

ASYNC void tess_unload_mesh(TessAssetSystem *as, TessMeshAsset *mesh) {
    scheduler_assert_task(); // this is an async function
    RenderMessage msg = {0};
    msg.type = Render_Message_Mesh_Destroy;
    msg.meshD.meshId = mesh->meshId;
    mesh->meshId = 0; // reduntant
    renderer_queue_message(as->renderer, &msg); // TODO: this should be async?

    for(int i = 0; i < mesh->numSections; i++) {
        msg.type = Render_Message_Material_Destroy;
        msg.matD.materialId = mesh->materials[i];
        mesh->materials[i] = 0; // redundant
        // TODO: decrement texture reference count, if material has one
        renderer_queue_message(as->renderer, &msg);
    }
    
    tess_unlist_asset(as, &mesh->asset);
    
    pool_free(as->meshPool, mesh);
}

ASYNC void fill_material_query(RenderMessage *msg, TTRMaterial *ttrMat, TessTTRContext *ttrCtx) {
    TTRAssetRef aref;
    TStr *textureAssetId;
    TessAsset *texAsset;
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
        aref = ttrMat->albedoTexARef;
        textureAssetId = tess_asset_id_from_aref(ttrCtx->as, ttrCtx->tbl, ttrCtx->imTbl, aref, ttrCtx->package);
        printf("OBJECT TEXTURE %s\n", textureAssetId->cstr); 
        texAsset = tess_force_load_asset(ttrCtx->as, textureAssetId);
        if(texAsset != NULL) {
            texAsset->refCount++;
        }
        break;
    }

    msg->type = Render_Message_Material_Query;
    msg->matQ.materialId = 0;
    msg->matQ.shaderId = ttrMat->shaderType;
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg->matQ.iData.unlitTextured.textureId = texa->textureId;
            } else {
                msg->matQ.iData.unlitTextured.textureId = 0;
            }
            msg->matQ.iData.unlitTextured.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Fade:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg->matQ.iData.unlitFade.textureId = texa->textureId;
            } else {
                msg->matQ.iData.unlitFade.textureId = 0;
            }
            msg->matQ.iData.unlitFade.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Color:
            msg->matQ.iData.unlitColor.color = ttrMat->tintColor;
            break;
        case Shader_Type_Gizmo:
            msg->matQ.iData.gizmoMat.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Vertex_Color:
            msg->matQ.iData.unlitVertexColor.color = ttrMat->tintColor;
            break;
        default:
            fprintf(stderr, "Object material had unknown shader type: %d\n", ttrMat->shaderType);
            assert(0); // invalid material, only here for debug
            break;
    }
}

ASYNC TessObjectAsset* tess_load_object(TessTTRContext *tctx, TessLoadingAsset *lasset, TTRObject *tobj) {
    scheduler_assert_task(); // async function
    AsyncTask task;
    TessAsset *meshAsset;
    TessObjectAsset *tessObj;
    TTRAssetRef aref = tobj->meshARef;
    TStr *curPackage = tess_get_asset_package_from_id(tctx->as, lasset->assetId);
    TStr *meshAssetId = tess_asset_id_from_aref(tctx->as, tctx->tbl, tctx->imTbl, aref, curPackage);
    TStr *textureAssetId;
    meshAsset = tess_force_load_asset(tctx->as, meshAssetId);
    meshAsset->refCount++;
    assert(meshAsset != NULL);
    lasset->type = Tess_Asset_Object;

    // TODO: this should be valid here
    //meshAsset = tess_get_asset(as, meshAssetId);
    assert(meshAsset && meshAsset->type == Tess_Asset_Mesh);

    tessObj = pool_allocate(tctx->as->objectPool);
    assert(tessObj);
    tessObj->asset.assetId = lasset->assetId;
    tessObj->asset.type = Tess_Asset_Object;
    tessObj->mesh = (TessMeshAsset*)meshAsset;

    tess_finalize_asset(tctx->as, lasset, &tessObj->asset, lasset->assetId, Tess_Asset_Object);

    return tessObj;
}

ASYNC void tess_unload_object(TessAssetSystem *as, TessObjectAsset *obj) {
    scheduler_assert_task(); // this is an async function 

    obj->mesh->asset.refCount--;
    tess_unlist_asset(as, &obj->asset);
    
    pool_free(as->objectPool, obj);
}


ASYNC TessMapAsset* tess_load_map_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TTRMap *map) {
    TTRMapObjectTable *mapObjTbl = TTR_REF_TO_PTR(TTRMapObjectTable, map->objectTableRef);
    TTRMapEntityTable *mapEntTbl = TTR_REF_TO_PTR(TTRMapEntityTable, map->entityTableRef);
    // TODO: use pool or something
    TessMapAsset *ret = malloc(sizeof(TessMapAsset));
    ret->mapObjectCount = mapObjTbl->numEntries;
    ret->mapEntityCount = mapEntTbl->numEntries;
    assert(ret);
    ret->objects = malloc(sizeof(ret->objects[0]) * mapObjTbl->numEntries);
    assert(ret->objects);
    for(int i = 0; i < mapObjTbl->numEntries; i++) {
        TStr *assetId = tess_intern_string_s(as->tstrings, mapObjTbl->entries[i].assetId, sizeof(mapObjTbl->entries[i].assetId));
        ret->objects[i] = (TessMapObject) {
            .objectid = mapObjTbl->entries[i].objectId,
            .assetId = assetId
        };
    }
    ret->entities = malloc(sizeof(ret->entities[0]) * mapEntTbl->numEntries);
    assert(ret->entities);
    for(int i = 0; i < mapEntTbl->numEntries; i++) {
        TTRMapEntityEntry *e = mapEntTbl->entries + i;
        ret->entities[i].position = e->position;
        ret->entities[i].rotation = e->rotation;
        ret->entities[i].scale = e->scale;
        ret->entities[i].objectId = e->objectId;
    }
    
    tess_finalize_asset(as, lasset, &ret->asset, lasset->assetId, Tess_Asset_Map);

    return ret;
}

ASYNC void tess_unload_map_asset(TessAssetSystem *as, TessMapAsset *map) {
    scheduler_assert_task(); // this is an async function 

    tess_unlist_asset(as, &map->asset);
    free(map->objects);
    free(map->entities);
    
    free(map);
}

ASYNC TessAsset* tess_load_asset(TessLoadingAsset *lasset) {
    AsyncTask task;
    TessAssetStatus status = tess_get_asset_status(lasset->as, lasset->assetId);
    // TODO: in case the status is InQueue, remove it from queue
    printf("status %s %d\n", lasset->assetId->cstr, status);
    assert(status == Tess_Asset_Status_None || status == Tess_Asset_Status_Loading);
    tess_set_asset_status(lasset->as, lasset->assetId, Tess_Asset_Status_Loading);
    printf("Load asset %s\n", lasset->assetId->cstr);
    TessAssetSystem *as = lasset->as;
    TStr *packageName = tess_get_asset_package_from_id(as, lasset->assetId);
    assert(lasset);
    char buf[AIKE_MAX_PATH];
    buf[0] = 0;
    // TODO: safe platform-independent path construction function
    strcpy(buf, "Packages/");
    strcat(buf, packageName->cstr);
    strcat(buf, "/");
    strcat(buf, lasset->fileName->cstr);
    // TODO: overflow protect?
    as->numOpenAssetFiles++;
    as->totalFileLoads++;
    tess_async_load_file(as->fileSystem, buf, &task);
    scheduler_wait_for(&task);
    return tess_process_asset_from_ttr(as, task.file, lasset);
}

ASYNC void tess_unload_asset(TessAssetSystem *as, TStr *assetId) {
    TessAsset *asset = tess_get_asset(as, assetId);
    if(asset != NULL) {
        switch(asset->type) {
            case Tess_Asset_Mesh:
                tess_unload_mesh(as, (TessMeshAsset*)asset);
                break;
            case Tess_Asset_Texture:
                tess_unload_texture(as, (TessTextureAsset*)asset);
                break;
            case Tess_Asset_Object:
                tess_unload_object(as, (TessObjectAsset*)asset);
                break;
            case Tess_Asset_Map:
                tess_unload_map_asset(as, (TessMapAsset*)asset);
                break;
            default:
                assert(0); // how did unknown asset type even get loaded?
                break;
        }
    }
}

ASYNC void tess_unlist_asset(TessAssetSystem *as, TessAsset *asset) {
    // unloading assets before loading has completed is not supported yet! TODO!
    assert(kh_get(64, as->loadingAssetMap, (intptr_t)asset->assetId) == kh_end(as->loadingAssetMap));
    kh_del(64, as->loadedAssetMap, (intptr_t)asset->assetId);
    kh_del(uint32, as->assetStatusMap, (intptr_t)asset->assetId);
    as->numLoadedAssets--;
}

ASYNC TessAsset* tess_force_load_asset(TessAssetSystem *as, TStr *assetId) {
    int dummy;
    printf("Force load asset %s\n", assetId->cstr);
    // TODO: event based, no polling!
    //assert(!tess_is_asset_loading(as, assetId));
    int i = 0;
    while(tess_is_asset_loading(as, assetId)) {
        scheduler_yield();
    }
    if(tess_is_asset_loaded(as, assetId)) {
        printf("Forced and already loaded %s\n", assetId->cstr);
        return tess_get_asset(as, assetId);
    }
    assert(!tess_is_asset_loaded(as, assetId));
    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName)) {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        loading_asset_init(as, lasset, fileName, assetId);
        printf("Forced and begin load %s\n", assetId->cstr);
        TessAsset *ret = tess_load_asset(lasset);
        printf("Forced and end load %s\n", assetId->cstr);
        return ret;
    } else {
        tess_set_asset_status(as, assetId, Tess_Asset_Status_Fail);
        return NULL;
    }
}

ASYNC void tess_task_load_asset(void *data) {
    scheduler_assert_task(); // async function

    TessLoadingAsset *lasset = (TessLoadingAsset*)data;
    tess_set_asset_status(lasset->as, lasset->assetId, Tess_Asset_Status_Loading);
    tess_load_asset(lasset);
    
    scheduler_task_end();
}

ASYNC TessAsset* tess_process_asset_from_ttr(TessAssetSystem *as, TessFile *tfile, TessLoadingAsset *lasset)
{
    TessAsset *asset = NULL;
    scheduler_assert_task(); // this function can only be called from within a task
    lasset->file = tfile;

    void* fileBuf = tfile->req.buffer;
    assert(tfile->req.fileOffset == 0);
    uint64_t fileSize = tfile->req.nBytes;

    // TODO: check everything before you read it!!!!!!
    auto header = (TTRHeader*)fileBuf;
    TessTTRContext tctx = (TessTTRContext) {
        .as = as,
        .header = header,
        .tbl = TTR_REF_TO_PTR(TTRDescTbl, header->descTblRef),
        .imTbl = TTR_REF_TO_PTR(TTRImportTbl, header->importTblRef),
        .package = tess_get_asset_package_from_id(as, lasset->assetId),
    };
    // TODO: maybe this should be cached in TessAsset?
    TStr *assetName = tess_get_asset_name_from_id(as, lasset->assetId);
    bool found = false;
    for(int i = 0; i < tctx.tbl->entryCount; i++)
    {
        TTRDescTblEntry *entry = &tctx.tbl->entries[i];
        char nameBuf[TTR_MAX_NAME_LEN];
        tess_replace_id_name_characters(nameBuf, entry->assetName, ARRAY_COUNT(nameBuf));
        TStr *internedName = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);

        // NOTE: if there would be any dependencies they would also be kicked off here

        if(internedName == assetName)
        {
            switch(entry->type) 
            {
                case 'HSEM': // MESH // TODO: multi byte characters are endianness sensitive
                    {
                        lasset->type = Tess_Asset_Mesh;
                        TTRMesh *tmesh = TTR_REF_TO_PTR(TTRMesh, tctx.tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_mesh(&tctx, lasset, tmesh);
                        found = true;
                    } break;
                case ' JBO': // OBJ
                    {
                        TTRObject *tobject = TTR_REF_TO_PTR(TTRObject, tctx.tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_object(&tctx,lasset, tobject);
                        found = true;
                    }break;
                case ' XET': // TEX
                    {
                        TTRTexture *ttex = TTR_REF_TO_PTR(TTRTexture, tctx.tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Texture;
                        asset = (TessAsset*)tess_load_texture(as, lasset, ttex);
                        found = true;
                    } break;
                // NOTE: loading map asset does not load the map
                // map asset is only a container of data that is
                // further handled by a different subsystem
                case ' PAM': // MAP
                    {
                        TTRMap *tmap = TTR_REF_TO_PTR(TTRMap, tctx.tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Map;
                        asset = (TessAsset*)tess_load_map_asset(as, lasset, tmap);
                        found = true;
                    } break;
                default:
                    fprintf(stderr, "Trying to load unknown asset type!\n");
                    break;
            }
        }
        if(found)
            break;
    }
    if(!found)
    {
        // nothing was loaded
        // TODO: log error or something??
        fprintf(stderr, "TTR file %s loaded, but asset %s was not found in it!\n", tfile->filePath, lasset->assetId->cstr);
    }
    return asset;
}

TStr* tess_asset_id_from_aref(TessAssetSystem *as, TTRDescTbl *tbl, TTRImportTbl *imTbl, TTRAssetRef aref, TStr *currentPackage)
{
    uint32_t meshEntryIdx = (aref.tblIndex & ~TTR_AREF_EXTERN_MASK);
    TStr *meshAssetName, *meshPackageName;
    if(TTR_IS_EXTERN_AREF(aref))
    {
        TTRImportTblEntry *imEnt = &imTbl->entries[meshEntryIdx];
        meshAssetName = tess_intern_string_s(as->tstrings, imEnt->assetName, TTR_MAX_NAME_LEN);
        meshPackageName = tess_intern_string_s(as->tstrings, imEnt->packageName, TTR_MAX_NAME_LEN);
    }
    else
    {
        meshAssetName = tess_intern_string_s(as->tstrings, tbl->entries[meshEntryIdx].assetName, TTR_MAX_NAME_LEN);
        meshPackageName = currentPackage;
    }
    TStr *meshAssetId = tess_get_asset_id(as, meshPackageName, meshAssetName);
    return meshAssetId;
}


TStr *tess_get_asset_id(TessAssetSystem *as, TStr *package, TStr *asset)
{
    char buf[TTR_MAX_NAME_LEN*2 + 2];
    assert(package->len + asset->len + 2 < ARRAY_COUNT(buf));
    strcpy(buf, package->cstr);
    strcat(buf, "/");
    strcat(buf, asset->cstr);
    return tess_intern_string_s(as->tstrings, buf, ARRAY_COUNT(buf));
}

TStr *tess_get_asset_name_from_id(TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    assert(strlen(name) > 1);
    return tess_intern_string(as->tstrings, name + 1);
}

TStr *tess_get_asset_package_from_id(TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    uint32_t len = name - assetId->cstr;
    TStr *ret = tess_intern_string_s(as->tstrings, assetId->cstr, len);
    return ret;
}

internal void loading_asset_init(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId) {
    int dummy;
    assert(lasset);
    lasset->fileName = fileName;
    lasset->as = as;
    lasset->file = NULL;
    lasset->assetId = assetId;
    lasset->type = Tess_Asset_Unknown;
    buf_push(as->loadingAssets, lasset);
    khiter_t k = kh_put(64, as->loadingAssetMap, (intptr_t)assetId, &dummy);
    kh_value(as->loadingAssetMap, k) = lasset;
}

bool tess_queue_asset(TessAssetSystem *as, TStr *assetId)
{
    int dummy;
    if(tess_is_asset_loaded(as, assetId))
        return true;
    if(tess_is_asset_loading(as, assetId))
        return false;
    
    printf("Queue asset %s\n", assetId->cstr);

    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName)) {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        loading_asset_init(as, lasset, fileName, assetId);
        tess_set_asset_status(as, assetId, Tess_Asset_Status_InQueue);
        scheduler_queue_task(tess_task_load_asset, (void*)lasset);
        // TODO: instead of immediately loading it, we just wait till the asset system picks it up itself
    }
    else {
        tess_set_asset_status(as, assetId, Tess_Asset_Status_Fail);
        printf("FAILED TO LOAD ASSET %s\n", assetId->cstr);
    }
    return false;
}

TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    if(k == kh_end(as->loadedAssetMap))
        return NULL;
    TessAsset *ret = kh_value(as->loadedAssetMap, k);
    return ret;
}

internal void asset_loaded(TessAssetSystem *as, TessAsset *asset)
{
    int dummy;
    assert(asset);
    khiter_t k = kh_put(64, as->loadedAssetMap, (intptr_t)asset->assetId, &dummy);
    kh_value(as->loadedAssetMap, k) = asset;

    DELEGATE_INVOKE(as->onAssetLoaded, as, asset);
}

internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, TessAssetStatus newStatus)
{
    khiter_t k;
    // asset loaded!
    assert(lasset->file);
    as->numOpenAssetFiles--;
    tess_unload_file(as->fileSystem, lasset->file);
    lasset->file = NULL;
    int find = buf_find_idx(as->loadingAssets, lasset);
    assert(find != -1);
    buf_remove_at(as->loadingAssets, find);

    // remove from loadingAssetMap
    k = kh_get(64, as->loadingAssetMap, (intptr_t)lasset->assetId);
    assert(k != kh_end(as->loadingAssetMap));
    kh_del(64, as->loadingAssetMap, k);

    // set new status
    k = kh_get(uint32, as->assetStatusMap, (intptr_t)lasset->assetId);
    assert(k != kh_end(as->assetStatusMap));
    kh_val(as->assetStatusMap, k) = newStatus;

    uint32_t count = buf_len(as->loadingAssets);
    pool_free(as->loadingAssetPool, lasset);
}

bool tess_are_all_loads_complete(TessAssetSystem *as)
{
    int cleft = buf_len(as->loadingAssets);
    //printf("\x1B[31mLeft to load: %d\n\x1B[0m", cleft);
    return cleft == 0;
}

#define STRUCT_IN_RANGE(start, size, ptr) ((uint8_t*)(ptr) >= (uint8_t*)(start) && ((uint8_t*)(ptr) + sizeof(*(ptr))) <= ((uint8_t*)(start)+(size)))
#define ARRAY_IN_RANGE(start, size, array, len) ((uint8_t*)(array) >= (uint8_t*)(start) && ((uint8_t*)(array) + sizeof((array)[0])*len) <= ((uint8_t*)(start)+(size)))

// all characters except A-z and 0-9 get replaced with '-'
// the intention of this is to make id names safe so we don't have to worry about
// someone hacking around with / /n /t %s and stuff like that
// maybe it's worth not being lazy here?
internal void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen)
{
    for(int i = 0; i < buflen; i++)
    {
        if(!name[i])
        {
            buf[i] = 0;
            return;
        }
        if((name[i] >= 'A' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9'))
            buf[i] = name[i];
        else
            buf[i] = '-';
    }
    fprintf(stderr, "string passed to tess_escape_id_name() wasn't null terminated!\n");
    assert(false);
}

TStr* intern_asset_id(TessAssetSystem *as, const char *package, const char *asset) {
    char nameBuf[TTR_MAX_NAME_LEN];
    tess_replace_id_name_characters(nameBuf, asset, ARRAY_COUNT(nameBuf));
    // TODO: no reason to intern name and package, do it directly
    TStr *internedName = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);
    tess_replace_id_name_characters(nameBuf, package, ARRAY_COUNT(nameBuf));
    TStr *internedPackage = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);
    return tess_get_asset_id(as, internedPackage, internedName);
}

internal void tess_refresh_package_list(TessAssetSystem *as)
{
    char pdirPath[AIKE_MAX_PATH];
    AikeFileEntry entries[64];
    int count;
    AikePlatform *platform = as->fileSystem->platform;
    strcpy(pdirPath, "Packages");
    AikeDirectory *dir = platform->open_directory(platform, pdirPath);
    if(!dir)
    {
        fprintf(stderr, "Unable to read 'Packages' directory\n");
        exit(-1);
    }
    buf_clear(as->packageList);
    while((count = platform->next_files(platform, dir, entries, ARRAY_COUNT(entries))))
    {
        for(int i = 0; i < count; i++)
        {
            if(entries[i].type == Aike_File_Entry_Directory)
            {
                TStr *packageName = tess_intern_string(as->tstrings, entries[i].name);
                buf_push(as->packageList, packageName);
            }
        }
    }
    platform->close_directory(platform, dir);
}

internal void tess_gen_lookup_cache_for_package(TessAssetSystem *as, TStr *packageName)
{
    char pdirPath[AIKE_MAX_PATH];
    AikeFileEntry entries[64];
    int count;
    int dummy;
    AikePlatform *platform = as->fileSystem->platform;

    // have we already created a cache for this package?
    // TODO: if packageName is interned then we don't even need string keys!
    khiter_t key = kh_get(str, as->packageAssetMap, packageName->cstr);
    if(key != kh_end(as->packageAssetMap))
    {
        printf("Package cache for %s already exists!\n", packageName->cstr);
        return;
    }

    AssetLookupCache *cache = pool_allocate(as->assetLookupCachePool);
    assert(cache);
    cache->entries = NULL;
    key = kh_put(str, as->packageAssetMap, packageName->cstr, &dummy);
    kh_value(as->packageAssetMap, key) = cache;

    strcpy(pdirPath, "Packages/");
    strcat(pdirPath, packageName->cstr);
    AikeDirectory *dir = platform->open_directory(platform, pdirPath);
    if(!dir)
        return;
    while((count = platform->next_files(platform, dir, entries, ARRAY_COUNT(entries))))
    {
        for(int i = 0; i < count; i++)
        {
            char *extension = strrchr(entries[i].name, '.');
            bool isttr = extension && strcmp(extension, ".ttr") == 0;
            if(entries[i].type == Aike_File_Entry_File && isttr)
            {
                strcpy(pdirPath, "Packages/");
                strcat(pdirPath, packageName->cstr);
                strcat(pdirPath, "/");
                strcat(pdirPath, entries[i].name);
                AikeMemoryBlock *block = platform->map_file(platform, pdirPath, 0, 0);
                TTRHeader *hdr = (TTRHeader*)block->memory;
                if(!STRUCT_IN_RANGE(block->memory, block->size, hdr))
                    goto unmap_file_and_continue; // missing header
                TTRDescTbl *dtbl = TTR_REF_TO_PTR(TTRDescTbl, hdr->descTblRef);
                if(!STRUCT_IN_RANGE(block->memory, block->size, dtbl))
                    goto unmap_file_and_continue; // corrupt header/descTbl
                if(!ARRAY_IN_RANGE(block->memory, block->size, dtbl->entries, dtbl->entryCount))
                    goto unmap_file_and_continue; // corrupt header/descTbl

                for(int j = 0; j < dtbl->entryCount; j++)
                {
                    char ebuf[TTR_MAX_NAME_LEN];
                    tess_replace_id_name_characters(ebuf, dtbl->entries[j].assetName, TTR_MAX_NAME_LEN);
                    AssetLookupEntry *entry = pool_allocate(as->assetLookupEntryPool);
                    assert(entry);
                    entry->packageName = packageName;
                    entry->fileName = tess_intern_string(as->tstrings, entries[i].name);
                    entry->assetName = tess_intern_string(as->tstrings, ebuf);
                    //printf("asset %s\n", ebuf);
                    entry->assetType = dtbl->entries[j].type;
                    buf_push(cache->entries, entry);
                }

unmap_file_and_continue:
                platform->unmap_file(platform, block);
            }
        }
    }
    platform->close_directory(platform, dir);
}

bool get_asset_file(TessAssetSystem *as, TStr *assetId, TStr **result)
{
    TStr *package = tess_get_asset_package_from_id(as, assetId);
    TStr *assetName = tess_get_asset_name_from_id(as, assetId);
    khiter_t k = kh_get(str, as->packageAssetMap, package->cstr);
    if(k == kh_end(as->packageAssetMap))
    {
        fprintf(stderr, "get_asset_file() called for package %s, but no cache for that package!\n", package->cstr);
        return false;
    }
    __auto_type cache = (AssetLookupCache*)kh_value(as->packageAssetMap, k);
    int count = buf_len(cache->entries);
    for(int i = 0; i < count; i++)
    {
        if(assetName->cstr == cache->entries[i]->assetName->cstr)
        {
            *result = cache->entries[i]->fileName;
            return true;
        }
    }
    return false;
}
