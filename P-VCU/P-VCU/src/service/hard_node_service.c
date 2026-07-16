#include "hard_node_service.h"


#include <utils/mq.h>
#include <utils/time_helper.h>
#include <utils/bmem.h>
#include <sys/sys_event.h>
#ifdef ENABLE_V1
#include <sound_play.h>
#endif
#include <periphery/gpio.h>
#include <periphery/gpio_port.h>

#define LOG_TAG "node_service"
#include <utils/log.h>
#include <dpbase.h>
#include <video_service.h>

#define MAX_NODE_MESSAGE         (16)
#define FAST_BLINK_TIMEOUT_MS   (500)
#define SLOW_BLINK_TIMEOUT_MS   (1500)




typedef struct node_cfg
{
	enum node_type type;                // 节点类型
	enum node_state_mode state;         // 节点状态模式
	char str_node_name[128];            // 节点名称
    uint32_t  duration;    			// 持续时间

	uint32_t gpio_id;                // GPIO值
	uint8_t  pwm_index;              // PWM索引
} node_cfg_t;

typedef struct node_state_
{
	enum node_state_mode last_state;     // 上一次的状态
	uint64_t start_time;                // 当前状态的触发时间
} node_last_state_t;

typedef struct gpio_mgnt
{
	mq_t *queue;
	pthread_t thread;
	volatile bool quit;

	node_last_state_t state[NODE_NUMBER];

	gpio_t *gpio[NODE_NUMBER];
} node_mgnt_t;




//GPIO的配置
static node_cfg_t g_node_cfg[NODE_NUMBER] =
{
    // 4个按键灯
	{NODE_KEY1_LED,  NODE_STATE_OFF,  "/sys/class/leds/led1/brightness",  1000, 	7/*39*/, 9},       // 第一按键灯
	{NODE_KEY2_LED,  NODE_STATE_OFF,  "/sys/class/leds/led2/brightness",  1000,		2/*41*/, 6},       // 第二按键灯
	{NODE_KEY3_LED,  NODE_STATE_OFF,  "/sys/class/leds/led3/brightness",  1000,		41/*2*/, 1},       // 第三按键灯
	{NODE_KEY4_LED,  NODE_STATE_OFF,  "/sys/class/leds/led4/brightness",  1000,		39/*7*/, 11},       // 第四按键灯
	// 状态指示灯
	{NODE_MSG_LED,   NODE_STATE_OFF,  "/sys/class/leds/led5/brightness",   0,		87,  0},        // 消息灯
	{NODE_LOCK_LED,  NODE_STATE_OFF,  "/sys/class/leds/led6/brightness",  1000,		21,  0},       // 开锁指示灯
};

typedef struct node_event
{
	enum node_type type;
	enum node_state_mode state;
} node_event_t;

static node_mgnt_t *g_node_mgnt = NULL;

#define	PWMINDEX	3
#define SYSFS_PWM_DIR "/sys/class/pwm"
int SetPwmValue(int index, int value)
{
    char ctrcmdstr[256] = {0};
    int pwmindex = index;
	LOGI("Ctrl Set pwm:%d  value  %d \n", pwmindex, value);
	int fd;
	char buf[16];
	int len;

	fd = open(SYSFS_PWM_DIR"/pwmchip0/export", O_WRONLY);
	if (fd < 0) {
		printf ("\nFailed export PWM_B1\n");
		return -1;
	}

    sprintf(ctrcmdstr, "%d", pwmindex);
    write(fd, ctrcmdstr, 2);
	close(fd);

    sprintf(ctrcmdstr, SYSFS_PWM_DIR "/pwmchip0/pwm%d/enable", pwmindex);
    fd = open(ctrcmdstr, O_WRONLY);
	if (fd < 0)
	{
		printf ("\nFailed disable PWM_B2\n");
		return -1;
	}
	write(fd, (char*)"0", 2);
	close(fd);

    sprintf(ctrcmdstr, SYSFS_PWM_DIR "/pwmchip0/pwm%d/period", pwmindex);
    fd = open(ctrcmdstr, O_WRONLY);
	if (fd < 0)
	{
		printf ("\nFailed disable PWM_B3\n");
		return -1;
	}
	write(fd, (char*)"10000", 6);
	close(fd);

    sprintf(ctrcmdstr, SYSFS_PWM_DIR "/pwmchip0/pwm%d/duty_cycle", pwmindex);
    fd = open(ctrcmdstr, O_WRONLY);
	if (fd < 0)
	{
		printf ("\nFailed disable PWM_B4\n");
		return -1;
	}
	len = sprintf(buf, "%d", value * 100);
	write(fd, buf, len);
	close(fd);

    sprintf(ctrcmdstr, SYSFS_PWM_DIR "/pwmchip0/pwm%d/enable", pwmindex);
    fd = open(ctrcmdstr, O_WRONLY);
	if (fd < 0)
	{
		printf ("\nFailed disable PWM_B5\n");
		return -1;
	}
	write(fd, (char*)"1", 2);
	close(fd);

	fd = open(SYSFS_PWM_DIR"/pwmchip0/unexport", O_WRONLY);
	if (fd < 0) {
		printf ("\nFailed export PWM_B6\n");
		return -1;
	}

	sprintf(ctrcmdstr, "%d", pwmindex);
    write(fd, ctrcmdstr, 2);
    close(fd);

	return 0;
}

int SetCameraLight(int light)
{
	SetPwmValue(PWMINDEX, light);
	return 0;
}

