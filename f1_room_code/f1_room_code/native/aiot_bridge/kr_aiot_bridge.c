#include "kr_aiot_bridge.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "goblin_aiot.h"
#include "goblin_aiot_callx.h"
#define LOG_TAG "aiot-bridge"
#include <utils/log.h>
#include <utils/bmem.h>
#include <utils/event_queue.h>
#include <utils/util.h>

#define KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL "simple_secondary_confirm"
#define KR_AIOT_SECONDARY_CONFIRM_PROXY_KIND "entryDoor"
#define KR_AIOT_LOGIN_RETRY_INTERVAL_SEC 30
#define KR_AIOT_CURL_OPT_WRITEDATA 10001
#define KR_AIOT_CURL_OPT_URL 10002
#define KR_AIOT_CURL_OPT_WRITEFUNCTION 20011
#define KR_AIOT_CURL_OPT_USERAGENT 10018
#define KR_AIOT_CURL_OPT_FOLLOWLOCATION 52
#define KR_AIOT_CURL_OPT_FAILONERROR 45
#define KR_AIOT_CURL_OPT_SSL_VERIFYPEER 64
#define KR_AIOT_CURL_OPT_SSL_VERIFYHOST 81
#define KR_AIOT_CURL_OPT_CONNECTTIMEOUT_MS 156
#define KR_AIOT_CURL_OPT_TIMEOUT_MS 155
#define KR_AIOT_CURL_OPT_NOSIGNAL 99

typedef void CURL;
typedef int CURLcode;

extern CURL *curl_easy_init(void);
extern CURLcode curl_easy_setopt(CURL *curl, int option, ...);
extern CURLcode curl_easy_perform(CURL *curl);
extern void curl_easy_cleanup(CURL *curl);

static void *kr_aiot_bmalloc(size_t size)
{
    return bmalloc(size);
}

static void kr_aiot_bfree(void *ptr)
{
    bfree(ptr);
}

static size_t kr_aiot_download_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    FILE *fp = (FILE *)userdata;
    if (fp == NULL)
    {
        return 0;
    }
    return fwrite(ptr, size, nmemb, fp);
}

struct kr_aiot_handle
{
    pthread_mutex_t lock;
    pthread_mutex_t login_retry_lock;
    pthread_cond_t login_retry_cond;
    bool lock_ready;
    bool login_retry_sync_ready;
    bool running;
    bool login_retry_running;
    bool login_retry_stop;
    pthread_t login_retry_thread;
    void *ctx;
    kr_aiot_config_t cfg;
    char last_session_id[32];
    bool parent_logged_in;
    bool secondary_source_online;
    bool secondary_login_pending;
    bool secondary_logged_in;
    bool secondary_logout_pending;
    char secondary_proxy_sn[64];
    char secondary_proxy_model[64];
    char secondary_logout_proxy_sn[64];
    char secondary_logout_proxy_model[64];
    bool call_busy;
    bool manager_audio_callx_config_active;
    bool resident_audio_callx_config_active;
    bool resident_app_invite_filter_active;
    int call_ring_timeout;
    int call_converse_timeout;
    char cached_invite_callid[64];
    callee_dev_info_t *cached_invite_callees;
    int cached_invite_callee_count;
    event_queue_t events;
};

typedef enum kr_aiot_secondary_action
{
    KR_AIOT_SECONDARY_ACTION_NONE = 0,
    KR_AIOT_SECONDARY_ACTION_LOGIN,
    KR_AIOT_SECONDARY_ACTION_LOGOUT,
} kr_aiot_secondary_action_t;

static void kr_aiot_restore_default_callx_config_if_needed(kr_aiot_handle_t *handle);

static void kr_aiot_fill_report_detail(kr_aiot_handle_t *handle, goblin_aiot_msg_data_t *msg)
{
    msg->msg_type = GOBLIN_AIOT_REPORT_DETAIL;
    safe_strncpy(msg->data.report_detail.application_version, handle->cfg.application_version, sizeof(msg->data.report_detail.application_version));
    safe_strncpy(msg->data.report_detail.system_version, handle->cfg.system_version, sizeof(msg->data.report_detail.system_version));
    safe_strncpy(msg->data.report_detail.hardware_version, handle->cfg.hardware_version, sizeof(msg->data.report_detail.hardware_version));
    safe_strncpy(msg->data.report_detail.mac, handle->cfg.mac, sizeof(msg->data.report_detail.mac));
    safe_strncpy(msg->data.report_detail.tf_card_stat, "normal", sizeof(msg->data.report_detail.tf_card_stat));
    safe_strncpy(msg->data.report_detail.call_modes_supported[0], "webrtc", sizeof(msg->data.report_detail.call_modes_supported[0]));
    msg->data.report_detail.tfcard_space_total = 0;
    msg->data.report_detail.tfcard_space_used = 0;
    msg->data.report_detail.battery_level = 100;
}

static void kr_aiot_fill_secondary_proxy(kr_aiot_handle_t *handle,
                                         goblin_aiot_msg_data_t *msg)
{
    if (handle == NULL || msg == NULL)
    {
        return;
    }

    safe_strncpy(msg->proxy_sn, handle->secondary_proxy_sn, sizeof(msg->proxy_sn));
    safe_strncpy(msg->proxy_model,
                 IS_VALID_STRING(handle->secondary_proxy_model) ?
                 handle->secondary_proxy_model : KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL,
                 sizeof(msg->proxy_model));
    safe_strncpy(msg->proxy_kind,
                 KR_AIOT_SECONDARY_CONFIRM_PROXY_KIND,
                 sizeof(msg->proxy_kind));
}

static void kr_aiot_fill_secondary_logout_proxy(kr_aiot_handle_t *handle,
                                                goblin_aiot_msg_data_t *msg)
{
    if (handle == NULL || msg == NULL)
    {
        return;
    }

    safe_strncpy(msg->proxy_sn,
                 IS_VALID_STRING(handle->secondary_logout_proxy_sn) ?
                 handle->secondary_logout_proxy_sn : handle->secondary_proxy_sn,
                 sizeof(msg->proxy_sn));
    safe_strncpy(msg->proxy_model,
                 IS_VALID_STRING(handle->secondary_logout_proxy_model) ?
                 handle->secondary_logout_proxy_model :
                 (IS_VALID_STRING(handle->secondary_proxy_model) ?
                  handle->secondary_proxy_model : KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL),
                 sizeof(msg->proxy_model));
    safe_strncpy(msg->proxy_kind,
                 KR_AIOT_SECONDARY_CONFIRM_PROXY_KIND,
                 sizeof(msg->proxy_kind));
}

static void kr_aiot_clear_cached_invite_locked(kr_aiot_handle_t *handle)
{
    if (handle == NULL)
    {
        return;
    }
    if (handle->cached_invite_callees != NULL)
    {
        bfree(handle->cached_invite_callees);
        handle->cached_invite_callees = NULL;
    }
    handle->cached_invite_callee_count = 0;
    handle->cached_invite_callid[0] = '\0';
}

static void kr_aiot_copy_invite_callee(callee_dev_info_t *dst, const callee_dev_info_t *src)
{
    if (dst == NULL || src == NULL)
    {
        return;
    }
    memset(dst, 0, sizeof(*dst));
    safe_strncpy(dst->kind,
                 IS_VALID_STRING(src->kind) ? src->kind : "device",
                 sizeof(dst->kind));
    safe_strncpy(dst->sn, src->sn, sizeof(dst->sn));
    dst->channel = src->channel;
    if (strcmp(dst->kind, "device") == 0 && dst->channel == 0)
    {
        dst->channel = 1;
    }
    safe_strncpy(dst->user_id, src->user_id, sizeof(dst->user_id));
    safe_strncpy(dst->web_username, src->web_username, sizeof(dst->web_username));
    safe_strncpy(dst->web_sessionId, src->web_sessionId, sizeof(dst->web_sessionId));
    safe_strncpy(dst->mode,
                 IS_VALID_STRING(src->mode) ? src->mode : "webrtc",
                 sizeof(dst->mode));
}

static bool kr_aiot_callee_invitable(const callee_dev_info_t *callee)
{
    if (callee == NULL || !callee->able)
    {
        return false;
    }
    if (strcmp(callee->kind, "device") == 0)
    {
        return IS_VALID_STRING(callee->sn);
    }
    if (strcmp(callee->kind, "app") == 0 || strcmp(callee->kind, "wechat") == 0)
    {
        return IS_VALID_STRING(callee->user_id);
    }
    if (strcmp(callee->kind, "web") == 0)
    {
        return IS_VALID_STRING(callee->web_username);
    }
    return IS_VALID_STRING(callee->sn) ||
           IS_VALID_STRING(callee->user_id) ||
           IS_VALID_STRING(callee->web_username);
}

static bool kr_aiot_callee_is_app_like(const callee_dev_info_t *callee)
{
    if (callee == NULL)
    {
        return false;
    }
    return strcmp(callee->kind, "app") == 0 ||
           strcmp(callee->kind, "wechat") == 0 ||
           strcmp(callee->kind, "web") == 0;
}

