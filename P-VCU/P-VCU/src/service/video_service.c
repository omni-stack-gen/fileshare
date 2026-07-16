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
#include <service/gpio_service.h>
#include <service/hard_node_service.h>
#include "video_service.h"
#ifdef ENABLE_V6
#include "vlcview.h"
#endif
#define MAX_VIDEO_EVENT          (32)



typedef uint8_t video_event_type;

typedef struct service_event
{
	video_event_type type;

	uint32_t param;
} video_service_event_t;

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

	uint8_t is_day_night_changed;
	//daynight status 0:day 1:night
 	uint32_t DAYNIGHT_status;

	bool  b_day_night_changed;

	//按键补光灯状态 0:白天 1:黑夜
	uint32_t m_keylight_change_status ;

	//视频编码参数是否改变了， 改变后要恢复回来
	bool  b_enc_param_changed;

	uint8_t cur_enc_level; //读取编码等级， 0为默认1080P
} video_service_mgnt_t;

static video_service_mgnt_t *video_service_mgnt = NULL;

void ReLoadDayNightParam(unsigned int isDay);
void ReLoadEncParam(unsigned int width, unsigned int height, unsigned int bit_rate, unsigned int frame_count);
void RequestCropFrame(unsigned int iscrop);
void set_video_ai_work_flag(bool flag);

static bool bchange_night_flag = false;

int get_day_night_status(void)
{
	if(!video_service_mgnt)
		return 0;
	return video_service_mgnt->m_keylight_change_status;
}

static void ReloadVideoEnc(void)
{
	return;
	static uint32_t level_change_count = 0;
	uint32_t ltick = time_relative_ms();
#if 1
	//重新加载视频编码
	Isp_run_type(0);
	usleep(1000*50);
	Isp_run_type(1);
#else
	_vlcview_exit();
	usleep(1000*50);
	_vlcview(NULL, 0);
#endif
	level_change_count++;
	LOGI("ReloadVideoEnc index:%u lost time:%d\n", level_change_count, time_relative_ms() - ltick);
}

