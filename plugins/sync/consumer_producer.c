#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>

static void signal_if_not_empty(consumer_producer_t* q) {
    if (q->count > 0) { 
        monitor_signal(&q->not_empty_monitor);
    }
}

static void signal_if_not_full(consumer_producer_t* q) {
    if (q->count < q->capacity) {
        monitor_signal(&q->not_full_monitor);
    }
}

const char* consumer_producer_init(consumer_producer_t* q, int capacity){
    if (!q) { 
        return "queue: null";
    }
    if (capacity <= 0) {
        return "queue: bad capacity";
    } 
    memset(q, 0, sizeof(*q));
    q -> items = (char**) calloc ((size_t)capacity, sizeof(char*));
    if(!q->items) { 
        return "queue: oom";
    }
    q -> capacity = capacity;
    if(pthread_mutex_init(&q->lock, NULL)!=0) { 
        return "queue: mutex init failed";
    }
    if (monitor_init(&q->not_full_monitor)!=0) {
        return "queue: monitor init failed";
    }
    if (monitor_init(&q->not_empty_monitor)!=0) {
        return "queue: monitor init failed";
    } 
    if (monitor_init(&q->finished_monitor)!=0) {
        return "queue: monitor init failed";
    }
    q->finishing = 0;
    return NULL;
}

void consumer_producer_destroy(consumer_producer_t* q) {
    if(!q) {
        return;
    }
    for (int i = 0; i < q -> count; i++) {
        int idx = (q->head + i) % q -> capacity;
        free(q -> items[idx]);
    }
    free( q-> items);
    monitor_destroy(&q -> not_full_monitor);
    monitor_destroy(&q -> not_empty_monitor);
    monitor_destroy(&q -> finished_monitor);
    pthread_mutex_destroy(& q-> lock);
}

const char* consumer_producer_put(consumer_producer_t* q, const char* s) {
    if(!q || !s) {
        return "queue: bad args";
    } 
    for (;;) {
        pthread_mutex_lock(& q-> lock);
        if (q -> count < q -> capacity){
            int idx = q -> tail;
            q -> items[idx] = (char*)s;
            q -> tail = (q -> tail + 1) % q -> capacity;
            q -> count++;
            signal_if_not_empty(q);
            pthread_mutex_unlock(&q -> lock);
            return NULL;
        }
        pthread_mutex_unlock(&q -> lock);
        monitor_wait(&q -> not_full_monitor);
    }
}

char* consumer_producer_get(consumer_producer_t* q){
    if (!q) {
        return NULL;
    } 
    for (;;) {
        pthread_mutex_lock(&q -> lock);
        if (q -> count > 0) {
            int idx = q -> head;
            char* s = q -> items[idx];
            q -> items[idx] = NULL;
            q -> head = (q -> head + 1) % q -> capacity;
            q -> count--;
            signal_if_not_full(q);
            pthread_mutex_unlock(&q -> lock);
            return s;
        }
        if (q -> finishing) {
            pthread_mutex_unlock(&q -> lock);
            return strdup("<END>");
        }
        pthread_mutex_unlock(&q -> lock);
        monitor_wait(&q -> not_empty_monitor);
    }
}

void consumer_producer_signal_finished(consumer_producer_t* q) {
    if (!q) {
        return;
    }
    pthread_mutex_lock(&q -> lock);
    q -> finishing = 1;
    pthread_mutex_unlock(&q -> lock);
    monitor_signal(&q -> not_empty_monitor);
    monitor_signal(&q -> not_full_monitor);
    monitor_signal(&q -> finished_monitor);
}

int consumer_producer_wait_finished(consumer_producer_t* q) {
    if (!q) {
        return -1;
    } 
    return monitor_wait(&q -> finished_monitor);
}