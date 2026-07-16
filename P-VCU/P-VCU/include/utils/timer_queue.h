/* Copyright (c) 2013, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __TIMER_QUEUE_H__
#define __TIMER_QUEUE_H__

#include <stddef.h>

struct timer_queue
{
	struct timer_queue *next;
	struct timer_queue *prev;
};

#define timer_queue_data(pointer, type, field)                                  \
	((type*) ((char*) (pointer) - offsetof(type, field)))

#define timer_queue_foreach(q, h)                                               \
	for ((q) = (h)->next; (q) != (h); (q) = (q)->next)

static inline void timer_queue_init(struct timer_queue *q)
{
	q->next = q;
	q->prev = q;
}

static inline int timer_queue_empty(const struct timer_queue *q)
{
	return q == q->next;
}

static inline struct timer_queue *timer_queue_head(const struct timer_queue *q)
{
	return q->next;
}

static inline struct timer_queue *timer_queue_next(const struct timer_queue *q)
{
	return q->next;
}

static inline void timer_queue_add(struct timer_queue *h, struct timer_queue *n)
{
	h->prev->next = n->next;
	n->next->prev = h->prev;
	h->prev = n->prev;
	h->prev->next = h;
}

static inline void timer_queue_split(struct timer_queue *h,
                                     struct timer_queue *q,
                                     struct timer_queue *n)
{
	n->prev = h->prev;
	n->prev->next = n;
	n->next = q;
	h->prev = q->prev;
	h->prev->next = h;
	q->prev = n;
}

static inline void timer_queue_move(struct timer_queue *h, struct timer_queue *n)
{
	if (timer_queue_empty(h))
	{
		timer_queue_init(n);
	}
	else
	{
		timer_queue_split(h, h->next, n);
	}
}

static inline void timer_queue_insert_head(struct timer_queue *h,
        struct timer_queue *q)
{
	q->next = h->next;
	q->prev = h;
	q->next->prev = q;
	h->next = q;
}

static inline void timer_queue_insert_tail(struct timer_queue *h,
        struct timer_queue *q)
{
	q->next = h;
	q->prev = h->prev;
	q->prev->next = q;
	h->prev = q;
}

static inline void timer_queue_remove(struct timer_queue *q)
{
	q->prev->next = q->next;
	q->next->prev = q->prev;
}

#endif /* __TIMER_QUEUE_H__ */
