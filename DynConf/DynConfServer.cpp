#if 0
g++ -std=c++11 DynConfServer.cpp -o dconfserver.out && ./dconfserver.out
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <deque>
#include <unordered_map>
#include <iterator>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

namespace DynConf {

class DynConfServer {
};

}

// Server waits for clients and stores their configuration
// Upon connect client sends all their configurable variables to the server along with the latest value
// Client can update its own configuration and also server can update it
// Value types: int/uint 8-32, float, (string), Vector3 (3 floats), Vector2 (2 floats)

// Client -> Server messages
// 1 - Register configuration
//      string identifier (used to remember configuration berween connections)
//      uint32_t count
//      [
//          string name
//          uint32_t type
//          dynamic value
//      ]
//
// 2 - Update configuration value
//      uint32_t id
//      uint32_t type
//      dynamic new_value

// Server -> Client messages
// 1 - Update configuration value
//      Same as Client -> Server 2

void fatal(const char * msg, ...) {
    va_list vl;
    va_start(vl, msg);
    vprintf(msg, vl);
    va_end(vl);
    exit(-1);
}

struct DataStreamState {
    uint32_t state;
    uint32_t dataLeft;
    std::deque<uint8_t> curData;
    std::deque<uint8_t> curPacket;
};

enum ConfigValueType {
    Config_Value_Type_None,
    Config_Value_Type_UInt8,
    Config_Value_Type_Int8,
    Config_Value_Type_Uint16,
    Config_Value_Type_Int16,
    Config_Value_Type_UInt32,
    Config_Value_Type_Int32,
    Config_Value_Type_Float,
    Config_Value_Type_String,
};

enum DynConfClientMessage {
    Config_Server_Message_Registration,
};

struct ConfigValue {
    std::string name;
    ConfigValueType type;
};

struct ConfigUInt32 : ConfigValue {
    uint32_t value;
};

struct ConfigFloat : ConfigValue {
    double value;
};

struct DynConfServerClient {
    int fd;
    DataStreamState dataState;
    std::unordered_map<int, ConfigValue*> configValues;
};

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0) {
        fatal("Failed to set socket nonblocking! %s\r\n", strerror(errno));
    }
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
    if(flags < 0) {
        fatal("Failed to set socket nonblocking! %s\r\n", strerror(errno));
    }
}

bool read_uint32(std::deque<uint8_t>& buffer, uint32_t& result) {
   if(buffer.size() >= 4) {
       result = buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
       buffer.erase(buffer.begin(), buffer.begin()+4);
       return true;
   } 
   return false;
}

bool read_float(std::deque<uint8_t>& buffer, float& result) {
    if(buffer.size() >= 4) {
        uint32_t temp = buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
        result = *((float*)&temp);
        return true;
    }
    return false;
}

bool read_string(std::deque<uint8_t>& buffer, std::string& result) {
    uint32_t len = 0;
    if(!read_uint32(buffer, len)) {
        return false;
    }
    if(buffer.size() >= len) {
        char *buf = new char[len];
        for(int i = 0; i < len; i++) {
            buf[i] = buffer[i];
        }
        result = std::string(buf, buf+len);
        delete[] buf;
        buffer.erase(buffer.begin(), buffer.begin()+len);
        return true;
    }
    return false;
}

void process_packet(DynConfServerClient *client, std::deque<uint8_t>& packetData) {
    uint32_t messageId;
    printf("packet %d\r\n", packetData.size());
    if(!read_uint32(packetData, messageId)) {
        return;
    }
    printf("msg %d\r\n", messageId);
    bool success = true;
    switch(messageId) {
        case 0: // configurable variables
        {
            uint32_t count = 0;
            success &= read_uint32(packetData, count);
            for(int i = 0; i < count; i++) {
                ConfigValue *val = nullptr;
                uint32_t type = 0;
                std::string name;
                success &= read_uint32(packetData, type);
                success &= read_string(packetData, name);
                switch(type) {
                    case Config_Value_Type_UInt32:
                    {
                        uint32_t value;
                        success &= read_uint32(packetData, value);
                        auto uint32val = new ConfigUInt32();
                        printf("uint32 %d\r\n", value);
                        uint32val->value = value;
                        val = uint32val;
                    }break;
                    case Config_Value_Type_Float:
                    {
                        float value;
                        success &= read_float(packetData, value);
                        auto floatval = new ConfigFloat();
                        printf("float %f\r\n", value);
                        floatval->value = value;
                        val = floatval;
                    }break;
                    default: // unkown type
                        return;
                }
                if(val != nullptr) {
                    val->type = (ConfigValueType)type;
                    val->name = name;
                    printf("var \"%s\"\r\n", val->name.c_str());
                    client->configValues.insert(std::make_pair(i, val));
                }
            }
        }break;
    }
}

