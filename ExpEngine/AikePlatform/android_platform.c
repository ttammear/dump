#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/native_window.h>
#include <android/log.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/times.h> 

#include <sys/mman.h>

#include "aike_platform.h"
#include "common_platform.h"

// TODO: shared with linux_main.c
#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))

const char *logTag = "AIKE_PLATFORM";

extern void aike_update_window(struct AikePlatform *platform, struct AikeWindow *win);
extern void aike_init(struct AikePlatform *platform);
extern void aike_deinit(struct AikePlatform *platform);
extern void aike_update(struct AikePlatform *platform);

AikeThread* create_thread(void *userData, thread_proc_t procedure);
bool join_thread(AikeThread *thread, void **result);
bool detach_thread(AikeThread *thread);
void* android_start(void *userData);

typedef struct AndroidPlatformState
{
    struct timespec monotonic_time_start;

    NativeWindowType window;

    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLSurface eglSurface;

    AikePlatform platform;

    AikeThread *gameThread;

    atomic_bool isWindowSet;
    atomic_bool running;
} AndroidPlatformState;

AndroidPlatformState android;

/* returns current time in seconds */
double now() {
    struct tms now_tms ;
    return (double)(times(&now_tms)) / 100.0 ;
}

//NativeWindowType displayWindow;

const EGLint config16bpp[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, 
    EGL_RED_SIZE,   5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE,  5,
    EGL_NONE
};


const EGLint config24bpp[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, 
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_NONE
};

const EGLint config32bpp[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, 
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
};


GLfloat colors[3][4] = {
    {1.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f}
};

GLfloat vertices[3][3] = {
    {0.0f, 0.7f, 0.0f},
    {-0.7f, -0.7f, 0.0f},
    {0.7f, -0.7f, 0.0f}
};

void setup();

void aike_wrapper_set_window(ANativeWindow *window)
{
    android.window = window;
    atomic_store(&android.isWindowSet, true);
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Got window %p\n", window);
}

void aike_wrapper_create()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper create()");
    atomic_store(&android.isWindowSet, false);
    atomic_store(&android.running, true);
    // TODO: this will never start again if stop() gets called!
    android.gameThread = create_thread(&android, android_start);
}

void aike_wrapper_destroy()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper destroy()");
}

void aike_wrapper_start()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper start()");
}

void aike_wrapper_stop()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper stop()");
    atomic_store(&android.running, false);
}

void aike_wrapper_pause()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper pause()");
}

void aike_wrapper_resume()
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "wrapper resume()");
}

void draw_tri() {
    /*glViewport(
        0,
        0,
        ANativeWindow_getWidth(android.window),
        ANativeWindow_getHeight(android.window)
    );
    glRotatef(0.5, 0, 0, 1) ;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glColorPointer(4, GL_FLOAT, 0, colors);
    glVertexPointer(3, GL_FLOAT, 0, vertices);

    // Draw the triangle (3 vertices) 
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY); */
}


