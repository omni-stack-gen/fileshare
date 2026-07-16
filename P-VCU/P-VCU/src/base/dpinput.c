#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <log.h>
#include <dpinput.h>

#include <errno.h>
#include <utils/input_event.h>
#include <utils/time_helper.h>
#include <sys/sys_event.h>
#if defined(ENABLE_V1) || defined(ENABLE_V6)
#include <keytone_mgnt.h>
#include <hard_node_service.h>
#include <plc_service.h>
#include <hardware_test.h>
#include <goblin_plc_process.h>
#include <myconfig.h>
#endif
static void OnKeyProcess(struct input_event *ev)
{
	static long long lasttick = 0;
	static long long curttick = 0;
#if defined(ENABLE_V1) || defined(ENABLE_V6)
	static unsigned int last_keypress_tick = 0;
	static unsigned int last_keyrelease_tick = 0;
#endif
#ifdef _4KEY_
	//四键时键值往强加，为了兼容五键界面配置
	ev->code = ev->code+1;
#endif

	if(ev->type == EV_KEY)
	{
		if(ev->value == 1)
		{
#ifdef ENABLE_V6
			bool bkeydown = false;
#endif
			// 按键按下
#if (__BITS_PER_LONG != 32 || !defined(__USE_TIME_BITS64)) && !defined(__KERNEL__)
			lasttick = ev->time.tv_sec * 1000 + ev->time.tv_usec / 1000;
#else
			lasttick = time_relative_ms();
#endif

			//DPPostMessage(HARDKBD_MESSAGE, KBD_DOWN, ev->code, 0, MSG_KEY_TYPE);
            LOGI("key %d 按下 \n", ev->code);
#ifdef ENABLE_V1
			play_keytone(100);
			hard_node_mgnt_control('4'-ev->code, NODE_STATE_ON);
#elif ENABLE_V6
			if(get_plc_dev_type() == PLC_DOOR_P_VCU_D1)
			{
				ev->code = 1;

				unsigned int cur_tick = time_relative_ms();
				if(last_keypress_tick==0 || cur_tick - last_keypress_tick > 100*5){
					bkeydown = true;
					play_keytone(100);
					hard_node_mgnt_control(2, NODE_STATE_ON);
					hard_node_mgnt_control(3, NODE_STATE_ON);
				}

				last_keypress_tick = cur_tick;
			}
			else if(get_plc_dev_type() == PLC_DOOR_P_VCU_D2)
			{
				ev->code = ev->code > 2 ? 2 : 1;
				unsigned int cur_tick = time_relative_ms();
				if(last_keypress_tick==0 || cur_tick - last_keypress_tick > 100*5){
					bkeydown = true;
					play_keytone(100);
					hard_node_mgnt_control(5-ev->code*2, NODE_STATE_ON);
					hard_node_mgnt_control(5-ev->code*2-1, NODE_STATE_ON);
				}

				last_keypress_tick = cur_tick;
			}
			else if(get_plc_dev_type() == PLC_DOOR_P_VCU_D4)
			{
				bkeydown = true;
				play_keytone(100);
				hard_node_mgnt_control(4-ev->code, NODE_STATE_ON);
			}

			if(bkeydown && hardware_test_is_test())
			{
				hardware_test_send(Goblin_PLC_HARDWARW_TEST_KEY, ev->code, 1);
			}
#endif
		}
		else if(ev->value == 0)
		{
			// 按键弹起
#if (__BITS_PER_LONG != 32 || !defined(__USE_TIME_BITS64)) && !defined(__KERNEL__)
			curttick = ev->time.tv_sec * 1000 + ev->time.tv_usec / 1000;
#else
			curttick = time_relative_ms();
#endif
			LOGI("key %d 弹起 , lost time:%lld\n", ev->code, curttick - lasttick);
#if defined(ENABLE_V1) || defined(ENABLE_V6)
			//是否上报按键消息
			bool breportkey = false;
			if(get_plc_dev_type() == PLC_DOOR_P_VCU_D1)
			{
				ev->code = 1;

				unsigned int cur_tick = time_relative_ms();
				if(last_keyrelease_tick==0 || cur_tick - last_keyrelease_tick > 100*5){
					breportkey = true;
				}

				last_keyrelease_tick = cur_tick;
			}
			else if(get_plc_dev_type() == PLC_DOOR_P_VCU_D2)
			{
				ev->code = ev->code > 2 ? 2 : 1;
				unsigned int cur_tick = time_relative_ms();
				if(last_keyrelease_tick==0 || cur_tick - last_keyrelease_tick > 100*5){
					breportkey = true;
				}

				last_keyrelease_tick = cur_tick;
			}
			else if(get_plc_dev_type() == PLC_DOOR_P_VCU_D4)
			{
				breportkey = true;
			}

			if(breportkey && hardware_test_is_test())
			{
				hardware_test_send(Goblin_PLC_HARDWARW_TEST_KEY, ev->code, 0);
				return ;
			}

			if(get_plc_dev_type() == PLC_DOOR_P_VCU_D4 && curttick - lasttick >= 1000*5)
			{
				//DPPostMessage(HARDKBD_MESSAGE, KBD_UP, ev->code | LONG_PRESSED, 0, MSG_KEY_TYPE);
				plc_service_key_event(99);
				return ;
			}
			else
			{
				//DPPostMessage(HARDKBD_MESSAGE, KBD_UP, ev->code, 0, MSG_KEY_TYPE);
			}
            //printf("key %d 弹起 \n", ev->code);
			lasttick = curttick = 0;

			if(breportkey)
			{
				sys_event_t e;
				SYS_EVENT_INIT_KEY(e, 0, ev->code);
				sys_event_publish(&e);
			}
#endif
		}
	}

	return ;
}