static int kr_aiot_cache_initiate_ack_round_locked(kr_aiot_handle_t *handle,
                                                   const char *callid,
                                                   const callee_rounds_info_t *round,
                                                   bool app_only)
{
    callee_dev_info_t *callees;
    int count = 0;
    int i;

    if (handle == NULL || round == NULL || round->callees == NULL || round->callees_num <= 0)
    {
        return 0;
    }

    kr_aiot_clear_cached_invite_locked(handle);
    callees = (callee_dev_info_t *)bzalloc(sizeof(callee_dev_info_t) * (size_t)round->callees_num);
    if (callees == NULL)
    {
        return -ENOMEM;
    }
    for (i = 0; i < round->callees_num; ++i)
    {
        if (!kr_aiot_callee_invitable(&round->callees[i]))
        {
            continue;
        }
        if (app_only && !kr_aiot_callee_is_app_like(&round->callees[i]))
        {
            continue;
        }
        kr_aiot_copy_invite_callee(&callees[count], &round->callees[i]);
        count++;
    }
    if (count == 0)
    {
        bfree(callees);
        return 0;
    }
    safe_strncpy(handle->cached_invite_callid, callid, sizeof(handle->cached_invite_callid));
    handle->cached_invite_callees = callees;
    handle->cached_invite_callee_count = count;
    return count;
}

static int kr_aiot_take_cached_invite_callees(kr_aiot_handle_t *handle,
                                              const char *callid,
                                              callee_dev_info_t **out_callees,
                                              int *out_count)
{
    if (out_callees == NULL || out_count == NULL)
    {
        return -EINVAL;
    }
    *out_callees = NULL;
    *out_count = 0;
    if (handle == NULL || !IS_VALID_STRING(callid))
    {
        return 0;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->cached_invite_callees != NULL &&
        handle->cached_invite_callee_count > 0 &&
        strcmp(handle->cached_invite_callid, callid) == 0)
    {
        *out_callees = handle->cached_invite_callees;
        *out_count = handle->cached_invite_callee_count;
        handle->cached_invite_callees = NULL;
        handle->cached_invite_callee_count = 0;
        handle->cached_invite_callid[0] = '\0';
    }
    pthread_mutex_unlock(&handle->lock);
    return *out_count;
}

static int kr_aiot_send_secondary_report_detail(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;

    if (handle == NULL || handle->ctx == NULL)
    {
        return 0;
    }

    memset(&msg, 0, sizeof(msg));
    kr_aiot_fill_report_detail(handle, &msg);
    kr_aiot_fill_secondary_proxy(handle, &msg);
    LOGI("secondary_report_detail proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
         msg.proxy_sn,
         msg.proxy_model,
         msg.proxy_kind);
    return goblin_aiot_send_message(handle->ctx, &msg);
}

static int kr_aiot_valid_timeout_or_default(int value, int fallback)
{
    return value > 0 ? value : fallback;
}

static void kr_aiot_fill_report_setting(kr_aiot_handle_t *handle,
                                        goblin_aiot_msg_data_t *msg)
{
    msg->msg_type = GOBLIN_AIOT_REPORT_SETTING;
    safe_strncpy(msg->data.report_setting.language,
                 "ko",
                 sizeof(msg->data.report_setting.language));
    msg->data.report_setting.call = true;
    msg->data.report_setting.call_manager = true;
    msg->data.report_setting.call_play_early_media_timeout = 10;
    msg->data.report_setting.call_ring_timeout =
        handle ? handle->call_ring_timeout : 30;
    msg->data.report_setting.call_converse_timeout =
        handle ? handle->call_converse_timeout : 150;
    msg->data.report_setting.hang_up_call_after_open = false;
    msg->data.report_setting.select_callee = false;
    safe_strncpy(msg->data.report_setting.call_order,
                 "sequential",
                 sizeof(msg->data.report_setting.call_order));
    msg->data.report_setting.visitor_remote_open = true;
    msg->data.report_setting.password_open_enabled = false;
    msg->data.report_setting.qrcode_open = false;
    safe_strncpy(msg->data.report_setting.open_voltage,
                 "nc",
                 sizeof(msg->data.report_setting.open_voltage));
    msg->data.report_setting.open_delay = 1;
    msg->data.report_setting.door_magnetic_detection = false;
    msg->data.report_setting.door_magnetic_alarm_grace_period = 0;
    msg->data.report_setting.tamper_detection = false;
    msg->data.report_setting.qrcode_scan_timeout = 0;
    msg->data.report_setting.upload_log = true;
    safe_strncpy(msg->data.report_setting.net,
                 "wired",
                 sizeof(msg->data.report_setting.net));
    msg->data.report_setting.microphone_volume = 100;
    msg->data.report_setting.speaker_volume = 100;
}

static int kr_aiot_send_report_setting(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;

    if (handle == NULL || handle->ctx == NULL)
    {
        return 0;
    }

    memset(&msg, 0, sizeof(msg));
    kr_aiot_fill_report_setting(handle, &msg);
    LOGI("report_setting send\n");
    return goblin_aiot_send_message(handle->ctx, &msg);
}

static int kr_aiot_send_secondary_report_setting(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;

    if (handle == NULL || handle->ctx == NULL)
    {
        return 0;
    }

    memset(&msg, 0, sizeof(msg));
    kr_aiot_fill_report_setting(handle, &msg);
    kr_aiot_fill_secondary_proxy(handle, &msg);
    LOGI("secondary_report_setting proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
         msg.proxy_sn,
         msg.proxy_model,
         msg.proxy_kind);
    return goblin_aiot_send_message(handle->ctx, &msg);
}

static int kr_aiot_send_secondary_login_message(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    int rc;

    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_LOGIN;
    msg.data.login.low_power = false;
    kr_aiot_fill_secondary_proxy(handle, &msg);

    LOGI("secondary_login proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
         msg.proxy_sn,
         msg.proxy_model,
         msg.proxy_kind);
    rc = goblin_aiot_send_message(handle->ctx, &msg);
    if (rc != 0)
    {
        LOGE("secondary_login send failed rc=%d proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
             rc,
             msg.proxy_sn,
             msg.proxy_model,
             msg.proxy_kind);
    }
    return rc;
}

static int kr_aiot_send_secondary_logout_message(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    int rc;

    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_LOGOUT;
    kr_aiot_fill_secondary_logout_proxy(handle, &msg);

    LOGI("secondary_logout proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
         msg.proxy_sn,
         msg.proxy_model,
         msg.proxy_kind);
    rc = goblin_aiot_send_message(handle->ctx, &msg);
    if (rc != 0)
    {
        LOGE("secondary_logout send failed rc=%d proxy_sn=%s proxy_model=%s proxy_kind=%s\n",
             rc,
             msg.proxy_sn,
             msg.proxy_model,
             msg.proxy_kind);
    }
    return rc;
}

static kr_aiot_secondary_action_t kr_aiot_prepare_secondary_action_locked(kr_aiot_handle_t *handle)
{
    if (handle->ctx == NULL || !handle->parent_logged_in)
    {
        return KR_AIOT_SECONDARY_ACTION_NONE;
    }

    if (handle->secondary_source_online)
    {
        if (!IS_VALID_STRING(handle->secondary_proxy_sn))
        {
            return KR_AIOT_SECONDARY_ACTION_NONE;
        }
        if (!handle->secondary_logged_in &&
            !handle->secondary_login_pending &&
            !handle->secondary_logout_pending)
        {
            handle->secondary_login_pending = true;
            return KR_AIOT_SECONDARY_ACTION_LOGIN;
        }
        return KR_AIOT_SECONDARY_ACTION_NONE;
    }

    if ((handle->secondary_logged_in || handle->secondary_login_pending) &&
        !handle->secondary_logout_pending)
    {
        handle->secondary_logout_pending = true;
        return KR_AIOT_SECONDARY_ACTION_LOGOUT;
    }

    return KR_AIOT_SECONDARY_ACTION_NONE;
}

static int kr_aiot_perform_secondary_action(kr_aiot_handle_t *handle,
                                            kr_aiot_secondary_action_t action)
{
    int rc = 0;

    switch (action)
    {
    case KR_AIOT_SECONDARY_ACTION_LOGIN:
        rc = kr_aiot_send_secondary_login_message(handle);
        if (rc != 0)
        {
            pthread_mutex_lock(&handle->lock);
            handle->secondary_login_pending = false;
            pthread_mutex_unlock(&handle->lock);
        }
        break;
    case KR_AIOT_SECONDARY_ACTION_LOGOUT:
        rc = kr_aiot_send_secondary_logout_message(handle);
        if (rc != 0)
        {
            pthread_mutex_lock(&handle->lock);
            handle->secondary_logout_pending = false;
            pthread_mutex_unlock(&handle->lock);
        }
        break;
    case KR_AIOT_SECONDARY_ACTION_NONE:
    default:
        break;
    }

    return rc;
}

static int kr_aiot_format_timezone(char *formatted, size_t formatted_len)
{
    time_t t = time(NULL);
    struct tm local_tm;
    char buffer[16] = {0};

    if (localtime_r(&t, &local_tm) == NULL)
    {
        return -1;
    }
    if (strftime(buffer, sizeof(buffer), "%z", &local_tm) != 5)
    {
        return -1;
    }

    snprintf(formatted, formatted_len, "%.3s:%s", buffer, buffer + 3);
    return 0;
}

static void kr_aiot_send_sync_time(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_SYNC_TIME;
    if (kr_aiot_format_timezone(msg.data.sync_time.timezone,
                                sizeof(msg.data.sync_time.timezone)) != 0)
    {
        snprintf(msg.data.sync_time.timezone, sizeof(msg.data.sync_time.timezone), "+08:00");
    }
    (void)goblin_aiot_send_message(handle->ctx, &msg);
}

static void kr_aiot_request_webrtc_account_internal(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_GET_LICENSE_RH_WEBRTC;
    (void)goblin_aiot_send_message(handle->ctx, &msg);
}

