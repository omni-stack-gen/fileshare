#ifndef __GOBLIN_IOT_H__
#define __GOBLIN_IOT_H__

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GOBLIN_AIOT_API 	__attribute__ ((visibility ("default")))

#define GOBLIN_AIOT_VER_STRING	"0.1.28"
#define IOT_VERSION_MAJOR        0
#define IOT_VERSION_MINOR        1
#define IOT_VERSION_PATCH        28

typedef enum 
{
    AIOT_LOG_DEBUG = 0,
    AIOT_LOG_INFO,
    AIOT_LOG_WARN,
    AIOT_LOG_ERROR,
    AIOT_LOG_NONE,
} aiot_log_level_t;

typedef enum
{
    // mqtt连接成功
    GOBLIN_AIOT_CONNECT_SUCCESS = 0,
    // mqtt连接失败
    GOBLIN_AIOT_CONNECT_FAIL,
    // mqtt连接断开
    GOBLIN_AIOT_OFFLINE,

} aiot_conn_status_t;

typedef enum 
{
    /* 
    *  device 登入
    *  device ---> cloud
    */
    GOBLIN_AIOT_LOGIN           = 0x1001,

    /***
     * 设备登入ack
     *  cloud ---> device
     */
    GOBLIN_AIOT_LOGIN_ACK,
   
    /*
    *  device 登出
    *  device ---> cloud
    */
    GOBLIN_AIOT_LOGOUT,

    /*
    *  device 登出ack
    *  cloud ---> device
    */
    GOBLIN_AIOT_LOGOUT_ACK,

    /* 
    *  device 上报详情
    *  device ---> cloud
    */
    GOBLIN_AIOT_REPORT_DETAIL,

    /* 
    *  device 上报详情ack
    *  cloud ---> device
    */
    GOBLIN_AIOT_REPORT_DETAIL_ACK,

    /*
    *  device 上报设置
    *  device ---> cloud
    */
    GOBLIN_AIOT_REPORT_SETTING,

    /*
    *  device 上报设置ack
    *  cloud ---> device
    */
    GOBLIN_AIOT_REPORT_SETTING_ACK,

    /* 
    *  获取润华webrtc账号
    *  device ---> cloud
    */
    GOBLIN_AIOT_GET_LICENSE_RH_WEBRTC,

    /***
     * 获取润华webrtc账号ack
     * cloud ---> device
     */
    GOBLIN_AIOT_GET_LICENSE_RH_WEBRTC_ACK,

    /* 
    *  获取涂鸦IOT账号
    *  device ---> cloud
    */
    GOBLIN_AIOT_GET_LICENSE_TY_IOT,

    /***
     * 获取涂鸦IOT账号ack
     * cloud ---> device
     */
    GOBLIN_AIOT_GET_LICENSE_TY_IOT_ACK,

    /*** 
     * 上报涂鸦IOT账号
     * device ---> clone
     */
    GOBLIN_AIOT_REPORT_LICENSE_TY_IOT,

    /***
     * 上报涂鸦IOT账号ack
     * clone ----> device
     */
    GOBLIN_AIOT_REPORT_LICENSE_TY_IOT_ACK,

    /***
     * 同步时间
     * device ---> cloud
     */
    GOBLIN_AIOT_SYNC_TIME,

    /***
     * 同步时间ack
     * cloud ---> device
     */
    GOBLIN_AIOT_SYNC_TIME_ACK,

    /***
     * 命令执行
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_EXEC,

    /***
     * 命令执行ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_EXEC_ACK,

    /***
     *  执行设备指令
     *  cloud ---> device
     */
    GOBLIN_AIOT_CMD_EVAL,

    /***
     *  执行设备指令ack
     *  device ---> cloud
     */
    GOBLIN_AIOT_CMD_EVAL_ACK,

    /***
     *  执行设备指令ack 内存释放
     */
    GOBLIN_AIOT_CMD_EVAL_ACK_FREE,

    /***
     *  重启消息
     *  cloud ---> device
     *  无ack
     */
    GOBLIN_AIOT_CMD_REBOOT,

    /***
     *  重启消息ack
     *  device ---> cloud
     *  无ack
     */
    GOBLIN_AIOT_CMD_REBOOT_ACK,

    /*** 
     *  恢复出厂设置
     *  cloud ---> device
     *  无ack
     */
    GOBLIN_AIOT_CMD_RESET,

    /***
     *  恢复出厂设置ack
     *  device ---> cloud
     *  无ack
     */
    GOBLIN_AIOT_CMD_RESET_ACK,

    /***
     * 清除设备数据
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_ERASE,

    /***
     * 清除设备数据ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_ERASE_ACK,

    /***
     * 抓取日志列表
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_LOG_LIST,

    /***
     *  抓取日志列表ack
     *  device ---> cloud
     */
    GOBLIN_AIOT_CMD_LOG_LIST_ACK,

    /***
     *  上层释放日志列表
     *  释放 log_list_ack 中 log_paths 资源
     */
    GOBLIN_AIOT_CMD_LOG_LIST_FREE,

    /***
     * 抓取日志
     *  cloud ---> device
     */
    GOBLIN_AIOT_CMD_LOG_FETCH,

    /***
     * 抓取日志ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_LOG_FETCH_ACK,

    /***
     * 上报上传日志进度
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_LOG_UPLOAD_FEEDBACK,

    /***
     *  升级
     *  device ---> cloud
     */
    GOBLIN_AIOT_CMD_UPGRADE,

    /***
     *  升级ack
     *  cloud ---> device
     */
    GOBLIN_AIOT_CMD_UPGRADE_ACK,

    /***
     * 设备被绑定
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_BOUND,

    /***
     * 设备被绑定 ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_BOUND_ACK,

    /***
     * 设备解绑
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_UNBOUND,

    /***
     * 设备解绑 ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_UNBOUND_ACK,

    /***
     * 检查绑定状态
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_CHECK_BOUND,

    /***
     * 检查绑定状态 ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_CHECK_BOUND_ACK,

    /***
     * 报告关联设备
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REPORT_DEVICES,

    /***
     * 报告关联设备ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REPORT_DEVICES_ACK,

    /***
     * 报告IPC摄像头
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REPORT_IPCS,

    /***
     * 报告IPC摄像头ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REPORT_IPCS_ACK,

    /***
     * 自动创建设备
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_AUTO_CREATE_DEVICES,

    /***
     * 自动创建设备ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_AUTO_CREATE_DEVICES_ACK,

    /***
     * 抓取录像回放日期列表
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REQUEST_PLAYBACK_DATES,

    /***
     * 抓取录像回放日期列表ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REQUEST_PLAYBACK_DATES_ACK,

    /***
     * 抓取录像回放列表
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS,

    /***
     * 抓取录像回放列表ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS_ACK,

    /***
     * GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS_ACK 中 times 申请的内存可在此处释放
     */
    GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS_FREE,

    /***
     * 低功耗设备唤醒 
     */
    GOBLIN_AIOT_CMD_WAKEUP,

    /***
     * 低功耗设备唤醒 ack
     */
    GOBLIN_AIOT_CMD_WAKEUP_ACK,

    /***
     * 配网
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_PROVISIONING,

    /***
     * 配网 ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_PROVISIONING_ACK,

    /***
     * 同步设置
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_SYNCSETTING,

    /***
     * 同步设置ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_SYNCSETTING_ACK,

    /**
     * 报警记录
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_ALARM_RECORD,

     /**
     * 报警记录ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_ALARM_RECORD_ACK,

    /**
     * 报警记录更新
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_ALARM_RECORD_UPDATE,

    /**
     * 报警记录更新ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_ALARM_RECORD_UPDATE_ACK,

    /**
     * 获取报警记录列表
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_GET_ALARM_RECORDS,

    /**
     * 获取报警记录列表ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_GET_ALARM_RECORDS_ACK,

    /**
     * 获取报警提示音
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_GET_ALARM_TONE,

    /**
     * 获取报警提示音ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_GET_ALARM_TONE_ACK,

    /**
     * 抓拍
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REQUIRE_SNAPSHOT,

    /**
     * 抓拍反馈
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REQUIRE_SNAPSHOT_FEEDBACK,

    /**
     * 抓取回放文件
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REQUIRE_PLAYBACK_FILE,

    /**
     * 抓取回放文件ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REQUIRE_PLAYBACK_FILE_ACK,

    /**
     * 抓取回放文件反馈
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_REQUIRE_PLAYBACK_FILE_FEEDBACK,

    /**
     * 抓取回放文件反馈ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_REQUIRE_PLAYBACK_FILE_FEEDBACK_ACK,

    /**
     * 发起开锁
     * device ---> cloud
     */
    GOBLIN_AIOT_CMD_OPEN_ASK,

    /**
     * 发起开锁ack
     * cloud ---> device
     */
    GOBLIN_AIOT_CMD_OPEN_ASK_ACK,

    /*
    *  device 上报状态
    *  device ---> cloud
    */
    GOBLIN_AIOT_REPORT_STATUS,

    /*
    *  device 上报状态ack
    *  cloud ---> device
    */
    GOBLIN_AIOT_REPORT_STATUS_ACK,
} goblin_aiot_msg_t;

