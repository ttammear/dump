
#define TESS_LOADED_FILE_POOL_SIZE 100
#define TESS_MESH_POOL_SIZE 100
#define TESS_OBJECT_POOL_SIZE 100
#define TESS_LOADING_ASSET_POOL_SIZE 100
#define TESS_DEP_NODE_POOL_SIZE 1000
#define TESS_ASSET_LOOKUP_ENTRY_POOL_SIZE 1000
#define TESS_ASSET_LOOKUP_CACHE_POOL_SIZE 100

#define POOL_FROM_ARENA(pool, arena, size) (pool_init_with_memory((pool), arena_push_size((arena), pool_calc_size((pool), (size))), (size)))

void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen);

void tess_init(struct TessState *tess, AikePlatform *platform, struct Renderer *renderer)
{
    memset(tess, 0, sizeof(struct TessState));
    tess->platform = platform;

    fixed_arena_init(platform, &tess->arena, 512 * 1024); 

    // Init strings
    //
    fixed_arena_init_from_arena(&tess->strings.stringArena, &tess->arena, 64 * 1024);
    tess->strings.internedStrings = NULL;

    // Init file system
    //
    uint32_t poolSize = pool_calc_size(tess->fileSystem.loadedFilePool, TESS_LOADED_FILE_POOL_SIZE);
    void *poolMem = arena_push_size(&tess->arena, poolSize);
    pool_init_with_memory(tess->fileSystem.loadedFilePool, poolMem, TESS_LOADED_FILE_POOL_SIZE);
    tess->fileSystem.platform = platform;
    // Virtual table tells file system where to pass the data once it's done loading
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_None] = NULL;
    tess->fileSystem.pipeline_vtbl[Tess_File_Pipeline_TTR] = (FilePipelineProc)tess_process_ttr_file;
    tess->fileSystem.pipeline_ptrs[Tess_File_Pipeline_TTR] = &tess->assetSystem;

    // Init asset system
    //
    struct TessAssetSystem *as = &tess->assetSystem;
    as->fileSystem = &tess->fileSystem;
    POOL_FROM_ARENA(as->meshPool, &tess->arena, TESS_MESH_POOL_SIZE);
    POOL_FROM_ARENA(as->objectPool, &tess->arena, TESS_OBJECT_POOL_SIZE);
    POOL_FROM_ARENA(as->loadingAssetPool, &tess->arena, TESS_LOADING_ASSET_POOL_SIZE);
    POOL_FROM_ARENA(as->assetLookupCachePool, &tess->arena, TESS_ASSET_LOOKUP_CACHE_POOL_SIZE);
    POOL_FROM_ARENA(as->assetLookupEntryPool, &tess->arena, TESS_ASSET_LOOKUP_ENTRY_POOL_SIZE);
    tess->assetSystem.loadedAssets = NULL;
    tess->assetSystem.loadingAssets = NULL;
    tess->assetSystem.tstrings = &tess->strings;
    tess->assetSystem.renderer = renderer;
    tess->assetSystem.packageAssetMap = kh_init(str);
    tess->assetSystem.loadedAssetMap = kh_init(64);
    tess->assetSystem.loadingAssetMap = kh_init(64);

    // Init game system
    //
    for(int i = 0; i < ARRAY_COUNT(tess->gameSystem.objectTable); i++)
    {
        tess->gameSystem.objectTable[i] = (struct TessObject){0};
        tess->gameSystem.objectTable[i].id = i;
    }
    tess->gameSystem.assetSystem = &tess->assetSystem;
    POOL_FROM_ARENA(tess->gameSystem.entityPool, &tess->arena, TESS_MAX_ENTITIES);
    tess->gameSystem.activeEntities = NULL;
    tess->gameSystem.renderSystem = &tess->renderSystem;
    tess->gameSystem.activeCamera = &tess->gameSystem.defaultCamera;

    // Init input
    //
    tess->inputSystem.platform = platform;
    tess_input_init(&tess->inputSystem);

    // Init render system
    //
    render_system_init(&tess->renderSystem);
    tess->renderSystem.renderer = renderer;

    // Init UI
    //
    tess->uiSystem.renderSystem = &tess->renderSystem;
    tess->uiSystem.platform = platform;
    tess_ui_init(&tess->uiSystem);

    // Init editor
    tess->editor.init = false;
    tess->editor.cursorObjectIdBuf = 0xFFFFFFFF;
    tess->editor.renderSystem = &tess->renderSystem;
    tess->editor.platform = platform;
    tess->editor.uiSystem = &tess->uiSystem;
    tess->editor.tess = tess;
    tess->editor.inputSystem = &tess->inputSystem;
    tess->editor.world = &tess->gameSystem;
    POOL_FROM_ARENA(tess->editor.edEntityPool, &tess->arena, 1000);
}

