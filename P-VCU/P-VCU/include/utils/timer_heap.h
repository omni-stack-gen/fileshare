#ifndef __TIMER_HEAP_H__
#define __TIMER_HEAP_H__

#include <common_defs.h>

#include <utils/heap.h>
#include <utils/timer_queue.h>

enum
{
	TIMER_ACTIVE  = 1,
	TIMER_CLOSING = 2,
	TIMER_CLOSED  = 4,
};

#define timer_is_active(h) \
	(((h)->flags & TIMER_ACTIVE) != 0)

#define timer_is_closing(h) \
	(((h)->flags & (TIMER_CLOSING | TIMER_CLOSED)) != 0)

#define timer_handle_start(h)                               \
	do {                                                    \
		assert(((h)->flags & TIMER_CLOSING) == 0);          \
		if (((h)->flags & TIMER_ACTIVE) != 0) break;        \
		(h)->flags |= TIMER_ACTIVE;                         \
	}                                                       \
	while (0)

#define timer_handle_stop(h)                                \
	do {                                                    \
		assert(((h)->flags & TIMER_CLOSING) == 0);          \
		if (((h)->flags & TIMER_ACTIVE) == 0) break;        \
		(h)->flags &= ~TIMER_ACTIVE;                        \
	}                                                       \
	while (0)

struct timer_entry;
struct timer_heap;

typedef void (*timer_callback_t)(struct timer_entry *handle);

typedef struct timer_entry
{
	struct timer_heap *ht;
	unsigned int flags;

	timer_callback_t timer_cb;

	union
	{
		struct heap_node heap;
		struct timer_queue queue;
	} node;

	uint64_t timeout;
	uint64_t repeat;
	uint64_t start_id;
} timer_entry_t;

typedef struct timer_heap
{
	struct heap timer_heap;
	uint64_t timer_counter;
	uint64_t time;

	pthread_mutex_t mutex;
} timer_heap_t;

int timer_init(timer_heap_t *ht, timer_entry_t *handle);
int timer_start(timer_entry_t *handle,
                timer_callback_t cb,
                uint64_t timeout,
                uint64_t repeat);

int timer_stop(timer_entry_t *handle);
int timer_again(timer_entry_t *handle);
void timer_set_repeat(timer_entry_t *handle, uint64_t repeat);
uint64_t timer_get_repeat(const timer_entry_t *handle);
uint64_t timer_get_due_in(const timer_entry_t *handle);

timer_heap_t *timer_heap_create();
void timer_heap_destroy(timer_heap_t *ht);

int timer_heap_next_timeout(const timer_heap_t *ht);
void timer_heap_poll(timer_heap_t *ht);
void timer_heap_timer_close(timer_entry_t *handle);

#endif /* __TIMER_HEAP_H__ */

