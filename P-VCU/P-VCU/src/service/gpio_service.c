#include "gpio_service.h"

#include <periphery/gpio.h>
#include <periphery/gpio_port.h>

#include <utils/mq.h>
#include <utils/time_helper.h>
#include <utils/bmem.h>
#include <sys/sys_event.h>
#include <sound_play.h>
#include <hard_node_service.h>
#include <video_service.h>

#define LOG_TAG "gpio_service"
#include <utils/log.h>
#include <utils/sem.h>
#include <dpbase.h>
#include <hardware_test.h>
#include <goblin_plc_process.h>
#include "data/DBLanguage.h"

#define MAX_GPIO_MESSAGE         (16)
#define FAST_BLINK_TIMEOUT_MS   (500)
#define SLOW_BLINK_TIMEOUT_MS   (1500)


#define     PE0     128
#define     PF0     160
#define     PG0     192
#define     PH0     224

typedef enum _LOCK_TYPE_E_
{
	LOCK_TYPE_INVALID = 0,
    LOCK_TYPE_LO_Z,
    LOCK_TYPE_HI_Z,
    LOCK_TYPE_MAX
} LOCK_TYPE_E;



//E2\E3
//PH11 icut 11
typedef int (*GpioStateCallback)(void *param, enum gpio_type type, gpio_state_t state);
typedef struct gpio_cfg
{
	enum gpio_type type;
	enum gpio_mode mode;
	uint32_t gpio_pin;
	uint32_t orgin_value;   			// 输出时作为初始值, 输入时作为使能值
    uint8_t  is_duration;    			// 是否需要持续时间
	gpio_direction_t  direction;        // 输入还是输出

	GpioStateCallback callback;
} gpio_cfg_t;

typedef struct gpio_state_
{
	enum gpio_mode mode;     // 当前gpio配置的模式
	uint64_t start_time;    // 当前状态的启动时间
	uint32_t last_state;    // 上一次的状态
    uint32_t duration;      // 持续时间
} gpio_last_state_t;

typedef struct gpio_mgnt
{
	mq_t *queue;
	pthread_t thread;
	pthread_t unlock_thread;
	volatile bool quit;
	gpio_t *gpio[GPIO_NUMBER];
	gpio_last_state_t state[GPIO_NUMBER];


	void *unlock_sem;
	uint16_t unlock_roomid;

	//供电锁的类型
	LOCK_TYPE_E m_lock0_type;

	//是否在测试模式
	bool is_test_ing;
	unsigned int test_start_tick;
} gpio_mgnt_t;


static int ResetKeyCallback(void *param, enum gpio_type type, gpio_state_t state);
static int AddressSetCallback(void *param, enum gpio_type type, gpio_state_t state);
static int DayNightDetCallback(void *param, enum gpio_type type, gpio_state_t state);