void tess_destroy(struct TessState *tess)
{
    // Destroy file system
    //
    
    // Destroy asset system
    //
    // TODO: loadedAssets deinit?
    // free AssetLookupCahce entry buf and map
    khiter_t k;
    __auto_type h = tess->assetSystem.packageAssetMap;
    int tes;
    for(k = kh_begin(h); k != kh_end(h); ++k)
    {
        if(kh_exist(h, k))
        {
            struct AssetLookupCache *cache = (struct AssetLookupCache*)kh_value(h, k);
            buf_free(cache->entries);
        }
    }
    kh_destroy(str, tess->assetSystem.packageAssetMap);
    kh_destroy(64, tess->assetSystem.loadedAssetMap);
    kh_destroy(64, tess->assetSystem.loadingAssetMap);
    buf_free(tess->assetSystem.loadedAssets);
    buf_free(tess->assetSystem.loadingAssets);

    // Destroy strings
    //
    buf_free(tess->strings.internedStrings);

    // Destroy game system
    //
    buf_free(tess->gameSystem.activeEntities);

    // Destroy render system
    //
    render_system_destroy(&tess->renderSystem);

    // Destroy UI
    //
    tess_ui_destroy(&tess->uiSystem);

    // Destroy tess 
    //
    fixed_arena_free(tess->platform, &tess->arena);
}

bool tess_load_file(struct TessFileSystem *fs, const char* fileName, uint32_t pipeline, void *userData)
{
    struct TessFile *tfile = pool_allocate(fs->loadedFilePool);
    assert(tfile);
    AikeFile *file = fs->platform->open_file(fs->platform, fileName, Aike_File_Mode_Read);
    assert(file);
    assert(file->size > 0);
    strncpy(tfile->filePath, fileName, ARRAY_COUNT(tfile->filePath));
    AikeIORequest *req = &tfile->req;
    req->type = Aike_IO_Request_Read;
    req->file = file;
    req->buffer = aligned_alloc(4096, file->size);
    assert(req->buffer);
    req->bufferSize = file->size;
    req->fileOffset = 0;
    req->nBytes = file->size;
    bool success = true;
    success = fs->platform->submit_io_request(fs->platform, req);
    assert(success);
    tfile->pipeline = pipeline;
    tfile->userData = userData;
    printf("load file %s\n", tfile->filePath);
    return success;
}

// TODO: call this
void tess_unload_file(struct TessFileSystem *fs, struct TessFile *tfile)
{
    free(tfile->req.buffer);
    pool_free(fs->loadedFilePool, tfile);
    printf("unload file %s\n", tfile->filePath);
}

void tess_process_io_events(struct TessFileSystem *fs)
{
    AikeIOEvent event;
    while(fs->platform->get_next_io_event(fs->platform, &event))
    {
        struct TessFile *tfile = (struct TessFile*)event.request;
        assert(event.status == Aike_IO_Status_Completed); // TODO: error handling
        assert(tfile->pipeline > Tess_File_Pipeline_None && tfile->pipeline < Tess_File_Pipeline_Count);
        fs->platform->close_file(fs->platform, tfile->req.file);
        tfile->req.file = NULL;
        fs->pipeline_vtbl[tfile->pipeline](fs->pipeline_ptrs[tfile->pipeline], tfile);
    }
}

