#include <utils/timer_heap.h>
#include <utils/heap.h>
#include <utils/bmem.h>
#include <utils/time_helper.h>

#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/time.h>

#define IDLE_TIMEOUT_MS	(100)

#ifndef container_of
#define container_of(ptr, type, field)                      \
	((type*) ((char*) (ptr) - offsetof(type, field)))
#endif

static int timer_less_than(const struct heap_node *ha,
                           const struct heap_node *hb)
{
	const timer_entry_t *a;
	const timer_entry_t *b;

	a = container_of(ha, timer_entry_t, node.heap);
	b = container_of(hb, timer_entry_t, node.heap);

	if (a->timeout < b->timeout)
	{
		return 1;
	}

	if (b->timeout < a->timeout)
	{
		return 0;
	}

	/* Compare start_id when both have the same timeout. start_id is
	 * allocated with ht->timer_counter in timer_start().
	 */
	return a->start_id < b->start_id;
}

int timer_init(timer_heap_t *ht, timer_entry_t *handle)
{
	// uv__handle_init(loop, (uv_handle_t *)handle, UV_TIMER);

	memset(handle, 0, sizeof(timer_entry_t));

	handle->ht = ht;

	timer_queue_init(&handle->node.queue);

	return 0;
}

int timer_start(timer_entry_t *handle,
                timer_callback_t cb,
                uint64_t timeout,
                uint64_t repeat)
{
	uint64_t clamped_timeout;

	if (timer_is_closing(handle) || cb == NULL)
	{
		return -1;
	}

	timer_stop(handle);

	clamped_timeout = handle->ht->time + timeout;

	if (clamped_timeout < timeout)
	{
		clamped_timeout = (uint64_t) -1;
	}

	handle->timer_cb = cb;
	handle->timeout = clamped_timeout;
	handle->repeat = repeat;
	/* start_id is the second index to be compared in timer_less_than() */
	handle->start_id = handle->ht->timer_counter++;

	heap_insert(&handle->ht->timer_heap, &handle->node.heap, timer_less_than);
	timer_handle_start(handle);

	return 0;
}

int timer_stop(timer_entry_t *handle)
{
	if (timer_is_active(handle))
	{
		heap_remove(&handle->ht->timer_heap, &handle->node.heap, timer_less_than);
		timer_handle_stop(handle);
	}
	else
	{
		timer_queue_remove(&handle->node.queue);
	}

	timer_queue_init(&handle->node.queue);

	return 0;
}

int timer_again(timer_entry_t *handle)
{
	if (handle->timer_cb == NULL)
	{
		return -1;
	}

	if (handle->repeat)
	{
		timer_stop(handle);
		timer_start(handle, handle->timer_cb, handle->repeat, handle->repeat);
	}

	return 0;
}

void timer_set_repeat(timer_entry_t *handle, uint64_t repeat)
{
	handle->repeat = repeat;
}

uint64_t timer_get_repeat(const timer_entry_t *handle)
{
	return handle->repeat;
}

uint64_t timer_get_due_in(const timer_entry_t *handle)
{
	if (handle->ht->time >= handle->timeout)
	{
		return 0;
	}

	return handle->timeout - handle->ht->time;
}

timer_heap_t *timer_heap_create()
{
	timer_heap_t *ht = bmalloc(sizeof(timer_heap_t));

	if (ht == NULL)
	{
		return NULL;
	}

	memset(ht, 0, sizeof(timer_heap_t));

	// pthread_mutex_init(&ht->mutex, NULL);

	heap_init(&ht->timer_heap);

	ht->time = time_now();

	return ht;
}

void timer_heap_destroy(timer_heap_t *ht)
{
	if (ht)
	{
		bfree(ht);
		ht = NULL;
	}
}

int timer_heap_next_timeout(const timer_heap_t *ht)
{
	const struct heap_node *heap_node;
	const timer_entry_t *handle;
	uint64_t diff;

	heap_node = heap_min(&ht->timer_heap);

	if (heap_node == NULL)
	{
		// return -1;    /* block indefinitely */
		return IDLE_TIMEOUT_MS;
	}

	handle = container_of(heap_node, timer_entry_t, node.heap);

	if (handle->timeout <= ht->time)
	{
		return 0;
	}

	diff = handle->timeout - ht->time;

	if (diff > INT_MAX)
	{
		diff = INT_MAX;
	}

	return (int) diff;
}

void timer_heap_poll(timer_heap_t *ht)
{
	ht->time = time_now();

	struct heap_node *heap_node;
	timer_entry_t *handle;
	struct timer_queue *queue_node;
	struct timer_queue ready_queue;

	timer_queue_init(&ready_queue);

	for (;;)
	{
		heap_node = heap_min(&ht->timer_heap);

		if (heap_node == NULL)
		{
			break;
		}

		handle = container_of(heap_node, timer_entry_t, node.heap);

		if (handle->timeout > ht->time)
		{
			break;
		}

		timer_stop(handle);
		timer_queue_insert_tail(&ready_queue, &handle->node.queue);
	}

	while (!timer_queue_empty(&ready_queue))
	{
		queue_node = timer_queue_head(&ready_queue);
		timer_queue_remove(queue_node);
		timer_queue_init(queue_node);
		handle = container_of(queue_node, timer_entry_t, node.queue);

		timer_again(handle);
		handle->timer_cb(handle);
	}
}

void timer_heap_timer_close(timer_entry_t *handle)
{
	timer_stop(handle);
}