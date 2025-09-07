#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H
#include "monitor.h"

typedef struct {
    char** items;
    int capacity, count, head, tail;
    monitor_t not_full_monitor;
    monitor_t not_empty_monitor;
    monitor_t finished_monitor;
    pthread_mutex_t lock;
    int finishing;
} consumer_producer_t;

const char* consumer_producer_init(consumer_producer_t* q, int capacity);
void consumer_producer_destroy(consumer_producer_t* q);
const char* consumer_producer_put(consumer_producer_t* q, const char* s);
char* consumer_producer_get(consumer_producer_t* q);
void consumer_producer_signal_finished(consumer_producer_t* q);
int consumer_producer_wait_finished(consumer_producer_t* q);

#endif