static void kr_aiot_send_upgrade_request(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    if (handle == NULL || handle->ctx == NULL)
    {
        return;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_CMD_UPGRADE;
    msg.data.upgrade.scene = AIOT_UPGRADE_SCENE_REMOTE;
    msg.data.upgrade.packaging = AIOT_UPGRADE_PACKAGING_APP;
    safe_strncpy(msg.data.upgrade.version,
                 handle->cfg.application_version,
                 sizeof(msg.data.upgrade.version));
    safe_strncpy(msg.data.upgrade.hardware_version,
                 handle->cfg.hardware_version,
                 sizeof(msg.data.upgrade.hardware_version));
    LOGI("upgrade request scene=remote packaging=app version=%s hardware=%s model=%s\n",
         msg.data.upgrade.version,
         msg.data.upgrade.hardware_version,
         SAFE_STRING(handle->cfg.model));
    (void)goblin_aiot_send_message(handle->ctx, &msg);
}

static void kr_aiot_send_login(kr_aiot_handle_t *handle)
{
    goblin_aiot_msg_data_t msg;
    if (handle == NULL || handle->ctx == NULL)
    {
        return;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_LOGIN;
    safe_strncpy(msg.data.login.last_session_id, handle->last_session_id, sizeof(msg.data.login.last_session_id));
    msg.data.login.low_power = false;
    (void)goblin_aiot_send_message(handle->ctx, &msg);
}

static void kr_aiot_timespec_after_seconds(struct timespec *ts, int seconds)
{
    if (ts == NULL)
    {
        return;
    }
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += seconds;
}

static void *kr_aiot_login_retry_thread(void *arg)
{
    kr_aiot_handle_t *handle = (kr_aiot_handle_t *)arg;

    if (handle == NULL)
    {
        return NULL;
    }

    LOGI("login retry thread start interval=%d\n", KR_AIOT_LOGIN_RETRY_INTERVAL_SEC);
    pthread_mutex_lock(&handle->login_retry_lock);
    while (!handle->login_retry_stop)
    {
        struct timespec ts;
        int wait_rc;

        kr_aiot_timespec_after_seconds(&ts, KR_AIOT_LOGIN_RETRY_INTERVAL_SEC);
        wait_rc = pthread_cond_timedwait(&handle->login_retry_cond,
                                         &handle->login_retry_lock,
                                         &ts);
        if (handle->login_retry_stop)
        {
            break;
        }
        if (wait_rc == 0)
        {
            continue;
        }
        pthread_mutex_unlock(&handle->login_retry_lock);
        LOGI("login retry send\n");
        kr_aiot_send_login(handle);
        pthread_mutex_lock(&handle->login_retry_lock);
    }
    handle->login_retry_running = false;
    pthread_mutex_unlock(&handle->login_retry_lock);
    LOGI("login retry thread exit\n");
    return NULL;
}

static void kr_aiot_start_login_retry(kr_aiot_handle_t *handle)
{
    int rc;

    if (handle == NULL || !handle->login_retry_sync_ready)
    {
        return;
    }

    pthread_mutex_lock(&handle->login_retry_lock);
    if (handle->login_retry_running)
    {
        pthread_mutex_unlock(&handle->login_retry_lock);
        return;
    }
    handle->login_retry_stop = false;
    handle->login_retry_running = true;
    rc = pthread_create(&handle->login_retry_thread,
                        NULL,
                        kr_aiot_login_retry_thread,
                        handle);
    if (rc != 0)
    {
        handle->login_retry_running = false;
        LOGE("login retry thread create failed rc=%d\n", rc);
    }
    else
    {
        LOGI("login retry scheduled interval=%d\n", KR_AIOT_LOGIN_RETRY_INTERVAL_SEC);
    }
    pthread_mutex_unlock(&handle->login_retry_lock);
}

static void kr_aiot_stop_login_retry(kr_aiot_handle_t *handle)
{
    bool should_join;
    pthread_t thread;

    if (handle == NULL || !handle->login_retry_sync_ready)
    {
        return;
    }

    pthread_mutex_lock(&handle->login_retry_lock);
    should_join = handle->login_retry_running;
    thread = handle->login_retry_thread;
    handle->login_retry_stop = true;
    pthread_cond_signal(&handle->login_retry_cond);
    pthread_mutex_unlock(&handle->login_retry_lock);

    if (should_join)
    {
        LOGI("login retry join begin\n");
        pthread_join(thread, NULL);
        LOGI("login retry join done\n");
    }
}

static void kr_aiot_set_unsupported_msg(goblin_aiot_msg_data_t *msg, const char *reason)
{
    if (!msg->is_cloud_request)
    {
        return;
    }
    msg->ack_code = AIOT_MSG_ACK_FAIL;
    safe_strncpy(msg->reason, reason, sizeof(msg->reason));
}

static void kr_aiot_set_unsupported_callx(goblin_aiot_callx_data_t *msg, const char *reason)
{
    if (!msg->is_cloud_request)
    {
        return;
    }
    msg->ack_code = AIOT_CALLX_ACK_FAIL;
    safe_strncpy(msg->reason, reason, sizeof(msg->reason));
}

static void kr_aiot_conn_status_cb(aiot_conn_status_t status, void *userdata)
{
    kr_aiot_handle_t *handle = (kr_aiot_handle_t *)userdata;
    kr_aiot_event_t event;
    const char *status_text = "unknown";
    switch (status)
    {
    case GOBLIN_AIOT_CONNECT_SUCCESS:
        status_text = "connect_success";
        break;
    case GOBLIN_AIOT_CONNECT_FAIL:
        status_text = "connect_fail";
        break;
    case GOBLIN_AIOT_OFFLINE:
        status_text = "offline";
        break;
    default:
        break;
    }
    LOGI("conn_status status=%d text=%s\n", (int)status, status_text);
    memset(&event, 0, sizeof(event));
    event.kind = KR_AIOT_EVENT_CONN_STATUS;
    event.conn_status = (int)status;
    (void)event_queue_push(&handle->events, &event);

    pthread_mutex_lock(&handle->lock);
    handle->parent_logged_in = false;
    handle->secondary_login_pending = false;
    handle->secondary_logged_in = false;
    handle->secondary_logout_pending = false;
    handle->secondary_logout_proxy_sn[0] = '\0';
    handle->secondary_logout_proxy_model[0] = '\0';
    handle->call_busy = false;
    pthread_mutex_unlock(&handle->lock);

    if (status == GOBLIN_AIOT_CONNECT_SUCCESS)
    {
        kr_aiot_stop_login_retry(handle);
        kr_aiot_send_login(handle);
    }
}

static void kr_aiot_msg_cb(goblin_aiot_msg_data_t *msg, void *userdata)
{
    kr_aiot_handle_t *handle = (kr_aiot_handle_t *)userdata;
    kr_aiot_event_t event;
    memset(&event, 0, sizeof(event));
    event.kind = KR_AIOT_EVENT_MESSAGE;
    event.msg_type = (int)msg->msg_type;
    event.ack_code = (int)msg->ack_code;
    event.is_cloud_request = msg->is_cloud_request ? 1 : 0;
    event.seq = msg->seq;

    switch (msg->msg_type)
    {
    case GOBLIN_AIOT_LOGIN_ACK:
    {
        kr_aiot_secondary_action_t secondary_action = KR_AIOT_SECONDARY_ACTION_NONE;
        bool secondary_ack;
        bool secondary_source_online = false;
        bool send_secondary_report = false;

        pthread_mutex_lock(&handle->lock);
        secondary_ack = IS_VALID_STRING(msg->proxy_sn) ||
                        (handle->secondary_login_pending && handle->parent_logged_in);
        if (secondary_ack)
        {
            const char *proxy_sn = IS_VALID_STRING(msg->proxy_sn) ?
                                   msg->proxy_sn : handle->secondary_proxy_sn;
            handle->secondary_login_pending = false;
            safe_strncpy(event.text1, proxy_sn, sizeof(event.text1));
            secondary_source_online = handle->secondary_source_online;
            if (msg->ack_code == AIOT_MSG_ACK_OK)
            {
                handle->secondary_logged_in = true;
                if (handle->secondary_source_online)
                {
                    send_secondary_report = true;
                }
                else if (!handle->secondary_logout_pending)
                {
                    handle->secondary_logout_pending = true;
                    secondary_action = KR_AIOT_SECONDARY_ACTION_LOGOUT;
                }
            }
            else
            {
                handle->secondary_logged_in = false;
            }
        }
        else
        {
            safe_strncpy(handle->last_session_id, msg->data.login_ack.session_id, sizeof(handle->last_session_id));
            safe_strncpy(event.text1, msg->data.login_ack.session_id, sizeof(event.text1));
            if (msg->ack_code == AIOT_MSG_ACK_OK)
            {
                handle->parent_logged_in = true;
                secondary_action = kr_aiot_prepare_secondary_action_locked(handle);
            }
            else
            {
                handle->parent_logged_in = false;
                handle->secondary_login_pending = false;
                handle->secondary_logged_in = false;
                handle->secondary_logout_pending = false;
            }
        }
        pthread_mutex_unlock(&handle->lock);

        if (secondary_ack)
        {
            LOGI("secondary_login_ack ack=%d proxy_sn=%s source_online=%d\n",
                 msg->ack_code,
                 event.text1,
                 secondary_source_online ? 1 : 0);
            if (send_secondary_report)
            {
                (void)kr_aiot_send_secondary_report_detail(handle);
                (void)kr_aiot_send_secondary_report_setting(handle);
            }
            (void)kr_aiot_perform_secondary_action(handle, secondary_action);
            break;
        }

        if (msg->ack_code == AIOT_MSG_ACK_OK)
        {
            kr_aiot_stop_login_retry(handle);
            goblin_aiot_msg_data_t report;
            memset(&report, 0, sizeof(report));
            kr_aiot_fill_report_detail(handle, &report);
            (void)goblin_aiot_send_message(handle->ctx, &report);
            (void)kr_aiot_send_report_setting(handle);
            kr_aiot_send_sync_time(handle);
            kr_aiot_request_webrtc_account_internal(handle);
            (void)kr_aiot_perform_secondary_action(handle, secondary_action);
        }
        else if (!secondary_ack)
        {
            LOGW("parent login ack failed ack=%d reason=%s; retry in %d sec\n",
                 msg->ack_code,
                 SAFE_STRING(msg->reason),
                 KR_AIOT_LOGIN_RETRY_INTERVAL_SEC);
            kr_aiot_start_login_retry(handle);
        }
        break;
    }
    case GOBLIN_AIOT_LOGOUT_ACK:
    {
        kr_aiot_secondary_action_t secondary_action = KR_AIOT_SECONDARY_ACTION_NONE;
        bool secondary_ack;

        pthread_mutex_lock(&handle->lock);
        secondary_ack = IS_VALID_STRING(msg->proxy_sn) || handle->secondary_logout_pending;
        if (secondary_ack)
        {
            const char *proxy_sn = IS_VALID_STRING(msg->proxy_sn) ?
                                   msg->proxy_sn :
                                   (IS_VALID_STRING(handle->secondary_logout_proxy_sn) ?
                                    handle->secondary_logout_proxy_sn : handle->secondary_proxy_sn);
            safe_strncpy(event.text1, proxy_sn, sizeof(event.text1));
            handle->secondary_logout_pending = false;
            handle->secondary_logged_in = false;
            handle->secondary_logout_proxy_sn[0] = '\0';
            handle->secondary_logout_proxy_model[0] = '\0';
            secondary_action = kr_aiot_prepare_secondary_action_locked(handle);
        }
        pthread_mutex_unlock(&handle->lock);

        if (secondary_ack)
        {
            LOGI("secondary_logout_ack ack=%d proxy_sn=%s\n",
                 msg->ack_code,
                 event.text1);
            (void)kr_aiot_perform_secondary_action(handle, secondary_action);
        }
        break;
    }
    case GOBLIN_AIOT_REPORT_DETAIL_ACK:
        break;
    case GOBLIN_AIOT_SYNC_TIME_ACK:
        event.kind = KR_AIOT_EVENT_SYNC_TIME;
        safe_strncpy(event.text1, msg->data.sync_time_ack.timezone, sizeof(event.text1));
        safe_strncpy(event.text2, msg->data.sync_time_ack.time, sizeof(event.text2));
        break;
    case GOBLIN_AIOT_GET_LICENSE_RH_WEBRTC_ACK:
        event.kind = KR_AIOT_EVENT_WEBRTC_ACCOUNT;
        safe_strncpy(event.text1, msg->data.rh_webrtc_ack.license, sizeof(event.text1));
        safe_strncpy(event.text2, msg->data.rh_webrtc_ack.addr, sizeof(event.text2));
        safe_strncpy(event.text3, msg->data.rh_webrtc_ack.init_string, sizeof(event.text3));
        LOGI(
            "aiot-webrtc-account: ack=%d license_len=%zu addr_len=%zu init_len=%zu copied_addr_len=%zu copied_init_len=%zu\n",
            msg->ack_code,
            strlen(msg->data.rh_webrtc_ack.license),
            msg->data.rh_webrtc_ack.addr != NULL ? strlen(msg->data.rh_webrtc_ack.addr) : 0,
            msg->data.rh_webrtc_ack.init_string != NULL ? strlen(msg->data.rh_webrtc_ack.init_string) : 0,
            strlen(event.text2),
            strlen(event.text3)
        );
        break;
    case GOBLIN_AIOT_CMD_REBOOT:
        msg->ack_code = AIOT_MSG_ACK_OK;
        event.kind = KR_AIOT_EVENT_REBOOT;
        break;
    case GOBLIN_AIOT_CMD_RESET:
        msg->ack_code = AIOT_MSG_ACK_OK;
        event.kind = KR_AIOT_EVENT_RESET;
        break;
    case GOBLIN_AIOT_CMD_EXEC:
        if (strcmp(msg->data.exec.command, "upgrade") == 0)
        {
            msg->ack_code = AIOT_MSG_ACK_OK;
            LOGI("exec upgrade received seq=%llu\n", (unsigned long long)msg->seq);
            kr_aiot_send_upgrade_request(handle);
        }
        else
        {
            kr_aiot_set_unsupported_msg(msg, "unsupported");
        }
        break;
    case GOBLIN_AIOT_CMD_UPGRADE_ACK:
        LOGI("upgrade ack ack=%d should=%d packaging=%d version=%s hardware=%s url=%s\n",
             msg->ack_code,
             msg->data.upgrade_ack.should ? 1 : 0,
             (int)msg->data.upgrade_ack.packaging,
             msg->data.upgrade_ack.version,
             msg->data.upgrade_ack.hardware_version,
             IS_VALID_STRING(msg->data.upgrade_ack.url) ? "present" : "empty");
        if (msg->ack_code == AIOT_MSG_ACK_OK &&
            msg->data.upgrade_ack.should &&
            IS_VALID_STRING(msg->data.upgrade_ack.url))
        {
            event.kind = KR_AIOT_EVENT_UPGRADE;
            event.value0 = 1;
            event.value1 = (int)msg->data.upgrade_ack.packaging;
            safe_strncpy(event.snapshot_download_url,
                         msg->data.upgrade_ack.url,
                         sizeof(event.snapshot_download_url));
            safe_strncpy(event.text1,
                         msg->data.upgrade_ack.md5,
                         sizeof(event.text1));
            safe_strncpy(event.text2,
                         msg->data.upgrade_ack.version,
                         sizeof(event.text2));
        }
        break;
    case GOBLIN_AIOT_CMD_EVAL:
    case GOBLIN_AIOT_CMD_LOG_LIST:
    case GOBLIN_AIOT_CMD_LOG_FETCH:
    case GOBLIN_AIOT_CMD_BOUND:
    case GOBLIN_AIOT_CMD_UNBOUND:
    case GOBLIN_AIOT_CMD_WAKEUP:
    case GOBLIN_AIOT_CMD_SYNCSETTING:
        kr_aiot_set_unsupported_msg(msg, "unsupported");
        break;
    case GOBLIN_AIOT_CMD_LOG_LIST_FREE:
        if (msg->data.log_list_ack.log_paths != NULL)
        {
            int i;
            for (i = 0; i < msg->data.log_list_ack.log_count; ++i)
            {
                bfree(msg->data.log_list_ack.log_paths[i]);
            }
            bfree(msg->data.log_list_ack.log_paths);
            msg->data.log_list_ack.log_paths = NULL;
        }
        if (msg->data.log_list_ack.log_sizes != NULL)
        {
            bfree(msg->data.log_list_ack.log_sizes);
            msg->data.log_list_ack.log_sizes = NULL;
        }
        break;
    case GOBLIN_AIOT_CMD_EVAL_ACK_FREE:
        if (msg->data.eval_ack.directive != NULL)
        {
            bfree(msg->data.eval_ack.directive);
            msg->data.eval_ack.directive = NULL;
        }
        break;
    case GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS_FREE:
        if (msg->data.playbacks_ack.times != NULL)
        {
            bfree(msg->data.playbacks_ack.times);
            msg->data.playbacks_ack.times = NULL;
        }
        break;
    default:
        break;
    }

    (void)event_queue_push(&handle->events, &event);
}

static void kr_aiot_callx_cb(void *data, void *userdata)
{
    goblin_aiot_callx_data_t *msg = (goblin_aiot_callx_data_t *)data;
    kr_aiot_handle_t *handle = (kr_aiot_handle_t *)userdata;
    kr_aiot_event_t event;
    int queue_rc;
    memset(&event, 0, sizeof(event));
    event.kind = KR_AIOT_EVENT_CALLX;
    event.callx_type = (int)msg->callx_type;
    event.ack_code = (int)msg->ack_code;
    event.is_cloud_request = msg->is_cloud_request ? 1 : 0;
    event.seq = msg->seq;
    safe_strncpy(event.callid, msg->callid, sizeof(event.callid));
    LOGI("callx_cb enter type=%d callid=%s proxy_sn=%s cloud=%d seq=%llu\n",
         (int)msg->callx_type,
         SAFE_STRING(msg->callid),
         SAFE_STRING(msg->proxy_sn),
         msg->is_cloud_request ? 1 : 0,
         (unsigned long long)msg->seq);

    if (IS_VALID_STRING(msg->proxy_sn))
    {
        LOGW("callx unsupported_proxy_sn=%s type=%d callid=%s\n",
             msg->proxy_sn,
             (int)msg->callx_type,
             msg->callid);
        kr_aiot_set_unsupported_callx(msg, "unsupported_proxy_sn");
        return;
    }

    switch (msg->callx_type)
    {
    case GOBLIN_AIOT_CALLX_INITIATE_ACK:
        if (msg->data.initiate_ack.rounds_info != NULL &&
            msg->data.initiate_ack.rounds_num > 0 &&
            msg->data.initiate_ack.rounds_info[0].callees != NULL &&
            msg->data.initiate_ack.rounds_info[0].callees_num > 0)
        {
            callee_rounds_info_t *round = &msg->data.initiate_ack.rounds_info[0];
            callee_dev_info_t *callee = &round->callees[0];
            int cached_count;
            bool app_only;
            pthread_mutex_lock(&handle->lock);
            app_only = handle->resident_app_invite_filter_active;
            handle->resident_app_invite_filter_active = false;
            cached_count = kr_aiot_cache_initiate_ack_round_locked(handle, event.callid, round, app_only);
            pthread_mutex_unlock(&handle->lock);
            LOGI("CALLX_INITIATE_ACK cached callees callid=%s total=%d cached=%d app_only=%d\n",
                 event.callid,
                 round->callees_num,
                 cached_count,
                 app_only ? 1 : 0);
            safe_strncpy(event.text1, callee->sn, sizeof(event.text1));
            safe_strncpy(event.text2, callee->mode, sizeof(event.text2));
            safe_strncpy(event.text3, callee->alias, sizeof(event.text3));
            safe_strncpy(event.callee_kind, callee->kind, sizeof(event.callee_kind));
            safe_strncpy(event.callee_user_id, callee->user_id, sizeof(event.callee_user_id));
            safe_strncpy(event.callee_web_username, callee->web_username, sizeof(event.callee_web_username));
            safe_strncpy(event.callee_web_session_id, callee->web_sessionId, sizeof(event.callee_web_session_id));
            event.callee_channel = callee->channel;
            event.callee_count = cached_count > 0 ? cached_count : round->callees_num;
            event.value0 = round->ring_timeout;
            event.value1 = round->converse_timeout;
        }
        break;
    case GOBLIN_AIOT_CALLX_INVITE_ACK:
        break;
    case GOBLIN_AIOT_CALLX_OPEN_LOCK:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        event.kind = KR_AIOT_EVENT_OPEN_LOCK;
        event.value0 = msg->data.open_lock.channel;
        event.value1 = msg->data.open_lock.lock_number;
        break;
    case GOBLIN_AIOT_CALLX_INVITED:
    {
        char caller_sn[64];
        char caller_device_kind[64];

        safe_strncpy(caller_sn, msg->data.invited.caller_sn, sizeof(caller_sn));
        safe_strncpy(caller_device_kind,
                     msg->data.invited.caller_device_kind,
                     sizeof(caller_device_kind));
        pthread_mutex_lock(&handle->lock);
        if (handle->call_busy)
        {
            pthread_mutex_unlock(&handle->lock);
            msg->ack_code = AIOT_CALLX_ACK_OK;
            msg->data.invited_ack.able = false;
            safe_strncpy(msg->data.invited_ack.unable_cause,
                         "busy",
                         sizeof(msg->data.invited_ack.unable_cause));
            msg->data.invited_ack.ring_timeout = 0;
            msg->data.invited_ack.converse_timeout = 0;
            LOGI("CALLX_INVITED busy reject callid=%s caller_sn=%s caller_device_kind=%s\n",
                 event.callid,
                 caller_sn,
                 caller_device_kind);
            (void)kr_aiot_send_callx_hangup(handle, event.callid, true, "busy");
            return;
        }
        pthread_mutex_unlock(&handle->lock);
        msg->ack_code = AIOT_CALLX_ACK_OK;
        safe_strncpy(event.text1, msg->data.invited.mode, sizeof(event.text1));
        safe_strncpy(event.text2, caller_sn, sizeof(event.text2));
        safe_strncpy(event.caller_device_kind, caller_device_kind, sizeof(event.caller_device_kind));
        safe_strncpy(event.snapshot_download_url,
                     msg->data.invited.snapshot_upload_url,
                     sizeof(event.snapshot_download_url));
        LOGI("CALLX_INVITED callid=%s caller_sn=%s caller_device_kind=%s snapshot_url=%s\n",
             event.callid,
             event.text2,
             event.caller_device_kind,
             IS_VALID_STRING(event.snapshot_download_url) ? "present" : "empty");
        // event.value0 = msg->data.invited.ring_timeout;
        // event.value1 = msg->data.invited.converse_timeout;
        event.value0 = 120;
        event.value1 = 150;
        break;
    }
    case GOBLIN_AIOT_CALLX_REPLIED:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        event.value0 = msg->data.replied.able ? 1 : 0;
        event.value1 = msg->data.replied.ring_timeout;
        safe_strncpy(event.text1, msg->data.replied.unable_cause, sizeof(event.text1));
        safe_strncpy(event.callee_user_id,
                     msg->data.replied.callee_user_id,
                     sizeof(event.callee_user_id));
        safe_strncpy(event.callee_web_username,
                     msg->data.replied.callee_web_username,
                     sizeof(event.callee_web_username));
        safe_strncpy(event.callee_web_session_id,
                     msg->data.replied.callee_web_sessionId,
                     sizeof(event.callee_web_session_id));
        event.callee_channel = msg->data.replied.callee_channel;
        if (IS_VALID_STRING(msg->data.replied.callee_user_id))
        {
            safe_strncpy(event.callee_kind, "app", sizeof(event.callee_kind));
        }
        else if (IS_VALID_STRING(msg->data.replied.callee_web_username))
        {
            safe_strncpy(event.callee_kind, "web", sizeof(event.callee_kind));
        }
        else if (IS_VALID_STRING(msg->data.replied.callee_sn))
        {
            safe_strncpy(event.callee_kind, "device", sizeof(event.callee_kind));
            safe_strncpy(event.text2, msg->data.replied.callee_sn, sizeof(event.text2));
        }
        if (!msg->data.replied.able)
        {
            kr_aiot_restore_default_callx_config_if_needed(handle);
        }
        break;
    case GOBLIN_AIOT_CALLX_SNAPSHOTTED:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        safe_strncpy(event.text1, msg->data.snapshotted.url, sizeof(event.text1));
        break;
    case GOBLIN_AIOT_CALLX_ACCEPTED:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        safe_strncpy(event.text1, msg->data.accepted.caller_media, sizeof(event.text1));
        safe_strncpy(event.text2, msg->data.accepted.callee_media, sizeof(event.text2));
        break;
    case GOBLIN_AIOT_CALLX_CONFIRMED:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        event.value0 = msg->data.confirmed.able ? 1 : 0;
        safe_strncpy(event.text1, msg->data.confirmed.mode, sizeof(event.text1));
        if (msg->data.confirmed.able)
        {
            safe_strncpy(event.text2, msg->data.confirmed.opposite_webrtc_license, sizeof(event.text2));
            safe_strncpy(event.text3, msg->data.confirmed.webrtc_session_id, sizeof(event.text3));
        }
        else
        {
            safe_strncpy(event.text2, msg->data.confirmed.unable_cause, sizeof(event.text2));
        }
        break;
    case GOBLIN_AIOT_CALLX_HUNGUP:
        msg->ack_code = AIOT_CALLX_ACK_OK;
        kr_aiot_restore_default_callx_config_if_needed(handle);
        event.value0 = msg->data.hungup.entirely ? 1 : 0;
        safe_strncpy(event.text1, msg->data.hungup.cause, sizeof(event.text1));
        safe_strncpy(event.text2, msg->data.hungup.callee_sn, sizeof(event.text2));
        LOGI("CALLX_HUNGUP callid=%s cause=%s entirely=%d callee_sn=%s\n",
             event.callid,
             event.text1,
             event.value0,
             event.text2);
        break;
    default:
        break;
    }

    queue_rc = event_queue_push(&handle->events, &event);
    if (queue_rc < 0)
    {
        LOGE("callx_cb queue failed rc=%d type=%d callid=%s\n",
             queue_rc,
             event.callx_type,
             event.callid);
    }
    else
    {
        LOGI("callx_cb queued type=%d callid=%s seq=%llu\n",
             event.callx_type,
             event.callid,
             (unsigned long long)event.seq);
    }
}

static void kr_aiot_timeout_cb(goblin_aiot_msg_data_t *data, void *userdata)
{
    kr_aiot_handle_t *handle = (kr_aiot_handle_t *)userdata;
    kr_aiot_event_t event;
    memset(&event, 0, sizeof(event));
    event.kind = KR_AIOT_EVENT_ERROR;
    event.msg_type = (int)data->msg_type;
    event.seq = data->seq;
    safe_strncpy(event.text1, "timeout", sizeof(event.text1));
    (void)event_queue_push(&handle->events, &event);
}

static int kr_aiot_reconnect_time_cb(int retry_time, void *userdata)
{
    (void)userdata;
    if (retry_time >= 8)
    {
        return -1;
    }
    return 30 + retry_time * 30;
}

static int kr_aiot_log_read_cb(aiot_log_type_t type, int log_id, char *buf, int size, void *userdata)
{
    (void)type;
    (void)log_id;
    (void)buf;
    (void)size;
    (void)userdata;
    return 0;
}

static void kr_aiot_set_callx_defaults(kr_aiot_handle_t *handle)
{
    aiot_callx_config_t callx_cfg;
    memset(&callx_cfg, 0, sizeof(callx_cfg));
    callx_cfg.can_open = false;
    callx_cfg.can_liftcall = false;
    callx_cfg.can_cancelalarm = false;
    snprintf(callx_cfg.supported_modes[0], sizeof(callx_cfg.supported_modes[0]), "webrtc");
    snprintf(callx_cfg.supported_medias[0], sizeof(callx_cfg.supported_medias[0]), "video");
    snprintf(callx_cfg.supported_medias[1], sizeof(callx_cfg.supported_medias[1]), "audio");
    snprintf(callx_cfg.supported_medias[2], sizeof(callx_cfg.supported_medias[2]), "none");
    snprintf(callx_cfg.required_medias[0], sizeof(callx_cfg.required_medias[0]), "video");
    snprintf(callx_cfg.required_medias[1], sizeof(callx_cfg.required_medias[1]), "audio");
    snprintf(callx_cfg.required_medias[2], sizeof(callx_cfg.required_medias[2]), "none");
    (void)goblin_aiot_set_callx_config(handle->ctx, &callx_cfg);
}

static int kr_aiot_apply_audio_only_callx_config(kr_aiot_handle_t *handle, const char *reason)
{
    aiot_callx_config_t callx_cfg;
    int rc;

    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }

    memset(&callx_cfg, 0, sizeof(callx_cfg));
    callx_cfg.can_open = false;
    callx_cfg.can_liftcall = false;
    callx_cfg.can_cancelalarm = false;
    snprintf(callx_cfg.supported_modes[0], sizeof(callx_cfg.supported_modes[0]), "webrtc");
    snprintf(callx_cfg.supported_medias[0], sizeof(callx_cfg.supported_medias[0]), "audio");
    snprintf(callx_cfg.required_medias[0], sizeof(callx_cfg.required_medias[0]), "audio");
    rc = goblin_aiot_set_callx_config(handle->ctx, &callx_cfg);
    if (rc == 0)
    {
        LOGI("callx config set audio-only reason=%s\n", SAFE_STRING(reason));
    }
    else
    {
        LOGE("callx config set audio-only failed reason=%s rc=%d\n", SAFE_STRING(reason), rc);
    }
    return rc;
}

static int kr_aiot_apply_manager_audio_callx_config(kr_aiot_handle_t *handle)
{
    int rc = kr_aiot_apply_audio_only_callx_config(handle, "manager");
    if (rc == 0)
    {
        pthread_mutex_lock(&handle->lock);
        handle->manager_audio_callx_config_active = true;
        pthread_mutex_unlock(&handle->lock);
    }
    return rc;
}

static int kr_aiot_apply_resident_audio_callx_config(kr_aiot_handle_t *handle)
{
    aiot_callx_config_t callx_cfg;
    int rc;

    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }

    memset(&callx_cfg, 0, sizeof(callx_cfg));
    callx_cfg.can_open = false;
    callx_cfg.can_liftcall = false;
    callx_cfg.can_cancelalarm = false;
    snprintf(callx_cfg.supported_modes[0], sizeof(callx_cfg.supported_modes[0]), "webrtc");
    snprintf(callx_cfg.supported_medias[0], sizeof(callx_cfg.supported_medias[0]), "video");
    snprintf(callx_cfg.supported_medias[1], sizeof(callx_cfg.supported_medias[1]), "audio");
    snprintf(callx_cfg.required_medias[0], sizeof(callx_cfg.required_medias[0]), "audio");
    rc = goblin_aiot_set_callx_config(handle->ctx, &callx_cfg);
    if (rc == 0)
    {
        pthread_mutex_lock(&handle->lock);
        handle->resident_audio_callx_config_active = true;
        pthread_mutex_unlock(&handle->lock);
        LOGI("callx config set secondary resident supported=video,audio required=audio\n");
    }
    else
    {
        LOGE("callx config set secondary resident failed rc=%d\n", rc);
    }
    return rc;
}

static void kr_aiot_restore_default_callx_config_if_needed(kr_aiot_handle_t *handle)
{
    bool should_restore;
    bool manager_active;
    bool resident_active;

    if (handle == NULL || handle->ctx == NULL)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    manager_active = handle->manager_audio_callx_config_active;
    resident_active = handle->resident_audio_callx_config_active;
    should_restore = manager_active || resident_active;
    handle->manager_audio_callx_config_active = false;
    handle->resident_audio_callx_config_active = false;
    pthread_mutex_unlock(&handle->lock);

    if (should_restore)
    {
        kr_aiot_set_callx_defaults(handle);
        LOGI("callx config restored default after audio-only call manager=%d resident=%d\n",
             manager_active ? 1 : 0,
             resident_active ? 1 : 0);
    }
}

int kr_aiot_create(kr_aiot_handle_t **out_handle, const kr_aiot_config_t *cfg)
{
    kr_aiot_handle_t *handle;
    int rc;
    if (out_handle == NULL || cfg == NULL)
    {
        return -EINVAL;
    }

    handle = (kr_aiot_handle_t *)bzalloc(sizeof(*handle));
    if (handle == NULL)
    {
        return -ENOMEM;
    }

    rc = pthread_mutex_init(&handle->lock, NULL);
    if (rc != 0)
    {
        bfree(handle);
        return -rc;
    }
    handle->lock_ready = true;
    rc = pthread_mutex_init(&handle->login_retry_lock, NULL);
    if (rc != 0)
    {
        pthread_mutex_destroy(&handle->lock);
        bfree(handle);
        return -rc;
    }
    rc = pthread_cond_init(&handle->login_retry_cond, NULL);
    if (rc != 0)
    {
        pthread_mutex_destroy(&handle->login_retry_lock);
        pthread_mutex_destroy(&handle->lock);
        bfree(handle);
        return -rc;
    }
    handle->login_retry_sync_ready = true;
    handle->call_ring_timeout = 30;
    handle->call_converse_timeout = 150;
    rc = event_queue_init(&handle->events, sizeof(kr_aiot_event_t));
    if (rc != 0)
    {
        pthread_cond_destroy(&handle->login_retry_cond);
        pthread_mutex_destroy(&handle->login_retry_lock);
        pthread_mutex_destroy(&handle->lock);
        bfree(handle);
        return rc;
    }
    handle->cfg = *cfg;
    handle->cfg.interface = bstrdup(cfg->interface);
    handle->cfg.sn = bstrdup(cfg->sn);
    handle->cfg.model = bstrdup(cfg->model);
    handle->cfg.server_addr = bstrdup(cfg->server_addr);
    handle->cfg.mqtt_server_addr = bstrdup(cfg->mqtt_server_addr);
    handle->cfg.application_version = bstrdup(cfg->application_version);
    handle->cfg.system_version = bstrdup(cfg->system_version);
    handle->cfg.hardware_version = bstrdup(cfg->hardware_version);
    handle->cfg.mac = bstrdup(cfg->mac);

    if ((cfg->interface != NULL && handle->cfg.interface == NULL) ||
        (cfg->sn != NULL && handle->cfg.sn == NULL) ||
        (cfg->model != NULL && handle->cfg.model == NULL) ||
        (cfg->server_addr != NULL && handle->cfg.server_addr == NULL) ||
        (cfg->mqtt_server_addr != NULL && handle->cfg.mqtt_server_addr == NULL) ||
        (cfg->application_version != NULL && handle->cfg.application_version == NULL) ||
        (cfg->system_version != NULL && handle->cfg.system_version == NULL) ||
        (cfg->hardware_version != NULL && handle->cfg.hardware_version == NULL) ||
        (cfg->mac != NULL && handle->cfg.mac == NULL))
    {
        (void)kr_aiot_destroy(handle);
        return -ENOMEM;
    }

    *out_handle = handle;
    return 0;
}

