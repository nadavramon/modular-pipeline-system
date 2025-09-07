#ifndef MONITOR_H
#define MONITOR_H
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int signaled;
} monitor_t;

int  monitor_init(monitor_t* m);
void monitor_destroy(monitor_t* m);
void monitor_signal(monitor_t* m);
void monitor_reset(monitor_t* m);
int  monitor_wait(monitor_t* m);

#endif