void setup() {

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "main\n");

    EGLint majorVersion, minorVersion;
    EGLContext eglContext;
    EGLSurface eglSurface;
    EGLConfig eglConfig;
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint format;
    const EGLint* config = NULL ;
    int numConfigs;
    int windowFormat;
    double start_time  = now() ;

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "got display?\n");


    /* 
     * create a window surface that covers the entire screen.
     * This function is from libui. 
     */
    //displayWindow = android_createDisplaySurface();
    //__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "created display surface\n");

    eglInitialize(eglDisplay, &majorVersion, &minorVersion);
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "EGL version: %d.%d\n",majorVersion,minorVersion);

    if(android.window == 0) {
       __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Could not create window\n") ;
       //__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Started from an on-device shell ?\n") ;
       //__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "use: adb shell <path_to_exe>/%s\n",argv[0]) ;
       exit(-1) ;
    }

    /* get the format of the window. */
    windowFormat = ANativeWindow_getFormat(android.window) ;

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Window specs: %d*%d format=%d\n",
           ANativeWindow_getWidth(android.window),
           ANativeWindow_getHeight(android.window),
           windowFormat
    ) ;

    /* choose the config according to the format of the window. */
    switch(windowFormat) {
     case WINDOW_FORMAT_RGBA_8888:
       config = config32bpp ;
       break ;
     case WINDOW_FORMAT_RGBX_8888:
       config = config24bpp ;       
       break ;
     case WINDOW_FORMAT_RGB_565:
       config = config16bpp ;       
       break ;
     default:
       __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Unknown window format\n") ;
       exit(-1) ;
    }

    if (!eglChooseConfig(eglDisplay, config32bpp, &eglConfig, 1, &numConfigs)) {
        __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "eglChooseConfig failed\n");
        if (eglContext==0) __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Error code: %x\n", eglGetError());
        exit(-1) ;
    }

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglContext = eglCreateContext(eglDisplay, eglConfig,  EGL_NO_CONTEXT, context_attribs);
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "GL context: %p\n", eglContext);
    if (eglContext==0) {
       __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Error code: %x\n", eglGetError());
       exit(-1) ;
    }

    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, android.window, NULL);

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "GL surface: %p\n", eglSurface);
    if (eglSurface==0) {
       __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Error code: %x\n", eglGetError());
       exit(-1);
    }

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", 
        "Vendor: %s, Renderer: %s, Version: %s\n",
        glGetString(GL_VENDOR),
        glGetString(GL_RENDERER),
        glGetString(GL_VERSION)
    ) ;

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Extensions: %s\n", glGetString(GL_EXTENSIONS)) ;

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Spinning triangle for 20s\n") ;

    /*while (now() - start_time < 20.0) {
        draw_tri();
        eglSwapBuffers(eglDisplay, eglSurface);
    }*/

    draw_tri();
    eglSwapBuffers(eglDisplay, eglSurface);

    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "End (tap the screen of the phone to continue).\n") ;
}

void hello(const char *str)
{
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "hello there! %s\n", str);
    __android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "\n this is log messge %s \n", str);
}

void create_opengl_context(AikeWindow *window)
{
    EGLint majorVersion, minorVersion;
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, &majorVersion, &minorVersion);

    int windowFormat = ANativeWindow_getFormat(android.window);

    const EGLint* config = NULL;
    switch(windowFormat) {
     case WINDOW_FORMAT_RGBA_8888:
       config = config32bpp ;
       break ;
     case WINDOW_FORMAT_RGBX_8888:
       config = config24bpp ;       
       break ;
     case WINDOW_FORMAT_RGB_565:
       config = config16bpp ;       
       break ;
     default:
       __android_log_print(ANDROID_LOG_ERROR, logTag, "Unknown window format\n") ;
       exit(-1) ;
    }

    EGLConfig eglConfig;
    int numConfigs;
    if (!eglChooseConfig(eglDisplay, config, &eglConfig, 1, &numConfigs)) {
        __android_log_print(ANDROID_LOG_ERROR, logTag, "eglChooseConfig() failed\n");
        exit(-1) ;
    }

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig,  EGL_NO_CONTEXT, context_attribs);
    if (eglContext == 0) {
       __android_log_print(ANDROID_LOG_ERROR, logTag, "eglCreateContext() returned NULL! Error code: %x\n", eglGetError());
       exit(-1) ;
    }

    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, android.window, NULL);

    if (eglSurface==0) {
       __android_log_print(ANDROID_LOG_ERROR, logTag, "eglCreateWindowSurface() returned NULL! Error code: %x\n", eglGetError());
       exit(-1);
    }

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    __android_log_print(ANDROID_LOG_DEBUG, logTag, 
        "Vendor: %s, Renderer: %s, Version: %s\n",
        glGetString(GL_VENDOR),
        glGetString(GL_RENDERER),
        glGetString(GL_VERSION)
    ) ;

    __android_log_print(ANDROID_LOG_DEBUG, logTag, "Extensions: %s\n", glGetString(GL_EXTENSIONS)) ;

    android.eglDisplay = eglDisplay;
    android.eglContext = eglContext;
    android.eglSurface = eglSurface;
}

void destroy_opengl_context(AikeWindow *window)
{
    // TODO
    eglDestroySurface(android.eglDisplay, android.eglSurface);
    eglDestroyContext(android.eglDisplay, android.eglContext);
    eglTerminate(android.eglDisplay);

    android.eglDisplay = 0;
    android.eglContext = 0;
}

bool mouse_coord_valid(struct AikePlatform *platform, AikeWindow *window)
{
    return true;
}

void present_frame(struct AikePlatform *platform, AikeWindow *window)
{
    // TODO: any window?
    assert(window == &android.platform.mainWin);
    eglSwapBuffers(android.eglDisplay, android.eglSurface);
}