int kr_aiot_start(kr_aiot_handle_t *handle)
{
    goblin_aiot_config_t cfg;
    LOGI("kr_aiot_start enter handle=%p\n", (void *)handle);
    (void)goblin_aiot_init_mem_hook(kr_aiot_bmalloc, kr_aiot_bfree);
    if (handle == NULL)
    {
        LOGE("kr_aiot_start failed null_handle\n");
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->running)
    {
        pthread_mutex_unlock(&handle->lock);
        LOGI("kr_aiot_start skipped already_running\n");
        return 0;
    }
    event_queue_reset_cancel(&handle->events);
    pthread_mutex_unlock(&handle->lock);

    memset(&cfg, 0, sizeof(cfg));
    safe_strncpy(cfg.interface, handle->cfg.interface, sizeof(cfg.interface));
    safe_strncpy(cfg.sn, handle->cfg.sn, sizeof(cfg.sn));
    safe_strncpy(cfg.model, handle->cfg.model, sizeof(cfg.model));
    cfg.server_addr = (char *)handle->cfg.server_addr;
    cfg.mqtt_server_addr = NULL;// "ssl://192.168.253.40:8883";
    cfg.is_mqtts = handle->cfg.is_mqtts;
    cfg.keepalive = handle->cfg.keepalive;
    cfg.conn_status_cb = kr_aiot_conn_status_cb;
    cfg.msg_cb = kr_aiot_msg_cb;
    cfg.callx_cb = kr_aiot_callx_cb;
    cfg.timeout_cb = kr_aiot_timeout_cb;
    cfg.reconnect_time_cb = kr_aiot_reconnect_time_cb;
    cfg.log_read_cb = kr_aiot_log_read_cb;
    cfg.userdata = handle;

    LOGI(
            "kr_aiot_start init interface=%s sn=%s model=%s server=%s mqtt_server=%s keepalive=%d mqtts=%d\n",
            cfg.interface,
            cfg.sn,
            cfg.model,
            SAFE_STRING(cfg.server_addr),
            SAFE_STRING(cfg.mqtt_server_addr),
            cfg.keepalive,
            cfg.is_mqtts);
    goblin_aiot_log_set_level(AIOT_LOG_DEBUG);
    handle->ctx = goblin_aiot_init(&cfg, GOBLIN_AIOT_VER_STRING);
    if (handle->ctx == NULL)
    {
        LOGE("kr_aiot_start failed goblin_aiot_init_null\n");
        return -EINVAL;
    }

    kr_aiot_set_callx_defaults(handle);

    pthread_mutex_lock(&handle->lock);
    handle->running = true;
    pthread_mutex_unlock(&handle->lock);
    LOGI("kr_aiot_start done running=1\n");
    return 0;
}