//GPIO的配置
static gpio_cfg_t g_gpio_cfg[GPIO_NUMBER] =
{
#ifdef ENABLE_V1
    // 两把锁， E2 E3
	/*
	供电锁（LOCK1）开锁输出信号，H=开锁 .开锁时间，根据锁类型来定：
	Lo-Z Locker：LOCK1驱动10mS开锁（持续10mS高电平）平时为低电平。
	Hi-Z Locker：LOCK1驱动5S开锁（持续5S低电平）平时为高电平。
	*/
	{GPIO_LOCK_0,  GPIO_MODE_OFF,  PE0+2,  0, 1, GPIO_DIR_OUT, NULL},       // 第一把锁
	{GPIO_LOCK_1,  GPIO_MODE_OFF, PE0+3,  0, 1, GPIO_DIR_OUT, NULL},       // 第二把锁
	//ICUT PH11
	{GPIO_I_CUT,  GPIO_MODE_OFF, PH0+11,  0, 1, GPIO_DIR_OUT, NULL},       // 摄像头icut
	// PH12 默认设定按钮输入。L=按钮按下。可以定义为长时间（比如5秒）按下为恢复默认状态。
	{GPIO_RESET_KEY,  GPIO_MODE_OFF, PH0+12,  0, 5, GPIO_DIR_IN, ResetKeyCallback},       // 复位按钮
	//PH13 主从机设定。H=主机，L=从机
	{GPIO_ADDRESS_SET,  GPIO_MODE_OFF, PH0+13,  0, 0, GPIO_DIR_IN, AddressSetCallback},       // 拨码开关

	/*
	LOCK1（供电锁）在打开之前必须进行锁类型检测：
	1，首先把LOCK_CHECK置1；
	2，等待5mS
	3，读取LOCK_OVER_DET1/2电平，确定锁的类型：
	1）DET1=H，DET2=H，LOCK1=Lo-Z Locker
	2）DET1=H，DET2=L，LOCK1=Lo-Z Locker
	3）DET1=L，DET2=H，LOCK1=Hi-Z Locker
	4）DET1=L，DET2=L，不存在这种情况
	4，立即把LOCK_CHECK置0。注意：LOCK_CHECK不能置高太多时间，一般5ms~8ms之内完成。
	锁类型检测完成。
	*/
	{GPIO_OVER_DET1,  GPIO_MODE_OFF, PE0+0,  0, 1, GPIO_DIR_IN, NULL},       //
	{GPIO_OVER_DET2,  GPIO_MODE_OFF, PE0+1,  0, 1, GPIO_DIR_IN, NULL},       //
	{GPIO_LOCK_CHECK,  GPIO_MODE_OFF, PF0+6,  0, 1, GPIO_DIR_OUT, NULL},       //

	//PE4 摄像头补光灯使能。H=使能
	{GPIO_LED_EN,  GPIO_MODE_OFF, PE0+4,  0, 0, GPIO_DIR_OUT, NULL},       //
	//PE5 白天/黑夜检测。H=黑夜，L=白天
	{GPIO_DNIGHT_DET,  GPIO_MODE_OFF, PE0+5,  0, 0, GPIO_DIR_IN, DayNightDetCallback},       //
#else
	 // 两把锁，LOCK1_P5_6   LOCK2_P5_3
	/*
	供电锁（LOCK1）开锁输出信号，H=开锁 .开锁时间，根据锁类型来定：
	Lo-Z Locker：LOCK1驱动10mS开锁（持续10mS高电平）平时为低电平。
	Hi-Z Locker：LOCK1驱动5S开锁（持续5S低电平）平时为高电平。
	*/
	{GPIO_LOCK_0,  GPIO_MODE_OFF,  -1,  0, 1, GPIO_DIR_OUT, NULL},       // 第一把锁
	{GPIO_LOCK_1,  GPIO_MODE_OFF, 5*8+3,  0, 1, GPIO_DIR_OUT, NULL},       // 第二把锁
	//ICUT PH11
	{GPIO_I_CUT,  GPIO_MODE_OFF, -1,  0, 1, GPIO_DIR_OUT, NULL},       // 摄像头icut
	// GPIO10_6 默认设定按钮输入。L=按钮按下。可以定义为长时间（比如5秒）按下为恢复默认状态。
	{GPIO_RESET_KEY,  GPIO_MODE_OFF, 86,  0, 5, GPIO_DIR_IN, ResetKeyCallback},       // 复位按钮
	//GPIO10_1 主从机设定。H=主机，L=从机
	{GPIO_ADDRESS_SET,  GPIO_MODE_OFF, 81,  0, 0, GPIO_DIR_IN, AddressSetCallback},       // 拨码开关

	/*
	LOCK1（供电锁）在打开之前必须进行锁类型检测：
	1，首先把LOCK_CHECK置1；
	2，等待5mS
	3，读取LOCK_OVER_DET1/2电平，确定锁的类型：
	1）DET1=H，DET2=H，LOCK1=Lo-Z Locker
	2）DET1=H，DET2=L，LOCK1=Lo-Z Locker
	3）DET1=L，DET2=H，LOCK1=Hi-Z Locker
	4）DET1=L，DET2=L，不存在这种情况
	4，立即把LOCK_CHECK置0。注意：LOCK_CHECK不能置高太多时间，一般5ms~8ms之内完成。
	锁类型检测完成。
	*/
	{GPIO_OVER_DET1,   GPIO_MODE_OFF,  -1,   0, 1, GPIO_DIR_IN,  NULL},       //
	{GPIO_OVER_DET2,   GPIO_MODE_OFF,  -1,   0, 1, GPIO_DIR_IN,  NULL},       //
	{GPIO_LOCK_CHECK,  GPIO_MODE_OFF,  -1,   0, 1, GPIO_DIR_OUT, NULL},       //

	//LED_EN_P4_3_PWM3 摄像头补光灯使能。H=使能 4*8+3
	{GPIO_LED_EN,  GPIO_MODE_OFF, -1,  0, 0, GPIO_DIR_OUT, NULL},       //
	//DNIGHT_DET_P5_4 白天/黑夜检测。H=黑夜，L=白天
	{GPIO_DNIGHT_DET,  GPIO_MODE_OFF, 5*8+4,  0, 0, GPIO_DIR_IN, DayNightDetCallback},       //
#endif
};

