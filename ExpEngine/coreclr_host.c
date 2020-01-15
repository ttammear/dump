char* add_files_from_directory_tpa_list(AikePlatform *platform, const char *directory) {
    char *tpaList = malloc(128*1024); // TODO: arbirary size buffer, probably not the best idea!
    tpaList[0] = 0;

    const char *const tpaExtensions[] = {
        ".ni.dll",
        ".dll",
        ".ni.exe",
        ".exe",
    };
    AikeDirectory *dir = platform->open_directory(platform, directory);
    if(dir == NULL) {
        return NULL;
    }
    khash_t(str) *addedAssemblies;
    addedAssemblies = kh_init(str);

    AikeFileEntry entry;
    while(platform->next_files(platform, dir, &entry, 1) != 0) {
        // TODO: no links?
        for(int extIndex = 0; extIndex < sizeof(tpaExtensions) / sizeof(tpaExtensions[0]); extIndex++) {
            const char *ext = tpaExtensions[extIndex];
            int extLength = strlen(ext);

            char *filename = entry.name;
            int extPos = strlen(filename) - extLength;
            if(extPos <= 0 || strncmp(filename+extPos, ext, extLength) != 0) {
                continue;
            }
            char *filenameWithoutExt = malloc(extPos+1);
            strncpy(filenameWithoutExt, filename, extPos);
            filenameWithoutExt[extPos] = 0;
            khiter_t k;
            if((k = kh_get(str, addedAssemblies, filenameWithoutExt)) == kh_end(addedAssemblies)) {
                int ret;
                kh_put(str, addedAssemblies, filenameWithoutExt, &ret);
                strcat(tpaList, directory);
                //strcat(tpaList, "/");
                strcat(tpaList, filename);
                strcat(tpaList, ":");
            }
        }
    }
    // delete hash map (and strings stored in it)
    for(khiter_t k = kh_begin(addedAssemblies); k != kh_end(addedAssemblies); k++) {
        if(kh_exist(addedAssemblies, k)) {
            free((void*)kh_key(addedAssemblies, k));
        }
    }
    kh_destroy(str, addedAssemblies);

    platform->close_directory(platform, dir);

    int len = strlen(tpaList)+1;
    char *ret = malloc(strlen(tpaList)+1);
    strcpy(ret, tpaList);
    free(tpaList);
    assert(strlen(ret)+1 == len);
    return ret;
}


static coreclr_initialize_ptr coreclr_initialize;
static coreclr_execute_assembly_ptr coreclr_execute_assembly;
static coreclr_shutdown_2_ptr coreclr_shutdown_2;
static coreclr_create_delegate_ptr coreclr_create_delegate;

// TODO: platform independent
bool coreclr_loadlib() {
    // TODO: this directory changes when updates happen
    void *clrLib = dlopen("./coreclr/libcoreclr.so",RTLD_NOW | RTLD_LOCAL);
    // TODO: dlclose
    if(clrLib == NULL) {
        return false;
    }
    coreclr_initialize = dlsym(clrLib, "coreclr_initialize");
    coreclr_execute_assembly = dlsym(clrLib, "coreclr_execute_assembly");
    coreclr_shutdown_2 = dlsym(clrLib, "coreclr_shutdown_2");
    coreclr_create_delegate = dlsym(clrLib, "coreclr_create_delegate");
    assert(coreclr_initialize != NULL && coreclr_execute_assembly != NULL &&
            coreclr_shutdown_2 != NULL && coreclr_create_delegate != NULL);
    return true;
}

