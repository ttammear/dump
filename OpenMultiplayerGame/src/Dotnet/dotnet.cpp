#include "dotnet.h"

#include <dlfcn.h>
#include <cstddef>
#include <string>
#include <dirent.h>
#include <set>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>

Dotnet::Dotnet()
{

}

Dotnet::~Dotnet()
{
    if(clrHost != nullptr)
    {
        if(dlclose(clrHost) != 0)
            fprintf(stderr, "dlclose failed!\n");
        else
            clrHost = nullptr;
    }
}

// from coreclr coreruncommon.cpp
void AddFilesFromDirectoryToTpaList(const char* directory, std::string& tpaList)
{
    const char * const tpaExtensions[] = {
                ".ni.dll",      // Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
                ".dll",
                ".ni.exe",
                ".exe",
                };

    DIR* dir = opendir(directory);
    if (dir == nullptr)
    {
        return;
    }

    std::set<std::string> addedAssemblies;

    // Walk the directory for each extension separately so that we first get files with .ni.dll extension,
    // then files with .dll extension, etc.
    for (int extIndex = 0; extIndex < sizeof(tpaExtensions) / sizeof(tpaExtensions[0]); extIndex++)
    {
        const char* ext = tpaExtensions[extIndex];
        int extLength = strlen(ext);

        struct dirent* entry;

        // For all entries in the directory
        while ((entry = readdir(dir)) != nullptr)
        {
            // We are interested in files only
            switch (entry->d_type)
            {
            case DT_REG:
                break;

            // Handle symlinks and file systems that do not support d_type
            case DT_LNK:
            case DT_UNKNOWN:
                {
                    std::string fullFilename;

                    fullFilename.append(directory);
                    fullFilename.append("/");
                    fullFilename.append(entry->d_name);

                    struct stat sb;
                    if (stat(fullFilename.c_str(), &sb) == -1)
                    {
                        continue;
                    }

                    if (!S_ISREG(sb.st_mode))
                    {
                        continue;
                    }
                }
                break;

            default:
                continue;
            }

            std::string filename(entry->d_name);

            // Check if the extension matches the one we are looking for
            int extPos = filename.length() - extLength;
            if ((extPos <= 0) || (filename.compare(extPos, extLength, ext) != 0))
            {
                continue;
            }

            std::string filenameWithoutExt(filename.substr(0, extPos));

            // Make sure if we have an assembly with multiple extensions present,
            // we insert only one version of it.
            if (addedAssemblies.find(filenameWithoutExt) == addedAssemblies.end())
            {
                addedAssemblies.insert(filenameWithoutExt);

                tpaList.append(directory);
                tpaList.append("/");
                tpaList.append(filename);
                tpaList.append(":");
            }
        }
        
        // Rewind the directory stream to be able to iterate over it for the next extension
        rewinddir(dir);
    }
    
    closedir(dir);
}

bool Dotnet::loadClrLib()
{
    this->clrLib = dlopen("/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/libcoreclr.so",RTLD_NOW | RTLD_LOCAL);
    if(this->clrLib == NULL)
        return false;

    this->coreclr_initialize = (coreclr_initialize_ptr)dlsym(clrLib, "coreclr_initialize");
    this->coreclr_execute_assembly = (coreclr_execute_assembly_ptr)dlsym(clrLib, "coreclr_execute_assembly");
    this->coreclr_shutdown_2 = (coreclr_shutdown_2_ptr)dlsym(clrLib, "coreclr_shutdown_2");
    this->coreclr_create_delegate = (coreclr_create_delegate_ptr)dlsym(clrLib, "coreclr_create_delegate");
    if(this->coreclr_initialize == NULL)
        return false;
    if(this->coreclr_execute_assembly == NULL)
        return false;
    if(this->coreclr_shutdown_2 == NULL)
        return false;
    if(this->coreclr_create_delegate == NULL)
        return false;

    return true;
}

void Dotnet::startHost()
{
    const char *propertyKeys[] = {
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_PATHS",
        "APP_NI_PATHS",
        "NATIVE_DLL_SEARCH_DIRECTORIES",
        "AppDomainCompatSwitch"
    };

    std::string tpaList;
    AddFilesFromDirectoryToTpaList("/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/", tpaList);

    std::string assemblydir = "/keep/Projects/OpenMultiplayerGame/";

    std::string nativeDllSearchDirs = assemblydir + ":" + "/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/";

    const char *propertyValues[] = {
        tpaList.c_str(),
        assemblydir.c_str(),
        assemblydir.c_str(),
        nativeDllSearchDirs.c_str(),
        "UseLatestBehaviorWhenTFMNotSpecified"
    };

    int status = -1;

    status = coreclr_initialize (
          assemblydir.c_str(),
          "simpleCoreCLRHost",
          sizeof(propertyKeys) / sizeof(propertyKeys[0]),
          propertyKeys,
          propertyValues,
          &this->clrHost,
          &domainId
    );

    if(status < 0)
    {   
        fprintf(stderr, "coreclr_initialize status %d\n", status);
        exit(-1);
    }

    hostRunning = true;
}

void (*csharp_main)(void);

void Dotnet::test()
{
    if(!hostRunning)
    {
        fprintf(stderr, "HOST NOT RUNNING\n");
        return;
    }
    int status = -1;
    status = coreclr_create_delegate (
        clrHost,
        domainId,
        "OpenMultiplayerGame",
        "OpenMultiplayerGame.Program",
        "testing",
        reinterpret_cast<void**>(&csharp_main)
    );
    if(status < 0)
    {
        fprintf(stderr, "coreclr_create_delegate failed with 0x%x\n", status);
        return;
    }
    csharp_main();
}

void Dotnet::stopHost()
{
    if(!hostRunning)
        return;
    
    int status = -1;
    int latchedExitCode = 0;
    status = coreclr_shutdown_2(clrHost, domainId, &latchedExitCode); 
    if(status < 0)
    {
        fprintf(stderr, "cureclr_shutdown_2 status %d\n", status);
        exit(-1);
    }
    hostRunning = false;
}