typedef struct gpio_event
{
	enum gpio_type type;
	enum gpio_mode mode;
	uint32_t param;
} gpio_event_t;

static gpio_mgnt_t *g_gpio_mgnt = NULL;


static int ring_play_id = 0;
static void stop_play_unlock_voice(void)
{
	if (ring_play_id != 0)
	{
		sound_play_stop(ring_play_id);
		ring_play_id = 0;
	}
}


static void start_play_unlock_voice(uint16_t roomid)
{
	static uint32_t unlock_tick = 0;
	uint32_t cur_tick = time_relative_ms();
	if(cur_tick - unlock_tick < 1000){
		LOGW("start_play_unlock_voice too soon\n", cur_tick - unlock_tick);
		return;
	}
	unlock_tick = cur_tick;
	stop_play_unlock_voice();

	if (ring_play_id == 0)
	{
		char playfilepath[128] = {0};
		LanguageList_T languageinfo ={0};
		if(CheckLanguage(roomid, &languageinfo))
		{
			LOGI("roomid:%d languageid:%d\n", roomid, languageinfo.languageid);
			snprintf(playfilepath, sizeof(playfilepath), "%s%d.mp3", UNLOCK_MP3_PATH, languageinfo.languageid);
		}

		if(access(playfilepath, F_OK) != 0)
		{
			LOGW("playfilepath:%s not exist\n", playfilepath);
			memset(playfilepath, 0, sizeof(playfilepath));
			strncpy(playfilepath, UNLOCK_MP3, sizeof(playfilepath));
		}
	 	ring_play_id = sound_play_start(playfilepath, 1, GetSoundcallVol(), false);
	}

}

static int ResetKeyCallback(void *param, enum gpio_type type, gpio_state_t state)
{
	gpio_mgnt_t *mgnt = (gpio_mgnt_t *)param;
	if (mgnt->state[type].last_state != state)
	{
		/* code */
		LOGW("ResetKeyCallback type:%d state:%d \n", type, state);
		mgnt->state[type].last_state = state;
		if(mgnt->is_test_ing)
		{
			//TODO传给远端
			hardware_test_send(Goblin_PLC_HARDWARW_TEST_RESET, 0, 1-(int)state);
			return 1;
		}

		//按下的情况下开始计时
		if((uint8_t)mgnt->state[type].mode == (uint8_t)state)
			mgnt->state[type].start_time = time_now();
		mgnt->state[type].duration = 5000;
		//松开检测按下持续的时间
		if ((uint8_t)mgnt->state[type].mode != (uint8_t)state)
		{
			LOGW("ResetKeyCallback type:%d lostime:%lld state:%d, ready to reboot\n", type, time_now() - mgnt->state[type].start_time, state);
			if (time_now() - mgnt->state[type].start_time >= mgnt->state[type].duration)
			{

				sys_event_t e = {0};
				SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_FACTORY_RESET);
				//长按15秒彻底删除
				if (time_now() - mgnt->state[type].start_time >= mgnt->state[type].duration*3)
				{
					e.body.base_msg.cmd = SYS_EVENT_BASE_FACTORY_RESET_ALL;
					sound_play_start(BEEP_WAV, 3, 100, false);
				}
				else
				{
					sound_play_start(BEEP_WAV, 1, 100, false);
				}

				sys_event_publish(&e);
			}
		}
	}
	else if (mgnt->state[type].last_state == mgnt->state[type].mode)
	{
		if (time_now() - mgnt->state[type].start_time >= mgnt->state[type].duration){
#if 0// defined(ENABLE_V1) || defined(ENABLE_V6)
			LOGI("ResetKeyCallback type:%d durtime:%lld state:%d, ready to reboot\n", type, time_now() - mgnt->state[type].start_time, state);
			sound_play_start(BEEP_WAV, 1, 100, false);
			sys_event_t e = {0};
			SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_FACTORY_RESET);
			sys_event_publish(&e);
#endif
		}
	}

	return 0;
}

