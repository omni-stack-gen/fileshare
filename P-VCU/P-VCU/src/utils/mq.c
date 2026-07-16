#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <utils/util.h>
#include <utils/mq.h>
#include <utils/bmem.h>

#define LOG_TAG "mq"
#include <utils/log.h>

struct mq_message
{
	struct mq_message *next;
};

static pthread_mutex_t g_mq_list_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list_head g_mq_list = LIST_HEAD_INIT(g_mq_list);

static void add_ms_to_ts(struct timespec *ts, int milliseconds)
{
	ts->tv_sec  += milliseconds / 1000;
	ts->tv_nsec += (milliseconds % 1000) * 1000000;

	if (ts->tv_nsec > 1000000000)
	{
		ts->tv_sec += 1;
		ts->tv_nsec -= 1000000000;
	}
}

mq_t *mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs)
{
	uint32_t i = 0;

	uint32_t total_size = 0;

	uint32_t align_msg_size = 0;

	struct mq_message *head = NULL;

	mq_t *queue = NULL;

#if 1
	// 需要四字节对齐？
	align_msg_size = ALIGN_UP(msg_size, 4);
	total_size = sizeof(mq_t) + ((sizeof(struct mq_message) + align_msg_size) * max_msgs);
#else
	align_msg_size = msg_size;
	total_size = sizeof(mq_t) + ((sizeof(struct mq_message) + msg_size) * max_msgs);
#endif

	queue = (mq_t *)bmalloc(total_size);

	if (!queue)
	{
		return NULL;
	}

	memset(queue, 0, total_size);

	INIT_LIST_HEAD(&queue->linked_node);

	strncpy(queue->name, name, MQ_NAME_MAX-1);

	pthread_mutex_init(&queue->mutex, NULL);

	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	pthread_cond_init(&queue->cond_write, &cond_attr);
	pthread_cond_init(&queue->cond_read, &cond_attr);
	pthread_condattr_destroy(&cond_attr);

	queue->max_msgs = max_msgs;
	queue->msg_size = align_msg_size;
	queue->buffer = (void *)(queue + 1);

	for (i = 0; i < queue->max_msgs; i++)
	{
		head = (struct mq_message *)((uint8_t *)queue->buffer + i * (queue->msg_size + sizeof(struct mq_message)));

		head->next = (struct mq_message *)queue->queue_free;

		queue->queue_free = head;
	}

	pthread_mutex_lock(&g_mq_list_lock);

	list_add_tail(&queue->linked_node, &g_mq_list);

	pthread_mutex_unlock(&g_mq_list_lock);

	return queue;
}

void mq_destroy(mq_t *q)
{
	if (q)
	{
		pthread_cond_destroy(&q->cond_write);
		pthread_cond_destroy(&q->cond_read);

		pthread_mutex_destroy(&q->mutex);

		pthread_mutex_lock(&g_mq_list_lock);

		list_del(&q->linked_node);

		pthread_mutex_unlock(&g_mq_list_lock);

		bfree(q);
	}
}

int mq_send_wait(mq_t *q, const void *buffer, uint32_t size, int milliseconds)
{
	int ret = 0;

	bool signalled = false;

	struct mq_message *msg = NULL;

	if (!q || !buffer || !size)
	{
		LOGE("Invalid param %p %p %d\n", q, buffer, size);
		return -1;
	}

	if (size > q->msg_size)
	{
		LOGE("[%s] Invalid size %d %d\n", q->name, size, q->msg_size);
		return -1;
	}

	pthread_mutex_lock(&q->mutex);

	// 使用循环避免伪唤醒
	while (q->entry == q->max_msgs)
	{
		if (milliseconds != MQ_MAX_TIMEOUT)
		{
			struct timespec ts;

			clock_gettime(CLOCK_MONOTONIC, &ts);

			add_ms_to_ts(&ts, milliseconds);

			ret = pthread_cond_timedwait(&q->cond_write, &q->mutex, &ts);
		}
		else
		{
			ret = pthread_cond_wait(&q->cond_write, &q->mutex);
		}

		if (ret != 0)
		{
			LOGE("mq [%s] send error errno:%d %s\n", q->name, ret, strerror(ret));
			pthread_mutex_unlock(&q->mutex);
			return ret;
		}
	}

	msg = (struct mq_message *)q->queue_free;

	q->queue_free = msg->next;

	msg->next = NULL;

	memcpy(msg + 1, buffer, size);

	if (q->queue_tail != NULL)
	{
		((struct mq_message *)q->queue_tail)->next = msg;
	}

	q->queue_tail = msg;

	if (q->queue_head == NULL)
	{
		q->queue_head = msg;
	}

	if (q->entry == 0)
	{
		signalled = true;
	}

	q->entry++;

	if (signalled)
	{
		pthread_cond_signal(&q->cond_read);
	}

	pthread_mutex_unlock(&q->mutex);

	return ret;
}

