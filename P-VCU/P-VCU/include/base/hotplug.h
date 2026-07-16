#ifndef __HOTPLUG_H__
#define __HOTPLUG_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum sdcard_mount_state
{
	SDCARD_UNMOUNT = 1,
	SDCARD_MOUNT
} sdcard_mount_state_t;

typedef void (*sdcard_mount_state_cb)(sdcard_mount_state_t state, void *user);

int sdcard_monitor_init(void);

void hotplug_monitor_deinit(void);

bool check_sdcard_status(void);



#ifdef __cplusplus
}
#endif

#endif /* __HOTPLUG_H__ */