static void node_write_state(char *node_path, uint8_t value)
{
#ifdef ENABLE_V1
    int fd = open(node_path, O_WRONLY);
    if (fd > 0)
    {
        char buf[64] = {0};
        int len = snprintf(buf, sizeof(buf), "%d", value);
        write(fd, buf, len);
        close(fd);
        LOGI( "set %s to %d\n", node_path, value);
    }
    else{
        LOGW("open %s failed\n", node_path);
    }
#elif ENABLE_V6
	for (size_t i = 0; i < NODE_NUMBER; i++)
	{
		if (strcmp(g_node_cfg[i].str_node_name, node_path) == 0)
		{
			if (value == 1)
			{
				gpio_write(g_node_mgnt->gpio[i], 1);
			}
			else
			{
				gpio_write(g_node_mgnt->gpio[i], 0);
			}
			break;
		}
	}
#endif
}


static void node_state_check(node_mgnt_t *mgnt)
{
	uint64_t tp_now = time_now();

	int i = 0;
	for (i = 0; i < NODE_NUMBER; ++i)
	{
        //printf("<%d>", i, mgnt->state[i].last_state, g_node_cfg[i].state);
        if (g_node_cfg[i].duration && mgnt->state[i].last_state != g_node_cfg[i].state && tp_now - mgnt->state[i].start_time > g_node_cfg[i].duration)
        {
            /* code */
			if(i <= NODE_KEY4_LED && !get_day_night_status()){
				SetPwmValue(g_node_cfg[i].pwm_index, 0);
			}
			else
            	node_write_state(g_node_cfg[i].str_node_name, 0);
            mgnt->state[i].last_state = g_node_cfg[i].state ;
		    mgnt->state[i].start_time = tp_now;
        }
	}
}

static void node_process(node_mgnt_t *mgnt, node_event_t *event)
{
	uint64_t tp_now = time_now();

	if (event->state == NODE_STATE_OFF)
	{
		if(event->type <= NODE_KEY4_LED && !get_day_night_status()){
			SetPwmValue(g_node_cfg[event->type].pwm_index, 0);
		}
		else
			node_write_state(g_node_cfg[event->type].str_node_name, 0);

		mgnt->state[event->type].last_state = NODE_STATE_OFF;
		mgnt->state[event->type].start_time = tp_now;
	}
	else if (event->state == NODE_STATE_ON)
	{
		if(event->type <= NODE_KEY4_LED && !get_day_night_status()){
			SetPwmValue(g_node_cfg[event->type].pwm_index, 50);
		}
		else
			node_write_state(g_node_cfg[event->type].str_node_name, 1);

		mgnt->state[event->type].last_state = NODE_STATE_ON;
		mgnt->state[event->type].start_time = tp_now;
	}
}

static void *node_process_thread(void *args)
{
	LOGI("node_thread start \n");

	node_mgnt_t *mgnt = (node_mgnt_t *)args;

	node_event_t event; // 节点事件

	bool is_fast_blink = true;
	int index = 0;
	while (!mgnt->quit)
	{
		if (mq_recv(mgnt->queue, &event, sizeof(node_event_t), 100) == 0)
		{
			node_process(mgnt, &event);
		}
		else if(is_fast_blink)
		{
			index++;
			if(index > 10)
			{
				is_fast_blink = false;
				//一秒后关闭所有灯
				for (size_t i = 0; i < NODE_NUMBER; i++)
				{
					hard_node_mgnt_control(i, NODE_STATE_OFF);
				}
				SetCameraLight(0);
			}
		}

		node_state_check(mgnt);
	}

	LOGI("node_thread stop \n");

	return NULL;
}

int hard_node_mgnt_init(void)
{
	int ret = 0;

	if (g_node_mgnt == NULL)
	{
		g_node_mgnt = bmalloc(sizeof(node_mgnt_t));
		memset(g_node_mgnt, 0, sizeof(node_mgnt_t));

		g_node_mgnt->queue = mq_create("hard_node_queue", sizeof(node_event_t), MAX_NODE_MESSAGE);


        for (int i = 0; i < NODE_NUMBER; i++)
        {
#ifdef ENABLE_V6
			gpio_config_t config = {0};
			g_node_mgnt->gpio[i] = gpio_create();

			memset(&config, 0, sizeof(config));
			config.direction = GPIO_DIR_OUT;
			config.edge = GPIO_EDGE_NONE;
			config.reversed = false;

			gpio_open(g_node_mgnt->gpio[i], g_node_cfg[i].gpio_id, &config);
#endif
            node_write_state(g_node_cfg[i].str_node_name, 0);
        }

		pthread_create(&g_node_mgnt->thread, NULL, node_process_thread, g_node_mgnt);
				//第一次上电把灯都点亮
		// for (size_t i = 0; i < NODE_NUMBER; i++)
		// {
		// 	//补光灯不打开
		// 	//if(NODE_MSG_LED != i)
		// 		hard_node_mgnt_control(i, NODE_STATE_ON);
		// }
		//SetCameraLight(10);
		//只打开消息灯和开锁灯
		hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_ON);
		hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);

		ret = 1;
	}

	return ret;
}

void hard_node_mgnt_deinit(void)
{
	if (g_node_mgnt)
	{
        g_node_mgnt->quit = true;
        pthread_join(g_node_mgnt->thread, NULL);
        mq_destroy(g_node_mgnt->queue);

		bfree(g_node_mgnt);
		g_node_mgnt = NULL;
	}
}

int hard_node_mgnt_control(enum node_type type, enum node_state_mode state)
{
	int ret = -1;

	if (g_node_mgnt)
	{
		node_event_t event={0};
		event.type = type;
		event.state = state;

		ret = mq_send(g_node_mgnt->queue, &event, sizeof(event));
	}

	return ret;
}