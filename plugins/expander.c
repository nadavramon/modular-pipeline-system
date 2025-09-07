#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

static const char* expand_transform(const char* in) {
size_t n=strlen(in); 
char* out = (char*) malloc (n ? (2*n) : 1); 
if(!out) {
    return NULL;
} 
if (n == 0) {
    out[0]='\0';
    return out;
}
for (size_t i = 0; i < n; ++i) {
    out[2*i] = in[i];
    if (i < n - 1) {
        out[2 * i + 1] = ' ';
    }
}
out[2 * n - 1] = '\0';
return out;
}

__attribute__((visibility("default")))
const char* plugin_init(int q) {
    return common_plugin_init(expand_transform, "expander", q);
}