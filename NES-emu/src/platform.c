#include <stdio.h>
#include <string.h>
#include "ttTypes.h"

//#ifdef __linux__
b32 linux_choose_file(char *buffer, int bufSize) {
    FILE *pipe = popen("zenity --file-selection --file-filter=\"*nes\" --title=\"Choose ROM\"", "r");
    if (!pipe)
        return 0;
    buffer[0] = 0;
    while(!feof(pipe)) {
        if(fgets(buffer, bufSize, pipe) == NULL)
            break;
    }
    // remove newline
    char *pos;
    if ((pos=strchr(buffer, '\n')) != NULL)
        *pos = '\0';
    return pclose(pipe) == 0;
}
//#endif

b32 platform_choose_file(char *buffer, int bufSize) {
//#ifdef __linux__
    return linux_choose_file(buffer, bufSize);
//#else
//#error platform_choose_file not supported on this platform
//#endif
}