void make_window_current(struct AikePlatform *platform, AikeWindow *window)
{
    // TODO: any window?
    assert(window == &android.platform.mainWin);
    eglMakeCurrent(android.eglDisplay, android.eglSurface, android.eglSurface, android.eglContext);
}

// TODO: shared with linux_main.c
AikeTime linux_get_monotonic_time(AikePlatform *platform)
{
    AikeTime ret;
    
    struct timespec curTime;
    clock_gettime(CLOCK_MONOTONIC, &curTime);

    struct timespec startTime = android.monotonic_time_start;
    ret.sec = curTime.tv_sec - startTime.tv_sec;
    int64_t nsecSinceStart = curTime.tv_nsec - startTime.tv_nsec;
    if(nsecSinceStart < 0)
    {
        nsecSinceStart += 1000000000;
        ret.sec--;
    }
    ret.nsec = nsecSinceStart;
    assert(ret.sec >= 0 && nsecSinceStart >=0);
    return ret;
}

// TODO: shared with linux_main.c
void linux_sleep(uint32_t us)
{
    usleep(us);
}

bool swap_interval(struct AikeWindow *window, int interval)
{
    // TODO: is there such feature on android?
    return false;
}

// TODO: shared with linux_main.c
AikeThread* create_thread(void *userData, thread_proc_t procedure)
{
    AikeThread *ret = (AikeThread*)malloc(sizeof(AikeThread));
    ret->exited = false;
    pthread_t *tstate = (pthread_t*)malloc(sizeof(pthread_t));
    ret->nativePtr = tstate;

    if(pthread_create(tstate, NULL, procedure, userData))
    {
        free(tstate);
        free(ret);
        fprintf(stderr, "Creating thread failed!\n");
        return NULL;
    }

    return ret;
}

// TODO: shared with linux_main.c
bool join_thread(AikeThread *thread, void **result)
{
    pthread_t *state = (pthread_t*)thread->nativePtr;
    void *ret;
    if(pthread_join(*state, result))
    {
        return false;
    }
    free(thread->nativePtr);
    // TODO: we never free the AikeThread, is this ok?
    thread->exited = true;
    return true;
}

// TODO: shared with linux_main.c
bool detach_thread(AikeThread *thread)
{
    pthread_t *state = (pthread_t*)thread->nativePtr;
    if(pthread_detach(*state))
    {
        return false;
    }
    free(thread->nativePtr);
    // TODO: we never free the AikeThread, is this ok?
    thread->exited = true;
    return true;
}

// TODO: shared with linux_main.c
AikeMemoryBlock *allocate_memory(AikePlatform *platform, size_t size, uint32_t flags)
{
    long pagesize = sysconf(_SC_PAGESIZE);
    assert(pagesize > 0);
    // align header to cache line
    uint32_t headerSize = ALIGN_UP(sizeof(AikeMemoryBlock), 8*AIKE_EXPECTED_CACHE_LINE_SIZE);
    size_t totalSize = ALIGN_UP(size + headerSize, 8*pagesize);
    int prot = PROT_READ | PROT_WRITE;
    if((flags & Aike_Memory_Flag_AllowExec) == 0)
        prot |= PROT_EXEC;
    int mmapflags = MAP_PRIVATE | MAP_ANONYMOUS;
    if((flags & Aike_Memory_Flag_Commit) != 0)
        mmapflags |= MAP_POPULATE;
    void *mem = mmap(NULL, totalSize, prot, mmapflags, -1, 0);
    if(mem)
    {
        // already zeroed, dont need to zero
        AikeMemoryBlock *block = (AikeMemoryBlock*)mem;
        block->flags = flags;
        block->size = size;
        block->realSize = totalSize;
        block->memory = ((uint8_t*)mem) + headerSize;
        assert(((uintptr_t)block->memory & (AIKE_EXPECTED_CACHE_LINE_SIZE-1)) == 0);
        return block;
    }
    fprintf(stderr, "Memory allocation failed! Size: %llu Flags: %d\n", (unsigned long long)size, flags);
    return NULL;
}

// TODO: shared with linux_main.c
void free_memory(AikePlatform *platform, AikeMemoryBlock *block)
{
    munmap((void*)block, block->realSize);
};

// TODO: shared with linux_main.c
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

