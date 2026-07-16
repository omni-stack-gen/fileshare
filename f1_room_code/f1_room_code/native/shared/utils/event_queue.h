#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include <utils/list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct event_queue
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool initialized;
    bool canceled;
    size_t event_size;
    struct list_head events;
} event_queue_t;

int event_queue_init(event_queue_t *queue, size_t event_size);
void event_queue_reset_cancel(event_queue_t *queue);
void event_queue_cancel(event_queue_t *queue);
int event_queue_push(event_queue_t *queue, const void *event);
int event_queue_wait(event_queue_t *queue, void *out_event, int timeout_ms);
void event_queue_clear(event_queue_t *queue);
void event_queue_destroy(event_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif
