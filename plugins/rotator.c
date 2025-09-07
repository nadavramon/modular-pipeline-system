#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

static const char* rotator_transform(const char* in){
    size_t n=strlen(in); char* out=(char*)malloc(n+1); 
    if (!out) {
        return NULL;
    }
    if (n) { 
        out[0]=in[n-1]; memcpy(out+1,in,n-1); 
    } 
    out[n]='\0'; return out;
}

__attribute__((visibility("default")))
const char* plugin_init(int q){ 
    return common_plugin_init(rotator_transform,"rotator",q); 
}