typedef enum 
{
    AIOT_MSG_ACK_OK = 0,
    AIOT_MSG_ACK_FAIL,
} goblin_aiot_msg_ack_t;

typedef enum
{
    AIOT_UPGRADE_SCENE_AUTO = 0,
    AIOT_UPGRADE_SCENE_MANUAL,
    AIOT_UPGRADE_SCENE_REMOTE,
} aiot_upgrade_scene_t;

typedef enum
{
    AIOT_UPGRADE_PACKAGING_APP = 0,
    AIOT_UPGRADE_PACKAGING_SYSTEM,
    AIOT_UPGRADE_PACKAGING_SYSTEM_PATCH,
} aiot_upgrade_packaging_t;

typedef enum
{
    // 日志类型: 大小
    AIOT_LOG_TYPE_SIZE = 0,
    // 日志类型: 内容
    AIOT_LOG_TYPE_DATA,
} aiot_log_type_t;

typedef struct goblin_aiot_report_devinfo
{
    char sn[64];
    char model[32];
    int channel;
    int lock_num;  // 锁数量
    int lock_number[8]; // 锁编号
} goblin_aiot_report_devinfo_t;

typedef struct goblin_aiot_report_ipcinfo
{
    char ip_address[16];        // ip地址
} goblin_aiot_report_ipcinfo_t;