int kr_aiot_stop(kr_aiot_handle_t *handle)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    kr_aiot_stop_login_retry(handle);

    pthread_mutex_lock(&handle->lock);
    handle->running = false;
    handle->parent_logged_in = false;
    handle->secondary_login_pending = false;
    handle->secondary_logged_in = false;
    handle->secondary_logout_pending = false;
    handle->secondary_logout_proxy_sn[0] = '\0';
    handle->secondary_logout_proxy_model[0] = '\0';
    kr_aiot_clear_cached_invite_locked(handle);
    pthread_mutex_unlock(&handle->lock);
    kr_aiot_restore_default_callx_config_if_needed(handle);
    event_queue_cancel(&handle->events);

    if (handle->ctx != NULL)
    {
        (void)goblin_aiot_deinit(handle->ctx);
        handle->ctx = NULL;
    }
    return 0;
}

int kr_aiot_destroy(kr_aiot_handle_t *handle)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    (void)kr_aiot_stop(handle);
    event_queue_destroy(&handle->events);

    if (handle->login_retry_sync_ready)
    {
        pthread_cond_destroy(&handle->login_retry_cond);
        pthread_mutex_destroy(&handle->login_retry_lock);
        handle->login_retry_sync_ready = false;
    }
    if (handle->lock_ready)
    {
        pthread_mutex_destroy(&handle->lock);
    }
    bfree((void *)handle->cfg.interface);
    bfree((void *)handle->cfg.sn);
    bfree((void *)handle->cfg.model);
    bfree((void *)handle->cfg.server_addr);
    bfree((void *)handle->cfg.mqtt_server_addr);
    bfree((void *)handle->cfg.application_version);
    bfree((void *)handle->cfg.system_version);
    bfree((void *)handle->cfg.hardware_version);
    bfree((void *)handle->cfg.mac);
    bfree(handle);
    return 0;
}