static int AddressSetCallback(void *param, enum gpio_type type, gpio_state_t state)
{
	gpio_mgnt_t *mgnt = (gpio_mgnt_t *)param;
	if (mgnt->state[type].last_state != state)
	{
		mgnt->state[type].last_state = state;
		/* code */
		LOGW("AddressSetCallback type:%d state:%d", type, state);
		if(mgnt->is_test_ing)
		{
			//TODO传给远端
			hardware_test_send(Goblin_PLC_HARDWARW_TEST_DIP_SWITCH, 0, (int)state);
			return 1;
		}


		//通知地址改变
		sys_event_t e = {0};
		SYS_EVENT_ADDR_SWITCH(e, state);
		sys_event_publish(&e);
	}
	return 0;
}

static int DayNightDetCallback(void *param, enum gpio_type type, gpio_state_t state)
{
	gpio_mgnt_t *mgnt = (gpio_mgnt_t *)param;
	if (mgnt->state[type].last_state != state)
	{
		/* code */
		LOGW("DayNightDetCallback type:%d state:%d \n", type, state);
#ifdef ENABLE_V1
		mgnt->state[type].last_state = state;
		gpio_control(GPIO_LED_EN, state == GPIO_HIGH ? GPIO_MODE_ON : GPIO_MODE_OFF);
		void video_switch_night(bool bnight);
		//video_switch_night(state == GPIO_HIGH ? true : false);
#elif defined(ENABLE_V6)
		//补光灯没有打开的的情况下
		//if(mgnt->state[GPIO_LED_EN].last_state != GPIO_HIGH)
		{
			mgnt->state[type].last_state = state;
		}
		video_service_push_event(VIDEO_ENC_DAYORNIGHT, state);
		//
#endif//#ifdef ENABLE_V1
		if(mgnt->is_test_ing)
		{
			//TODO传给远端
			hardware_test_send(Goblin_PLC_HARDWARW_TEST_LDR, 0, (int)state);
			return 1;
		}
	}
	return 0;
}

static void gpio_check(gpio_mgnt_t *mgnt)
{
	uint64_t tp_now = time_now();

	int i = 0;
	gpio_state_t state =  GPIO_LOW;

	for (i = 0; i < GPIO_NUMBER; ++i)
	{
        if (g_gpio_cfg[i].direction == GPIO_DIR_OUT && g_gpio_cfg[i].is_duration && mgnt->state[i].mode != g_gpio_cfg[i].mode && tp_now-mgnt->state[i].start_time >= mgnt->state[i].duration)
        {
			if(GPIO_LOCK_0 == i)
			{
				gpio_write(mgnt->gpio[GPIO_LOCK_0], g_gpio_cfg[i].mode);
				g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].mode = g_gpio_cfg[i].mode;
				g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].last_state = g_gpio_cfg[i].mode;
				g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].start_time = tp_now;
			}
			else
            	gpio_control(i, g_gpio_cfg[i].mode);
        }
		else if (g_gpio_cfg[i].direction == GPIO_DIR_IN && g_gpio_cfg[i].callback)
		{
			if(mgnt->gpio[i])
				gpio_read(mgnt->gpio[i], &state);

			g_gpio_cfg[i].callback(mgnt, g_gpio_cfg[i].type, state);
		}
	}
}
#ifdef ENABLE_V6
static void Lock0_TimeSet(char *ptime)
{
	int fd = open("/sys/class/gpadc/gpadc0/lock_last_time",  O_WRONLY | O_CLOEXEC);
	if(fd < 0)
	{
		LOGE("open lock_last_time error \n");
		return;
	}

	if (write(fd, ptime, strlen(ptime)) < 0)
	{
		LOGE("write lock_last_time error \n");
		close(fd);
		return;
	}

	close(fd);
}