#define GOBLIN_AIOT_REPORT_SETTING_MAX_CAMERAS       (8)

typedef struct goblin_aiot_report_setting_camera
{
    char sn[64];
    char ip[32];
    char account[64];
    char password[64];
} goblin_aiot_report_setting_camera_t;

typedef struct goblin_aiot_palyback_time
{
    uint64_t start_time;
    uint64_t end_time;
    char media[16];
    char kind[32];
} goblin_aiot_playback_time_t;

typedef struct goblin_aiot_alarm_record
{
    char id[64];        /* 报警记录ID */
    char sn[32];        /* 报警设备序列号 */
    char kind[32];      /* 报警类型	*/
    char time[32];      /* 报警时间 格式为2006-01-02 15:04:05 */
    char *position;     /* 报警位置 */
} goblin_aiot_alarm_record_t;


typedef struct goblin_aiot_batteries
{
    char    kind[16];   // 电池类型	dry为干电池，li为锂电池
    int     power;      // 电池电量	最低为0，最高为100
} goblin_aiot_batteries_t;

typedef struct goblin_aiot_snapshot_header
{
    char *key;          // 图片上传的HTTP请求头键名
    char *value;        // 图片上传的HTTP请求头值
} goblin_aiot_snapshot_header_t;

typedef struct goblin_aiot_alarm_record_snapshot
{
    char media[16];     /* 抓拍媒体类型 image/video */
    bool upload;        /* 抓拍是否上传 */
} goblin_aiot_alarm_record_snapshot_t;