int kr_aiot_wait_event(kr_aiot_handle_t *handle, kr_aiot_event_t *out_event, int timeout_ms)
{
    if (handle == NULL || out_event == NULL)
    {
        return -EINVAL;
    }

    return event_queue_wait(&handle->events, out_event, timeout_ms);
}

int kr_aiot_request_webrtc_account(kr_aiot_handle_t *handle)
{
    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }
    kr_aiot_request_webrtc_account_internal(handle);
    return 0;
}

int kr_aiot_set_secondary_confirm_online(
    kr_aiot_handle_t *handle,
    bool online,
    const char *proxy_sn,
    const char *proxy_model
)
{
    kr_aiot_secondary_action_t action;
    bool changed;
    bool identity_changed = false;
    bool identity_logout = false;
    bool parent_logged_in;
    bool login_pending;
    bool logged_in;
    bool logout_pending;
    char next_proxy_sn[64] = {0};
    char next_proxy_model[64] = {0};

    if (handle == NULL)
    {
        return -EINVAL;
    }
    if (online)
    {
        if (!IS_VALID_STRING(proxy_sn))
        {
            return -EINVAL;
        }
        safe_strncpy(next_proxy_sn, proxy_sn, sizeof(next_proxy_sn));
        safe_strncpy(next_proxy_model,
                     IS_VALID_STRING(proxy_model) ?
                     proxy_model : KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL,
                     sizeof(next_proxy_model));
    }

    pthread_mutex_lock(&handle->lock);
    if (online)
    {
        identity_changed =
            strcmp(handle->secondary_proxy_sn, next_proxy_sn) != 0 ||
            strcmp(handle->secondary_proxy_model, next_proxy_model) != 0;
        if (identity_changed &&
            IS_VALID_STRING(handle->secondary_proxy_sn) &&
            (handle->secondary_logged_in || handle->secondary_login_pending) &&
            !handle->secondary_logout_pending)
        {
            safe_strncpy(handle->secondary_logout_proxy_sn,
                         handle->secondary_proxy_sn,
                         sizeof(handle->secondary_logout_proxy_sn));
            safe_strncpy(handle->secondary_logout_proxy_model,
                         IS_VALID_STRING(handle->secondary_proxy_model) ?
                         handle->secondary_proxy_model : KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL,
                         sizeof(handle->secondary_logout_proxy_model));
            handle->secondary_logout_pending = true;
            handle->secondary_login_pending = false;
            identity_logout = true;
        }
        safe_strncpy(handle->secondary_proxy_sn,
                     next_proxy_sn,
                     sizeof(handle->secondary_proxy_sn));
        safe_strncpy(handle->secondary_proxy_model,
                     next_proxy_model,
                     sizeof(handle->secondary_proxy_model));
    }
    else if (IS_VALID_STRING(handle->secondary_proxy_sn) &&
             !IS_VALID_STRING(handle->secondary_logout_proxy_sn))
    {
        safe_strncpy(handle->secondary_logout_proxy_sn,
                     handle->secondary_proxy_sn,
                     sizeof(handle->secondary_logout_proxy_sn));
        safe_strncpy(handle->secondary_logout_proxy_model,
                     IS_VALID_STRING(handle->secondary_proxy_model) ?
                     handle->secondary_proxy_model : KR_AIOT_SECONDARY_CONFIRM_DEFAULT_MODEL,
                     sizeof(handle->secondary_logout_proxy_model));
    }
    changed = handle->secondary_source_online != online || identity_changed;
    handle->secondary_source_online = online;
    action = identity_logout ?
             KR_AIOT_SECONDARY_ACTION_LOGOUT :
             kr_aiot_prepare_secondary_action_locked(handle);
    parent_logged_in = handle->parent_logged_in;
    login_pending = handle->secondary_login_pending;
    logged_in = handle->secondary_logged_in;
    logout_pending = handle->secondary_logout_pending;
    pthread_mutex_unlock(&handle->lock);

    LOGI("secondary_source online=%d changed=%d identity_changed=%d parent_logged_in=%d login_pending=%d logged_in=%d logout_pending=%d proxy_sn=%s proxy_model=%s\n",
         online ? 1 : 0,
         changed ? 1 : 0,
         identity_changed ? 1 : 0,
         parent_logged_in ? 1 : 0,
         login_pending ? 1 : 0,
         logged_in ? 1 : 0,
         logout_pending ? 1 : 0,
         online ? next_proxy_sn : SAFE_STRING(proxy_sn),
         online ? next_proxy_model : SAFE_STRING(proxy_model));

    return kr_aiot_perform_secondary_action(handle, action);
}

int kr_aiot_set_call_busy(kr_aiot_handle_t *handle, bool busy)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->call_busy = busy;
    pthread_mutex_unlock(&handle->lock);
    LOGI("call_busy set busy=%d\n", busy ? 1 : 0);
    return 0;
}

int kr_aiot_set_call_timeouts(
    kr_aiot_handle_t *handle,
    int ring_timeout,
    int converse_timeout
)
{
    bool should_report;
    int applied_ring_timeout;
    int applied_converse_timeout;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->call_ring_timeout = ring_timeout >= 0 ? ring_timeout : 30;
    handle->call_converse_timeout = converse_timeout >= 0 ? converse_timeout : 150;
    applied_ring_timeout = handle->call_ring_timeout;
    applied_converse_timeout = handle->call_converse_timeout;
    should_report = handle->parent_logged_in && handle->ctx != NULL;
    pthread_mutex_unlock(&handle->lock);
    LOGI("call timeouts set ring=%d converse=%d\n",
         applied_ring_timeout,
         applied_converse_timeout);
    if (should_report)
    {
        (void)kr_aiot_send_report_setting(handle);
    }
    return 0;
}

int kr_aiot_send_open_ask(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *sn
)
{
    goblin_aiot_msg_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = GOBLIN_AIOT_CMD_OPEN_ASK;
    safe_strncpy(msg.data.open_ask.callid, callid, sizeof(msg.data.open_ask.callid));
    safe_strncpy(msg.data.open_ask.sn, sn, sizeof(msg.data.open_ask.sn));
    LOGI("open_ask callid=%s sn=%s\n",
         msg.data.open_ask.callid,
         msg.data.open_ask.sn);
    return goblin_aiot_send_message(handle->ctx, &msg);
}

int kr_aiot_send_callx_invited_ack(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool able,
    const char *unable_cause,
    int ring_timeout,
    int converse_timeout
)
{
    goblin_aiot_callx_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_INVITED_ACK;
    safe_strncpy(msg.callid, callid, sizeof(msg.callid));
    msg.data.invited_ack.able = able;
    safe_strncpy(msg.data.invited_ack.unable_cause, unable_cause, sizeof(msg.data.invited_ack.unable_cause));
    msg.data.invited_ack.ring_timeout = ring_timeout;
    msg.data.invited_ack.converse_timeout = converse_timeout;
    return goblin_aiot_send_callx_msg(handle->ctx, &msg);
}

