
void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen);
void tess_fill_mesh_data(Renderer *renderer, MeshQueryResult *mqr, void *userData);
void tess_finalize_mesh(Renderer *renderer, MeshReady *mr, void *userData);

void tess_load_mesh(TessAssetSystem *as, TTRMesh *tmesh, TessLoadingAsset *lasset)
{
    TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(TTRMeshDesc, tmesh->descRef);
    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.userData = (void*)lasset;
    msg.meshQuery.onComplete = tess_fill_mesh_data;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = tmesh->numVertices;
    msg.meshQuery.indexCount = tmesh->numIndices;
    for(int j = 0; j < ttrMeshDesc->numAttrs; j++)
        msg.meshQuery.attributeTypes[j] = ttrMeshDesc->attrs[j];
    renderer_queue_message(as->renderer, &msg);
    printf("LOAD MESH\n");
}

void tess_fill_mesh_data(Renderer *renderer, MeshQueryResult *mqr, void *userData)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)userData;
    TTRMesh *tmesh = lasset->meshData.tmesh;
    TessAssetSystem *as = lasset->meshData.as;
    TTRBuffer *vbuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->vertBufRef);
    TTRBuffer *ibuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->indexBufRef);
    TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(TTRMeshDesc, tmesh->descRef);
    memcpy(mqr->vertBufPtr, vbuf->data, vbuf->size);
    memcpy(mqr->idxBufPtr, ibuf->data, ibuf->size);
    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Update;
    msg.meshUpdate.meshId = mqr->meshId;
    msg.meshUpdate.onComplete = tess_finalize_mesh;
    msg.meshUpdate.userData = (void*)lasset;
    renderer_queue_message(renderer, &msg);
    printf("FILL MESH\n");
}

void tess_finalize_mesh(Renderer *renderer, MeshReady *mr, void *userData)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)userData;
    TessAssetSystem *as = lasset->meshData.as;
    TessMeshAsset *mesh = pool_allocate(as->meshPool);
    assert(mesh);
    mesh->meshId = mr->meshId;
    mesh->asset.assetId = lasset->assetId;
    mesh->asset.type = Tess_Asset_Mesh;
    buf_push(as->loadedAssets, &mesh->asset);
    lasset->status = Tess_Asset_Dependency_Status_Done;
    lasset->asset = &mesh->asset;
    printf("FINALIZE MESH\n");
}

void tess_process_ttr_file(TessAssetSystem *as, TessFile *tfile)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)tfile->userData;
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

        if(internedName == assetName)// TODO: should break if found
        {
            switch(entry->type) 
            {
                case TTR_4CHAR("MESH"):
                    {
                        lasset->meshData.as = as;
                        TTRMesh *tmesh = TTR_REF_TO_PTR(TTRMesh, tbl->entries[i].ref);
                        lasset->meshData.tmesh = tmesh;
                        lasset->type = Tess_Asset_Mesh;
                        tess_load_mesh(as, tmesh, lasset);
                        found = true;
                    } break;
                case TTR_4CHAR("OBJ "):
                    {
                        lasset->status = Tess_Asset_Dependency_Status_Done;
                        TTRObject *tobject = TTR_REF_TO_PTR(TTRObject, tbl->entries[i].ref);
                        TTRAssetRef aref = tobject->meshARef;
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
                            meshPackageName = packageName;
                        }

                        TessObjectAsset *tessObj = pool_allocate(as->objectPool);
                        tessObj->asset.assetId = lasset->assetId;
                        tessObj->asset.type = Tess_Asset_Object;
                        lasset->asset = &tessObj->asset;

                        lasset->objectData.meshAssetId = tess_get_asset_id(as, meshPackageName, meshAssetName);
                        tess_load_asset_if_not_loaded(as, lasset->objectData.meshAssetId);
                        lasset->type = Tess_Asset_Object;
                        found = true;
                    }break;
                default:
                    fprintf(stderr, "Trying to load unknown asset type!\n");
                    break;
            }
        }
    }
    if(!found)
    {
        // nothing was loaded
        // TODO: log error or something??
        fprintf(stderr, "TTR file loaded, but asset %s was not found in it!", lasset->assetId->cstr);
    }
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

bool tess_is_asset_loaded(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadedAssetMap);
}

bool tess_is_asset_loading(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadingAssetMap);
}

