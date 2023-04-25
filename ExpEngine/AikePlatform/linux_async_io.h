#ifndef AIKE_LINUX_AIO_H
#define AIKE_LINUX_AIO_H

#define _GNU_SOURCE

#include "aike_aio.h"
#include <sys/syscall.h>
//#include <libaio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h> 


#endif

#define LINUX_AIO_IMPLEMETATION
#ifdef LINUX_AIO_IMPLEMENTATION

static inline int io_setup(unsigned nr, aio_context_t *ctxp) {
	return syscall(__NR_io_setup, nr, ctxp);
}

static inline int io_destroy(aio_context_t ctx) {
	return syscall(__NR_io_destroy, ctx);
}

static inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp) {
	return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

static inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
		struct io_event *events, struct timespec *timeout) {
	return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

// TODO: move this to platform struct instead of global?
typedef struct AikeIOState
{
    AikePlatform *platform;
    aio_context_t ctx;
    uint32_t numEvents;
    AikeIOEvent events[AIO_EVENT_BUF_SIZE];
    AikeQueuedIORequest *ioReqHead;
    AikeQueuedIORequest *ioReqTail;
} AikeIOState;

AikeIOState *aio;

void init_async_io(struct AikePlatform *platform)
{
    aio = (AikeIOState*)malloc(sizeof(AikeIOState));
    assert(aio);
    *aio = (AikeIOState){0};
    aio->platform = platform;
    int ret = io_setup(AIO_MAX_EVENTS, &aio->ctx);
    if(ret < 0)
    {
        fprintf(stderr, "Initializing Linux AIO failed!\n");
        assert(false);
    }
}

bool try_submit_io_request(struct AikePlatform *platform, AikeIORequest *req, bool *again) {
    struct iocb *cbs = &req->linuxIocb;
    int ret = io_submit(aio->ctx, 1, &cbs);
    assert(req->file);
    assert(req->linuxIocb.aio_fildes == req->file->fd);
    *again = false;
    if(ret < 1) { // should indicate number of iocbs submitted
        if(errno == EAGAIN) {
            AikeQueuedIORequest* qreq = calloc(1, sizeof(AikeQueuedIORequest));
            qreq->req = req;
            qreq->next = NULL;
            if(aio->ioReqHead == NULL) {
                assert(aio->ioReqTail == NULL);
                aio->ioReqHead = qreq;
            } else {
                aio->ioReqTail->next = qreq;
            }
            aio->ioReqTail = qreq;
            *again = true;
            return true;
        }
        fprintf(stderr, "Linux IO request on %s failed isRead: %d error code: %d\n", req->file->filePath, req->type == Aike_IO_Request_Read, errno);
        return false;
    }
    return true;
}

bool submit_io_request(struct AikePlatform *platform, AikeIORequest *req)
{
    assert(req->type == Aike_IO_Request_Read || req->type == Aike_IO_Request_Write);

    bool isRead = req->type == Aike_IO_Request_Read;
    if(req->file->fd < 0)
    {
        fprintf(stderr, "File has invalid descriptor: %s\n", req->file->filePath);
        return false;
    }

    uint64_t size = req->nBytes;
    assert(size > 0);

    if(req->nBytes > req->bufferSize)
    {
        fprintf(stderr, "Buffer smaller than nBytes (%lu < %lu) for file %s\n", req->bufferSize, req->nBytes, req->file->filePath);
        return false;
    }

    req->linuxIocb = (struct iocb){0};
    req->linuxIocb.aio_fildes = req->file->fd;
    req->linuxIocb.aio_lio_opcode = isRead ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    req->linuxIocb.aio_buf = (uintptr_t)req->buffer;
    req->linuxIocb.aio_offset = req->fileOffset;
    req->linuxIocb.aio_nbytes = req->nBytes;

    bool again;
    return try_submit_io_request(platform, req, &again);
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

void linux_process_aio()
{
    if(NULL == aio)
        return;

    // resubmit failed requests
    AikeQueuedIORequest *r;
    while((r = aio->ioReqHead) != NULL) {
        aio->ioReqHead = r->next;
        if(r->next == NULL) {
            aio->ioReqTail = NULL;
        }

        bool again;
        bool result = try_submit_io_request(aio->platform, r->req, &again);
        free(r);
        // NOTE: do not remove this, this also prevents endless loop on failure (the same element
        // will be pushed to tail and then read from head)
        // when io fails it makes sense to let the system settle (usually it will be EAGAIN)
        if(!result || again) {
            break;
        }
    }

    // get new eveents
    int slotsLeft = AIO_EVENT_BUF_SIZE - aio->numEvents;
    if(aio && slotsLeft > 0)
    {
        struct io_event events[AIO_EVENT_DEQUEUE_COUNT];
        struct timespec timeout = (struct timespec){0};
        int ret = 0;

        int maxEvents = MIN(AIO_EVENT_DEQUEUE_COUNT, slotsLeft);

        while(maxEvents > 0 && (ret = io_getevents(aio->ctx, 1, maxEvents, events, &timeout)) > 0)
        {
            assert(aio->numEvents + ret <= AIO_EVENT_BUF_SIZE);
            AikeIOEvent *start = &aio->events[aio->numEvents];
            for(int i = 0; i < ret; i++)
            {
                bool completed = events[i].res >= 0 && events[i].res2 >= 0;
                start[i].type = Aike_IO_Event_IO_Request_Done;
                start[i].ioRequestDone.status = completed ? Aike_IO_Status_Completed : Aike_IO_Status_Failed;
                start[i].ioRequestDone.request = (AikeIORequest*)events[i].obj;
                assert(start[i].ioRequestDone.request->file);
            }
            aio->numEvents += ret;
            maxEvents = MIN(AIO_EVENT_DEQUEUE_COUNT, AIO_EVENT_BUF_SIZE - aio->numEvents);
        }
    }
}

// TODO: make thread safe along with linux_process_aio
bool get_next_io_event(struct AikePlatform *platform, AikeIOEvent *event)
{
    if(aio->numEvents > 0)
    {
        *event = aio->events[--aio->numEvents];
        return true;
    }
    else // TODO: remove?
        linux_process_aio();
    return false;
}

void destroy_async_io(struct AikePlatform *platform)
{
    io_destroy(aio->ctx);
    free(aio);
    aio = NULL;
}


#endif