void SetLock1Delay(int delay)
{
	char strbuf[32]={0};
	sprintf(strbuf, "%d", delay*1000);
	Lock0_TimeSet(strbuf);
	LOGI("SetLock1Delay:%d\n", delay);
}

#endif

/*
测试的两个高阻锁都是高电平开锁，驱动默认配置的就是高电平开锁
获取锁类型
echo 0 > /sys/class/gpadc/gpadc0/lock_type
cat /sys/bus/iio/devices/iio:device0/in_voltag0_raw
获取的值如果大于2500 这个锁开锁持续时间5s ，如果获取的值大于2200小于2300是另一种锁，这个锁开锁持续时间10ms

设置开锁持续时间5s
echo 5000 > /sys/class/gpadc/gpadc0/lock_last_time
开锁
cat /sys/bus/iio/devices/iio:device0/in_voltag0_raw


   1）Hi-Z Locker，LOCK1驱动5S开锁（持续5S高电平）
   2）Lo-Z Locker,LOCK1驱动10mS开锁（持续10mS高电平）
   3）short circuit，不能开锁（LOCK1持续低），并给出声音提示
*/
static LOCK_TYPE_E check_lock_0_type(gpio_mgnt_t *mgnt)
{
	LOCK_TYPE_E lock_type = LOCK_TYPE_INVALID;
#ifdef ENABLE_V1
	uint64_t tp_now = time_now();
	gpio_write(mgnt->gpio[GPIO_LOCK_CHECK], 1);
	usleep(5*1000);

	gpio_state_t state1 =  GPIO_LOW;
	gpio_state_t state2 =  GPIO_LOW;
	gpio_read(mgnt->gpio[GPIO_OVER_DET1], &state1);
	gpio_read(mgnt->gpio[GPIO_OVER_DET2], &state2);
	gpio_write(mgnt->gpio[GPIO_LOCK_CHECK], 0);


	printf("########## lost time:%lld state1:%d state2:%d ##########\n", time_now()-tp_now, state1, state2);
	if (state1==GPIO_HIGH)
	{
		printf("########## Lo-Z Locker ##########\n");
		// gpio_write(mgnt->gpio[GPIO_LOCK_0], 1);
		// usleep(10*1000);
		// gpio_write(mgnt->gpio[GPIO_LOCK_0], 0);
		lock_type = LOCK_TYPE_LO_Z;
	}
	else if (state1==GPIO_LOW && state2==GPIO_HIGH)
	{
		printf("########## Hi-Z Locker ##########\n");
		lock_type = LOCK_TYPE_HI_Z;
	}
#elif defined(ENABLE_V6)
	int fd = open("/sys/class/gpadc/gpadc0/lock_type",  O_WRONLY | O_CLOEXEC);
	if(fd < 0)
	{
		LOGE("open lock_type error \n");
		return LOCK_TYPE_INVALID;
	}
	char buf[8] = "0";

	if (write(fd, buf, strlen(buf)) < 0)
	{
		close(fd);
		LOGE("write lock_type error \n");
		return LOCK_TYPE_INVALID;
	}

	close(fd);

	fd = open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", O_RDONLY | O_CLOEXEC);

	if(fd < 0)
	{
		LOGE("open in_voltage0_raw error \n");
		return LOCK_TYPE_INVALID;
	}

	if (read(fd, buf, 8) < 0)
	{
		LOGE("read in_voltage0_raw error \n");
		close(fd);
		return LOCK_TYPE_INVALID;
	}

	printf("in_voltage0_raw:%s\n", buf);

	close(fd);


	int value = atoi(buf);
	if (value > 2500)
	{
		LOGI("########## Hi-Z Locker ##########\n");
		//开锁持续时间5s
		lock_type = LOCK_TYPE_HI_Z;
		char buf[16]={0};
		sprintf(buf, "%d", GetLockDelay()*1000); //获取开锁持续时间
		Lock0_TimeSet(buf); //设置开锁持续时间5s 5000
		printf("########## Hi-Z Locker delay:%d ##########\n", GetLockDelay());
	}
	else if (value > 2200 && value < 2300)
	{
		LOGI("########## Lo-Z Locker ##########\n");
		//开锁持续时间10ms
		lock_type = LOCK_TYPE_LO_Z;
		Lock0_TimeSet("10"); //设置开锁持续时间10ms
	}
	else
	{
		LOGI("########## Invalid Locker ##########\n");
		lock_type = LOCK_TYPE_INVALID;
	}
#endif
	return lock_type;
}

