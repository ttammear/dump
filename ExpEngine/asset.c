
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
internal void prepare_loading_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId);
internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, TessAssetStatus newStatus);
internal void asset_loaded(TessAssetSystem *as, TessAsset *asset);

internal inline void tess_set_asset_status(TessAssetSystem *as, TStr *assetId, uint32_t status)
{
    int dummy;
    khiter_t k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
    kh_value(as->assetStatusMap, k) = status;
}

internal inline TessAssetStatus tess_get_asset_status(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(uint32, as->assetStatusMap, (intptr_t)assetId);
    return k == kh_end(as->assetStatusMap) ? Tess_Asset_Status_None : kh_value(as->assetStatusMap, k);
}

internal inline bool tess_is_asset_loaded(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadedAssetMap);
}

internal inline bool tess_is_asset_loading(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadingAssetMap);
}

void tess_get_asset_metrics(struct TessAssetSystem *as, struct TessAssetSystemMetrics *tasm) {
    tasm->numLoadingAssets = buf_len(as->loadingAssets);
    tasm->numLoadedAssets = buf_len(as->loadedAssets);
    tasm->numOpenedAssetFiles = as->numOpenAssetFiles;
    tasm->totalFileLoads = as->totalFileLoads;
}

void tess_finalize_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TessAsset *asset) {
    printf("Asset loaded! %s\n", lasset->assetId->cstr);
    tess_loading_asset_done(as, lasset, Tess_Asset_Status_Loaded);
    asset_loaded(as, asset);
    buf_push(as->loadedAssets, asset);
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
    texture->asset.assetId = lasset->assetId;
    texture->asset.type = Tess_Asset_Texture;

    tess_finalize_asset(as, lasset, &texture->asset);

    return texture;
}

ASYNC TessMeshAsset* tess_load_mesh(TessAssetSystem *as, TessLoadingAsset *lasset, TTRMesh *tmesh) {
    AsyncTask task;
    TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(TTRMeshDesc, tmesh->descRef);
    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = tmesh->numVertices;
    msg.meshQuery.indexCount = tmesh->numIndices;
    for(int j = 0; j < ttrMeshDesc->numAttrs; j++) {
        msg.meshQuery.attributeTypes[j] = ttrMeshDesc->attrs[j];
    }
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TTRBuffer *vbuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->vertBufRef);
    TTRBuffer *ibuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->indexBufRef);
    // TODO: check if buffer sizes match
    memcpy(task.renderMsg->meshQueryResult.vertBufPtr, vbuf->data, vbuf->size);
    memcpy(task.renderMsg->meshQueryResult.idxBufPtr, ibuf->data, ibuf->size);
    msg = (const RenderMessage){};
    msg.type = Render_Message_Mesh_Update;
    msg.meshUpdate.meshId = task.renderMsg->meshQueryResult.meshId;
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TessMeshAsset *mesh = pool_allocate(as->meshPool);
    assert(mesh);
    mesh->meshId = task.renderMsg->meshR.meshId;
    mesh->asset.assetId = lasset->assetId;
    mesh->asset.type = Tess_Asset_Mesh;

    tess_finalize_asset(as, lasset, &mesh->asset);

    return mesh;
}

