#if 0
g++ DynConfClient.cpp -o dconfclient.out && ./dconfclient.out
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <algorithm>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

void fatal(const char *msg, ...) {
    va_list vl;
    va_start(vl, msg);
    vprintf(msg, vl);
    va_end(vl);
    exit(-1);
}

#pragma pack(push, 1)
struct testpacket {
    uint32_t size;
    uint32_t msgId;
    uint32_t count;
    uint32_t type;
    uint32_t strlen;
    char str[5];
    float value;
};
#pragma pack(pop)

int main(int argc, char **argv) {
    struct sockaddr_un addr = {0};
    char buf[256];
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0) {
        fatal("socket() failed! %s\r\n", strerror(errno));
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/dynconf/dynsock.socket", sizeof(addr.sun_path)-1);

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fatal("connect() failed! %s\r\n", strerror(errno));
    }

    testpacket test;
    test.size = sizeof(testpacket)-4;
    test.msgId = 0;
    test.count = 1;
    test.type = 7;
    test.strlen = 5;
    char data[] = {'H','e','l','l','o'};
    std::copy(data, data+5, test.str);
    test.value = 1337.1f;

    int res;

    write(fd, &test, sizeof(testpacket)); 
    write(fd, &test, sizeof(testpacket)); 
    write(fd, &test, sizeof(testpacket)); 

    close(fd);

    return 0;
}

// usage code

class DynConfig {
public:
    DynConfig(std::string appname);

    void addFloat(std::string name, float *valRef);
    void addUInt32(std::string name, uint32_t *valRef);
    void addInt32(std::string name, int32_t *valRef);
    
    void addFloat(std::string name, std::function<void(float), float defVal = 0.0f);
    void addUInt32(std::string name, std::function<void(uint32_t), uint32_t defVal = 0);
    void addInt32(std::string name, std::function<void(int32_t), int32_t defVal = 0);

    void addFloat(std::string name, float defVal = 0.0f);
    void addUInt32(std::string name, uint32_t defVal = 0);
    void addInt32(std::string name, int32_t defVal = 0);

    void updateValue(std::string name, float value);
    void updateValue(std::string name, uint32_t value);
    void updateValue(std::string name, int32_t value);

    void begin(bool waitConfig = true);
    void end();
};

#ifdef test

void updateUint32(uint32_t uint, void *userData) {
}

void configurationEvent(std::string name, DynConfEvent& e, void *userData) {
}

void test() {
    StateStructure state;

    float confFloat;

    DynConfig dconf("MyAwesomeApplication");
    dconf.userData = &state;
    // automatic value update (by pointer)
    dconf.addFloat("SomeFloatWithName", &confFloat);
    // update with callback
    dconf.addUint32("MyUint32", updateUint32, 42); // must provide default value
    // in case user doesn't want to use callback per value or direct updating
    dconf.addInt8("Manual", 2); // also must provide default value
    dconf.eventCallback = configurationEvent;
    // client can also update values (function overload for each type)
    dconf.updateValue("SomeFloatWithName", 1.0f);
    // this will block until all values are synced with conf server
    dconf.begin(); // configuration structure is now frozen (but values can of course still be updated)

    while(1) {
        // main loop
    }

    dconf.end(); // stop configuration
}
#endif