void process_data(DynConfServerClient *client, DataStreamState& state, uint8_t *data, size_t count) {
    state.curData.insert(state.curData.end(), data, data+count);
    while(true) {
        if(state.state == 0) {
            if(!read_uint32(state.curData, state.dataLeft)) {
                break;
            }
            state.state = 1;
        }
        if(state.state == 1) {
            uint32_t dataCount = std::min(state.curData.size(), (size_t)state.dataLeft);
            state.curPacket.insert(state.curPacket.end(), std::make_move_iterator(state.curData.begin()), std::make_move_iterator(state.curData.begin()+dataCount));
            state.curData.erase(state.curData.begin(), state.curData.begin()+dataCount);
            state.dataLeft -= dataCount;
            if(state.dataLeft == 0) {
                process_packet(client, state.curPacket);
                state.curPacket.clear();
                state.state = 0;
            } else { // need more data for packet but all data already processed
                break;
            }
        }
    }
}

int main(int argc, char **argv) {

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0) {
        fatal("socket() failed!\r\n");
    }

    // create directory in run if it doesn't exist
    struct stat st = {0};
    if(stat("/tmp/dynconf", &st) == -1) {
        int res = mkdir("/tmp/dynconf", 0700);
        if(res < 0) {
            fatal("Failed to create folder for unix domain sockets. %s\r\n", strerror(errno));
        }
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    const char *spath = "/tmp/dynconf/dynsock.socket";
    unlink(spath);
    strncpy(addr.sun_path, spath, sizeof(addr.sun_path)-1);
    if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fatal("bind() failed! %s \r\n", strerror(errno));
    }

    if(listen(fd, 10) < 0) {
        fatal("listen() failed! %s \r\n", strerror(errno));
    }

    set_non_blocking(fd);

    std::vector<pollfd> readFds;
    std::vector<int> deadPollFds;
    std::unordered_map<int, DynConfServerClient*> clientMap;

    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    readFds.push_back(pfd);
    std::deque<DynConfServerClient*> clients;
    int res = -1;
    int clientFd = -1;
    uint8_t buf[4096];

    DataStreamState dstate;

    while(1) {
        int res = poll(&readFds[0], readFds.size(), 500);
        if(res > 0) {
            for(int i = 0; i < readFds.size(); i++) {
                if(readFds[i].revents & POLLIN) {
                    if(readFds[i].fd == fd) {
                        clientFd = accept(fd, NULL, NULL);
                        if(clientFd >= 0) {
                            auto client = new DynConfServerClient();
                            client->fd = clientFd;
                            clients.push_back(client);

                            struct pollfd pfd;
                            pfd.fd = clientFd;
                            pfd.events = POLLIN;

                            readFds.push_back(pfd);
                            clientMap.insert(std::make_pair(clientFd, client));

                            set_non_blocking(clientFd);

                            printf("Client connected\r\n");
                        }
                    } else {
                        // TODO: cleint data
                        auto clientIt = clientMap.find(readFds[i].fd);
                        if(clientIt != clientMap.end()) {
                            auto client = clientIt->second;
                            while((res = read(readFds[i].fd, buf, sizeof(buf))) > 0) {
                                //printf("%s\r\n", buf);
                                process_data(client, dstate, buf, res);
                            }
                            if(res == 0) {
                                printf("Client disconnected\r\n");
                                // TODO: remove from poll list
                                close(readFds[i].fd);
                                deadPollFds.push_back(i);
                                clientMap.erase(clientIt);
                                delete client;
                            } else {
                                printf("read() failed! %s\r\n", strerror(errno));
                            }
                        } else {
                            printf("Warning: data from disconnected client!\r\n");
                        }
                    }
                }
            }
            // remove back to front, so indices don't change
            for(int i = deadPollFds.size()-1; i >= 0; i--) {
                readFds.erase(readFds.begin()+deadPollFds[i]);
            }
            deadPollFds.clear();
        }
    }

    return 0;
}
