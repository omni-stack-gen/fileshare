/*********************************************************************************
  *Copyright(C),2015-2020,
  *TUYA
  *www.tuya.comm
  *FileName:    tuya_ipc_motion_detect_demo
**********************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>

#include "plog.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_event.h"

#include "ty_sdk_common.h"

//According to different chip platforms, users need to implement whether there is motion alarm in the current frame.
static int fake_md_status = 0;

VOID IPC_APP_set_motion_status(int status)
{
    fake_md_status = status;
}

static int get_motion_status()
{
    return fake_md_status;
}

#define MAX_SNAPSHOT_BUFFER_SIZE            (100*1024)  //in BYTE

VOID *thread_md_proc(VOID *arg)
{
    int motion_flag = 0;
    int motion_alarm_is_triggerd = FALSE;
    char snap_addr[MAX_SNAPSHOT_BUFFER_SIZE] = {0};
    int snap_size = MAX_SNAPSHOT_BUFFER_SIZE;
    int md_enable = 0;
    int ret = 0;
    TIME_T current_time;
    TIME_T last_md_time;

    while (1)
    {
        usleep(100*1000);
        tuya_ipc_get_utc_time(&current_time);
        motion_flag = get_motion_status();
        if(motion_flag)
        {
            if(!motion_alarm_is_triggerd)
            {
                last_md_time = current_time;
                motion_alarm_is_triggerd = TRUE;
                //start Local SD Card Event Storage and Cloud Storage Events
                tuya_ipc_start_storage(E_ALARM_SD_STORAGE);
                tuya_ipc_start_storage(E_ALARM_CLOUD_STORAGE);
#if (ENABLE_CLOUD_RULE == 1)
                /*NOTE:
                ONE：The prerequisite for using this feature is to have enabled cloud AI detection value-added services or video message value-added services
                TWO：Cloud results obtained through TUYA_APP_result_cb
                */
                tuya_ipc_rule_trigger_event(NOTIFICATION_NAME_MOTION);
#endif
                IPC_APP_get_snapshot(snap_addr,&snap_size);
                if(snap_size > 0)
                {
                    md_enable = IPC_APP_get_alarm_function_onoff();
                    // md_enable is TRUE, upload message to message center
                    tuya_ipc_notify_alarm(snap_addr, snap_size, NOTIFICATION_NAME_MOTION, md_enable, NULL);
                }

                /*NOTE:
                ONE：Considering the real-time performance of push and storage, the above interfaces can be executed asynchronously in different tasks.
                TWO：When event cloud storage is turned on, it will automatically stop beyond the maximum event time in SDK.
                THREE:If you need to maintain storage for too long without losing it, you can use the interface (tuya_ipc_ss_get_status and tuya_ipc_cloud_storage_get_event_status).
                      to monitor whether there are stop event videos in SDK and choose time to restart new events
                */
            }
            else
            {
                //Storage interruption caused by maximum duration of internal events, restart new events
                if(SS_WRITE_MODE_EVENT == tuya_ipc_ss_get_write_mode() && E_STORAGE_STOP == tuya_ipc_ss_get_status())
                {
                    tuya_ipc_start_storage(E_ALARM_SD_STORAGE);
                }

                if(ClOUD_STORAGE_TYPE_EVENT == tuya_ipc_cloud_storage_get_store_mode()
                   && FALSE == tuya_ipc_cloud_storage_get_status())
                {
                    tuya_ipc_start_storage(E_ALARM_CLOUD_STORAGE);
                }
            }
        }
        else
        {
            //No motion detect for more than 10 seconds, stop the event
            if(current_time - last_md_time > 10 && motion_alarm_is_triggerd)
            {
                tuya_ipc_stop_storage(E_ALARM_SD_STORAGE);
                tuya_ipc_stop_storage(E_ALARM_CLOUD_STORAGE);
                motion_alarm_is_triggerd = FALSE;
            }
        }
    }

    return NULL;
}
 
OPERATE_RET TUYA_APP_Enable_Motion_Detect()
{

    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);    

    return OPRT_OK;
}

/**
 * @brief        The developer first calls "tuya_ipc_rule_trigger_event" to trigger an event, 
                 and then the AI detection results and video address will be notified to the upper layer through this callback
 * @param[out]   result_list: ai result list from cloud, json string, such as ["ai_package","ai_car","ai_bird","ai_fire"]
 * @param[out]   result_msg:  message from cloud, such as {"type":"xxx","ai_bucket":"xxx","ai_path":"xxx","ai_expire":"xxx","store_bucket":"xxx","store_path":"xxx","store_expire":"xxx"}
 * @param[out]   msg_time:    time of cloud message
 * @param[out]   type:        message type.Specifically refer to RULE_RESULT_TYPE_E
 * @return       VOID
 */
VOID TUYA_APP_result_cb(OUT CHAR_T *result_list, OUT CHAR_T *result_msg, OUT UINT_T msg_time, OUT RULE_RESULT_TYPE_E type)
{
#if (ENABLE_CLOUD_RULE == 1)
    int md_enable = 0;
    // if md_enable is TRUE, upload message to message center
    md_enable = IPC_APP_get_alarm_function_onoff();
    //cloud result contains ai detect result
    if(type & RULE_AI_RESULT == RULE_AI_RESULT) {
        // Detect AI targets (such as ai bird) and then report intelligent messages
        if (result_list && strstr(result_list, "ai_bird")) { //have AI target:ai_bird
            tuya_ipc_rule_report_ai_msg(result_msg, md_enable);
        } else if (result_list && 0 == strncmp(result_list, "[]", 2)){ // No AI targets detected
            tuya_ipc_rule_report_event(NOTIFICATION_NAME_MOTION, result_msg, md_enable);
        }
    } else if(type & RULE_STORE_RESULT == RULE_STORE_RESULT){ // cloud result only contains video address
         debug("cloud result only contain video msg");
         //Using demo file for simulation, should be replaced by real snapshot when events happen.
         int snapshot_size = 150*1024;
         char *snapshot_buf = (char *)malloc(snapshot_size);
         int ret = IPC_APP_get_snapshot(snapshot_buf, &snapshot_size);
         if(ret != 0) {
             error("Get snap fail!\n");
             free(snapshot_buf);
             return;
         }
         tuya_ipc_notify_alarm(snapshot_buf, snapshot_size, NOTIFICATION_NAME_MOTION, md_enable, result_msg);
         free(snapshot_buf);
    }
#endif

    return;
}

