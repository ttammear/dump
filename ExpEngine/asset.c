
internal void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen);
TStr* tess_asset_id_from_aref(TessAssetSystem *as, TTRDescTbl *tbl, TTRImportTbl *imTbl, TTRAssetRef aref, TStr *currentPackage);
internal inline void add_dependency(TessLoadingAsset *lasset, TStr *assetId);
void tess_fill_mesh_data(Renderer *renderer, MeshQueryResult *mqr, void *userData);
void tess_finalize_mesh(Renderer *renderer, MeshReady *mr, void *userData);
void tess_fill_texture_data(Renderer *renderer, TextureQueryResponse *tqr, void *userData);
void tess_finalize_texture(Renderer *renderer, TextureReady *tr, void *userData);
void tess_finalize_object(Renderer *renderer, MaterialReady *mr, void *userData);
void tess_load_object2(TessAssetSystem *as, TessLoadingAsset *lasset);
TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId);

internal void tess_load_texture(TessAssetSystem *as, TTRTexture *ttex, TessLoadingAsset *lasset)
{
    RenderMessage msg = {};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.userData = (void*)lasset;
    msg.texQ.onComplete = tess_fill_texture_data;
    msg.texQ.textureId = 0;
    msg.texQ.width = ttex->width;
    msg.texQ.height = ttex->height;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Trilinear;
    renderer_queue_message(as->renderer, &msg);
    printf("LOAD TEXTURE\n");
}

void tess_fill_texture_data(Renderer *renderer, TextureQueryResponse *tqr, void *userData)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)userData;
    TTRTexture *ttex = lasset->texData.ttex;
    TessAssetSystem *as = lasset->texData.as;
    TTRBuffer *tbuf = TTR_REF_TO_PTR(TTRBuffer, ttex->bufRef);
    assert(tbuf->size == ttex->width*ttex->height*4);
    memcpy(tqr->textureDataPtr, tbuf->data, tbuf->size);
    RenderMessage msg = {0};
    msg.type = Render_Message_Texture_Update;
    msg.texU.textureId = tqr->textureId;
    msg.texU.userData = userData;
    msg.texU.onComplete = tess_finalize_texture;
    renderer_queue_message(renderer, &msg);
    printf("FILL TEXTURE\n");
}

void tess_finalize_texture(Renderer *renderer, TextureReady *tr, void *userData)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)userData;
    TessAssetSystem *as = lasset->texData.as;
    TessTextureAsset *texture = pool_allocate(as->texturePool);
    assert(texture);
    texture->textureId = tr->textureId;
    texture->asset.assetId = lasset->assetId;
    texture->asset.type = Tess_Asset_Texture;
    buf_push(as->loadedAssets, &texture->asset);
    lasset->status = Tess_Asset_Dependency_Status_Done;
    lasset->asset = &texture->asset;
    printf("FINALIZE TEXTURE %d\n", tr->textureId);
}

internal void tess_load_mesh(TessAssetSystem *as, TTRMesh *tmesh, TessLoadingAsset *lasset)
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
    // TODO: check if buffer sizes match
    memcpy(mqr->vertBufPtr, vbuf->data, vbuf->size);
    memcpy(mqr->idxBufPtr, ibuf->data, ibuf->size);
    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Update;
    msg.meshUpdate.meshId = mqr->meshId;
    msg.meshUpdate.onComplete = tess_finalize_mesh;
    msg.meshUpdate.userData = (void*)lasset;
    renderer_queue_message(renderer, &msg);
    printf("FILL MESH %d %d\n", tmesh->numVertices, tmesh->numIndices);
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
    printf("FINALIZE MESH %d\n", mr->meshId);
}

void tess_load_object(TessAssetSystem *as, TTRObject *tobject, TessLoadingAsset *lasset, TTRDescTbl *tbl, TTRImportTbl *imTbl)
{
    TTRAssetRef aref = tobject->meshARef;
    TStr *curPackage = tess_get_asset_package_from_id(as, lasset->assetId);
    TStr *meshAssetId = tess_asset_id_from_aref(as, tbl, imTbl, aref, curPackage);
    TStr *textureAssetId;
    lasset->objectData.meshAssetId = meshAssetId;

    add_dependency(lasset, meshAssetId);

    TTRMaterial *ttrMat = TTR_REF_TO_PTR(TTRMaterial, tobject->materialRef);
    switch(ttrMat->shaderType)
    {
        case Shader_Type_Unlit_Textured:
            aref = ttrMat->albedoTexARef;
            textureAssetId = tess_asset_id_from_aref(as, tbl, imTbl, aref, curPackage);
            add_dependency(lasset, textureAssetId);
            lasset->objectData.textureAssetId = textureAssetId;
            break;
    }

    lasset->status = Tess_Asset_Dependency_Status_Wait_Dependencies;
    lasset->type = Tess_Asset_Object;
    lasset->objectData.tobj = tobject;
    lasset->objectData.as = as;
    lasset->onDependenciesLoaded = tess_load_object2;
}

