#ifndef __AOV_DEMO_H__
#define __AOV_DEMO_H__

#include <semaphore.h>
#include "config.h"

extern int g_sig_stop;
// extern int g_aov_sleep_flag;
extern int g_aov_resume_reason;
extern pthread_mutex_t g_aov_mutex;
extern void aov_creat(void);
extern unsigned long long fh_sys_get_time(void);
extern int linux_video_ready(void);
extern int linux_aov_config(int grpid, int enable, int interval_ms, int fream_num);
extern int linux_resume_view(void);
#endif /*__AOV_DEMO_H__*/
