#include <utils/event_queue.h>

#include <errno.h>
#include <string.h>
#include <time.h>

#include <utils/bmem.h>

typedef struct event_node
{
    struct list_head node;
    unsigned char event[];
} event_node_t;

static void event_queue_free_nodes_locked(event_queue_t *queue)
{
    struct list_head *pos;
    struct list_head *next;

    list_for_each_safe(pos, next, &queue->events)
    {
        event_node_t *node = list_entry(pos, event_node_t, node);
        list_del(pos);
        bfree(node);
    }
}

static int event_queue_pop_locked(event_queue_t *queue, void *out_event)
{
    event_node_t *node;

    if (list_empty(&queue->events))
    {
        return 0;
    }

    node = list_first_entry(&queue->events, event_node_t, node);
    list_del(&node->node);
    memcpy(out_event, node->event, queue->event_size);
    bfree(node);
    return 1;
}

int event_queue_init(event_queue_t *queue, size_t event_size)
{
    pthread_condattr_t cond_attr;
    int rc;

    if (queue == NULL || event_size == 0)
    {
        return -EINVAL;
    }

    memset(queue, 0, sizeof(*queue));
    rc = pthread_mutex_init(&queue->lock, NULL);
    if (rc != 0)
    {
        return -rc;
    }

    rc = pthread_condattr_init(&cond_attr);
    if (rc != 0)
    {
        pthread_mutex_destroy(&queue->lock);
        return -rc;
    }
    rc = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    if (rc == 0)
    {
        rc = pthread_cond_init(&queue->cond, &cond_attr);
    }
    pthread_condattr_destroy(&cond_attr);
    if (rc != 0)
    {
        pthread_mutex_destroy(&queue->lock);
        return -rc;
    }

    queue->initialized = true;
    queue->event_size = event_size;
    INIT_LIST_HEAD(&queue->events);
    return 0;
}

void event_queue_reset_cancel(event_queue_t *queue)
{
    if (queue == NULL || !queue->initialized)
    {
        return;
    }

    pthread_mutex_lock(&queue->lock);
    queue->canceled = false;
    pthread_mutex_unlock(&queue->lock);
}

void event_queue_cancel(event_queue_t *queue)
{
    if (queue == NULL || !queue->initialized)
    {
        return;
    }

    pthread_mutex_lock(&queue->lock);
    queue->canceled = true;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

int event_queue_push(event_queue_t *queue, const void *event)
{
    event_node_t *node;

    if (queue == NULL || !queue->initialized || event == NULL)
    {
        return -EINVAL;
    }

    node = (event_node_t *)bmalloc(sizeof(*node) + queue->event_size);
    if (node == NULL)
    {
        return -ENOMEM;
    }
    INIT_LIST_HEAD(&node->node);
    memcpy(node->event, event, queue->event_size);

    pthread_mutex_lock(&queue->lock);
    list_add_tail(&node->node, &queue->events);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int event_queue_wait(event_queue_t *queue, void *out_event, int timeout_ms)
{
    int rc;

    if (queue == NULL || !queue->initialized || out_event == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&queue->lock);

    rc = event_queue_pop_locked(queue, out_event);
    if (rc != 0)
    {
        pthread_mutex_unlock(&queue->lock);
        return rc;
    }

    if (queue->canceled)
    {
        pthread_mutex_unlock(&queue->lock);
        return -ECANCELED;
    }

    if (timeout_ms <= 0)
    {
        pthread_mutex_unlock(&queue->lock);
        return 0;
    }

    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }

        while (list_empty(&queue->events) && !queue->canceled)
        {
            rc = pthread_cond_timedwait(&queue->cond, &queue->lock, &ts);
            if (rc == ETIMEDOUT)
            {
                pthread_mutex_unlock(&queue->lock);
                return 0;
            }
            if (rc != 0)
            {
                pthread_mutex_unlock(&queue->lock);
                return -rc;
            }
        }
    }

    if (queue->canceled)
    {
        pthread_mutex_unlock(&queue->lock);
        return -ECANCELED;
    }

    rc = event_queue_pop_locked(queue, out_event);
    pthread_mutex_unlock(&queue->lock);
    return rc;
}

void event_queue_clear(event_queue_t *queue)
{
    if (queue == NULL || !queue->initialized)
    {
        return;
    }

    pthread_mutex_lock(&queue->lock);
    event_queue_free_nodes_locked(queue);
    pthread_mutex_unlock(&queue->lock);
}

void event_queue_destroy(event_queue_t *queue)
{
    if (queue == NULL || !queue->initialized)
    {
        return;
    }

    event_queue_cancel(queue);
    event_queue_clear(queue);
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->lock);
    memset(queue, 0, sizeof(*queue));
}
