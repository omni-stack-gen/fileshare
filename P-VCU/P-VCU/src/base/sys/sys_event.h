#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#include <common_defs.h>

// #include <network/eth_mgnt.h>
// #include <periphery_mgnt/button_mgnt.h>
// #include <periphery_mgnt/key_mgnt.h>
typedef enum
{
    SYS_EVENT_BASE_REBOOT,
	SYS_EVENT_BASE_FACTORY_RESET,
	SYS_EVENT_BASE_UPGRADE,
	SYS_EVENT_BASE_UPGRADE_PLC,
	SYS_EVENT_BASE_FACTORY_RESET_ALL,
}sys_event_base_type_e;

#define SYS_EVENT_BASE                             0x00010000

#define SYS_EVENT_TIME                             SYS_EVENT_BASE + 0x00000001
#define SYS_EVENT_BUTTON                           SYS_EVENT_BASE + 0x00000002
#define SYS_EVENT_KEY                              SYS_EVENT_BASE + 0x00000003
#define SYS_EVENT_NET                              SYS_EVENT_BASE + 0x00000004
#define SYS_EVENT_CALL                             SYS_EVENT_BASE + 0x00000005
#define SYS_EVENT_PLC                              SYS_EVENT_BASE + 0x00000006
#define SYS_EVENT_ADDR                             SYS_EVENT_BASE + 0x00000007

typedef struct sys_event
{
	uint32_t type;

	union
	{
		struct
		{
			sys_event_base_type_e cmd;
			uint16_t code;
			uint32_t devid;
		} base_msg;

		struct
		{
			uint8_t state;
			uint8_t code;
		} kbd_msg;

		struct
		{
			char dev_name[16];
			// eth_link_state_t link_state;
			// eth_state_t state;
		} net_msg;

		struct
		{
			int msg;
		} call_msg;

		struct
		{
			uint32_t state;
		} addr_switch;
	} body;

} sys_event_t;

#define SYS_EVENT_INIT_TIME(e)          \
	do                                  \
	{                                   \
		(e).type = SYS_EVENT_TIME;		\
	} while (0)

#if 1
#define SYS_EVENT_INIT_BASE(e, s)          \
	do                                  \
	{                                   \
		(e).type = SYS_EVENT_BASE;		\
		(e).body.base_msg.cmd = s;     \
	} while (0)
#define SYS_EVENT_INIT_BUTTON(e, s, c)	\
	do                                  \
	{                                   \
		(e).type = SYS_EVENT_BUTTON;    \
		(e).body.btn_msg.state = s;     \
		(e).body.btn_msg.code = c;      \
	} while (0)

#define SYS_EVENT_INIT_KEY(e, s, c)     \
	do                                  \
	{                                   \
		(e).type = SYS_EVENT_KEY;   	\
		(e).body.kbd_msg.state = s;     \
		(e).body.kbd_msg.code = c;      \
	} while (0)

#define SYS_EVENT_ADDR_SWITCH(e, s)     	\
	do                                  	\
	{                                   	\
		(e).type = SYS_EVENT_ADDR;   		\
		(e).body.addr_switch.state = s;     \
	} while (0)

#endif

#define SYS_EVENT_INIT_NET(e, n, l, s)                                             	\
	do                                                                             	\
	{                                                                              	\
		(e).type = SYS_EVENT_NET;                                                  	\
		(e).body.net_msg.link_state = l;                                           	\
		(e).body.net_msg.state = s;                                                	\
		strncpy((e).body.net_msg.dev_name, n, sizeof((e).body.net_msg.dev_name));	\
		(e).body.net_msg.dev_name[sizeof((e).body.net_msg.dev_name) - 1] = 0;      	\
	} while (0)

#ifdef __cplusplus
extern "C"
{
#endif

int sys_event_mgnt_init();

void sys_event_mgnt_deinit();

int sys_event_publish(sys_event_t *event);

int sys_event_dispatch(sys_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_EVENT_H__ */