static void gpio_process(gpio_mgnt_t *mgnt, gpio_event_t *event)
{
	uint64_t tp_now = time_now();
	printf("->>>>>>>>> io type:%d mode:%d <<<<<<<<<<<\n", event->type, event->mode);
	if (event->type == GPIO_LOCK_0)
	{
		// printf("########## lost time:%lld state1:%d state2:%d ##########\n", time_now()-tp_now, state1, state2);
#ifdef ENABLE_V1
		start_play_unlock_voice(0);
		hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);
		if (g_gpio_mgnt->m_lock0_type == LOCK_TYPE_LO_Z)
		{
			printf("########## Lo-Z Locker : type:%d ##########\n", g_gpio_mgnt->m_lock0_type);
			gpio_write(mgnt->gpio[GPIO_LOCK_0], 1);
			usleep(20*1000);
			gpio_write(mgnt->gpio[GPIO_LOCK_0], 0);
		}
		else if (g_gpio_mgnt->m_lock0_type == LOCK_TYPE_HI_Z)
		{
			printf("########## Hi-Z Locker ##########\n");
			gpio_write(mgnt->gpio[GPIO_LOCK_0], GPIO_MODE_OFF);
			g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].mode = GPIO_MODE_OFF;
			g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].last_state = GPIO_MODE_OFF;
			g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].start_time = tp_now;

			mgnt->state[event->type].duration = GetLockDelay()*1000;
		}
#elif defined(ENABLE_V6)
		mgnt->unlock_roomid = event->param;
		SetSemaphore(mgnt->unlock_sem);
#endif
	}
	else if (event->mode == GPIO_MODE_OFF)
	{
		gpio_write(mgnt->gpio[event->type], 0);

		mgnt->state[event->type].mode = GPIO_MODE_OFF;
		mgnt->state[event->type].last_state = GPIO_LOW;
		mgnt->state[event->type].start_time = tp_now;
	}
	else if (event->mode == GPIO_MODE_ON)
	{
		gpio_write(mgnt->gpio[event->type], 1);

		mgnt->state[event->type].mode = GPIO_MODE_ON;
		mgnt->state[event->type].last_state = GPIO_HIGH;
		mgnt->state[event->type].start_time = tp_now;

		if(event->type == GPIO_LOCK_1)
		{
			mgnt->state[event->type].duration = GetLock1Delay()*1000;
        	start_play_unlock_voice(event->param);
			hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);
		}
	}
	else
	{
		mgnt->state[event->type].mode = event->mode;
		mgnt->state[event->type].start_time = tp_now;
	}
}