void coreclr_init(AikePlatform *platform, GameServer *gsCtx) {
    // TODO: free
    char *path = (char*)malloc(PATH_MAX);
    path =  getcwd(path, PATH_MAX);
    if(path == NULL) {
        fprintf(stderr, "Failed to get workind directory for .NET assembly directory\r\n");
        exit(-1);
    }
    char *assemblyDir = malloc(strlen(path) + 2);
    strcpy(assemblyDir, path);
    strcat(assemblyDir, "/");

    //const char *libsLoc = "/usr/share/dotnet/shared/Microsoft.NETCore.App/2.2.8/";
    const char *coreClrPath = "coreclr/";
    char *libsLoc = malloc(strlen(path)+strlen(coreClrPath)+2);
    strcpy(libsLoc, path);
    strcat(libsLoc, "/");
    strcat(libsLoc, coreClrPath);
    char *tpa = add_files_from_directory_tpa_list(platform, libsLoc);
    printf("TPALIST %s\n", tpa);
    //const char *assemblyDir = "/home/ttammear/Projects/ExpEngineBuild/";
    int len = strlen(assemblyDir)+strlen(libsLoc)+3;
    char *nativeDllSearchDirs = malloc(len);
    nativeDllSearchDirs[0] = 0;
    strcat(nativeDllSearchDirs, assemblyDir);
    strcat(nativeDllSearchDirs, ":");
    strcat(nativeDllSearchDirs, libsLoc);
    strcat(nativeDllSearchDirs, ":");
    assert(len-1 == strlen(nativeDllSearchDirs));
    printf(".NET Core runtime assembly directories: %s\n", nativeDllSearchDirs);

    const char *propertyKeys[] = {
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_PATHS",
        "APP_NI_PATHS",
        "NATIVE_DLL_SEARCH_DIRECTORIES",
        "AppDomainCompatSwitch"
    };
    const char *propertyValues[] = {
        tpa,
        assemblyDir,
        assemblyDir,
        nativeDllSearchDirs,
        "UseLatestBehaviorWhenTFMNotSpecified"
    };

    bool loaded = coreclr_loadlib();
    assert(loaded);

    int status = -1;
    void *clrHost;
    unsigned int domainId = 0;
    status = coreclr_initialize(
            assemblyDir,
            "simpleCoreCLRHost",
            sizeof(propertyKeys) / sizeof(propertyKeys[0]),
            propertyKeys,
            propertyValues,
            &clrHost,
            &domainId
    );
    if(status < 0) {
        fprintf(stderr, "coreclr_initliaze failed with status code 0x%x\n", status);
        exit(-1);
    }

    const char *msg = "Failed to create delegate! 0x%x\n";
    #define CHECK_STATUS(msg) if(status < 0) {printf(msg, status);exit(-1);}

    void (*csharp_hello)(void);
    void (*managed_set_native_api_func)(int nativeApiFunc, void *procPtr);
    void (*managed_set_context)(void *ctx);

    status = coreclr_create_delegate(clrHost, domainId, "Managed", "Sandbox", "Hello", (void**)&csharp_hello);
    CHECK_STATUS(msg);
    status = coreclr_create_delegate(clrHost, domainId, "Managed", "Sandbox", "SetNativeApiFunc", (void**)&managed_set_native_api_func);
    CHECK_STATUS(msg);
    status = coreclr_create_delegate(clrHost, domainId, "Managed", "Sandbox", "SetContext", (void**)&managed_set_context);
    CHECK_STATUS(msg);

    managed_set_context(gsCtx);
    managed_set_native_api_func(Native_Api_Proc_InternAssetId, native_intern_asset_id);
    managed_set_native_api_func(Native_Api_Proc_SetMap, native_set_map);
    managed_set_native_api_func(Native_Api_Proc_SetServerTitle, native_set_server_title);
    managed_set_native_api_func(Native_Api_Proc_SetGamemodeTitle, native_set_gamemode_title);
    managed_set_native_api_func(Native_Api_Proc_SetMaxPlayers, native_set_max_players);

    csharp_hello(); // TODO: delete this and move Start() out of it

    int latchedExitCode = 0;
    status = coreclr_shutdown_2(clrHost, domainId, &latchedExitCode);
    if(status < 0) {
        fprintf(stderr, "coreclr_shutdown_2 failed with status code 0x%x! .NET Core runtime could still be running!\n", status);
    }
}
