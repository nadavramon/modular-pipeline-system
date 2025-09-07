#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* log_transform(const char* in) {
    printf("[logger] %s\n", in);
    fflush(stdout);
    return strdup(in);
}

__attribute__((visibility("default")))
const char* plugin_init(int q) {
    return common_plugin_init(log_transform, "logger", q);
}