int mq_send(mq_t *q, const void *buffer, uint32_t size)
{
	return mq_send_wait(q, buffer, size, 0);
}

int mq_send_urgent_wait(mq_t *q, const void *buffer, uint32_t size, int milliseconds)
{
	int ret = 0;

	bool signalled = false;

	struct mq_message *msg = NULL;

	if (!q || !buffer || !size)
	{
		LOGE("Invalid param %p %p %d\n", q, buffer, size);
		return -1;
	}

	if (size > q->msg_size)
	{
		LOGE("mq [%s] Invalid size %d %d\n", q->name, size, q->msg_size);
		return -1;
	}

	pthread_mutex_lock(&q->mutex);

	if (q->entry == q->max_msgs)
	{
		if (milliseconds != MQ_MAX_TIMEOUT)
		{
			struct timespec ts;

			clock_gettime(CLOCK_MONOTONIC, &ts);

			add_ms_to_ts(&ts, milliseconds);

			ret = pthread_cond_timedwait(&q->cond_write, &q->mutex, &ts);
		}
		else
		{
			ret = pthread_cond_wait(&q->cond_write, &q->mutex);
		}
	}

	if (ret == 0)
	{
		msg = (struct mq_message *)q->queue_free;

		q->queue_free = msg->next;

		memcpy(msg + 1, buffer, size);

		msg->next = (struct mq_message *)q->queue_head;

		q->queue_head = msg;

		if (q->queue_tail == NULL)
		{
			q->queue_tail = msg;
		}

		if (q->entry == 0)
		{
			signalled = true;
		}

		q->entry++;

		if (signalled)
		{
			pthread_cond_signal(&q->cond_read);
		}
	}
	else
	{
		LOGE("mq [%s] overflow errno:%d %s\n", q->name, ret, strerror(ret));
	}

	pthread_mutex_unlock(&q->mutex);

	return ret;
}

int mq_send_urgent(mq_t *q, const void *buffer, uint32_t size)
{
	return mq_send_urgent_wait(q, buffer, size, 0);
}

int mq_recv(mq_t *q, void *buffer, uint32_t size, int milliseconds)
{
	int ret = 0;

	bool signalled = false;

	struct mq_message *msg = NULL;

	if (!q || !buffer || !size)
	{
		LOGE("Invalid param %p %p %d\n", q, buffer, size);
		return -1;
	}

	pthread_mutex_lock(&q->mutex);

	if (q->entry == 0)
	{
		if (milliseconds != MQ_MAX_TIMEOUT)
		{
			struct timespec ts;

			clock_gettime(CLOCK_MONOTONIC, &ts);

			add_ms_to_ts(&ts, milliseconds);

			ret = pthread_cond_timedwait(&q->cond_read, &q->mutex, &ts);
		}
		else
		{
			ret = pthread_cond_wait(&q->cond_read, &q->mutex);
		}
	}

	if (ret == 0)
	{
		msg = (struct mq_message *)q->queue_head;

		q->queue_head = msg->next;

		if (q->queue_tail == msg)
		{
			q->queue_tail = NULL;
		}

		memcpy(buffer, msg + 1, q->msg_size);

		msg->next = (struct mq_message *)q->queue_free;

		q->queue_free = msg;

		if (q->entry == q->max_msgs)
		{
			signalled = true;
		}

		if (q->entry > q->max_used)
		{
			q->max_used = q->entry;
		}

		q->entry--;

		if (signalled)
		{
			pthread_cond_signal(&q->cond_write);
		}
	}

	pthread_mutex_unlock(&q->mutex);

	return ret;
}

void list_mq()
{
	mq_t *node = NULL, *next = NULL;

	int maxlen = MQ_NAME_MAX;

	pthread_mutex_lock(&g_mq_list_lock);

	list_for_each_entry_safe(node, next, &g_mq_list, linked_node)
	{
		LOGI("%-*.*s entry [%d] max [%d] max_used [%d]\n", maxlen, MQ_NAME_MAX, node->name, node->entry, node->max_msgs,
		     node->max_used);
	}

	pthread_mutex_unlock(&g_mq_list_lock);
}