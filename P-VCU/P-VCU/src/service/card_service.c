#include "card_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>

#include <utils/bmem.h>

#include <utils/util.h>
#include <utils/list.h>
#include <utils/mq.h>
#include <utils/time_helper.h>
#include <utils/log.h>
#include <utils/time_helper.h>
#include <hardware_test.h>
#include <goblin_plc_process.h>
#include <sound_play.h>

#include "data/DBICCard.h"
#include "gpio_service.h"
#include "card_service.h"

#define MAX_CARD_EVENT          (32)



typedef struct service_event
{
	card_mode_status_e mode;

	uint32_t param;
	uint32_t param2;
} card_service_event_t;

typedef struct _plc_service_mgnt
{
    //会话链表锁
	pthread_mutex_t msg_mutex;

	//事件队列
	mq_t *event_queue;
	//线程句柄
	pthread_t thread;
	pthread_t thread_read;
	//线程退出标志
    uint8_t isrun;

	//远程操作的设备地址
	uint32_t remote_device_id;
	//上次操作时间点
	unsigned int last_opt_tick;
	//当前模式
	card_mode_status_e cur_mode;
} card_service_mgnt_t;

static card_service_mgnt_t *card_service_mgnt = NULL;

#define MODE 1 //0:poll 1:select

static void *card_read_thread(void *args)
{
	card_service_mgnt_t *mgnt = (card_service_mgnt_t *)args;
	int fd;
	int ret = -1;
	unsigned int  value = 0;
	unsigned int  value_old = 0;
	unsigned int  value_last = 0;
	struct timeval tv;

#if MODE
	fd_set fds;
#else
	struct pollfd fds;
#endif

	fd = open("/dev/idcard", O_RDWR);
	if (fd < 0) {
		LOGW("Open /dev/idcard err!\n");
		return NULL;
	}

#if MODE
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
#else
	fds.fd = fd;
	fds.events = POLLIN;
#endif

	uint32_t lasttick = 0;
	LOGI("card_read_thread start\n");
	while (mgnt->isrun)
	{
#if MODE
		FD_ZERO (&fds);
		FD_SET (fd, &fds);

		tv.tv_sec = 1;			/* Timeout. */
		tv.tv_usec = 0;

		ret = select(fd+1, &fds, NULL, NULL, &tv);
		if(ret == -1){
			LOGW("select error\n");
			break;
		}
		else if(ret == 0){
			//LOGW("select timeout,not detect idcard\n");
			continue;
		}
		else{
			if (FD_ISSET(fd, &fds)) {
				read(fd, &value,4);
				uint32_t tick = time_relative_ms();

				if(value_last != value)
				{
					value_last = value;
					continue;
				}
				value_last = 0;
				if(value != value_old || tick - lasttick >= 2000){
					lasttick = tick;
					LOGI("value = 0x%x value %u\n",value,value);
					//测试模式就直接上报
					if(hardware_test_is_test())
					{
						hardware_test_send(Goblin_PLC_HARDWARW_TEST_CARD, 0, value);
						value_old = value;
						continue;
					}

					//常态下检测读到的卡号
					if(mgnt->cur_mode == CARD_MODE_STATUS_NORMAL)
					{
						CardList_T cardinfo = {0};
						memset(&cardinfo, 0, sizeof(cardinfo));
						if(CheckICCard(value, &cardinfo))
						{
							LOGI("Card ID: %llx, ExpDate: %u\n", cardinfo.idcode, cardinfo.ExpDate);
							if(cardinfo.ExpDate & 0x2){
								gpio_control_Ex(GPIO_LOCK_1 , GPIO_MODE_ON, cardinfo.roomid);
								LOGI("Lock 1\n");
							}

							if(cardinfo.ExpDate & 0x1){
								gpio_control_Ex(GPIO_LOCK_0 , GPIO_MODE_ON, cardinfo.roomid);
								LOGI("Lock 0\n");
							}

						}
						else{
							sound_play_start(BEEP_WAV, 1, 100, false);
						}
					}
					else if (mgnt->cur_mode == CARD_MODE_STATUS_ADD)
					{
						CardList_T cardinfo = {0};
						memset(&cardinfo, 0, sizeof(cardinfo));

						cardinfo.idtype = 1;
						cardinfo.idcode = value;
						cardinfo.roomid = (mgnt->remote_device_id>>8) & 0xffff;
						//默认只打开上电锁1
						cardinfo.ExpDate = 0x1;
						if(AddICCard(&cardinfo))
							card_service_push_event(CARD_MODE_STATUS_READ, value, 0);

						sound_play_start(BEEP_WAV, 2, 100, false);
					}
					else if (mgnt->cur_mode == CARD_MODE_STATUS_DEL)
					{
						card_service_push_event(CARD_MODE_STATUS_READ, value, 0);

						DelICCardByRoomAndID(value, (mgnt->remote_device_id>>8) & 0xffff);

						sound_play_start(BEEP_WAV, 2, 100, false);
					}
				}

				value_old = value;
			}
		}

#else
		ret = poll(&fds, 1, 5000);
		if (ret > 0) {
			if(fds.revents == POLLIN){
				read(fd, &value,1);
				printf("value = 0x%llx\n", value);
			}
		}
		else if(ret == 0) {
			printf("select timeout,not detect idcard .\n");
		}
		else{
				printf("error\n");
				goto end;
		}
#endif
    }

    close(fd);
	LOGI("card_read_thread end\n");
	return NULL;
}