//进入休眠
int goto_aov_sleep(void);
static void *video_process_thread(void *args)
{
	video_service_mgnt_t *mgnt = (video_service_mgnt_t *)args;

	LOGI("video_process_thread start [%p] event_queue:[%p]\n", mgnt, mgnt->event_queue);
	video_service_event_t event = {0};

	bool enc_start = false;

	int pwdindex_array[4] = {1,6,9,11};
	int day_night_status = 0;
	uint32_t day_night_change_tick = 0;
	//按键补光灯夜晚模式切换
	uint32_t keylight_change_night_tick = 0;
#ifdef ENABLE_AOV
#ifdef NDEBUG
	//是否第一次休眠
	bool bfirstsleep = true;
	//进入休眠的时间
	uint32_t sleep_tick = time_relative_ms();
#endif
#endif

	if(bchange_night_flag){
		LOGI("=================== night exist =================== \n");
		day_night_change_tick = time_relative_ms();
		day_night_status = 1;
	}

	while (mgnt->isrun)
	{
		if (0 == mq_recv(mgnt->event_queue, &event, sizeof(video_service_event_t), 100))
		{
            switch (event.type)
            {
            case VIDEO_ENC_START:
				{
					//_vlcview(NULL, 0);
					//Isp_run_type(1);
					if(mgnt->is_day_night_changed)
					{
						if(mgnt->DAYNIGHT_status)
						{
							//gpio_control(GPIO_LED_EN, GPIO_MODE_ON);
							SetCameraLight(60);
						}

					}
					hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_ON);
					enc_start = true;
					mgnt->cur_enc_level = 0;
					RequestCropFrame(0);
					set_video_ai_work_flag(0);
				}
                break;
            case VIDEO_ENC_STOP:
				{
					//_vlcview_exit();
					//Isp_run_type(0);
					//if(mgnt->is_day_night_changed)
					{
						//gpio_control(GPIO_LED_EN, GPIO_MODE_OFF);
						SetCameraLight(0);
					}
					hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_OFF);

					enc_start = false;

					if(mgnt->b_day_night_changed)
					{
						//如果退出黑夜模式，则按键补光灯也要切换到白天模式
						if(mgnt->m_keylight_change_status == 1 && mgnt->DAYNIGHT_status == 0)
						{
							keylight_change_night_tick = time_relative_ms();
						}
					}

					if(mgnt->b_enc_param_changed)
					{
						mgnt->b_enc_param_changed = false;
						mgnt->b_day_night_changed = false;
#if defined(ENABLE_V6)
						ReLoadEncParam(1920, 1088, 1024*512*4, 15);
#endif
						ReloadVideoEnc();
					}
					if(mgnt->b_day_night_changed)
					{
						mgnt->b_day_night_changed = false;
						ReloadVideoEnc();

						//如果退出黑夜模式，则按键补光灯也要切换到白天模式
						if(mgnt->m_keylight_change_status == 1 && mgnt->DAYNIGHT_status == 0)
						{
							keylight_change_night_tick = time_relative_ms();
						}
					}
				}
                break;
            case VIDEO_ENC_END:
                /* code */
				{

				}
                break;
			case VIDEO_ENC_DAYORNIGHT:
				{
					LOGI("VIDEO_ENC_DAYORNIGHT:%d\n", event.param);
#if 0
					mgnt->is_day_night_changed = 1;
					mgnt->DAYNIGHT_status = event.param;
					ReLoadDayNightParam(1-mgnt->DAYNIGHT_status);

					if(!enc_start)
					{
						ReloadVideoEnc();
					}
					else{
						//mgnt->b_day_night_changed = true;
					}
#else
					if(mgnt->DAYNIGHT_status != event.param){
						LOGI("Video ready to change day_night status:%d\n", event.param);
						day_night_change_tick = time_relative_ms();
						day_night_status = event.param;
					}
					else{
						day_night_change_tick = 0;
					}
#endif
				}
				break;
			case VIDEO_ENC_RELOAD:
				{
					LOGI("VIDEO_ENC_RELOAD:%d\n", event.param);
#if defined(ENABLE_V6)
					//如果相同就不重新切换
					if(event.param == mgnt->cur_enc_level){
						LOGW("Video enc param is same:%d\n", event.param);
						break;
					}
					mgnt->b_enc_param_changed = true;
					if(event.param == 1)
					{
						ReLoadEncParam(1920, 1088, 1024*512*2, 15);
					}
					else if (event.param == 2)
					{
						ReLoadEncParam(1920, 1088, 1024*512*2, 10);
					}
					else if (event.param == 3)
					{
						ReLoadEncParam(1920, 1088, 1024*512*1, 10);
					}
					else if (event.param == 4)
					{
						ReLoadEncParam(1920, 1088, 1024*512*1, 5);
					}
					else if (event.param == 5)
					{
						ReLoadEncParam(1920, 1088, 1024*256*1, 5);
					}
					else if (event.param == 11)
					{
						ReLoadEncParam(1920, 1088, 1024*128*1, 5);
					}
					else if (event.param == 12)
					{
						ReLoadEncParam(1920, 1088, 1024*128*1, 1);
					}
					else{
						ReLoadEncParam(1920, 1088, 1024*512*4, 15);
					}
#endif
					ReloadVideoEnc();
					mgnt->cur_enc_level = event.param;
				}
				break;
			case VIDEO_TICK_REFRESH:
				{
#ifdef ENABLE_AOV
#ifdef NDEBUG
					sleep_tick = time_relative_ms();
#endif
#endif
				}
				break;
            default:
                break;
            }
		}

		//状态持续3秒以上才改变
		if(day_night_change_tick > 0 && time_relative_ms() - day_night_change_tick > 2*1000)
		{
			LOGI("day_night_change_tick %d:%d \n", day_night_status, time_relative_ms() - day_night_change_tick);
			day_night_change_tick = 0;

			mgnt->is_day_night_changed = 1;
			mgnt->DAYNIGHT_status = day_night_status;
			ReLoadDayNightParam(1-mgnt->DAYNIGHT_status);

			if(!enc_start)
			{
				keylight_change_night_tick = 0;
				mgnt->b_day_night_changed = false;
				mgnt->m_keylight_change_status = mgnt->DAYNIGHT_status;
				ReloadVideoEnc();

				//不是编码状态根据光敏切换
				for(int i=0; i < 4; i++)
				{
					SetPwmValue(pwdindex_array[i], mgnt->DAYNIGHT_status);
				}
			}
			else{
				mgnt->b_day_night_changed = true;

				if(mgnt->DAYNIGHT_status==1)
				{
					mgnt->m_keylight_change_status = 1;
					mgnt->b_day_night_changed = false;
					// ReloadVideoEnc();
					// SetCameraLight(80);
					//在编码状态只从白天切换黑夜的
					for(int i=0; i < 4; i++)
					{
						SetPwmValue(pwdindex_array[i], mgnt->DAYNIGHT_status);
					}
				}
			}
		}

		//按键补光灯夜晚模式切换
		if(keylight_change_night_tick > 0 && mgnt->m_keylight_change_status == 1 && mgnt->DAYNIGHT_status == 0 &&
			time_relative_ms() - keylight_change_night_tick > 3*1000)
		{
			keylight_change_night_tick = 0;
			mgnt->m_keylight_change_status = 0;
			for(int i=0; i < 4; i++)
			{
				SetPwmValue(pwdindex_array[i], mgnt->m_keylight_change_status);
			}
		}