typedef struct goblin_aiot_alarm_record_snapshot_upload
{
    char media[16];     /* 媒体类型 image/video */
    char method[16];    /* 文件上传的HTTP方法 */
    char *url;          /* 文件上传的URL */
    goblin_aiot_snapshot_header_t *headers;
    int header_count;
} goblin_aiot_alarm_record_snapshot_upload_t;

#define GOBLIN_AIOT_SYNC_SETTING_MAX_MASKS              (8)
#define GOBLIN_AIOT_SYNC_SETTING_MAX_MASK_POINTS        (16)
#define GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS         (8)

typedef struct goblin_aiot_sync_setting_mask
{
    int point_count;
    double points[GOBLIN_AIOT_SYNC_SETTING_MAX_MASK_POINTS][2];
} goblin_aiot_sync_setting_mask_t;

typedef struct goblin_aiot_sync_setting_channel_narration
{
    int channel;
    char audio[64];
    char url[256];
} goblin_aiot_sync_setting_channel_narration_t;

typedef struct goblin_aiot_sync_setting_sip_account
{
    char server_host[64];
    int server_port;
    char username[64];
    char password[64];
} goblin_aiot_sync_setting_sip_account_t;


typedef struct goblin_aiot_msg_data
{
    goblin_aiot_msg_t msg_type;

    // 回复的状态码
    goblin_aiot_msg_ack_t ack_code;

    char reason[64];

    /*** 序列号
     *   只有云端请求时需要处理该字段, 如果上面回复ack消息要保存此字段，并在发送回复消息时赋值seq_ack字段
     */
    uint64_t seq;

    /*** 是否时云端请求
     *   当为ture时，表示是云端请求
     *   需要填写回调结构体中的 ack_code 和 reason 以及对应结构体的信息
     *   如果 ackcode 没有填写，则需要上面发送ack消息
     */
    bool is_cloud_request;

    // 需要代理的设备sn, 如果为0则未本机
    char proxy_sn[64];
    // 需要代理的设备的model
    char proxy_model[64];
    // 需要代理的设备类型, 如entryDoor为小门口机；代理上线未填写时默认entryDoor
    char proxy_kind[64];

    // 消息联合体
    union
    {
        struct
        {
            char last_session_id[32];
            bool low_power; // 是否低功耗设备
        } login;  /* GOBLIN_AIOT_LOGIN */

        struct 
        {
            char session_id[32];
        } login_ack; /* GOBLIN_AIOT_LOGIN_ACK */
        
        struct
        {
            char mac[32];
            char application_version[32];
            char system_version[32];
            char hardware_version[32];
            char tf_card_stat[16];
            char language[16];
            char call_modes_supported[8][16];
            uint64_t tfcard_space_total;
            uint64_t tfcard_space_used;
            int battery_level;
            goblin_aiot_batteries_t *batteries;
            int batteries_num;
        } report_detail; /* GOBLIN_AIOT_REPORT_DETAIL */

        struct
        {
            char password_open[16];
            char language[16];
            char engineering_password[32];
            bool upload_illegal_record;
            bool upload_record_image;
            bool ic_card_encrypted;
            bool face_recognition;
            bool face_anti_spoofing;
            bool face_recognition_by_people_aware;
            bool one_face_per_card;
            bool face_registration;
            bool sync_other_face_engine;
            int face_recognition_threshold;
            int face_recognition_timeout;
            bool call;
            bool call_manager;
            int call_play_early_media_timeout;
            int call_ring_timeout;
            int call_converse_timeout;
            bool hang_up_call_after_open;
            bool select_callee;
            char call_order[32];
            bool visitor_remote_open;
            bool password_open_enabled;
            bool qrcode_open;
            char open_voltage[16];
            int open_delay;
            bool door_magnetic_detection;
            int door_magnetic_alarm_grace_period;
            char door_magnetic_voltage[16];
            bool tamper_detection;
            int qrcode_scan_timeout;
            bool upload_log;
            char net[16];
            int microphone_volume;
            int speaker_volume;
            char sip_server_host[64];
            int sip_server_port;
            char sip_username[64];
            char sip_password[64];
            goblin_aiot_report_setting_camera_t cameras[GOBLIN_AIOT_REPORT_SETTING_MAX_CAMERAS];
            int camera_count;
        } report_setting; /* GOBLIN_AIOT_REPORT_SETTING */

        struct
        {
            char take_apart[16];    // 防拆状态 alarm/normal
            char magnetic[16];      // 门磁状态 alarm/normal
            char net[16];           // 网络类型 4g/4gRouter/wifi/wired
            int net_strength;       // 网络信号强度
            int net_quality;        // 网络信号质量
            int net_level;          // 网络信号强度等级
        } report_status; /* GOBLIN_AIOT_REPORT_STATUS */

        struct 
        {
            // 时区 格式为: +/- xx:xx (e.g. +08:00)
            char timezone[16];
        } sync_time;  /* GOBLIN_AIOT_SYNC_TIME */

        struct
        {
            // 时区 格式为: +/- xx:xx (e.g. +08:00)
            char timezone[16];
            char time[32];
        } sync_time_ack; /* GOBLIN_AIOT_SYNC_TIME_ACK */

        struct 
        {
            char license[64];
            char *addr;
            char *init_string;
        } rh_webrtc_ack; /* GOBLIN_AIOT_GET_LICENSE_RH_WEBRTC_ACK */

        struct 
        {
            char pid[64];
        } ty_iot;  /* GOBLIN_AIOT_GET_LICENSE_TY_IOT */

        struct 
        {
            char pid[64];
            char uuid[64];
            char key[64];
        } ty_iot_ack;   /* GOBLIN_AIOT_GET_LICENSE_TY_IOT_ACK */

        struct 
        {
            char pid[64];
            char uuid[64];
            char key[64];
        } report_ty_iot;    /* GOBLIN_AIOT_REPORT_LICENSE_TY_IOT */

        struct 
        {
            char command[128];
        } exec;  /* GOBLIN_AIOT_CMD_EXEC */
        
        struct 
        {
            int seq_ack;
        } exec_ack;  /* GOBLIN_AIOT_CMD_EXEC_ACK */

        struct 
        {
            char directive[128];
        } eval;  /* GOBLIN_AIOT_CMD_EVAL */

        struct 
        {
            int seq_ack;
            char *directive;
        } eval_ack;

        struct 
        {
            int seq_ack;
            char **log_paths;
            int  *log_sizes;
            int log_count;
        } log_list_ack; /* GOBLIN_AIOT_CMD_LOG_LIST_ACK */

        struct
        {
            int log_id;
            char *log_path;
        } log_upload;   /* GOBLIN_AIOT_CMD_LOG_FETCH */
    
        struct 
        {
            char taskid[128];
            int ulnow;
            int ultotal;
        } log_upload_feedback;  /* GOBLIN_AIOT_CMD_LOG_UPLOAD_FEEDBACK */

        struct 
        {
            aiot_upgrade_scene_t scene;
            aiot_upgrade_packaging_t packaging;
            char version[32];
            char hardware_version[32];
        } upgrade;      /* GOBLIN_AIOT_CMD_UPGRADE */ 

        struct 
        {
            bool should;
            aiot_upgrade_packaging_t packaging;
            char version[32];
            char hardware_version[32];
            char **apply_versions;
            int apply_version_count;
            char *description;
            char *md5;
            char *url;
        } upgrade_ack;  /* GOBLIN_AIOT_CMD_UPGRADE_ACK */
        
        struct 
        {
            int seq_ack;
        } bound;  /* GOBLIB_AIOT_CMD_BOUND */
        
        struct 
        {
            int seq_ack;
        } unbound;  /* GOBLIB_AIOT_CMD_UNBOUND */

        struct 
        {
            bool is_bound;
        } check_bound_ack;  /* GOBLIB_AIOT_CMD_CHECK_BOUND_ACK */

        struct 
        {
            int device_count;
            goblin_aiot_report_devinfo_t *devices;
        } report_devices;  /* GOBLIN_AIOT_CMD_REPORT_DEVICES */

        struct 
        {
            int ipc_count;
            goblin_aiot_report_ipcinfo_t *ipcs;
        } report_ipcs;  /* GOBLIN_AIOT_CMD_REPORT_IPCS */
        
        struct 
        {
            char install_code[32];
            char kind[32];
        } auto_create_device;   /* GOBLIN_AIOT_CMD_AUTO_CREATE_DEVICE*/

        struct 
        {
            int channel;    /* 通道号, 一般指设备的通道号 */
            char ipc[32];   /* ipc的ip地址 */
            int year;       /* 回放记录：年 */
            int month;      /* 回放记录：月 */
        } playback_date;    /* GOBLIN_AIOT_CMD_REQUEST_PLAYBACK_DATES */

        struct
        {
            int seq_ack;
            int year;           /* 回放记录：年 */
            int month;          /* 回放记录：月 */
            unsigned int day;   /* 回放记录：日. 以位数做标记 0:无记录 1：有记录  1位为1号 2位为2号 以此类推 */
        } playback_date_ack; /* GOBLIN_AIOT_CMD_REQUEST_PLAYBACK_DATES_ACK */

        struct
        {
            int channel;    /* 通道号, 一般指设备的通道号 */
            char ipc[32];   /* ipc的ip地址 */
            int year;       /* 回放记录：年 */
            int month;      /* 回放记录：月 */
            int day;        /* 回放记录：日 */
        } playbacks;    /* GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS */

        struct
        {
            int seq_ack;
            int count;      /* 当天的回放记录数量 */
            goblin_aiot_playback_time_t *times;
        }playbacks_ack; /* GOBLIN_AIOT_CMD_REQUEST_PLAYBACKS_ACK */

        struct
        {
            char event[32];     /* 唤醒事件 */
            char callid[64];    /* 呼叫ID */
        } wake; /* GOBLIN_AIOT_CMD_WAKEUP */

        struct
        {
            int seq_ack;
        } wake_ack; /* GOBLIN_AIOT_CMD_WAKEUP_ACK*/

        struct
        {
            char watchid[64];
        } provisioning; /* GOBLIN_AIOT_CMD_PROVISIONING */

        struct
        {
            char language[16];
            int microphone_volume;
            int speaker_volume_alarm;
            char password_open[16];
            char door_contact[32];
            bool door_contact_enabled;
            int door_contact_grace_period;
            char battery[32];
            char battery_preferred[32];
            char record_storage_kind[32];
            char smarthome_mode[32];
            char alarm_tone[32];
            bool alarm_loitering_enabled;
            int alarm_loitering_grace_period;
            /* 兼容旧字段，优先使用alarm_loitering_* */
            bool alarm_stay_enabled;
            int alarm_stay_grace_period;
            bool mask_enabled;
            goblin_aiot_sync_setting_mask_t masks[GOBLIN_AIOT_SYNC_SETTING_MAX_MASKS];
            int mask_count;
            char engineering_password[32];
            bool upload_illegal_record;
            bool upload_open_image;
            bool ic_card_encrypted;
            bool enable_call;
            int call_play_early_media_timeout;
            int call_ring_timeout;
            int call_converse_timeout;
            int interval_between_each_call;
            int frequently_call_period;
            int frequently_call_times;
            char call_order[32];
            char outgoing_call_sips[GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS][64];
            int outgoing_call_sip_count;
            char outgoing_call_rescue_tels[GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS][64];
            int outgoing_call_rescue_tel_count;
            char channel_names[GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS][64];
            int channel_name_count;
            goblin_aiot_sync_setting_channel_narration_t channel_narrations[GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS];
            int channel_narration_count;
            char sip_server_host[64];
            int sip_server_port;
            char sip_username[64];
            char sip_password[64];
            goblin_aiot_sync_setting_sip_account_t sip_accounts[GOBLIN_AIOT_SYNC_SETTING_MAX_LIST_ITEMS];
            int sip_account_count;
            bool enable_visitor_open;
            int open_delay;
            int repair_press_duration;
            bool check_health;
            char check_health_province[32];
            int speaker_volume;
            bool open_check_face_and_card;
        } sync_setting;

        struct 
        {
            uint64_t start_time;        /* 开始时间戳 */
            char     kind[64];          /* 报警类型 doorContact：门磁触发。 lowBattery：电池电量低。 motion：移动侦测。loitering：长时间停留。*/
            int      video_duration;    /* 录像时长 */
            int      recording_duration;/* 兼容旧字段，优先使用 video_duration */
            goblin_aiot_alarm_record_snapshot_t *snapshots;
            int      snapshot_count;
        } alarm_record;

        struct
        {
            char id[64];        /* 报警记录ID */
            goblin_aiot_alarm_record_snapshot_upload_t *snapshots;
            int snapshot_count;
        } alarm_record_ack;

        struct
        {
            char id[64];        /* 报警记录ID */
            int  video_duration;/* 录像时长 */
        } alarm_record_update;

        struct
        {
            int seq_ack;
        } alarm_record_update_ack;

        struct
        {
            char callid[64];    /* 呼叫ID */
            char sn[64];        /* 开锁设备序列号 */
        } open_ask;

        struct
        {
            int seq_ack;
        } open_ask_ack;

        struct
        {
            int     page;               /* 页码 最大100 */
            int     amount;             /* 每页数量 最大100 */
        } get_alarm_records;

        struct
        {
            int num;
            goblin_aiot_alarm_record_t *alarm_records;
        } get_alarm_records_ack;

        struct
        {
            char tone[32];      /* 报警提示音 */
        } get_alarm_tone;

        struct
        {
            char *url;          /* 下载URL */
        } get_alarm_tone_ack;

        struct
        {
            char method[16];    /* 图片上传的HTTP方法 */
            char *url;          /* 图片上传的URL */
            goblin_aiot_snapshot_header_t *headers;
            int header_count;
        } require_snapshot;

        struct
        {
            bool success;       /* 图片上传是否成功 */
        } require_snapshot_feedback;

        struct
        {
            uint64_t start_time;    /* 开始时间戳 */
            char media[16];         /* 媒体类型 */
            char method[16];        /* 文件上传的HTTP方法 */
            char *url;              /* 文件上传的URL */
            goblin_aiot_snapshot_header_t *headers;
            int header_count;
        } require_playback_file;

        struct
        {
            int seq_ack;
        } require_playback_file_ack;

        struct
        {
            uint64_t start_time;    /* 开始时间戳 */
            char media[16];         /* 媒体类型 */
            bool success;           /* 文件上传是否成功 */
        } require_playback_file_feedback;
        
    } data;

} goblin_aiot_msg_data_t;