// TODO: shared with linux_main.c
uint32_t next_files(AikePlatform *platform, AikeDirectory *adir, AikeFileEntry buf[], uint32_t bufLen)
{
    uint32_t curIndex = 0;
    struct dirent *entry;
    struct stat statBuf;
    for(int i = 0; i < bufLen; i++)
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
    return curIndex;
}

// TODO: shared with linux_main.c
void close_directory(AikePlatform *platform, AikeDirectory *adir)
{
    assert(adir);
    closedir(adir->directory);
    free(adir);
}

// TODO: shared with linux_main.c
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

// TODO: shared with linux_main.c
void unmap_file(struct AikePlatform *platform, AikeMemoryBlock *block)
{
    int res = munmap(block->memory, block->size);
    assert(res >= 0);
    assert(block);
    free(block);
}

// TODO: shared with linux_main.c
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

// TODO: shared with linux_main.c
void close_file(struct AikePlatform *platform, AikeFile *file)
{
   int res = close(file->fd); 
   if(res < 0)
   {
       fprintf(stderr, "Failed to close file %s, return code: %d. Was it even open?\n", file->filePath, res);
   }
   free(file);
}


uint32_t next_character(struct AikePlatform *platform)
{
    // TODO: implement
    return 0;
}

void* android_start(void *userData)
{
    while(!atomic_load(&android.isWindowSet))
    {
        __android_log_print(ANDROID_LOG_DEBUG, logTag, "Window not set waiting...\n") ;
        linux_sleep(1000000);
    }
    __android_log_print(ANDROID_LOG_DEBUG, logTag, "Starting!\n");

    platform_init(&android.platform);

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    #warning not setting running directory!

    android.platform.create_opengl_context = create_opengl_context;
    android.platform.destroy_opengl_context = destroy_opengl_context;
    android.platform.create_borderless_window = NULL;
    android.platform.destroy_window = NULL;
    android.platform.resize_window = NULL;
    android.platform.move_window = NULL;
    android.platform.screen_to_window_coord = NULL;
    android.platform.mouse_coord_valid = mouse_coord_valid;
    android.platform.set_cursor = NULL;
    android.platform.present_frame = present_frame;
    android.platform.make_window_current = make_window_current;
    android.platform.get_monotonic_time = linux_get_monotonic_time;
    android.platform.sleep = linux_sleep;
    android.platform.swap_interval = swap_interval;
    android.platform.create_thread = create_thread;
    android.platform.join_thread = join_thread;
    android.platform.detach_thread = detach_thread;
    android.platform.allocate_memory = allocate_memory;
    android.platform.free_memory = free_memory;
    android.platform.open_directory = open_directory;
    android.platform.next_files = next_files;
    android.platform.close_directory = close_directory;
    android.platform.open_file = open_file;
    android.platform.close_file = close_file;
    android.platform.map_file = map_file;
    android.platform.unmap_file = unmap_file;
    android.platform.next_character = next_character;

#ifdef AIKE_AIO
    android.platform.init_async_io = NULL;
    android.platform.submit_io_request = NULL;
    android.platform.get_next_io_event = NULL;
    android.platform.destroy_async_io = NULL;


    android.platform.tcp_listen = NULL;
    android.platform.tcp_close_server = NULL;
    android.platform.tcp_accept = NULL;
    android.platform.tcp_connect = NULL;
    android.platform.tcp_close_connection = NULL;
    android.platform.tcp_recv = NULL;
    android.platform.tcp_send = NULL;
#endif

    // TODO: mainWin
    android.platform.mainWin.width = (float)ANativeWindow_getWidth(android.window);
    android.platform.mainWin.height = (float)ANativeWindow_getHeight(android.window);
    android.platform.mainWin.screenX = 0.0f;
    android.platform.mainWin.screenY = 0.0f;
    android.platform.mainWin.doubleBuffered = true;
    android.platform.mainWin.nativePtr = NULL;

    clock_gettime(CLOCK_MONOTONIC, &android.monotonic_time_start);

    // TODO: create new thread for engine
    aike_init(&android.platform);
    // TODO: no mainWin
    aike_update_window(&android.platform, &android.platform.mainWin);

    while(atomic_load(&android.running))
    {
        // TODO: input events
        aike_update(&android.platform);
    }

    __android_log_print(ANDROID_LOG_DEBUG, logTag, "Done, wrapping up!\n");

    aike_deinit(&android.platform);
    
    return (void*)0xCAFEBABE;
}
