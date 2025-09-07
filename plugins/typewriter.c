#include "plugin_common.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static const char* tw_transform(const char* in) {
    fputs("[typewriter] ", stdout);
    for (const char* p=in; *p; ++p) { 
        fputc(*p, stdout); 
        fflush(stdout); 
        usleep(100000); 
    }
    fputc('\n', stdout); 
    fflush(stdout);
    return strdup(in);
}
__attribute__((visibility("default")))
const char* plugin_init(int q) { 
    return common_plugin_init(tw_transform,"typewriter",q); 
}