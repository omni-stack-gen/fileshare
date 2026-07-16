#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <utils/bmem.h>

#include <utils/util.h>
#include <utils/list.h>
#include <utils/mq.h>
#include <utils/time_helper.h>
#include <utils/log.h>
#include "hardware_test.h"
#include "goblin_plc_process.h"
#include <gpio_service.h>
#include "sound_play.h"
#include "hard_node_service.h"
#include "plc_service.h"
#include "dpbase.h"

#define MAX_TEST_EVENT          (32)

void *Audio_test_Play_start(void);
void Audio_test_Play_stop(void*pmgnt);
void Audio_test_Record_stop(void);
void Audio_test_Record_start(void);


typedef uint8_t test_event_type;

typedef struct hardwaretest_event
{
	test_event_type type;

	union
	{
		uint32_t remoteid;
	};
} hardware_service_event_t;

typedef struct _plc_service_mgnt
{
    //会话链表锁
	pthread_mutex_t msg_mutex;

	//事件队列
	mq_t *event_queue;
	//线程句柄
	pthread_t thread;
	//线程退出标志
    uint8_t isrun;

	//是否是测试模式
	bool is_test;
	//测试模式下，测试的远程设备ID
	uint32_t test_remoteid;

	void *pplay_mgnt;
	int splay_id ;

	bool is_camer_led; // 是否是摄像头灯
	uint32_t led_last_tick;

	//麦克风是否录制
	bool is_mic_record;
	uint32_t mic_last_tick;
} hard_test_service_mgnt_t;

static hard_test_service_mgnt_t *hardtest_service_mgnt = NULL;

bool hardware_test_is_test(void)
{
	if (hardtest_service_mgnt)
	{
		return hardtest_service_mgnt->is_test;
	}
	return false;
}

int hardware_test_send(int cmd, int index, unsigned int state)
{
	int ret = -1;
	golbin_hardware_test_s event = {0};
	if (hardtest_service_mgnt)
	{
		event.operate_cmd = cmd;
		event.index = index;
		if(state != -1){
			if(Goblin_PLC_HARDWARW_TEST_CARD == cmd){
				//卡号按16进制格式显示
				snprintf(event.strstate, sizeof(event.strstate), "%X", state);
			}
			else{
				snprintf(event.strstate, sizeof(event.strstate), "%d", state);
			}
		}

		ret = goblin_plc_hardware_test(hardtest_service_mgnt->test_remoteid, &event, 1000);
	}

	return ret;
}