void tess_load_object2(TessAssetSystem *as, TessLoadingAsset *lasset)
{
    printf("process %s\n", lasset->assetId->cstr);
    TTRObject *tobj = lasset->objectData.tobj;
    TTRMaterial *ttrMat = TTR_REF_TO_PTR(TTRMaterial, tobj->materialRef);
    RenderMessage msg = {};
    msg.type = Render_Message_Material_Query;
    msg.matQ.userData = lasset;
    msg.matQ.onComplete = tess_finalize_object;
    msg.matQ.materialId = 0;
    msg.matQ.shaderId = ttrMat->shaderType;

    TessAsset *texAsset;

    switch(ttrMat->shaderType)
    {
        case Shader_Type_Unlit_Textured:
            // TODO: get tex
            texAsset = tess_get_asset(as, lasset->objectData.textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture)
            {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg.matQ.iData.unlitTextured.textureId = texa->textureId;
            }
            else
                msg.matQ.iData.unlitTextured.textureId = 0;
            msg.matQ.iData.unlitTextured.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Color:
            msg.matQ.iData.unlitColor.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Vertex_Color:
            msg.matQ.iData.unlitVertexColor.color = ttrMat->tintColor;
            break;
        default:
            lasset->status = Tess_Asset_Dependency_Status_Fail;
            fprintf(stderr, "Object material had unknown shader type: %d\n", ttrMat->shaderType);
            break;
    };
    renderer_queue_message(as->renderer, &msg);
}

void tess_finalize_object(Renderer *renderer, MaterialReady *mr, void *userData)
{
    TessLoadingAsset *lasset = (TessLoadingAsset*)userData;
    TessAssetSystem *as = lasset->objectData.as;
    TessObjectAsset *tessObj = pool_allocate(as->objectPool);
    assert(tessObj);
    tessObj->asset.assetId = lasset->assetId;
    tessObj->asset.type = Tess_Asset_Object;
    tessObj->materialId = mr->materialId;
    lasset->asset = &tessObj->asset;
    lasset->status = Tess_Asset_Dependency_Status_Done;
    printf("FINALIZE OBJECT MAT %d\n", mr->materialId);
}

internal inline void add_dependency(TessLoadingAsset *lasset, TStr *assetId)
{
    // TODO: use pool
    struct TessAssetDependency *dep = malloc(sizeof(struct TessAssetDependency));
    dep->next = lasset->dependencies;
    dep->assetId = assetId;
    lasset->dependencies = dep;
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
                case 'HSEM': // MESH // TODO: multi byte characters are endianness sensitive
                    {
                        lasset->meshData.as = as;
                        TTRMesh *tmesh = TTR_REF_TO_PTR(TTRMesh, tbl->entries[i].ref);
                        lasset->meshData.tmesh = tmesh;
                        lasset->type = Tess_Asset_Mesh;
                        tess_load_mesh(as, tmesh, lasset);
                        found = true;
                    } break;
                case ' JBO': // OBJ
                    {
                        TTRObject *tobject = TTR_REF_TO_PTR(TTRObject, tbl->entries[i].ref);
                        tess_load_object(as, tobject, lasset, tbl, imTbl);
                        found = true;
                    }break;
                case ' XET':
                    {
                        lasset->texData.as = as;
                        TTRTexture *ttex = TTR_REF_TO_PTR(TTRTexture, tbl->entries[i].ref);
                        lasset->texData.ttex = ttex;
                        lasset->type = Tess_Asset_Texture;
                        tess_load_texture(as, ttex, lasset);
                        found = true;
                    } break;
                default:
                    lasset->status = Tess_Asset_Dependency_Status_Fail;
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

uint32_t tess_get_asset_status(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(uint32, as->assetStatusMap, (intptr_t)assetId);
    return k == kh_end(as->assetStatusMap) ? Tess_Asset_Status_None : kh_value(as->assetStatusMap, k);
}

internal inline void tess_set_asset_status(TessAssetSystem *as, TStr *assetId, uint32_t status)
{
    int dummy;
    khiter_t k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
    kh_value(as->assetStatusMap, k) = status;
}

bool tess_load_asset_if_not_loaded(TessAssetSystem *as, TStr *assetId)
{
    int dummy;
    if(tess_is_asset_loaded(as, assetId))
        return true;
    if(tess_is_asset_loading(as, assetId))
        return false;
    
    printf("Load asset Asset: %s\n", assetId->cstr);

    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName))
    {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        // TODO: make a queue so this never fails
        assert(lasset);
        lasset->file = NULL;
        lasset->asset = NULL;
        lasset->dependencies = NULL;
        lasset->assetId = assetId;
        lasset->type = Tess_Asset_Unknown;
        lasset->status = Tess_Asset_Dependency_Status_Loading;

        buf_push(as->loadingAssets, lasset);
        khiter_t k = kh_put(64, as->loadingAssetMap, (intptr_t)assetId, &dummy);
        kh_value(as->loadingAssetMap, k) = lasset;

        k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
        kh_value(as->assetStatusMap, k) = Tess_Asset_Status_Loading;

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
        khiter_t k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
        kh_value(as->assetStatusMap, k) = Tess_Asset_Status_Fail;
        fprintf(stderr, "Asset not found %s \n", assetId->cstr);
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

internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, uint32_t status)
{
    khiter_t k;
    // asset loaded!
    assert(lasset->file);
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
    kh_val(as->assetStatusMap, k) = status;

    uint32_t count = buf_len(as->loadingAssets);
    pool_free(as->loadingAssetPool, lasset);
}

