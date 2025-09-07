#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H
#include <pthread.h>
#include "sync/consumer_producer.h"

typedef const char* (*transform_fn)(const char*);

typedef struct {
    const char* name;
    consumer_producer_t queue;
    pthread_t worker;
    transform_fn next_place_work;
    transform_fn process_function;
    int initialized;
    int finished;
} plugin_context_t;

void* plugin_consumer_thread(void* arg);
void log_error(plugin_context_t* ctx, const char* msg);
void log_info(plugin_context_t* ctx, const char* msg);

const char* common_plugin_init(transform_fn process, const char* name, int queue_size);

__attribute__((visibility("default"))) const char* plugin_init(int queue_size);
__attribute__((visibility("default"))) const char* plugin_fini(void);
__attribute__((visibility("default"))) const char* plugin_place_work(const char* s);
__attribute__((visibility("default"))) void plugin_attach(transform_fn next_place_work);
__attribute__((visibility("default"))) const char* plugin_wait_finished(void);
__attribute__((visibility("default"))) const char* plugin_get_name(void);

#endif