static void ReloadLock0Type(void *param)
{
#ifdef ENABLE_V1
	g_gpio_mgnt->m_lock0_type = GetbLockType()==1 ? LOCK_TYPE_LO_Z : LOCK_TYPE_HI_Z;
	LOGI("lock0 type:%d", g_gpio_mgnt->m_lock0_type);
	if (g_gpio_mgnt->m_lock0_type == LOCK_TYPE_HI_Z)
	{
		g_gpio_cfg[GPIO_LOCK_0].mode = GPIO_MODE_ON;
		g_gpio_cfg[GPIO_LOCK_0].is_duration = 1;
		g_gpio_cfg[GPIO_LOCK_0].orgin_value = GPIO_MODE_ON;

		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].duration = 5000;
		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].mode = GPIO_MODE_ON;
		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].last_state = GPIO_MODE_ON;

		gpio_write(g_gpio_mgnt->gpio[GPIO_LOCK_0], g_gpio_cfg[GPIO_LOCK_0].orgin_value);
	}
	else
	{
		g_gpio_cfg[GPIO_LOCK_0].mode = GPIO_MODE_OFF;
		g_gpio_cfg[GPIO_LOCK_0].is_duration = 0;
		g_gpio_cfg[GPIO_LOCK_0].orgin_value = GPIO_MODE_OFF;

		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].duration = 0;
		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].mode = GPIO_MODE_OFF;
		g_gpio_mgnt[GPIO_LOCK_0].state[GPIO_LOCK_0].last_state = GPIO_MODE_OFF;

		gpio_write(g_gpio_mgnt->gpio[GPIO_LOCK_0], g_gpio_cfg[GPIO_LOCK_0].orgin_value);
	}
#else

#endif
}

//开锁高低阻态
static void Unlock_Type_0(uint16_t roomid)
{
	start_play_unlock_voice(roomid);
	hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);

	uint32_t starttick = time_relative_ms();
	int fd = open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", O_RDONLY | O_CLOEXEC);

	if(fd < 0)
	{
		LOGE("open in_voltage0_raw error \n");
		return ;
	}

	char buf[8] = {0};
	if (read(fd, buf, 8) < 0)
	{
		LOGE("read in_voltage0_raw error \n");
		close(fd);
		return ;
	}

	printf("in_voltage0_raw:%s, lost time:%d\n", buf, time_relative_ms()-starttick);

	close(fd);
}

//单独线程处理阻塞开锁
static void *unlock_thread(void *args)
{
	LOGI("unlock_thread start \n");

	gpio_mgnt_t *mgnt = (gpio_mgnt_t *)args;

	while (!mgnt->quit)
	{
		if(GetSemaphore(mgnt->unlock_sem, 1000))
		{
			Unlock_Type_0(mgnt->unlock_roomid);
		}
	}

	LOGI("unlock_thread stop \n");
	return NULL;
}

static void *gpio_thread(void *args)
{
	LOGI("led_thread start \n");

	gpio_mgnt_t *mgnt = (gpio_mgnt_t *)args;

	gpio_event_t event;

	while (!mgnt->quit)
	{
		if (mq_recv(mgnt->queue, &event, sizeof(gpio_event_t), 100) == 0)
		{
			if(event.type == GPIO_LOCK0TYPE_RELAOD)
			{
				//ReloadLock0Type(NULL);
				continue;
			}
			else if (GPIO_TEST_TYPE == event.type)
			{
				/* code */
				mgnt->is_test_ing = event.mode==GPIO_MODE_ON;
				mgnt->test_start_tick = time_relative_ms();
				LOGI("GPIO_TEST_TYPE:%d", mgnt->is_test_ing);
				//进入测试模式，发送当前值过去
				if(mgnt->is_test_ing)
				{
					hardware_test_send(Goblin_PLC_HARDWARW_TEST_RESET, 0, 1-(int)mgnt->state[GPIO_RESET_KEY].last_state);
					hardware_test_send(Goblin_PLC_HARDWARW_TEST_DIP_SWITCH, 0, (int)mgnt->state[GPIO_ADDRESS_SET].last_state);
					hardware_test_send(Goblin_PLC_HARDWARW_TEST_LDR, 0, (int)mgnt->state[GPIO_DNIGHT_DET].last_state);
				}
				continue;
			}

			gpio_process(mgnt, &event);
		}
		else{
			if(mgnt->is_test_ing)
			{
#if 0
				//测试模式进入超时状态
				if(time_relative_ms() - mgnt->test_start_tick > 10*60*1000)
				{
					mgnt->is_test_ing = false;
					mgnt->test_start_tick = 0;
					LOGI("GPIO_TEST_TYPE timeout end");
				}
#endif
			}
		}

		gpio_check(mgnt);
	}

	LOGI("led_thread stop \n");

	return NULL;
}

