#include <spi_plc/spi_plc_mgnt.h>
#include <spi_plc/spi_plc_trans.h>

#include <utils/bmem.h>
#include <utils/util.h>
#include <utils/list.h>
#include <utils/time_helper.h>
#define LOG_TAG "spi_plc_mgnt"
#include <utils/log.h>
#include "DPDef.h"

typedef struct spi_plc_mgnt
{
	void *trans;
	int log_level;

	bool   send_thread_run;
	pthread_t send_thread;
	sem_t  send_sem;				//spi发送信号
	pthread_mutex_t  send_mutex;
	struct list_head send_list;
} spi_plc_mgnt_t;

typedef struct spi_plc_send_event
{
	struct list_head node;
	uint32_t dev;
	uint8_t port;
	uint32_t datalen;
	uint8_t *pdata;
}spi_plc_send_event_t;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_ON() pthread_mutex_lock(&mutex)
#define LOCK_OFF() pthread_mutex_unlock(&mutex)

static spi_plc_mgnt_t *g_spi_plc_mgnt = NULL;

static void *spi_send_thread(void *args)
{
	spi_plc_mgnt_t *pmgnt = (spi_plc_mgnt_t *)args;

	LOGI("spi_send_thread start [%p]\n", pmgnt);
	while (pmgnt->send_thread_run)
	{
		sem_wait(&pmgnt->send_sem);

		spi_plc_send_event_t *p = NULL;
		spi_plc_send_event_t *n = NULL;
		pthread_mutex_lock(&pmgnt->send_mutex);
		if (!list_empty_careful(&pmgnt->send_list))
		{
			list_for_each_entry_safe(p, n, &pmgnt->send_list, node)
			{
				list_del(&p->node);
				break;
			}
		}
		pthread_mutex_unlock(&pmgnt->send_mutex);
		if(p)
		{
			//static uint32_t lastt = 0;
			//uint32_t ltick = time_relative_ms();
			//printf("one frame lost time:%d \n", ltick-lastt);

			spi_plc_trans_send(pmgnt->trans, p->dev, p->port, p->pdata, p->datalen);
			// printf("send one frame lost time:%d \n", time_relative_ms()-ltick);
			// lastt = ltick;
			bfree(p);
		}

		//usleep(1000);
	}
	return NULL;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_init(int level, unsigned int dev_num)
{
	int ret = -1;

	LOCK_ON();

	if (g_spi_plc_mgnt == NULL)
	{
		g_spi_plc_mgnt = bmalloc(sizeof(spi_plc_mgnt_t));

		if (g_spi_plc_mgnt == NULL)
		{
			LOGE("malloc spi_plc_trans_mgnt_t failed \n");
			goto __exit;
		}

		g_spi_plc_mgnt->log_level = level;

		g_spi_plc_mgnt->trans = spi_plc_trans_create(level, dev_num);

		if (g_spi_plc_mgnt->trans == NULL)
		{
			LOGE("spi plc trans create failed \n");

			bfree(g_spi_plc_mgnt);
			g_spi_plc_mgnt = NULL;
			goto __exit;
		}

		INIT_LIST_HEAD(&g_spi_plc_mgnt->send_list);
		pthread_mutex_init(&g_spi_plc_mgnt->send_mutex, NULL);
		sem_init(&g_spi_plc_mgnt->send_sem, 0, 0); //无名信号量

		g_spi_plc_mgnt->send_thread_run = true;
		ret = pthread_create(&g_spi_plc_mgnt->send_thread, NULL, spi_send_thread, g_spi_plc_mgnt);
		if (ret != 0)
		{
			g_spi_plc_mgnt->send_thread_run = false;
			LOGE("pthread_create error:%d \n", ret);
			goto __exit;
		}

		ret = 0;
	}

__exit:

	LOCK_OFF();

	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_deinit()
{
	LOCK_ON();

	if (g_spi_plc_mgnt)
	{
		if (g_spi_plc_mgnt->trans)
		{
			spi_plc_trans_destroy(&g_spi_plc_mgnt->trans);
		}

		if (g_spi_plc_mgnt->send_thread)
		{
			g_spi_plc_mgnt->send_thread_run = false;
			sem_post(&g_spi_plc_mgnt->send_sem);
			pthread_join(g_spi_plc_mgnt->send_thread, NULL);
			g_spi_plc_mgnt->send_thread = 0;
			sem_destroy(&g_spi_plc_mgnt->send_sem);
		}

		spi_plc_send_event_t *node = NULL, *next = NULL;
		pthread_mutex_lock(&g_spi_plc_mgnt->send_mutex);
		list_for_each_entry_safe(node, next, &g_spi_plc_mgnt->send_list, node)
		{
			list_del(&node->node);
			bfree(node);
		}
		pthread_mutex_unlock(&g_spi_plc_mgnt->send_mutex);

		pthread_mutex_destroy(&g_spi_plc_mgnt->send_mutex);

		bfree(g_spi_plc_mgnt);
		g_spi_plc_mgnt = NULL;
	}

	LOCK_OFF();

	return 0;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_register_callback(uint32_t port,
                                   void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t len),
                                   void *param)
{
	int ret = -1;

	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_register(g_spi_plc_mgnt->trans, port, 0, callback, param);

fail:

	LOCK_OFF();

	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_register_callbackEx(uint32_t port, uint32_t remotedev,
                                   void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t len),
                                   void *param)
{
	int ret = -1;

	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_register(g_spi_plc_mgnt->trans, port, remotedev, callback, param);

fail:

	LOCK_OFF();

	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_unregister_callback(uint8_t port)
{
	int ret = -1;

	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_unregister(g_spi_plc_mgnt->trans, port, 0);

fail:

	LOCK_OFF();

	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_unregister_callbackEx(uint8_t port, uint32_t remotedev)
{
	int ret = -1;

	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_unregister(g_spi_plc_mgnt->trans, port, remotedev);

fail:

	LOCK_OFF();

	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_audio_tran(uint32_t dev, uint32_t param)
{
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	spi_plc_set_audio_tran(g_spi_plc_mgnt->trans, dev, param);

fail:

	LOCK_OFF();
	return 0;
}

int spi_plc_mgnt_retset_localdevid(uint32_t dev)
{
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	spi_plc_retset_localId(g_spi_plc_mgnt->trans, dev);

fail:

	LOCK_OFF();
	return 0;
}

int spi_plc_mgnt_off(void)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_off(g_spi_plc_mgnt->trans);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_open_dev(uint32_t devid)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_open_dev(g_spi_plc_mgnt->trans, devid);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_get_rssi(void)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_trans_get_rssi(g_spi_plc_mgnt->trans);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_upgrade(char *pfilepath)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_upgrade(g_spi_plc_mgnt->trans, pfilepath);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_upgrade_by_fd(FILE *pfd, uint32_t file_size)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_upgrade_by_fd(g_spi_plc_mgnt->trans, pfd, file_size);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_reboot(void)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	ret = spi_plc_reboot(g_spi_plc_mgnt->trans);
fail:

	LOCK_OFF();
	return ret;
}

int spi_plc_mgnt_multicast_ctrl(uint32_t multicastaddr, uint8_t isremove)
{
	int ret = -1;
	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");

	//void *handle, uint32_t multicastaddr, uint8_t isremove
	ret = spi_plc_multicast_ctrl(g_spi_plc_mgnt->trans, multicastaddr, isremove);
fail:

	LOCK_OFF();
	return ret;
}

DPLAN_PUBLIC_API int spi_plc_mgnt_send(uint32_t dev, uint8_t port, uint8_t *data, uint16_t len)
{
	int ret = -1;

	LOCK_ON();

	CHECK_VAL_GOTO_IF_FAIL(g_spi_plc_mgnt && g_spi_plc_mgnt->trans, "invalid handle");
#if 0
	ret = spi_plc_trans_send(g_spi_plc_mgnt->trans, dev, port, data, len);
#else
	spi_plc_send_event_t *pevent = (spi_plc_send_event_t*)bmalloc(sizeof(spi_plc_send_event_t)+len+1);
	pevent->dev = dev;
	pevent->port = port;
	pevent->datalen = len;
	if(pevent->datalen > 0)
	{
		pevent->pdata = (uint8_t*)(pevent+1);//bmalloc(pevent->datalen+1);
		memset(pevent->pdata, 0, pevent->datalen+1);
		memcpy(pevent->pdata, data, pevent->datalen);
	}

	pthread_mutex_lock(&g_spi_plc_mgnt->send_mutex);
	list_add_tail(&pevent->node, &g_spi_plc_mgnt->send_list);
	pthread_mutex_unlock(&g_spi_plc_mgnt->send_mutex);

	sem_post(&g_spi_plc_mgnt->send_sem);
	ret = 0;
#endif
fail:

	LOCK_OFF();

	return ret;
}