static void *card_process_thread(void *args)
{
	card_service_mgnt_t *mgnt = (card_service_mgnt_t *)args;

	LOGI("card_process_thread start [%p] event_queue:[%p]\n", mgnt, mgnt->event_queue);
	card_service_event_t event = {0};

	while (mgnt->isrun)
	{
		if (0 == mq_recv(mgnt->event_queue, &event, sizeof(card_service_event_t), 100))
		{
            switch (event.mode)
            {
				case CARD_MODE_STATUS_ADD:
					{
						mgnt->cur_mode = event.param==1 ? CARD_MODE_STATUS_ADD : CARD_MODE_STATUS_NORMAL;
						mgnt->last_opt_tick = time_relative_ms();
						mgnt->remote_device_id = event.param2;

						LOGI("Add Card mode: %d, remote_device_id: 0x%x \n", mgnt->cur_mode, mgnt->remote_device_id);
					}
					break;
				case CARD_MODE_STATUS_DEL:
					{
						mgnt->cur_mode = event.param==1 ? CARD_MODE_STATUS_DEL : CARD_MODE_STATUS_NORMAL;
						mgnt->last_opt_tick = time_relative_ms();
						mgnt->remote_device_id = event.param2;

						LOGI("Del Card mode: %d, remote_device_id: 0x%x \n", mgnt->cur_mode, mgnt->remote_device_id);
					}
					break;
				case CARD_MODE_STATUS_DEL_ALL:
					{
						LOGI("card_process_thread recv CARD_MODE_STATUS_DEL_ALL\n");
						ResetICCardDB();
					}
					break;
				case CARD_MODE_STATUS_READ:
					{
						LOGI("Read Card mode: %d, id: 0x%x \n", mgnt->cur_mode, event.param);
						//TODO:读取卡号
						goblin_plc_card_optinfo_s cardoptinfo = {0};
						memset(&cardoptinfo, 0, sizeof(cardoptinfo));
						sprintf(cardoptinfo.strcard, "%X", event.param);
						sprintf(cardoptinfo.stropt, "%s", mgnt->cur_mode==CARD_MODE_STATUS_ADD ? "add" : "del");

						goblin_plc_report_card_opt(mgnt->remote_device_id, &cardoptinfo, 1000);
					}
					break;
				default:
					break;
            }
        }
        else
        {
            //LOGI("card_process_thread recv timeout\n"); //超时处理
			//当前模式为非正常模式，且30秒没有操作，则切换到正常模式
			if(mgnt->cur_mode != CARD_MODE_STATUS_NORMAL && time_relative_ms() - mgnt->last_opt_tick >= 1000*60*3)
			{
				LOGI("card_process_thread recv timeout\n");
				mgnt->cur_mode = CARD_MODE_STATUS_NORMAL;
			}
        }
    }

    return NULL;
}

int card_service_init(void)
{
	int ret = -1;

    if (card_service_mgnt == NULL)
	{
		card_service_mgnt = bmalloc(sizeof(card_service_mgnt_t));

		if (card_service_mgnt == NULL)
		{
			LOGE("malloc plc_service_mgnt_t fail \n");
			return -1;
		}

		memset(card_service_mgnt, 0, sizeof(card_service_mgnt_t));
        //1.
		pthread_mutex_init(&card_service_mgnt->msg_mutex, NULL);

        //2.
		card_service_mgnt->event_queue = mq_create("card_event_queue", sizeof(card_service_event_t), MAX_CARD_EVENT);
        //3.
        card_service_mgnt->isrun = 1;
		pthread_create(&card_service_mgnt->thread_read, NULL, card_read_thread, card_service_mgnt);
		if(pthread_create(&card_service_mgnt->thread, NULL, card_process_thread, card_service_mgnt))
        {
            card_service_mgnt->isrun = 0;

            mq_destroy(card_service_mgnt->event_queue);
            pthread_mutex_destroy(&card_service_mgnt->msg_mutex);
            bfree(card_service_mgnt);
            card_service_mgnt = NULL;
            LOGE("pthread_create fail \n");
            ret = -1;
        }
        else
		    ret = 0;
	}
    return ret;
}


int card_service_deinit(void)
{
    int ret = -1;

    if (card_service_mgnt)
    {
        //1.
        if(card_service_mgnt->isrun){
            card_service_mgnt->isrun = 0;
            pthread_join(card_service_mgnt->thread, NULL);
			pthread_join(card_service_mgnt->thread_read, NULL);
        }

        //2.
        mq_destroy(card_service_mgnt->event_queue);
        //3.
		pthread_mutex_destroy(&card_service_mgnt->msg_mutex);
        //4.
        bfree(card_service_mgnt);
        card_service_mgnt = NULL;
        ret = 0;
    }

    return ret;
}

int card_service_push_event(card_mode_status_e mode, uint32_t param, uint32_t param2)
{
	if (card_service_mgnt && card_service_mgnt->event_queue)
	{
		card_service_event_t event = {0};

		event.mode = mode;
		event.param = param;
		event.param2 = param2;

		return mq_send(card_service_mgnt->event_queue, &event, sizeof(card_service_event_t));
	}

	return -1;
}



