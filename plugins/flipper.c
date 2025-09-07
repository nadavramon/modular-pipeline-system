#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

static const char* flip_transform(const char* in) {
    size_t n = strlen(in);
    char* out = (char*) malloc (n + 1);
    if (!out)
    {
        return "flipper: OOM";
    }
    for (size_t i = 0; i < n; ++i)
    {
        out[i] = in[n - 1 - i];
    }
    out[n] = '\0';
    return out;
}

__attribute__((visibility("default")))
const char* plugin_init(int q) {
    return common_plugin_init(flip_transform, "flipper", q);
}