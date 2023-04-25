#pragma once
#include <sys/socket.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#ifdef POSIX_IO_IMPLEMENTATION

AikeFile* open_file(struct AikePlatform *platform, const char* filePath, uint32_t mode)
{
    int fd = open(filePath, O_RDWR);
    if(fd < 0)
    {
        fprintf(stderr, "Failed to open file %s\n", filePath);
        return NULL;
    }
    struct stat st;
    fstat(fd, &st);

    // TODO: use a better allocator?
    AikeFile *ret = (AikeFile*)malloc(sizeof(AikeFile));
    ret->fd = fd;
    ret->size = st.st_size;
    ret->filePath = filePath; // TODO: copy string?
    return ret;
}

void close_file(struct AikePlatform *platform, AikeFile *file)
{
   int res = close(file->fd); 
   if(res < 0)
   {
       fprintf(stderr, "Failed to close file %s, return code: %d. Was it even open?\n", file->filePath, res);
   }
   free(file);
}

// sync functions
//

AikeDirectory* open_directory(AikePlatform *platform, const char *dirpath)
{
    DIR *odir = opendir(dirpath);
    if(odir)
    {
        // TODO: no malloc?
        int pathlen = strlen(dirpath);
        assert(pathlen < AIKE_MAX_PATH);
        AikeDirectory *adir = (AikeDirectory*)malloc(sizeof(AikeDirectory));
        assert(adir);
        adir->directory = odir;
        strncpy(adir->dirpath, dirpath, AIKE_MAX_PATH);
        if(adir->dirpath[pathlen-1] == '/' || adir->dirpath[pathlen-1] == '\\')
            adir->dirpath[pathlen-1] = 0;
        return adir;
    }
    return NULL;
}

uint32_t next_file(AikePlatform *platform, AikeDirectory *dir, AikeFileEntry *buf)
{
    assert(false);
    return 0;
}

uint32_t next_files(AikePlatform *platform, AikeDirectory *adir, AikeFileEntry buf[], uint32_t bufLen)
{
    uint32_t curIndex = 0;
    struct dirent *entry;
    struct stat statBuf;
    for(int i = 0; curIndex < bufLen; i++)
    {
        entry = readdir(adir->directory);
        if(!entry) // out of entries
            break;
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char fullpath[AIKE_MAX_PATH];
        uint32_t pathlen = strlen(adir->dirpath);
        uint32_t filePathLen = strlen(entry->d_name);
        // AIKE_MAX_PATH is wrong or something
        assert(pathlen + filePathLen + 2 < AIKE_MAX_PATH);
        strncpy(fullpath, adir->dirpath, pathlen);
        fullpath[pathlen] = '/';
        strncpy(&fullpath[pathlen+1], entry->d_name, filePathLen+1);
        int result = lstat(fullpath, &statBuf);
        if(result < 0) // file must have been deleted inbetween readdir() and lstat()
            continue;
        strncpy(buf[curIndex].name, entry->d_name, filePathLen+1);
        if(S_ISREG(statBuf.st_mode))
            buf[curIndex].type = Aike_File_Entry_File;
        else if(S_ISDIR(statBuf.st_mode))
            buf[curIndex].type = Aike_File_Entry_Directory;
        else // we only care about files and directories
            continue;
        curIndex++;
    }
    assert(curIndex <= bufLen);
    return curIndex;
}

void close_directory(AikePlatform *platform, AikeDirectory *adir)
{
    assert(adir);
    closedir(adir->directory);
    free(adir);
}


AikeMemoryBlock* map_file(struct AikePlatform *platform, const char *filePath, uint64_t offset, uint64_t size)
{
    int fd = open(filePath, O_RDWR);
    AikeMemoryBlock *ret = NULL;
    void * mem = NULL;

    if(fd < 0)
    {
        fprintf(stderr, "Failed to open file %s\n", filePath);
        return NULL; // didnt even open nothing to close
    }
    size_t msize = size;
    if(msize == 0)
    {
        struct stat statBuf;
        if(fstat(fd, &statBuf) < 0)
        {
            fprintf(stderr, "Failed to mmap file becuase fstat() failed!\n");
            goto map_file_exit;
        }
        msize = statBuf.st_size;
    }
    mem = mmap(NULL, msize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
    if(!mem)
    {
        fprintf(stderr, "Failed to mmap file %s\n", filePath);
        goto map_file_exit;
    }
    // TODO: no malloc?
    ret = (AikeMemoryBlock*)malloc(sizeof(AikeMemoryBlock));
    ret->flags = 0;
    ret->size = msize;
    ret->realSize = msize;
    ret->memory = (uint8_t*)mem;
map_file_exit:
    close(fd); // pretty sure we don't need to keep the file open
    return ret;
}

void unmap_file(struct AikePlatform *platform, AikeMemoryBlock *block)
{
    int res = munmap(block->memory, block->size);
    assert(res >= 0);
    assert(block);
    free(block);
}

// TCP
//

AikeTCPServer* tcp_listen(AikePlatform *platform, uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        fprintf(stderr, "Failed to create tcp socket!\n");
        return NULL;
    }
    // TODO: is there any reason to not use this?
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt , sizeof(int));
    struct sockaddr_in serveraddr = {0};
    serveraddr.sin_family = AF_INET; // TODO: ipv6 support
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    int res = bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(res < 0)
    {
        close(fd);
        fprintf(stderr, "Failed to bind socket at port %d\n", port);
        return NULL;
    }
    res = listen(fd, 5);
    if(res < 0)
    {
        close(fd);
        fprintf(stderr, "listen() failed\n");
        return NULL;
    }
    AikeTCPServer *ret = calloc(1, sizeof(AikeTCPServer));
    ret->fd = fd;
    ret->port = port;
    return ret;
}

