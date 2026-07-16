#ifndef __MQ_H__
#define __MQ_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <utils/list.h>

#define MQ_MAX_TIMEOUT (-1)
#define MQ_NAME_MAX    (32)

typedef struct mq
{
	struct list_head linked_node;

	char name[MQ_NAME_MAX];
	void *buffer;
	void *queue_head;
	void *queue_tail;
	void *queue_free;
	size_t msg_size;
	uint32_t max_msgs;
	uint32_t max_used;
	uint32_t entry;
	pthread_mutex_t mutex;
	pthread_cond_t cond_write;
	pthread_cond_t cond_read;
} mq_t;

#ifdef __cplusplus
extern "C"
{
#endif

mq_t *mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs);

void mq_destroy(mq_t *q);

int mq_send(mq_t *q, const void *buffer, uint32_t size);

int mq_send_wait(mq_t *q, const void *buffer, uint32_t size, int milliseconds);

int mq_send_urgent(mq_t *q, const void *buffer, uint32_t size);

int mq_send_urgent_wait(mq_t *q, const void *buffer, uint32_t size, int milliseconds);

int mq_recv(mq_t *q, void *buffer, uint32_t size, int milliseconds);

void list_mq();

#ifdef __cplusplus
}
#endif

#endif /* __MQ_H__ */