void tess_load_asset_if_not_loaded(TessAssetSystem *as, TStr *assetId)
{
    int dummy;
    if(tess_is_asset_loaded(as, assetId))
        return;
    if(tess_is_asset_loading(as, assetId))
        return;
    
    printf("Load asset Asset: %s\n", assetId->cstr);

    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName))
    {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        lasset->file = NULL;
        lasset->asset = NULL;
        lasset->assetId = assetId;
        lasset->type = Tess_Asset_Unknown;
        lasset->status = Tess_Asset_Dependency_Status_Loading;

        buf_push(as->loadingAssets, lasset);
        khiter_t k = kh_put(64, as->loadingAssetMap, (intptr_t)assetId, &dummy);
        kh_value(as->loadingAssetMap, k) = lasset;

        assert(lasset);
        char buf[AIKE_MAX_PATH];
        buf[0] = 0;
        strcpy(buf, "Packages/");
        strcat(buf, packageName->cstr);
        strcat(buf, "/");
        strcat(buf, fileName->cstr);
        // TODO: overflow protect?
        tess_load_file(as->fileSystem, buf, Tess_File_Pipeline_TTR, (void*)lasset);
    }
    else
    {
        fprintf(stderr, "Asset not found %s \n", assetId->cstr);
    }
}

TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    if(k == kh_end(as->loadedAssetMap))
        return NULL;
    TessAsset *ret = kh_value(as->loadedAssetMap, k);
    return ret;
}

void tess_check_complete(TessAssetSystem *as)
{
    int count = buf_len(as->loadingAssets);
    for(int i = 0; i < count;)
    {
        TessLoadingAsset *lasset = as->loadingAssets[i];
        bool done = false;
        switch(lasset->type)
        {
            case Tess_Asset_Mesh:
                if(lasset->status == Tess_Asset_Dependency_Status_Done)
                    done = true;
                break;
            case Tess_Asset_Object:
                assert(lasset->status == Tess_Asset_Dependency_Status_Done);
                if(tess_is_asset_loaded(as, lasset->objectData.meshAssetId))
                {
                    TessObjectAsset *obj = (TessObjectAsset*)lasset->asset;
                    TessAsset *meshAsset = tess_get_asset(as, lasset->objectData.meshAssetId);
                    assert(meshAsset);
                    // TODO: object expected it to be mesh
                    // but it wasnt, so load failed, unload whatever was loaded
                    assert(meshAsset->type == Tess_Asset_Mesh);
                    obj->mesh = (TessMeshAsset*)meshAsset;
                    done = true;
                }
                break;
            case Tess_Asset_Unknown: // most likely file not loaded yet
                break;
            default:
                fprintf(stderr, "Unknown asset, can't finalize! %d %s\n", lasset->type, lasset->assetId->cstr);
                break;
        }
        if(done)
        {
            int dummy;
            khiter_t k = kh_put(64, as->loadedAssetMap, (intptr_t)lasset->assetId, &dummy);
            assert(lasset->asset);
            kh_value(as->loadedAssetMap, k) = lasset->asset;

            // asset loaded!
            assert(lasset->file);
            tess_unload_file(as->fileSystem, lasset->file);
            int find = buf_find_idx(as->loadingAssets, lasset);
            assert(find != -1);
            buf_remove_at(as->loadingAssets, find);
            k = kh_get(64, as->loadingAssetMap, (intptr_t)lasset->assetId);
            assert(k != kh_end(as->loadingAssetMap));
            kh_del(64, as->loadingAssetMap, k);
            count = buf_len(as->loadingAssets);
            pool_free(as->loadingAssetPool, lasset);
            printf("Asset loaded! %s\n", lasset->assetId->cstr);

            // NOTE: no increment because we deleted this element
            // so next element will have this id
        }
        else            
            i++;
    }
}

bool tess_are_all_loads_complete(TessAssetSystem *as)
{
    return buf_len(as->loadingAssets) == 0;
}

#define STRUCT_IN_RANGE(start, size, ptr) ((uint8_t*)(ptr) >= (uint8_t*)(start) && ((uint8_t*)(ptr) + sizeof(*(ptr))) <= ((uint8_t*)(start)+(size)))
#define ARRAY_IN_RANGE(start, size, array, len) ((uint8_t*)(array) >= (uint8_t*)(start) && ((uint8_t*)(array) + sizeof((array)[0])*len) <= ((uint8_t*)(start)+(size)))

// all characters except A-z and 0-9 get replaced with '-'
// the intention of this is to make id names safe so we don't have to worry about
// someone hacking around with / /n /t %s and stuff like that
void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen)
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

void tess_gen_lookup_cache_for_package(TessAssetSystem *as, TStr *packageName)
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