void tcp_close_server(AikePlatform *platform, AikeTCPServer *server)
{
    close((int)server->fd);
    free(server);
}

AikeTCPConnection* tcp_accept(struct AikePlatform *platform, AikeTCPServer *server)
{
    struct timeval waitTimeval = {0};
    int serverFd = (int)server->fd;
    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    FD_SET((int)server->fd, &readFdSet);
    int res = select(((int)server->fd)+1, &readFdSet, NULL, NULL, &waitTimeval);
    if(res < 0)
    {
        fprintf(stderr, "select() on TCP server failed\n");
        return NULL;
    }
    if(res == 0)
        return NULL;
    struct sockaddr_in clientAddr;
    uint32_t addrLen = sizeof(struct sockaddr_in);
    int fd = accept(server->fd, &clientAddr, &addrLen);
    assert(fd > 0); // we verified with select that something is there, so it must succeed
    // set nonblocking
    uint32_t flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Failed to set socket nonblocking after accept()\n");
        close(fd);
        return NULL;
    }

    AikeTCPConnection *ret = calloc(1, sizeof(AikeTCPConnection));
    assert(clientAddr.sin_family == AF_INET); // TODO: ipv6 suppoert
    ret->destPort = clientAddr.sin_port;
    memcpy(ret->destAddr, &clientAddr.sin_addr.s_addr, sizeof(unsigned long));
    ret->destAddrLen = sizeof(unsigned long);
    ret->fd = fd;
    ret->server = server;
    return ret;
}

AikeTCPConnection* tcp_connect(struct AikePlatform *platform, const char *addrStr, uint16_t port)
{
    struct sockaddr_in serverAddr;
    int res = inet_aton(addrStr, &serverAddr.sin_addr);
    if(res < 0)
    {
        fprintf(stderr, "Invalid address passed to inet_aton!\n");
        return NULL;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        fprintf(stderr, "Failed to create socket!\n");
        return NULL;
    }
    serverAddr.sin_port = htons(port);
    serverAddr.sin_family = AF_INET;
    res = connect(fd, &serverAddr, sizeof(serverAddr));
    if(res < 0)
    {
        fprintf(stderr, "connect() failed!\n");
        close(fd);
        return NULL;
    }

    // set nonblocking
    uint32_t flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Failed to set socket nonblocking after accept()\n");
        close(fd);
        return NULL;
    }

    AikeTCPConnection *ret = calloc(1, sizeof(AikeTCPConnection));
    ret->fd = fd;
    ret->destPort = port;
    ret->destAddrLen = sizeof(unsigned long);
    memcpy(&ret->destAddr, &serverAddr.sin_addr.s_addr, sizeof(unsigned long));
    ret->server = NULL;
    return ret;
}

void tcp_close_connection(struct AikePlatform *platform, AikeTCPConnection *connection)
{
    close(connection->fd);
    free(connection);
}

bool tcp_recv(struct AikePlatform *platform, AikeTCPConnection *connection, void *buf, uint32_t bufLen, uint32_t *recvBytes)
{
   ssize_t recvSize = recv((int)connection->fd, buf, bufLen, MSG_DONTWAIT); 
   if(recvSize < 0)
   {
       // socket went bad, treat it as if the connection ended
       if(errno != EAGAIN && errno != EWOULDBLOCK)
       {
           *recvBytes = 0;
           return true;
       }
       return false;
   }
   *recvBytes = recvSize;
   return true;
}

void tcp_send(struct AikePlatform *platform, AikeTCPConnection *connection, void *buf, uint32_t nBytes)
{
    ssize_t sendSize = send((int)connection->fd, buf, nBytes, MSG_DONTWAIT);
    if(sendSize < 0)
        fprintf(stderr, "send() on TCP socket failed\n");
}


#endif