/***
 * 自定义内存申请函数
 * 用于替换系统默认的malloc和free函数
 */
typedef void *(*goblin_aiot_malloc_ptr) (size_t); 
typedef void (*goblin_aiot_free_ptr) (void *);

/***
 * aiot连接状态回调函数
 * 用于接收aiot连接状态的通知
 * @param status 连接状态
 * @param userdata 用户自定义数据
 */
typedef void (*goblin_aiot_conn_status_cb_t) (aiot_conn_status_t status, void *userdata);

/***
 * aiot消息回调函数
 * 用于接收aiot消息的通知
 * @param data 接收到的消息数据
 * @param userdata 用户自定义数据
 */
typedef void (*goblin_aiot_msg_cb_t) (goblin_aiot_msg_data_t *data, void *userdata);

/***
 * aiot callx 消息回调函数
 * 用于接收aiot callx 消息的通知
 * @param data 接收到的消息数据 (内容结构为 goblin_aiot_callx_data_t)
 * @param userdata 用户自定义数据
 */
typedef void (*goblin_aiot_callx_cb_t) (void *data, void *userdata);

/***
 *  aiot 超时事件回调函数
 *  用于接收aiot超时事件的通知
 *  @param data 超时消息数据
 *  @param userdata 用户自定义数据
 */