#ifdef ENABLE_AOV
#ifdef NDEBUG
		//启动1.5秒后进入休眠
		if(bfirstsleep && time_relative_ms() - sleep_tick > 3*512)
		{
			bfirstsleep = false;
			LOGI("first sleep now\n");
			goto_aov_sleep();
			LOGI("first sleep end\n");
		}

		//没有启动编码的情况下，30秒后进入休眠
		else if(!enc_start && sleep_tick>0 && time_relative_ms() - sleep_tick > 30*1000)
		{
			LOGI("sleep now\n");
			goto_aov_sleep();
			LOGI("sleep end\n");
			sleep_tick = time_relative_ms();
		}
#endif //#ifdef NDEBUG
#endif //#ifdef ENABLE_AOV
    }

    return NULL;
}

int video_service_init(void)
{
	int ret = -1;

    if (video_service_mgnt == NULL)
	{
		video_service_mgnt = bmalloc(sizeof(video_service_mgnt_t));

		if (video_service_mgnt == NULL)
		{
			LOGE("malloc plc_service_mgnt_t fail \n");
			return -1;
		}

		memset(video_service_mgnt, 0, sizeof(video_service_mgnt_t));
        //1.
		pthread_mutex_init(&video_service_mgnt->msg_mutex, NULL);

        //2.
		video_service_mgnt->event_queue = mq_create("video_event_queue",sizeof(video_service_event_t), MAX_VIDEO_EVENT);
        //3.
        video_service_mgnt->isrun = 1;
		pthread_create(&video_service_mgnt->thread, NULL, video_process_thread, video_service_mgnt);

		ret = 0;
	}
    return ret;
}


int video_service_deinit(void)
{
    int ret = -1;

    if (video_service_mgnt)
    {
        //1.
        video_service_mgnt->isrun = 0;
        pthread_join(video_service_mgnt->thread, NULL);
        //2.
        mq_destroy(video_service_mgnt->event_queue);
        //3.
		pthread_mutex_destroy(&video_service_mgnt->msg_mutex);
		bfree(video_service_mgnt);
        video_service_mgnt = NULL;
        ret = 0;
    }

    return ret;
}

int video_service_push_event(uint8_t typecode, uint32_t param)
{
	if (video_service_mgnt && video_service_mgnt->event_queue)
	{
		video_service_event_t event = {0};

		event.type = typecode;
		event.param = param;

		return mq_send(video_service_mgnt->event_queue, &event, sizeof(video_service_event_t));
	}
	else if(VIDEO_ENC_DAYORNIGHT==typecode)
	{
		bchange_night_flag = param==1;
	}
	return -1;
}