ASYNC TessObjectAsset* tess_load_object(TessAssetSystem *as, TessLoadingAsset *lasset, TTRObject *tobj, TTRDescTbl *ttbl, TTRImportTbl *titbl) {
    scheduler_assert_task(); // async function
    AsyncTask task;
    TessAsset *texAsset, *meshAsset;
    TessObjectAsset *tessObj;
    TTRAssetRef aref = tobj->meshARef;
    TStr *curPackage = tess_get_asset_package_from_id(as, lasset->assetId);
    TStr *meshAssetId = tess_asset_id_from_aref(as, ttbl, titbl, aref, curPackage);
    TStr *textureAssetId;
    meshAsset = tess_force_load_asset(as, meshAssetId);
    assert(meshAsset != NULL);
    TTRMaterial *ttrMat = TTR_REF_TO_PTR(TTRMaterial, tobj->materialRef);
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
        aref = ttrMat->albedoTexARef;
        textureAssetId = tess_asset_id_from_aref(as, ttbl, titbl, aref, curPackage);
        texAsset = tess_force_load_asset(as, textureAssetId);
        break;
    }
    lasset->type = Tess_Asset_Object;

    // TODO: this should be valid here
    //meshAsset = tess_get_asset(as, meshAssetId);
    assert(meshAsset && meshAsset->type == Tess_Asset_Mesh);

    RenderMessage msg = {};
    msg.type = Render_Message_Material_Query;
    msg.matQ.materialId = 0;
    msg.matQ.shaderId = ttrMat->shaderType;
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured:
        case Shader_Type_Unlit_Textured_Cutout:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg.matQ.iData.unlitTextured.textureId = texa->textureId;
            } else {
                msg.matQ.iData.unlitTextured.textureId = 0;
            }
            msg.matQ.iData.unlitTextured.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Fade:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg.matQ.iData.unlitFade.textureId = texa->textureId;
            } else {
                msg.matQ.iData.unlitFade.textureId = 0;
            }
            msg.matQ.iData.unlitFade.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Color:
            msg.matQ.iData.unlitColor.color = ttrMat->tintColor;
            break;
        case Shader_Type_Gizmo:
            msg.matQ.iData.gizmoMat.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Vertex_Color:
            msg.matQ.iData.unlitVertexColor.color = ttrMat->tintColor;
            break;
        default:
            fprintf(stderr, "Object material had unknown shader type: %d\n", ttrMat->shaderType);
            break;
    }
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    tessObj = pool_allocate(as->objectPool);
    assert(tessObj);
    tessObj->asset.assetId = lasset->assetId;
    tessObj->asset.type = Tess_Asset_Object;
    tessObj->materialId = task.renderMsg->matR.materialId;
    tessObj->mesh = (TessMeshAsset*)meshAsset;

    tess_finalize_asset(as, lasset, &tessObj->asset);

    return tessObj;
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

ASYNC TessAsset* tess_force_load_asset(TessAssetSystem *as, TStr *assetId) {
    int dummy;
    printf("Force load asset %s\n", assetId->cstr);
    // TODO: event based, no polling!
    //assert(!tess_is_asset_loading(as, assetId));
    bool sb = false;
    int i = 0;
    while(tess_is_asset_loading(as, assetId)) {
        sb = true;
        scheduler_yield();
    }
    if(tess_is_asset_loaded(as, assetId)) {
        return tess_get_asset(as, assetId);
    }
    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName)) {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        prepare_loading_asset(as, lasset, fileName, assetId);
        return tess_load_asset(lasset);
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
    TTRHeader *header = (TTRHeader*)fileBuf;
    TTRDescTbl *tbl = TTR_REF_TO_PTR(TTRDescTbl, header->descTblRef);
    TTRImportTbl *imTbl = TTR_REF_TO_PTR(TTRImportTbl, header->importTblRef);
    // TODO: maybe this should be cached in TessAsset?
    TStr *assetName = tess_get_asset_name_from_id(as, lasset->assetId);
    TStr *packageName = tess_get_asset_package_from_id(as, lasset->assetId);
    bool found = false;
    for(int i = 0; i < tbl->entryCount; i++)
    {
        TTRDescTblEntry *entry = &tbl->entries[i];
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
                        TTRMesh *tmesh = TTR_REF_TO_PTR(TTRMesh, tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_mesh(as, lasset, tmesh);
                        found = true;
                    } break;
                case ' JBO': // OBJ
                    {
                        TTRObject *tobject = TTR_REF_TO_PTR(TTRObject, tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_object(as, lasset, tobject, tbl, imTbl);
                        found = true;
                    }break;
                case ' XET': // TEX
                    {
                        TTRTexture *ttex = TTR_REF_TO_PTR(TTRTexture, tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Texture;
                        asset = (TessAsset*)tess_load_texture(as, lasset, ttex);
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

internal void prepare_loading_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId) {
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
        prepare_loading_asset(as, lasset, fileName, assetId);
        tess_set_asset_status(as, assetId, Tess_Asset_Status_InQueue);
        scheduler_queue_task(tess_task_load_asset, (void*)lasset);
        // instead of immediately loading it, we just wait till the asset system picks it up itself
    }
    else {
        tess_set_asset_status(as, assetId, Tess_Asset_Status_Fail);
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
                    printf("asset %s\n", ebuf);
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