int kr_aiot_send_callx_accept(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *caller_media,
    const char *callee_media
)
{
    goblin_aiot_callx_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_ACCEPT;
    safe_strncpy(msg.callid, callid, sizeof(msg.callid));
    safe_strncpy(msg.data.accept.caller_media,
                 caller_media != NULL ? caller_media : "video",
                 sizeof(msg.data.accept.caller_media));
    safe_strncpy(msg.data.accept.callee_media,
                 /*llee_media != NULL ? callee_media : "video"*/"audio",
                 sizeof(msg.data.accept.callee_media));

    msg.data.accept.reverse_underlying_call = false;
    msg.data.accept.no_other_function = false;
    return goblin_aiot_send_callx_msg(handle->ctx, &msg);
}

int kr_aiot_send_callx_hangup(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool entirely,
    const char *cause
)
{
    goblin_aiot_callx_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_HANGUP;
    safe_strncpy(msg.callid, callid, sizeof(msg.callid));
    msg.data.hangup.entirely = entirely;
    msg.data.hangup.cause = (char *)(SAFE_STRING(cause));
    kr_aiot_restore_default_callx_config_if_needed(handle);
    return goblin_aiot_send_callx_msg(handle->ctx, &msg);
}

int kr_aiot_send_callx_initiate(
    kr_aiot_handle_t *handle,
    const char *callee_sn,
    int callee_channel
)
{
    goblin_aiot_callx_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callee_sn == NULL || callee_sn[0] == '\0')
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_INITIATE;
    msg.data.initiate.channel = 1;
    msg.data.initiate.with_snapshot = false;
    msg.data.initiate.to_device = true;
    msg.data.initiate.devs_num = 1;
    msg.data.initiate.devs_info = bzalloc(sizeof(callee_dev_info_t));
    if (msg.data.initiate.devs_info == NULL)
    {
        return -ENOMEM;
    }
    safe_strncpy(msg.data.initiate.devs_info[0].sn, callee_sn, sizeof(msg.data.initiate.devs_info[0].sn));
    msg.data.initiate.devs_info[0].channel = callee_channel;

    {
        int rc = goblin_aiot_send_callx_msg(handle->ctx, &msg);
        bfree(msg.data.initiate.devs_info);
        return rc;
    }
}

int kr_aiot_send_callx_initiate_manager(kr_aiot_handle_t *handle)
{
    goblin_aiot_callx_data_t msg;
    int rc;

    if (handle == NULL || handle->ctx == NULL)
    {
        return -EINVAL;
    }

    rc = kr_aiot_apply_manager_audio_callx_config(handle);
    if (rc < 0)
    {
        return rc;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_INITIATE;
    msg.data.initiate.channel = 1;
    msg.data.initiate.with_snapshot = false;
    safe_strncpy(msg.data.initiate.call_kind, "visitor", sizeof(msg.data.initiate.call_kind));
    msg.data.initiate.to_device = false;
    msg.data.initiate.devs_info = NULL;
    msg.data.initiate.devs_num = 0;
    msg.data.initiate.to_manager = true;

    LOGI("callx initiate manager: to_manager=1 to_device=0 call_kind=visitor channel=1 media=audio");
    rc = goblin_aiot_send_callx_msg(handle->ctx, &msg);
    if (rc < 0)
    {
        kr_aiot_restore_default_callx_config_if_needed(handle);
    }
    return rc;
}

int kr_aiot_send_callx_initiate_resident(
    kr_aiot_handle_t *handle,
    const char *callee_apartment
)
{
    goblin_aiot_callx_data_t msg;
    int rc;

    if (handle == NULL || handle->ctx == NULL ||
        callee_apartment == NULL || callee_apartment[0] == '\0')
    {
        return -EINVAL;
    }

    rc = kr_aiot_apply_resident_audio_callx_config(handle);
    if (rc < 0)
    {
        return rc;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_INITIATE;
    msg.data.initiate.channel = 1;
    msg.data.initiate.with_snapshot = false;
    safe_strncpy(msg.data.initiate.call_kind, "visitor", sizeof(msg.data.initiate.call_kind));
    msg.data.initiate.to_device = false;
    msg.data.initiate.devs_info = NULL;
    msg.data.initiate.devs_num = 0;
    msg.data.initiate.to_manager = false;
    safe_strncpy(msg.data.initiate.callee_apartment,
                 callee_apartment,
                 sizeof(msg.data.initiate.callee_apartment));

    pthread_mutex_lock(&handle->lock);
    handle->resident_app_invite_filter_active = true;
    pthread_mutex_unlock(&handle->lock);

    LOGI("callx initiate resident: callee_apartment=%s to_manager=0 to_device=0 call_kind=visitor channel=1 supported=video,audio required=audio\n",
         callee_apartment);
    rc = goblin_aiot_send_callx_msg(handle->ctx, &msg);
    if (rc < 0)
    {
        pthread_mutex_lock(&handle->lock);
        handle->resident_app_invite_filter_active = false;
        pthread_mutex_unlock(&handle->lock);
        kr_aiot_restore_default_callx_config_if_needed(handle);
    }
    return rc;
}

int kr_aiot_send_callx_invite(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *callee_sn,
    int callee_channel,
    const char *mode,
    int ring_timeout,
    int converse_timeout
)
{
    goblin_aiot_callx_data_t msg;
    callee_dev_info_t *cached_callees = NULL;
    int cached_count = 0;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_INVITE;
    safe_strncpy(msg.callid, callid, sizeof(msg.callid));
    msg.data.invite.ring_timeout = ring_timeout;
    msg.data.invite.converse_timeout = converse_timeout;
    msg.data.invite.round = 0;

    (void)kr_aiot_take_cached_invite_callees(handle, callid, &cached_callees, &cached_count);
    if (cached_callees != NULL && cached_count > 0)
    {
        int i;
        msg.data.invite.dev_num = cached_count;
        msg.data.invite.dev_info = cached_callees;
        for (i = 0; i < cached_count; ++i)
        {
            if (!IS_VALID_STRING(msg.data.invite.dev_info[i].mode))
            {
                safe_strncpy(msg.data.invite.dev_info[i].mode,
                             mode != NULL ? mode : "webrtc",
                             sizeof(msg.data.invite.dev_info[i].mode));
            }
        }
        LOGI("CALLX_INVITE use cached callees callid=%s count=%d\n", callid, cached_count);
    }
    else
    {
        if (!IS_VALID_STRING(callee_sn))
        {
            return -EINVAL;
        }
        msg.data.invite.dev_num = 1;
        msg.data.invite.dev_info = bzalloc(sizeof(callee_dev_info_t));
        if (msg.data.invite.dev_info == NULL)
        {
            return -ENOMEM;
        }
        safe_strncpy(msg.data.invite.dev_info[0].kind, "device", sizeof(msg.data.invite.dev_info[0].kind));
        safe_strncpy(msg.data.invite.dev_info[0].sn, callee_sn, sizeof(msg.data.invite.dev_info[0].sn));
        msg.data.invite.dev_info[0].channel = callee_channel;
        safe_strncpy(msg.data.invite.dev_info[0].mode,
                     mode != NULL ? mode : "webrtc",
                     sizeof(msg.data.invite.dev_info[0].mode));
    }

    {
        int rc = goblin_aiot_send_callx_msg(handle->ctx, &msg);
        bfree(msg.data.invite.dev_info);
        return rc;
    }
}

int kr_aiot_send_callx_terminate(kr_aiot_handle_t *handle, const char *callid)
{
    goblin_aiot_callx_data_t msg;

    if (handle == NULL || handle->ctx == NULL || callid == NULL)
    {
        return -EINVAL;
    }

    memset(&msg, 0, sizeof(msg));
    msg.callx_type = GOBLIN_AIOT_CALLX_TERMINATE;
    safe_strncpy(msg.callid, callid, sizeof(msg.callid));
    kr_aiot_restore_default_callx_config_if_needed(handle);
    return goblin_aiot_send_callx_msg(handle->ctx, &msg);
}

int kr_aiot_download_file(
    kr_aiot_handle_t *handle,
    const char *url,
    const char *output_path,
    int timeout_ms
)
{
    CURL *curl;
    FILE *fp;
    CURLcode rc;
    long timeout;

    if (handle == NULL || url == NULL || url[0] == '\0' ||
        output_path == NULL || output_path[0] == '\0')
    {
        return -EINVAL;
    }

    timeout = timeout_ms > 0 ? timeout_ms : 5000;
    fp = fopen(output_path, "wb");
    if (fp == NULL)
    {
        LOGE("download open output failed path=%s\n", output_path);
        return -errno;
    }

    curl = curl_easy_init();
    if (curl == NULL)
    {
        fclose(fp);
        remove(output_path);
        return -ENOMEM;
    }

    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_URL, url);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_WRITEFUNCTION, kr_aiot_download_write_cb);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_WRITEDATA, fp);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_USERAGENT, "kr-room/1.0");
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_FOLLOWLOCATION, 1L);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_FAILONERROR, 1L);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_CONNECTTIMEOUT_MS, timeout);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_TIMEOUT_MS, timeout);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_NOSIGNAL, 1L);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_SSL_VERIFYPEER, 0L);
    (void)curl_easy_setopt(curl, KR_AIOT_CURL_OPT_SSL_VERIFYHOST, 0L);

    LOGI("download begin url=%s path=%s timeout_ms=%ld\n", url, output_path, timeout);
    rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);

    if (rc != 0)
    {
        LOGE("download failed rc=%d url=%s path=%s\n", (int)rc, url, output_path);
        remove(output_path);
        return -EIO;
    }

    LOGI("download ok path=%s\n", output_path);
    return 0;
}
