void tess_strings_init(TessStrings *tstrings, TessFixedArena *arena)
{
    fixed_arena_init_from_arena(&tstrings->stringArena, arena, 64 * 1024);
    tstrings->internedStrings = NULL;
    tstrings->empty = tess_intern_string(tstrings, "");
}

void tess_strings_destroy(TessStrings *tstrings)
{
    buf_free(tstrings->internedStrings);
}

void tess_file_system_init(TessFileSystem *fs, TessFixedArena *arena)
{
    POOL_FROM_ARENA(fs->loadedFilePool, arena, TESS_LOADED_FILE_POOL_SIZE);
}

void tess_asset_system_init(TessAssetSystem *as, TessFixedArena *arena)
{
    POOL_FROM_ARENA(as->meshPool, arena, TESS_MESH_POOL_SIZE);
    POOL_FROM_ARENA(as->texturePool, arena, TESS_TEXTURE_POOL_SIZE);
    POOL_FROM_ARENA(as->objectPool, arena, TESS_OBJECT_POOL_SIZE);
    POOL_FROM_ARENA(as->loadingAssetPool, arena, TESS_LOADING_ASSET_POOL_SIZE);
    POOL_FROM_ARENA(as->assetLookupCachePool, arena, TESS_ASSET_LOOKUP_CACHE_POOL_SIZE);
    POOL_FROM_ARENA(as->assetLookupEntryPool, arena, TESS_ASSET_LOOKUP_ENTRY_POOL_SIZE);
    as->loadedAssets = NULL;
    as->loadingAssets = NULL;
    as->packageList = NULL;
    as->packageAssetMap = kh_init(str);
    as->loadedAssetMap = kh_init(64);
    as->loadingAssetMap = kh_init(64);
    as->assetStatusMap = kh_init(uint32);

    TStr *emptyStr = tess_intern_string(as->tstrings, "");
    as->nullAsset = (TessAsset){
        .assetId = emptyStr, 
        .type = Tess_Asset_Unknown
    };
    as->nullMesh = (TessMeshAsset){
        .asset.assetId = emptyStr, 
        .asset.type = Tess_Asset_Mesh, 
        .meshId = 0
    };
    as->nullObject = (TessObjectAsset){
        .asset.assetId = emptyStr, 
        .asset.type = Tess_Asset_Object, 
        .mesh = &as->nullMesh, // TODO: everything should use ids not pointers?
        .materialId = 0 // TODO: this material might not exist!
    };

    DELEGATE_INIT(as->onAssetLoaded);
}

void tess_async_load_file(TessFileSystem *fs, const char* fileName, AsyncTask *task) {
    assert(g_scheduler->state == SCHEDULER_STATE_TASK);
    task->ctx = g_scheduler->curTaskCtx;
    tess_load_file(fs, fileName, Tess_File_Pipeline_Task, task);
}

void tess_task_file_loaded(void *what, TessFile *tfile) {
    AsyncTask *task = (AsyncTask*)tfile->userData;
    task->file = tfile;
    scheduler_event(SCHEDULER_EVENT_FILE_LOADED, NULL, tfile->userData);
}

void tess_asset_system_destroy(TessAssetSystem *as)
{
    DELEGATE_CLEAR(as->onAssetLoaded);

    // TODO: loadedAssets deinit?
    // free AssetLookupCahce entry buf and map
    khiter_t k;
    __auto_type h = as->packageAssetMap;
    int tes;
    for(k = kh_begin(h); k != kh_end(h); ++k)
    {
        if(kh_exist(h, k))
        {
            AssetLookupCache *cache = (AssetLookupCache*)kh_value(h, k);
            buf_free(cache->entries);
        }
    }
    kh_destroy(str, as->packageAssetMap);
    kh_destroy(64, as->loadedAssetMap);
    kh_destroy(64, as->loadingAssetMap);
    kh_destroy(uint32, as->assetStatusMap);
    buf_free(as->loadedAssets);
    buf_free(as->loadingAssets);
    buf_free(as->packageList);
}

void tess_process_io_events(TessFileSystem *fs);
bool tess_load_file(TessFileSystem *fs, const char* fileName, uint32_t pipeline, void *userData)
{
    TessFile *tfile = pool_allocate(fs->loadedFilePool);
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
    // TODO: temp hack (fix if linux returns EAGAIN)
    while(true)
    {
        success = fs->platform->submit_io_request(fs->platform, req);
        if(success)
            break;
        else
        {
            tess_process_io_events(fs);
            fs->platform->sleep(1000);
            //break;
        }
    }
    assert(success);
    tfile->pipeline = pipeline;
    tfile->userData = userData;
    printf("load file %s\n", tfile->filePath);
    return success;
}

// TODO: call this
void tess_unload_file(TessFileSystem *fs, TessFile *tfile)
{
    free(tfile->req.buffer);
    pool_free(fs->loadedFilePool, tfile);
    printf("unload file %s\n", tfile->filePath);
}

void tess_process_io_events(TessFileSystem *fs)
{
    AikeIOEvent event;
    while(fs->platform->get_next_io_event(fs->platform, &event))
    {
        switch(event.type)
        {
            case Aike_IO_Event_IO_Request_Done:
            {
                AikeIORequestDoneEvent *ioDone = &event.ioRequestDone;
                TessFile *tfile = (TessFile*)ioDone->request;
                assert(ioDone->status == Aike_IO_Status_Completed); // TODO: error handling
                assert(tfile->pipeline > Tess_File_Pipeline_None && tfile->pipeline < Tess_File_Pipeline_Count);
                fs->platform->close_file(fs->platform, tfile->req.file);
                tfile->req.file = NULL;
                fs->pipeline_vtbl[tfile->pipeline](fs->pipeline_ptrs[tfile->pipeline], tfile);
            } break;

            default:
            fprintf(stderr, "Tess unknown io event\n");
            break; 
        }
    }
}

// TODO: NO LINEAR SEARCH!!!!!!!!!!!!1
TStr* tess_intern_string(TessStrings *tstrings, const char *string)
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
    TStr* newstr = (TStr*)fixed_arena_push_size(&tstrings->stringArena, sizeof(TStr) + strZLen, 8);
    newstr->len = strZLen - 1;
    memcpy(newstr->cstr, string, strZLen);
    buf_push(tstrings->internedStrings, newstr);
    return newstr;
}

// TODO: i don't think this is any safer
// also: NO LINEAR SEARCH !!!!!!!!!!!!!111111
TStr* tess_intern_string_s(TessStrings *tstrings, const char *string, uint32_t maxlen)
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
    TStr *newstr = (TStr*)fixed_arena_push_size(&tstrings->stringArena, sizeof(TStr) + strZLen, 8);
    newstr->len = strZLen - 1;
    memcpy(newstr->cstr, string, strZLen-1);
    newstr->cstr[strZLen-1] = 0; // always null terminate
    buf_push(tstrings->internedStrings, newstr);
    return newstr;
}