internal void tess_check_dependencies(TessAssetSystem *as, TessLoadingAsset *lasset)
{
    // TODO: free deps if done
    TessAssetDependency *dep = lasset->dependencies;
    while(dep != NULL)
    {
        uint32_t status;
        status = tess_get_asset_status(as, dep->assetId);
        switch(status)
        {
            case Tess_Asset_Status_Loaded:
                // TODO: should not traverse this dep again?
                dep->asset = tess_get_asset(as, dep->assetId);
                assert(dep->asset);
                break;
            case Tess_Asset_Status_Fail:
                lasset->status = Tess_Asset_Dependency_Status_Fail;
                printf("FFFFFFFFFFFFFFFFFFFFAIL\n");
                return;
            case Tess_Asset_Status_Loading:
                return;
            case Tess_Asset_Status_None:
                tess_load_asset_if_not_loaded(as, dep->assetId);
                return;
            default:
                assert(0);
                return;
        };
        dep = dep->next;
    }
    // only way to get here is if all were loaded
    printf("deps done %s\n", lasset->assetId->cstr);
    lasset->status = Tess_Asset_Dependency_Status_Loading;
    if(lasset->onDependenciesLoaded != NULL)
        lasset->onDependenciesLoaded(as, lasset);
}

internal void tess_check_complete(TessAssetSystem *as)
{
    for(int i = 0; i < buf_len(as->loadingAssets);)
    {
        TessLoadingAsset *lasset = as->loadingAssets[i];
        int curStatus = 0;

        if(lasset->status == Tess_Asset_Dependency_Status_Wait_Dependencies)
        {
            tess_check_dependencies(as, lasset);
            i++;
            continue;
        }
        
        if(lasset->status != Tess_Asset_Dependency_Status_Fail)
        {
            uint32_t status;
            switch(lasset->type)
            {
                case Tess_Asset_Mesh:
                    if(lasset->status == Tess_Asset_Dependency_Status_Done)
                        curStatus = 1;
                    break;
                case Tess_Asset_Texture:
                    if(lasset->status == Tess_Asset_Dependency_Status_Done)
                       curStatus = 1;
                    break;
                case Tess_Asset_Object:
                    if(lasset->status == Tess_Asset_Dependency_Status_Done)
                    {
                        status = tess_get_asset_status(as, lasset->objectData.meshAssetId);
                        if(status == Tess_Asset_Status_Loaded)
                        {
                            TessObjectAsset *obj = (TessObjectAsset*)lasset->asset;
                            TessAsset *meshAsset = tess_get_asset(as, lasset->objectData.meshAssetId);
                            assert(meshAsset);
                            // TODO: object expected it to be mesh
                            // but it wasnt, so load failed, unload whatever was loaded
                            assert(meshAsset->type == Tess_Asset_Mesh);
                            obj->mesh = (TessMeshAsset*)meshAsset;
                            curStatus = 1;
                        }
                        else if(status == Tess_Asset_Status_Fail)
                            curStatus = 2;
                    }
                    break;
                case Tess_Asset_Unknown: // most likely file not loaded yet
                    break;
                default:
                    fprintf(stderr, "Unknown asset, can't finalize! %d %s\n", lasset->type, lasset->assetId->cstr);
                    break;
            }
        }
        else
        {
            // NOTE: will delete current interated loadingAsset
            tess_loading_asset_done(as, lasset, Tess_Asset_Status_Fail);
            fprintf(stderr, "Failed to load asset %s\n", lasset->assetId->cstr);
            curStatus = 2;
            continue; // deleted current dont increment
        }

        if(curStatus == 1)
        {
            printf("Asset loaded! %s\n", lasset->assetId->cstr);
            // NOTE: will delete current interated loadingAsset
            tess_loading_asset_done(as, lasset, Tess_Asset_Status_Loaded);
            asset_loaded(as, lasset->asset);
            continue; // deleted current, no increment
        }
        else if(curStatus == 2)
        {
            tess_loading_asset_done(as, lasset, Tess_Asset_Status_Fail);
            fprintf(stderr, "Failed to load asset %s\n", lasset->assetId->cstr);
            continue; // deleted current, dont increment
        }
        i++;
    }
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
