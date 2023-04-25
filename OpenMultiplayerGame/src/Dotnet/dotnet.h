#pragma once

#include "coreclrhost.h"

class Dotnet
{
public:
    Dotnet();
    ~Dotnet();
    
    bool loadClrLib();
    void startHost();
    void stopHost();
    void test(void*, class World*);
    void update();

    void *clrHost = nullptr;
    void *clrLib = nullptr;
    bool hostRunning = false;
    unsigned int domainId = 0;

    coreclr_initialize_ptr coreclr_initialize = nullptr;
    coreclr_execute_assembly_ptr coreclr_execute_assembly = nullptr;
    coreclr_shutdown_2_ptr coreclr_shutdown_2 = nullptr;
    coreclr_create_delegate_ptr coreclr_create_delegate = nullptr;

};
