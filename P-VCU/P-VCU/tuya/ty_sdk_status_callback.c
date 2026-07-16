#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>

#include "plog.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "ty_sdk_common.h"
// #include "Spiwifi.h"
// #include "device.h"
// #include "SystemMsg.h"

STATIC INT_T s_mqtt_status = 0;
typedef VOID (*ON_IPC_STATUS_CHANGE)(VOID *arg);
typedef struct {
    TUYA_IPC_STATUS_E stat;
    ON_IPC_STATUS_CHANGE cb;
}IPC_STATUS_CHANGE_CB_MAP_T;


STATIC VOID __on_register_fail(VOID *arg)
{

    debug("%s, get registe fail status.", __func__);

    return;
}

STATIC VOID __on_netcfg_start(VOID *arg){

    debug("%s, netcfg start. You can start your own netcfg task now.", __func__);
    return;
}

STATIC VOID __on_netcfg_stop(VOID *arg){

    debug("%s, netcfg stop. You can stop your own netcfg task now.", __func__);
    return;
}

STATIC VOID __on_status_offline(VOID *arg){

    info("offline: network status MQTT disconnected"); //according to requirements of shadow device, do not modify!!!
    s_mqtt_status = 0;
    // SetLedStatus(TUYA_OFFLINE);
    return;
}

STATIC VOID __on_status_online(VOID *arg){

    info("online: network status MQTT connected"); //according to requirements of shadow device, do not modify!!!
    s_mqtt_status = 1;
    // SetLedStatus(TUYA_ONLINE);
    return;
}


static IPC_STATUS_CHANGE_CB_MAP_T status_change_cb_map[] = {
    {TUYA_IPC_STATUS_REGISTER,          NULL},
    {TUYA_IPC_STATUS_REGISTER_FAILED,   __on_register_fail},
    {TUYA_IPC_STATUS_ACTIVED,           NULL},
    {TUYA_IPC_STATUS_RESET,             NULL},
    {TUYA_IPC_STATUS_NETCFG_START,      __on_netcfg_start},
    {TUYA_IPC_STATUS_NETCFG_STOP,       __on_netcfg_stop},
    {TUYA_IPC_STATUS_WIFI_STA_UNCONN,   NULL},
    {TUYA_IPC_STATUS_WIFI_STA_CONN,     NULL},
    {TUYA_IPC_STATUS_WIRE_UNCONN,       NULL},
    {TUYA_IPC_STATUS_WIRE_CONN,         NULL},
    {TUYA_IPC_STATUS_ONLINE,            __on_status_online},
    {TUYA_IPC_STATUS_OFFLINE,           __on_status_offline},
};

VOID TUYA_IPC_Status_Changed_cb(IN TUYA_IPC_STATUS_GROUP_E changed_group, IN CONST TUYA_IPC_STATUS_E status[TUYA_IPC_STATUS_GROUP_MAX])
{
    TUYA_IPC_STATUS_E cur_status = status[changed_group];
    INT_T i;
    debug("status chaged: group[%d] status[%d]", changed_group, cur_status);

    for(i = 0; i < SIZEOF(status_change_cb_map)/SIZEOF(IPC_STATUS_CHANGE_CB_MAP_T); i++) {
        if(status_change_cb_map[i].stat == cur_status) {
            if(status_change_cb_map[i].cb) {
                status_change_cb_map[i].cb(NULL);
            } else {
                debug("status ignore. cb is null.");
            }
        }
    }
}

INT_T TUYA_IPC_sd_status_upload(INT_T status)
{
	IPC_APP_report_sd_status_changed(status);
	return 0;
}

VOID TUYA_IPC_qrcode_shorturl_cb(CHAR_T* shorturl)
{
    if(shorturl)
        info("shorturl len: %d, url: [ %s ]", strlen(shorturl), shorturl); //according to requirements of shadow device, do not modify!!!

    return;
}


/*
Callback when the user clicks on the APP to remove the device
*/
VOID TUYA_IPC_Reset_System_CB(GW_RESET_TYPE_E type)
{
    info("reset ipc success. please restart the ipc %d\n", type);

    //TODO
    /* Developers need to restart IPC operations */
    // DPPostMessage(SYS_MESSAGE, RESET, 0, 0);
}

VOID TUYA_IPC_Restart_Process_CB(VOID)
{
    info("sdk internal restart request. please restart the ipc\n");
    //TODO
    /* Developers need to implement restart operations. Restart the process or restart the device. */
    // DPPostMessage(SYS_MESSAGE, REBOOT, 0, 0);
}

INT_T TUYA_IPC_Get_MqttStatus()
{
    return s_mqtt_status;
}