int gpio_mgnt_init(uint32_t *paddrswitch_state)
{
	int ret = 0, i = 0;

	if (g_gpio_mgnt == NULL)
	{
		g_gpio_mgnt = bmalloc(sizeof(gpio_mgnt_t));
		memset(g_gpio_mgnt, 0, sizeof(gpio_mgnt_t));

		gpio_config_t config;
		memset(&config, 0, sizeof(config));


		for (i = 0; i < GPIO_NUMBER; ++i)
		{
			g_gpio_mgnt->gpio[i] = gpio_create();

			config.direction = g_gpio_cfg[i].direction;//GPIO_DIR_OUT;
			config.edge = GPIO_EDGE_NONE;
			config.reversed = false;

			if(g_gpio_cfg[i].gpio_pin != -1)
				gpio_open(g_gpio_mgnt->gpio[i], g_gpio_cfg[i].gpio_pin, &config);
			if(config.direction == GPIO_DIR_OUT)
				gpio_write(g_gpio_mgnt->gpio[i], g_gpio_cfg[i].orgin_value);

			g_gpio_mgnt->state[i].last_state = g_gpio_cfg[i].orgin_value;
			g_gpio_mgnt->state[i].start_time = time_now();
			g_gpio_mgnt->state[i].mode = g_gpio_cfg[i].mode;
		}

		g_gpio_mgnt->m_lock0_type = check_lock_0_type(g_gpio_mgnt);

		ReloadLock0Type(NULL);

		//拨码开关的状态，用于设置本机是否为主机
		//0=主机，1=从机
		gpio_state_t state = 0;
		gpio_read(g_gpio_mgnt->gpio[GPIO_ADDRESS_SET], &state);
		if(paddrswitch_state)
			*paddrswitch_state = state;

		g_gpio_mgnt->queue = mq_create("gpio_queue", sizeof(gpio_event_t), MAX_GPIO_MESSAGE);

		g_gpio_mgnt->unlock_sem = CreateSemaphore(0);
		//单独的开锁线程， 高低阻态的锁会阻塞，单独一个线程开锁
		pthread_create(&g_gpio_mgnt->unlock_thread, NULL, unlock_thread, g_gpio_mgnt);
		pthread_create(&g_gpio_mgnt->thread, NULL, gpio_thread, g_gpio_mgnt);
		ret = 1;
	}

	return ret;
}

void gpio_mgnt_deinit(void)
{
	if (g_gpio_mgnt)
	{
        g_gpio_mgnt->quit = true;
        pthread_join(g_gpio_mgnt->thread, NULL);
		pthread_join(g_gpio_mgnt->unlock_thread, NULL);
		DestorySemaphore(g_gpio_mgnt->unlock_sem);
        mq_destroy(g_gpio_mgnt->queue);

        for (int i = 0; i < GPIO_NUMBER; i++)
        {
            gpio_destroy(g_gpio_mgnt->gpio[i]);
        }

		bfree(g_gpio_mgnt);
		g_gpio_mgnt = NULL;
	}
}

int gpio_control(enum gpio_type type, enum gpio_mode mode)
{
	int ret = -1;

	if (g_gpio_mgnt)
	{
		gpio_event_t event={0};
		event.type = type;
		event.mode = mode;

		ret = mq_send(g_gpio_mgnt->queue, &event, sizeof(event));
	}

	return ret;
}

int gpio_control_Ex(enum gpio_type type, enum gpio_mode mode, uint32_t param)
{
	int ret = -1;

	if (g_gpio_mgnt)
	{
		gpio_event_t event={0};
		event.type = type;
		event.mode = mode;
		event.param = param;
		ret = mq_send(g_gpio_mgnt->queue, &event, sizeof(event));
	}

	return ret;
}