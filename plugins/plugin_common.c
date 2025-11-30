
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "plugin_common.h"
#include "sync/consumer_producer.h"
#include "sync/monitor.h"

#define END_TOKEN "<END>"
#define MIN_QUEUE_SIZE 1
#define MAX_QUEUE_SIZE 65536

static plugin_context_t g_ctx;

static int is_end(const char* s) {
    return s && strcmp(s, END_TOKEN) == 0;
}

void log_error(plugin_context_t* ctx, const char* msg) {
    if (!ctx || !ctx -> name || !msg) {
        return;
    }
    fprintf(stderr, "[ERROR][%s] %s\n", ctx -> name, msg);
}

void log_info(plugin_context_t* ctx, const char* msg) {
    (void)ctx;
    (void)msg;
}

void* plugin_consumer_thread(void* arg) {
    (void)arg;
    for (;;) {
        char* in = consumer_producer_get(&g_ctx.queue);
        if (!in) {
            continue;
        }
        const int saw_end = is_end(in);
        const char* out = in;
        if (!saw_end && g_ctx.process_function) {
            out = g_ctx.process_function(in);
        }
        if (out != in) {
            free(in);
        }
        if (g_ctx.next_place_work) {
            g_ctx.next_place_work(saw_end ? END_TOKEN : out);
        }
        free((void*)out);
        if (saw_end) {
            break;
        }
    }
    g_ctx.finished = 1;
    consumer_producer_signal_finished(&g_ctx.queue);
    return NULL;
}

const char* common_plugin_init(const char* (*process)(const char*), const char* name, int queue_size) {
    if (!process) {
        return "invalid process function";
    }
    if (queue_size <= 0) {
        return "queue_size must be >= 1";
    }
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.name = name ? name : "plugin";
    g_ctx.process_function = process;
    g_ctx.next_place_work = NULL;
    g_ctx.initialized = 0;
    g_ctx.finished = 0;
    
    const char* err = consumer_producer_init(&g_ctx.queue, queue_size);
    if (err) {
        return err;
    }
    int rc = pthread_create(&g_ctx.worker, NULL, plugin_consumer_thread, NULL);
    if (rc != 0) {
        consumer_producer_destroy(&g_ctx.queue);
        return "pthread_create_failed";
    }
    g_ctx.initialized = 1;
    return NULL;
}

__attribute__((visibility("default")))
const char* plugin_fini(void) {
    if (!g_ctx.initialized)     return "plugin not initialized";
    if (g_ctx.finished == 0) {
        // Ask the worker to finish if it hasn't already
        (void)plugin_place_work("<END>");
    }
    if (pthread_join(g_ctx.worker, NULL) != 0) {
        return "pthread_join failed";
    }
    consumer_producer_destroy(&g_ctx.queue);
    g_ctx.initialized = 0;
    return NULL;
}

__attribute__((visibility("default")))
const char* plugin_place_work(const char* s) {
    if (!g_ctx.initialized)     return "plugin not initialized";
    if (g_ctx.finished)         return "plugin already finished";
    if (!s)                     s = "";

    char* copy = strdup(s);                 // queue owns the copy
    if (!copy)                 return "out of memory";

    const char* err = consumer_producer_put(&g_ctx.queue, copy);
    if (err) { free(copy); return err; }
    return NULL;
}

__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*)) {
    // Attaching before init is okay; we just store the pointer.
    g_ctx.next_place_work = next_place_work;
}

__attribute__((visibility("default")))
const char* plugin_wait_finished(void) {
    if (!g_ctx.initialized) return "plugin not initialized";
    (void)consumer_producer_wait_finished(&g_ctx.queue);
    return NULL;
}

__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return g_ctx.name ? g_ctx.name : "plugin";
}
