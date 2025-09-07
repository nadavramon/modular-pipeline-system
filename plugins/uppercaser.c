#include "plugin_common.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* upper_transform(const char* in) {
    size_t n = strlen(in);
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < n; ++i) {
        out[i] = (char)toupper((unsigned char)in[i]);
    }
    out[n] = '\0';
    return out;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(upper_transform, "uppercaser", queue_size);
}