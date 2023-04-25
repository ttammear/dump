#pragma once

typedef enum AikeRequestType
{
    Aike_IO_Request_None,
    Aike_IO_Request_Read,
    Aike_IO_Request_Write,
} AikeRequestType;

typedef enum AikeIOEventType
{
    Aike_IO_Event_IO_Request_Done,
    Aike_IO_Event_TCP_Data,
    Aike_IO_Event_TCP_New_Connection,
    Aike_IO_Event_TCP_Connected,
    Aike_IO_Event_TCP_Disconnected,
} AikeIOEventType;

typedef enum AikeIOStatus
{
    Aike_IO_Status_Failed,
    Aike_IO_Status_Completed,
} AikeIOStatus;

typedef enum AikeFileMode
{
    Aike_File_Mode_Read,
    Aike_File_Mode_Write,
    Aike_File_Mode_ReadWrite,
} AikeFileMode;


typedef struct AikeTCPServer
{
    int64_t fd;
    uint32_t flags;
    uint16_t port;
} AikeTCPServer;

#define AIKE_MAX_NET_ADDR_LEN 16 // ipv6 length

typedef struct AikeTCPConnection
{
    int64_t fd;
    uint32_t flags;
    uint16_t destPort;
    uint8_t destAddr[AIKE_MAX_NET_ADDR_LEN];
    uint32_t destAddrLen;
    struct AikeTCPServer *server;
} AikeTCPConnection;

typedef struct AikeIORequestDoneEvent
{
    uint32_t status;
    struct AikeIORequest *request;
} AikeIORequestDoneEvent;

/*typedef struct AikeTCPNewConnectionEvent
{
    AikeTCPConnection *connection;
    AikeTCPServer *server;
} AikeTCPNewConnectionEvent;

typedef struct AikeTCPConnectedEvent
{
    AikeTCPConnection *connection;
} AikeTCPConnectedEvent;

typedef struct AikeTCPDisconnectedEvent
{
    AikeTCPConnection *connection;
    AikeTCPServer *server;
} AikeTCPDisconnectedEvent;

typedef struct AikeTCPDataEvent
{
    void *data;
    uint32_t dataLen;
    AikeTCPConnection *connection;
    AikeTCPServer *server; // NULL for client
} AikeTCPDataEvent;*/

#define AIO_MAX_EVENTS 64
#define AIO_EVENT_BUF_SIZE 1024
#define AIO_EVENT_DEQUEUE_COUNT 1024

//#define LINUX_USE_KERNEL_AIO

#if defined(__linux__) && defined(LINUX_USE_KERNEL_AIO)
#include <linux/aio_abi.h>
#include <dirent.h>

typedef struct AikeIORequest
{
    struct iocb linuxIocb;
    
    uint32_t type;
    union
    {
        AikeFile *file;
        AikeTCPConnection *connection;
    };
    
    void *buffer;
    uint64_t bufferSize;

    uint64_t fileOffset;
    uint64_t nBytes;
} AikeIORequest;

typedef struct AikeDirectory
{
    DIR *directory;
    char dirpath[AIKE_MAX_PATH];
} AikeDirectory;

// in case linux AIO fails with EAGAIN (or other temporary error condition), 
// we need to queue it so that user code doesn't have to deal with this
typedef struct AikeQueuedIORequest {
    AikeIORequest *req;
    struct AikeQueuedIORequest *next;
} AikeQueuedIORequest;

#elif defined(__linux__) // use POSIX AIO

#include <aio.h>
#include <dirent.h>

typedef struct AikeIORequest
{
    struct aiocb posixIocb;
    
    uint32_t type;
    union
    {
        AikeFile *file;
        AikeTCPConnection *connection;
    };
    
    void *buffer;
    uint64_t bufferSize;

    uint64_t fileOffset;
    uint64_t nBytes;
} AikeIORequest;

typedef struct AikeDirectory
{
    DIR *directory;
    char dirpath[AIKE_MAX_PATH];
} AikeDirectory;
typedef struct AikeQueuedIORequest {
    AikeIORequest *req;
    struct AikeQueuedIORequest *next;
} AikeQueuedIORequest;

#else
#error platform does not have AIO implemented!
#endif

typedef struct AikeIOEvent
{
    uint32_t type;
    union
    {
        AikeIORequestDoneEvent ioRequestDone;
        /*
        AikeTCPNewConnectionEvent newConnection;
        AikeTCPConnectedEvent connected;
        AikeTCPDisconnectedEvent disconnected;
        */
    };
} AikeIOEvent;

// NOTE: spectific platform has to provide AikeIORequest structure

struct AikePlatform;
struct AikeIORequest;

typedef void (*init_async_io_t)(struct AikePlatform *platform);
typedef bool (*submit_io_request_t)(struct AikePlatform *platform, struct AikeIORequest *req);
typedef bool (*get_next_io_event_t)(struct AikePlatform *platform, struct AikeIOEvent *event);
typedef void (*destroy_async_io_t)(struct AikePlatform *platform);

// TODO: should we somehow allow choosing WAN vs LAN?
typedef AikeTCPServer* (*tcp_listen_t)(struct AikePlatform *platform, uint16_t port);
typedef void (*tcp_close_server_t)(struct AikePlatform *platform, AikeTCPServer *server);
typedef AikeTCPConnection* (*tcp_accept_t)(struct AikePlatform *platform, AikeTCPServer *host);
typedef AikeTCPConnection* (*tcp_connect_t)(struct AikePlatform *platform, const char *addrStr, uint16_t port);
typedef void (*tcp_close_connection_t)(struct AikePlatform *platform, AikeTCPConnection *connection);
typedef bool (*tcp_recv_t)(struct AikePlatform *platform, AikeTCPConnection *connection, void *buf, uint32_t bufLen, uint32_t *recvBytes);
typedef void (*tcp_send_t)(struct AikePlatform *platform, AikeTCPConnection *connection, void *buf, uint32_t nBytes);