typedef void (*goblin_aiot_timeout_cb_t) (goblin_aiot_msg_data_t *data, void *userdata);

/***
 * aiot 重连时间回调函数
 * 用于计算aiot重连时间
 * @param retry_time 重连次数
 * @param userdata 用户自定义数据
 * @return 返回重连时间，单位为秒，-1表示不重连
 */
typedef int (*goblin_aiot_reconnect_time_cb_t) (int retry_time, void *userdata);

/***
 * aiot log 读取回调函数
 * 用于读取aiot log
 * @param type 获取的数据类型
 * @param log_id 由内部分配 GOBLIN_AIOT_CMD_LOG_FETCH 回调中返回, 用于区分获取的日志
 * @param buf 读取到的log缓存
 * @param size 缓存大小
 * @param userdata 用户自定义数据
 * @return log文件大小 或 返回读取到的log data的长度，如果返回0表示文件已读取完毕
 */
typedef int (*goblin_aiot_log_read_cb_t) (aiot_log_type_t type, int log_id, char *buf, int size, void *userdata);

typedef struct goblin_aiot_config
{
    // 使用的网卡
    char interface[16];

    // 设备的sn
    char sn[128];

    // 产品型号
    char model[32];

    // 服务器IP地址或域名 (eg: 192.168.1.123:45)
    char *server_addr;

    // mqtt 服务器地址
    char *mqtt_server_addr;

    bool is_mqtts;

    // 心跳间隔
    int  keepalive;

    // 连接状态回调函数
    goblin_aiot_conn_status_cb_t conn_status_cb;
    
    // 消息回调函数
    goblin_aiot_msg_cb_t msg_cb;

    // callx 消息回调函数
    goblin_aiot_callx_cb_t callx_cb;

    // 重连时间回调函数
    goblin_aiot_reconnect_time_cb_t reconnect_time_cb;

    // 超时事件回调函数
    goblin_aiot_timeout_cb_t timeout_cb;

    // 日志读取回调函数
    goblin_aiot_log_read_cb_t log_read_cb;

    // 用户自定义数据
    void *userdata;
} goblin_aiot_config_t;


