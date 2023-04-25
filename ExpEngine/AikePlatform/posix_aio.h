#ifndef AIKE_POSIX_AIO_H
#define AIKE_POSIX_AIO_H

#include <aio.h>
#include <signal.h>

#endif

typedef struct AikeIOState {
    AikePlatform *platform;
    uint32_t numEvents;
    AikeIOEvent events[AIO_EVENT_BUF_SIZE];
    AikeQueuedIORequest *ioReqHead;
    AikeQueuedIORequest *ioReqTail;
} AikeIOState;

AikeIOState *aio;

#ifdef POSIX_AIO_IMPLEMENTATION

static void                 /* Handler for I/O completion signal */
aio_sig_handler(int sig, siginfo_t *si, void *ucontext)
{
    // NOTE: DO NOT USE PRINTF IN A SIGNAL!
   if (si->si_code == SI_ASYNCIO) {
       if(si->si_errno == 0) {
           AikeIORequest *req = si->si_value.sival_ptr;
           assert(AIO_EVENT_BUF_SIZE - aio->numEvents > 0);
           int idx = aio->numEvents++;
           // TODO: can this fail?
           aio->events[idx].type = Aike_IO_Event_IO_Request_Done;
           aio->events[idx].ioRequestDone.status = Aike_IO_Status_Completed;
           aio->events[idx].ioRequestDone.request = req;
       } else {
           assert(false);
       }
   }
}

void init_async_io(struct AikePlatform *platform) {
    aio = (AikeIOState*)malloc(sizeof(AikeIOState));
    assert(aio);
    *aio = (AikeIOState){0};
    aio->platform = platform;

    // setup signal handlers
    struct sigaction sa;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = aio_sig_handler;
    if(sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction fail in init_async_io\n");
        abort();
    }
}

bool submit_io_request(struct AikePlatform *platform, AikeIORequest *req) {
    int result;
    assert(req->type == Aike_IO_Request_Read || req->type == Aike_IO_Request_Write);
    bool isRead = req->type == Aike_IO_Request_Read;
    if(req->file->fd < 0) {
        fprintf(stderr, "File has invalid descripter: %s\n", req->file->filePath);
        return false;
    }
    uint64_t size = req->nBytes;
    assert(size > 0);
    if(req->nBytes > req->bufferSize) {
        fprintf(stderr, "Buffer smaller than nBytes (%lu < %lu) for fole %s\n", req->bufferSize, req->nBytes, req->file->filePath);
        return false;
    }
    req->posixIocb = (struct aiocb){0};
    req->posixIocb.aio_fildes = req->file->fd;
    req->posixIocb.aio_buf = req->buffer;
    req->posixIocb.aio_offset = req->fileOffset;
    req->posixIocb.aio_nbytes = req->nBytes;

    req->posixIocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    req->posixIocb.aio_sigevent.sigev_signo = SIGUSR1;
    req->posixIocb.aio_sigevent.sigev_value.sival_ptr = req;

    if(isRead) {
        result = aio_read(&req->posixIocb);
    } else {
        result = aio_write(&req->posixIocb);
    }
    printf("aio result %d\n", result);
    return result >= 0;
}

void linux_process_aio() {
    /*if(NULL == aio) {
        return;
    }*/
    // Processing happens on threads started by the AIO system
}

bool get_next_io_event(struct AikePlatform *platform, AikeIOEvent *event) {
    if(aio->numEvents > 0)
    {
        *event = aio->events[--aio->numEvents];
        return true;
    }
    return false;
}

void destroy_async_io(struct AikePlatform *platform) {
    // TODO: do we need to do something with signals here?
    free(aio);
    aio = NULL;
}

#endif