void tess_fill_mesh_data(struct Renderer *renderer, struct MeshQueryResult *mqr, void *userData);
void tess_load_mesh(struct TessAssetSystem *as, struct TTRMesh *tmesh, struct TessLoadingAsset *lasset)
{
    struct TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(struct TTRMeshDesc, tmesh->descRef);
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

void tess_finalize_mesh(struct Renderer *renderer, struct MeshReady *mr, void *userData);
void tess_fill_mesh_data(struct Renderer *renderer, struct MeshQueryResult *mqr, void *userData)
{
    struct TessLoadingAsset *lasset = (struct TessLoadingAsset*)userData;
    struct TTRMesh *tmesh = lasset->meshData.tmesh;
    struct TessAssetSystem *as = lasset->meshData.as;
    struct TTRBuffer *vbuf = TTR_REF_TO_PTR(struct TTRBuffer, tmesh->vertBufRef);
    struct TTRBuffer *ibuf = TTR_REF_TO_PTR(struct TTRBuffer, tmesh->indexBufRef);
    struct TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(struct TTRMeshDesc, tmesh->descRef);
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

void tess_finalize_mesh(struct Renderer *renderer, struct MeshReady *mr, void *userData)
{
    struct TessLoadingAsset *lasset = (struct TessLoadingAsset*)userData;
    struct TessAssetSystem *as = lasset->meshData.as;
    struct TessMeshAsset *mesh = pool_allocate(as->meshPool);
    assert(mesh);
    mesh->meshId = mr->meshId;
    mesh->asset.assetId = lasset->assetId;
    mesh->asset.type = Tess_Asset_Mesh;
    buf_push(as->loadedAssets, &mesh->asset);
    lasset->status = Tess_Asset_Dependency_Status_Done;
    lasset->asset = &mesh->asset;
    printf("FINALIZE MESH\n");
}

void tess_process_ttr_file(struct TessAssetSystem *as, struct TessFile *tfile)
{
    struct TessLoadingAsset *lasset = (struct TessLoadingAsset*)tfile->userData;
    lasset->file = tfile;

    void* fileBuf = tfile->req.buffer;
    assert(tfile->req.fileOffset == 0);
    uint64_t fileSize = tfile->req.nBytes;

    // TODO: check everything before you read it!!!!!!
    struct TTRHeader *header = (struct TTRHeader*)fileBuf;
    struct TTRDescTbl *tbl = TTR_REF_TO_PTR(struct TTRDescTbl, header->descTblRef);
    struct TTRImportTbl *imTbl = TTR_REF_TO_PTR(struct TTRImportTbl, header->importTblRef);
    struct TessAssetDependency *dep;
    // TODO: maybe this should be cached in TessAsset?
    TStr *assetName = tess_get_asset_name_from_id(as, lasset->assetId);
    TStr *packageName = tess_get_asset_package_from_id(as, lasset->assetId);
    bool found = false;
    for(int i = 0; i < tbl->entryCount; i++)
    {
        struct TTRDescTblEntry *entry = &tbl->entries[i];
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
                        struct TTRMesh *tmesh = TTR_REF_TO_PTR(struct TTRMesh, tbl->entries[i].ref);
                        lasset->meshData.tmesh = tmesh;
                        lasset->type = Tess_Asset_Mesh;
                        tess_load_mesh(as, tmesh, lasset);
                        found = true;
                    } break;
                case TTR_4CHAR("OBJ "):
                    {
                        lasset->status = Tess_Asset_Dependency_Status_Done;
                        struct TTRObject *tobject = TTR_REF_TO_PTR(struct TTRObject, tbl->entries[i].ref);
                        struct TTRAssetRef aref = tobject->meshARef;
                        uint32_t meshEntryIdx = (aref.tblIndex & ~TTR_AREF_EXTERN_MASK);
                        TStr *meshAssetName, *meshPackageName;
                        if(TTR_IS_EXTERN_AREF(aref))
                        {
                            struct TTRImportTblEntry *imEnt = &imTbl->entries[meshEntryIdx];
                            meshAssetName = tess_intern_string_s(as->tstrings, imEnt->assetName, TTR_MAX_NAME_LEN);
                            meshPackageName = tess_intern_string_s(as->tstrings, imEnt->packageName, TTR_MAX_NAME_LEN);
                        }
                        else
                        {
                            meshAssetName = tess_intern_string_s(as->tstrings, tbl->entries[meshEntryIdx].assetName, TTR_MAX_NAME_LEN);
                            meshPackageName = packageName;
                        }

                        struct TessObjectAsset *tessObj = pool_allocate(as->objectPool);
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

TStr *tess_get_asset_id(struct TessAssetSystem *as, TStr *package, TStr *asset)
{
    char buf[TTR_MAX_NAME_LEN*2 + 2];
    assert(package->len + asset->len + 2 < ARRAY_COUNT(buf));
    strcpy(buf, package->cstr);
    strcat(buf, "/");
    strcat(buf, asset->cstr);
    return tess_intern_string_s(as->tstrings, buf, ARRAY_COUNT(buf));
}

TStr *tess_get_asset_name_from_id(struct TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    assert(strlen(name) > 1);
    return tess_intern_string(as->tstrings, name + 1);
}

TStr *tess_get_asset_package_from_id(struct TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    uint32_t len = name - assetId->cstr;
    TStr *ret = tess_intern_string_s(as->tstrings, assetId->cstr, len);
    return ret;
}

bool tess_is_asset_loaded(struct TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadedAssetMap);
}

bool tess_is_asset_loading(struct TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
    return k != kh_end(as->loadingAssetMap);
}

void tess_load_asset_if_not_loaded(struct TessAssetSystem *as, TStr *assetId)
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
        struct TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
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

struct TessAsset* tess_get_asset(struct TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    if(k == kh_end(as->loadedAssetMap))
        return NULL;
    struct TessAsset *ret = kh_value(as->loadedAssetMap, k);
    return ret;
}

void tess_check_complete(struct TessAssetSystem *as)
{
    int count = buf_len(as->loadingAssets);
    for(int i = 0; i < count;)
    {
        struct TessLoadingAsset *lasset = as->loadingAssets[i];
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
                    struct TessObjectAsset *obj = (struct TessObjectAsset*)lasset->asset;
                    struct TessAsset *meshAsset = tess_get_asset(as, lasset->objectData.meshAssetId);
                    assert(meshAsset);
                    // TODO: object expected it to be mesh
                    // but it wasnt, so load failed, unload whatever was loaded
                    assert(meshAsset->type == Tess_Asset_Mesh);
                    obj->mesh = (struct TessMeshAsset*)meshAsset;
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

bool tess_are_all_loads_complete(struct TessAssetSystem *as)
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

void tess_gen_lookup_cache_for_package(struct TessAssetSystem *as, TStr *packageName)
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

    struct AssetLookupCache *cache = pool_allocate(as->assetLookupCachePool);
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
                struct TTRHeader *hdr = (struct TTRHeader*)block->memory;
                if(!STRUCT_IN_RANGE(block->memory, block->size, hdr))
                    goto unmap_file_and_continue; // missing header
                struct TTRDescTbl *dtbl = TTR_REF_TO_PTR(struct TTRDescTbl, hdr->descTblRef);
                if(!STRUCT_IN_RANGE(block->memory, block->size, dtbl))
                    goto unmap_file_and_continue; // corrupt header/descTbl
                if(!ARRAY_IN_RANGE(block->memory, block->size, dtbl->entries, dtbl->entryCount))
                    goto unmap_file_and_continue; // corrupt header/descTbl

                for(int j = 0; j < dtbl->entryCount; j++)
                {
                    char ebuf[TTR_MAX_NAME_LEN];
                    tess_replace_id_name_characters(ebuf, dtbl->entries[j].assetName, TTR_MAX_NAME_LEN);
                    struct AssetLookupEntry *entry = pool_allocate(as->assetLookupEntryPool);
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

bool get_asset_file(struct TessAssetSystem *as, TStr *assetId, TStr **result)
{
    TStr *package = tess_get_asset_package_from_id(as, assetId);
    TStr *assetName = tess_get_asset_name_from_id(as, assetId);
    khiter_t k = kh_get(str, as->packageAssetMap, package->cstr);
    if(k == kh_end(as->packageAssetMap))
    {
        fprintf(stderr, "get_asset_file() called for package %s, but no cache for that package!\n", package->cstr);
        return false;
    }
    __auto_type cache = (struct AssetLookupCache*)kh_value(as->packageAssetMap, k);
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

// TODO: NO LINEAR SEARCH!!!!!!!!!!!!1
TStr* tess_intern_string(struct TessStrings *tstrings, const char *string)
{
    // try to find the string
    int count = buf_len(tstrings->internedStrings);
    for(int i = 0; i < count; i++)
    {
        if(strcmp(string, tstrings->internedStrings[i]->cstr) == 0)
            return tstrings->internedStrings[i];
    }
    // if not found create new
    uint32_t strZLen = strlen(string) + 1;
    TStr* newstr = (TStr*)arena_push_size(&tstrings->stringArena, sizeof(TStr) + strZLen);
    newstr->len = strZLen - 1;
    memcpy(newstr->cstr, string, strZLen);
    buf_push(tstrings->internedStrings, newstr);
    return newstr;
}

// TODO: i don't think this is any safer
// also: NO LINEAR SEARCH !!!!!!!!!!!!!111111
TStr* tess_intern_string_s(struct TessStrings *tstrings, const char *string, uint32_t maxlen)
{
     // try to find the string
    int count = buf_len(tstrings->internedStrings);
    for(int i = 0; i < count; i++)
    {
        if(strncmp(string, tstrings->internedStrings[i]->cstr, maxlen) == 0)
            return tstrings->internedStrings[i];
    }
    // if not found create new
    uint32_t strZLen = MIN(strlen(string)+1, maxlen+1);
    TStr *newstr = (TStr*)arena_push_size(&tstrings->stringArena, sizeof(TStr) + strZLen);
    newstr->len = strZLen - 1;
    memcpy(newstr->cstr, string, strZLen-1);
    newstr->cstr[strZLen-1] = 0; // always null terminate
    buf_push(tstrings->internedStrings, newstr);
    return newstr;
}