static void OnTouchProcess(struct input_event *ev)
{
	static int type = -1;

	//static unsigned int xydata[2] = {0, 0};

	static bool curdown = false;

	if(ev->type == EV_ABS)
	{
		// if (ev->code == ABS_X)
		// 	xydata[0] = ev->value;
		// else if (ev->code == ABS_Y)
		// 	xydata[1] = ev->value;
	}
	else if(ev->type == EV_KEY)
	{
		if (ev->code == BTN_TOUCH)
		{
			if(ev->value == 1)
				type = TOUCH_DOWN;
			else if(ev->value == 0)
				type = TOUCH_UP;
		}
	}
	else if(ev->type == EV_SYN && ev->code == 0 && ev->value == 0)
	{
		if(type == TOUCH_DOWN)
		{
			if(!curdown)
			{
				//DPPostMessage(TOUCH_RAW_MESSAGE, xydata[0], xydata[1], TOUCH_DOWN, MSG_TOUCH_TYPE);
				curdown = true;
			}
		}
		else if(type == TOUCH_UP)
		{
			//DPPostMessage(TOUCH_RAW_MESSAGE, xydata[0], xydata[1], TOUCH_UP, MSG_TOUCH_TYPE);
			curdown = false;
		}
	}

	return ;
}

static int InputEvent_CallBack(int fd, short revents, unsigned long ev_bits, void *data)
{
	struct input_event ev;

    int ret = input_event_read(fd, revents, &ev);

	if(ret < 0)
		return -1;

	if(ev_bits & (1 << ((EV_ABS) % (sizeof(long) * 8))))
		OnTouchProcess(&ev);
	else
		OnKeyProcess(&ev);

	return 0;
}

static void* InputEvenThread(void *argv)
{
	while(1)
	{
		if(0 == input_event_wait(-1))
			input_event_dispatch();
	}

	return NULL;
}

void DPCreateInputEvent(void)
{
	input_event_init(InputEvent_CallBack, NULL);

    pthread_t pid0;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x40000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&pid0, &attr, InputEvenThread, NULL);

	pthread_attr_destroy(&attr);
}