static void *hardware_test_process_thread(void *args)
{
	hard_test_service_mgnt_t *mgnt = (hard_test_service_mgnt_t *)args;

	LOGI("hardware_test_process_thread start [%p] event_queue:[%p]\n", mgnt, mgnt->event_queue);
	hardware_service_event_t event = {0};

	while (mgnt->isrun)
	{
		if (0 == mq_recv(mgnt->event_queue, &event, sizeof(hardware_service_event_t), 100))
		{
			LOGI("hardware_test_process_thread recv event type:%d\n", event.type);
            switch (event.type)
            {
            case Goblin_PLC_HARDWARW_TEST_START:
				mgnt->is_test = true;
				mgnt->test_remoteid = event.remoteid;
				LOGI("hardware_test_process_thread start, remote id:%x \n", event.remoteid);
				gpio_control(GPIO_TEST_TYPE, GPIO_MODE_ON);
                break;
            case Goblin_PLC_HARDWARW_TEST_END:
				mgnt->is_test = false;
				gpio_control(GPIO_TEST_TYPE, GPIO_MODE_OFF);
				LOGI("hardware_test_process_thread end \n");
                break;
            case Goblin_PLC_HARDWARW_TEST_SPEAKER_START:
                {
					if(mgnt->splay_id==0)
						mgnt->splay_id = sound_play_start(RING_MP3, PLAY_FOREVER, 0xffffff, false);
				}
                break;
			case Goblin_PLC_HARDWARW_TEST_SPEAKER_END:
				{
					if(mgnt->splay_id)
						sound_play_stop(mgnt->splay_id);
					mgnt->splay_id = 0;
				}
				break;
			case Goblin_PLC_HARDWARW_TEST_RECORD_START:
				{
					Audio_test_Record_start();
					mgnt->is_mic_record = true;
					mgnt->mic_last_tick = time_relative_ms();
				}
				break;
			case Goblin_PLC_HARDWARW_TEST_RECORD_END:
				{
					Audio_test_Record_stop();
					mgnt->is_mic_record = false;
				}
				break;
			case Goblin_PLC_HARDWARW_TEST_PLAY_START:
				{
					if(!mgnt->pplay_mgnt)
						mgnt->pplay_mgnt = Audio_test_Play_start();
				}
				break;
			case Goblin_PLC_HARDWARW_TEST_PLAY_END:
				{
					if(mgnt->pplay_mgnt)
						Audio_test_Play_stop(mgnt->pplay_mgnt);
					mgnt->pplay_mgnt = NULL;
				}
				break;
			case Goblin_PLC_HARDWARW_TEST_LIGHT_START:
				{
					LOGI("node_process_thread test\n");
					hard_node_mgnt_control(NODE_KEY1_LED, NODE_STATE_ON);
					hard_node_mgnt_control(NODE_KEY2_LED, NODE_STATE_ON);
					hard_node_mgnt_control(NODE_KEY3_LED, NODE_STATE_ON);
					hard_node_mgnt_control(NODE_KEY4_LED, NODE_STATE_ON);
					hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_ON);
					hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);

					gpio_control(GPIO_LED_EN, GPIO_MODE_ON);
					SetCameraLight(10);
					mgnt->is_camer_led = true;
					mgnt->led_last_tick = time_relative_ms();
				}
				break;
			case 99:
				{
					void Md5_File(char *pfilename, char *pOutBuf);
					file_pack_t filedata = {0};
					int seq = 1;
					//filedata.fileseq = seq++;
					char strfilename[128] = PLC_RUN_LOG_BK_PATH;
					FILE *fd = NULL;
					if (access(strfilename, F_OK) == 0 && (fd = fopen(strfilename, "rb")))
					{
						int len;
						char *pdata = NULL;

						fseek(fd, 0, SEEK_END);
						len = ftell(fd);
						fseek(fd, 0, SEEK_SET);

						pdata = (char *)malloc(len);
						fread(pdata, 1, len, fd);
						fclose(fd);

						char strmd5[64] = {0};
						Md5_File(strfilename, strmd5);
						printf("============file len:%d md5: %s\n", len, strmd5);

						filedata.file_type = 3;
						memcpy(filedata.filemd5, strmd5, 32);
						filedata.filelen = len;

						int filereslen = len;
						int sendlen = 0;
						int sendfilelen = 0;
						int ret = 0;

						uint32_t ltick = time_relative_ms();
						do
						{
							filedata.fileflag = 0;
							if(seq==1)
								filedata.fileflag |= FILE_START_FLAG;
							if(filereslen > FILE_PACK_SIZE)
							{
								sendlen = FILE_PACK_SIZE;
								filereslen -= FILE_PACK_SIZE;
							}
							else{
								sendlen = filereslen;
								filereslen = 0;
								filedata.fileflag |= FILE_END_FLAG;
							}
							filedata.fileseq = seq++;
							filedata.datalen = sendlen;
							memcpy(filedata.data, pdata+sendfilelen, sendlen);

							ltick = time_relative_ms();
							ret = goblin_plc_send_file_data(event.remoteid, &filedata, 1000);
							printf("send log file data ret:%d seq:%d len:%d lost time:%u\n 0x%02x 0x%02x\n", ret, filedata.fileseq, sendlen, time_relative_ms()-ltick,
												filedata.data[sendlen-2], filedata.data[sendlen-1]);
							sendfilelen+=sendlen;
						} while (filereslen);

						free(pdata);
						DPDeleteFile(strfilename);
					}
				}
				break;
            default:
                break;
            }
        }
		else
		{
			uint32_t tp_now = time_relative_ms();
			if (mgnt->is_camer_led && tp_now-mgnt->led_last_tick > 1000)
			{
				mgnt->is_camer_led = false;
				gpio_control(GPIO_LED_EN, GPIO_MODE_OFF);
				SetCameraLight(0);
				hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_OFF);
			}

			//录音十秒
			if(mgnt->is_mic_record && tp_now-mgnt->mic_last_tick > 10*1000)
			{
				mgnt->is_mic_record = false;
				Audio_test_Record_stop();
				LOGW("Mic Record time out \n");
			}
		}
    }

    return NULL;
}

int hareware_test_service_init(void)
{
	int ret = -1;

    if (hardtest_service_mgnt == NULL)
	{
		hardtest_service_mgnt = bmalloc(sizeof(hard_test_service_mgnt_t));

		if (hardtest_service_mgnt == NULL)
		{
			LOGE("malloc plc_service_mgnt_t fail \n");
			return -1;
		}

		memset(hardtest_service_mgnt, 0, sizeof(hard_test_service_mgnt_t));
        //1.
		pthread_mutex_init(&hardtest_service_mgnt->msg_mutex, NULL);

        //2.
		hardtest_service_mgnt->event_queue = mq_create("hardware_test_event_queue", sizeof(hardware_service_event_t), MAX_TEST_EVENT);
        //3.
        hardtest_service_mgnt->isrun = 1;
		pthread_create(&hardtest_service_mgnt->thread, NULL, hardware_test_process_thread, hardtest_service_mgnt);

		ret = 0;
	}
    return ret;
}


int hareware_test_service_deinit(void)
{
    int ret = -1;

    if (hardtest_service_mgnt)
    {
        //1.
        hardtest_service_mgnt->isrun = 0;
        pthread_join(hardtest_service_mgnt->thread, NULL);
        //2.
        mq_destroy(hardtest_service_mgnt->event_queue);
        //3.
		pthread_mutex_destroy(&hardtest_service_mgnt->msg_mutex);
		bfree(hardtest_service_mgnt);
        hardtest_service_mgnt = NULL;
        ret = 0;
    }

    return ret;
}

int hareware_test_service_push_event(uint8_t typecode, uint32_t param)
{
	if (hardtest_service_mgnt && hardtest_service_mgnt->event_queue)
	{
		hardware_service_event_t event = {0};

		event.type = typecode;
		event.remoteid = param;

		return mq_send(hardtest_service_mgnt->event_queue, &event, sizeof(hardware_service_event_t));
	}

	return -1;
}