/***
 * 设置内存分配函数
 * 用于替换系统默认的malloc和free函数
 * @param malloc_fn 自定义的malloc函数
 * @param free_fn 自定义的free函数
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_init_mem_hook(goblin_aiot_malloc_ptr malloc_fn, goblin_aiot_free_ptr free_fn);

/***
 * 设置日志级别
 * @param level 日志级别
 */
GOBLIN_AIOT_API void goblin_aiot_log_set_level(aiot_log_level_t level);

/***
 * 获取日志级别
 * @return 日志级别
 */
GOBLIN_AIOT_API int goblin_aiot_log_get_level(void);

/***
 * 设置日志回调函数
 * @param callback 日志回调函数
 */
GOBLIN_AIOT_API void goblin_aiot_log_set_callback(void (*callback)(int, const char *, const long, const char *, const char *, va_list));

/***
 * 初始化aiot服务
 * @param config 连接配置
 * @param user_aiot_ver aiot头文件版本号, GOBLIN_AIOT_VER_STRING
 * @return aiot服务句柄
 */
GOBLIN_AIOT_API void *goblin_aiot_init(goblin_aiot_config_t *config, char *user_aiot_ver);

/***
 * 反初始化aiot服务
 * @param ctx aiot服务句柄
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_deinit(void *ctx);

/***
 * aiot重连时, 快速连接服务器，不等待, 只生效一次
 * @param ctx aiot服务句柄
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_connect_fast(void *ctx);

/***
 * 设置aiot服务器地址
 * @param ctx aiot服务句柄
 * @param server_addr 服务器地址 (eg: 192.168.1.123:45)
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_set_server_addr(void *ctx, char *server_addr);

/***
 * aiot发送消息
 * @param ctx  aiot服务句柄
 * @param data 消息数据
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_send_message(void *ctx, goblin_aiot_msg_data_t *data);

/***
 * 使用Http协议获取Mqtt地址
 * @param config 配置信息
 * @param mqtt_service_url mqtt地址
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_get_mqtt_server_addr(goblin_aiot_config_t *config, char *mqtt_service_url);

#ifdef __cplusplus
}
#endif

#endif /* __GOBLIN_IOT_H__ */
