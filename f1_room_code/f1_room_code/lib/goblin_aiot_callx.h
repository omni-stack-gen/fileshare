#ifndef __GOBLIN_AIO_CALLX_H__
#define __GOBLIN_AIO_CALLX_H__

#include "goblin_aiot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CALLX_MAX_SUPPORTED_MODES       (8)
#define CALLX_MAX_SUPPORTED_MEDIAS      (8)
#define CALLX_MAX_CALLEE_USER_IDS       (8)

typedef enum
{
    /**
     * Initiate a call
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_INITIATE = 0x2001,

    /**
     * Initiate a call ACK
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_INITIATE_ACK,

    /**
     * call invite
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_INVITE,

    /**
     * call invite ACK
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_INVITE_ACK,

    /**
     * call invited
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_INVITED,

    /**
     * call invited ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_INVITED_ACK,

    /**
     * call replied
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_REPLIED,

    /**
     * call replied ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_REPLIED_ACK,

    /**
     * call Snapshotted
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_SNAPSHOTTED,

    /**
     * call Snapshotted ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_SNAPSHOTTED_ACK,

    /**
     * call Accept
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_ACCEPT,

    /**
     * call Accept ACK
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_ACCEPT_ACK,

    /**
     * call Accepted
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_ACCEPTED,

    /**
     * call Accepted ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_ACCEPTED_ACK,

    /**
     * call Confirmed
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_CONFIRMED,

    /**
     * call Confirmed ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_CONFIRMED_ACK,

    /**
     * call hangup
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_HANGUP,

    /**
     * call hangup ACK
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_HANGUP_ACK,

    /**
     * call huangup
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_HUNGUP,

    /**
     * call huangup ACK
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_HUNGUP_ACK,

    /**
     * call terminate
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_TERMINATE,

    /**
     * call terminate ACK
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_TERMINATE_ACK,

    /***
     * 下发开锁
     * cloud ---> device
     */
    GOBLIN_AIOT_CALLX_OPEN_LOCK,

    /***
     * 下发开锁ack
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_OPEN_LOCK_ACK,

    /**
     * 呼叫获取被邀请信息
     * device ---> cloud
     */
    GOBLIN_AIOT_CALLX_GET_INVITED,

    /**
     * 呼叫获取被邀请信息ack
     * cloud ---> device
     * 使用联合体中 invited 结构体
     */
    GOBLIN_AIOT_CALLX_GET_INVITED_ACK,

} goblin_aiot_callx_t;

typedef enum
{
    AIOT_CALLX_ACK_OK = 0,
    AIOT_CALLX_ACK_FAIL,
} goblin_aiot_callx_ack_t;

typedef struct callee_dev_info
{
    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼类型 app为APP，device为设备，tel为电话，web为Web，wechat为小程序
     * GOBLIN_AIOT_CALLX_INVITE: 被呼类型
     */
    char kind[32];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE: 被呼叫设备的序列号
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 呼叫设备的序列号, kind为device时使用
     * GOBLIN_AIOT_CALLX_INVITE: 被呼设备序列号, kind为device时使用
     */
    char sn[64];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE: 被呼叫设备的通道号
     * GOBLIN_AIOT_CALLX_INVITE: 被呼设备通道号, kind为device时使用, 且只有部分设备使用
     */
    int channel;

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼小程序用户ID, kind为app、wechat时使用
     * GOBLIN_AIOT_CALLX_INVITE: 被呼小程序用户ID, kind为app、wechat时使用
     */
    char user_id[64];

    /**
     *  GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼Web登录用户名
     *  GOBLIN_AIOT_CALLX_INVITE: 被呼Web登录用户名
     */
    char web_username[64];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼Web会话ID
     * GOBLIN_AIOT_CALLX_INVITE:  被呼Web会话ID
     */
    char web_sessionId[64];

    /**
     * GOBLIN_AIOT_CALLX_INVITE: 被呼的呼叫模式, 如callX、voip、webrtc
     */
    char mode[16];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼电话号码
     */
    char tel[32];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK: 被呼设备别名
     */
    char alias[128];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK： 支持的呼叫模式列表, 可为callX、voip、webrtc, 优先级依次从高到低
     */
    char supported_modes[CALLX_MAX_SUPPORTED_MODES][16];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK： 被呼小程序微信openId. .kind为wechat，且supported_modes有voip时使用
     */
    char wx_openId[64];

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK： 被呼小程序VoIP呼叫优先级
     *                                  当order_kind为simultaneous、kind为wechat，
     *                                  且supported_modes有voip时使用。此时也可省略，优先级最低
     */
    int voip_priority;

    /**
     * GOBLIN_AIOT_CALLX_INITIATE_ACK： 是否可以被呼
     */
    bool able;

    /***
     * GOBLIN_AIOT_CALLX_INITIATE_ACK：不可被呼的原因, 当able为false时使用，此时也可省略
     */
    char *unable_cause;

    /**
     * GOBLIN_AIOT_CALLX_INVITE: 被呼小程序是否使用带外的底层呼叫
     *                           当kind为wechat且mode为voip时使用。
     *                           带外的底层呼叫，即设备在请求GOBLIN_AIOT_CALLX_INITIATE后，
     *                           即可马上发起的底层呼叫，不再依赖后续协议
     */
    bool out_of_band_underlying_call;
} callee_dev_info_t;

typedef struct callee_rounds_info
{
    /**
     * 当前轮呼的设备信息
     */
    callee_dev_info_t *callees;
    int callees_num;

    /***
     * 呼叫层级
     */
    int level;

    /**
     * 呼叫顺序类型, sequential为轮呼，simultaneous为同呼
     */
    char order_kind[64];

    /**
     * 振铃超时秒数
     */
    int ring_timeout;

    /**
     * 通话超时秒数
     */
    int converse_timeout;

} callee_rounds_info_t;

typedef struct
{
    int     layer;
    char    *name;
} category_layers_t;

typedef struct
{
    char *name;
    char *kind;
} location_info_t;

typedef struct
{
    char *key;
    char *value;
} header_info_t;

typedef struct
{
    char method[16];
    char *url;
    header_info_t *headers;
    int header_num;
} snapshot_upload_info_t;

typedef struct goblin_aiot_callx_data
{
    goblin_aiot_callx_t callx_type;

    // 回复的状态码
    goblin_aiot_callx_ack_t ack_code;

    char reason[64];

    /*** 序列号
     *   只有云端请求时需要处理该字段, 如果上面回复ack消息要保存此字段，并在发送回复消息时赋值seq_ack字段
     */
    uint64_t seq;

    /*** 回复序列号
     *   只有设备端回复时需要处理该字段
     */
    int     seq_ack;

    /*** 是否时云端请求
     *   当为ture时，表示是云端请求
     *   需要填写回调结构体中的 ack_code 和 reason 以及对应结构体的信息
     *   如果 ackcode 没有填写，则需要上面发送ack消息
     */
    bool is_cloud_request;

    /**
     * 当前呼叫的ID, 由云端生成, 设备端应保存该字段, 并在回复/发送消息时赋值callid字段
     */
    char callid[64];

    // 需要代理的设备sn, 如果为0则未本机
    char proxy_sn[64];

    // 消息联合体
    union
    {
        struct
        {
            int     channel;                    // 主呼设备通道,填-1表示省略
            bool    with_snapshot;              // 是否带快照,填false表示不带快照
            char    snapshot_content_type[64];  // 快照内容类型,填空表示省略
            char    alarmid[64];                // 呼叫时刻报警ID,填空表示省略
            char    project_kind[64];           // 项目类型,填空表示省略
            char    call_kind[64];              // 呼叫类型,toDevice为true时可省略。monitor为监视，visitor为访客呼叫
            bool    to_device;                  // 是否呼叫设备,填false表示呼叫访客
            callee_dev_info_t *devs_info;       // 被呼叫设备的信息
            int     devs_num;                   // 被呼叫设备的数量
            char    callee_user_ids[CALLX_MAX_CALLEE_USER_IDS][64]; // 被呼用户ID列表
            int     callee_user_id_count;        // 被呼用户ID数量
            char    callee_config_table[64];     // 被呼配置表值
            bool    to_manager;                 // 是否呼叫物管
            char    callee_location_id[64];      // 被呼住户的位置ID
            char    callee_apartment[64];        // 被呼住户的住房房号
            char    *resident_locationid;       // 兼容旧字段，发送为calleeLocationId
        } initiate;

        struct
        {
            snapshot_upload_info_t snapshot_upload_info; // 快照上传信息
            callee_rounds_info_t *rounds_info;  // 被呼叫轮次信息
            int     rounds_num;                 // 被呼叫轮次数量
            char    picture_path[128];          // 快照上传地址
            char    callee_apartment_id[64];     // 被呼住户的住房ID
        } initiate_ack;

        struct
        {
            int     ring_timeout;            // 振铃超时秒数
            int     converse_timeout;        // 通话超时秒数
            int     round;                   // 当前轮次
            callee_dev_info_t *dev_info;     // 当前轮次被呼设备信息
            int     dev_num;                 // 当前轮次被呼设备数量
        } invite;

        struct
        {
            char    mode[16];                                               // 呼叫模式 callX、voip、webrtc
            char    supported_medias[CALLX_MAX_SUPPORTED_MEDIAS][16];       // 主呼支持的发送媒体类型列表 可为audio, none, video
            char    required_medias[CALLX_MAX_SUPPORTED_MEDIAS][16];        // 主呼要求的接收媒体类型列表
            bool    can_open;                                               // 主呼是否可被开锁
            bool    can_liftcall;                                           // 主呼是否可被呼梯
            bool    can_cancelalarm;                                        // 主呼是否可被消警
            char    alarm_id[64];                                           // 报警记录ID
            int     ring_timeout;                                           // 振铃超时秒数 为0时, 超时由被呼方控制
            int     converse_timeout;                                       // 通话超时秒数 为0时, 超时由被呼方控制
            char    caller_sn[64];                                          // 被呼设备序列号
            category_layers_t    *caller_category_layers;                   // 被呼设备分类层级
            int     layers_num;                                             // 被呼设备分类层级数量
            int     caller_channel;                                         // 被呼设备通道
            bool    caller_is_secondary;                                    // 主呼设备是否为副机
            char    caller_user_id[64];                                     // 被呼小程序用户ID
            char    caller_web_username[64];                                // 被呼Web登录用户名
            char    caller_web_sessionId[64];                               // 被呼Web会话ID
            char    caller_api_userid[64];                                  // 被呼API小程序用户ID
            char    *caller_role;                                           // 主呼身份
            location_info_t *caller_location;                               // 被呼设备位置信息
            int     location_num;                                           // 被呼设备位置信息数量
            char    *caller_alias;                                          // 主呼的显示别名
            int     callee_channel;                                         // 被呼设备通道, 部分设备使用
            char    *snapshot_upload_url;                                   // 下载抓拍图的URL
            char    callee_ipc[32];                                         // IPC ip地址
            bool    playback;                                               // 是否为回放
            uint64_t playback_timestamp;                                    // 回放时间戳
            char    *webrtc_license;                                        // 分配的webrtcLicense
            char    caller_device_kind[64];                                 // 主呼设备类型
        } invited;

        struct
        {
            bool    able;                    // 是否可被呼
            char    unable_cause[128];       // 不能被呼的原因, 当able为false时使用
            int     ring_timeout;            // 振铃超时秒数
            int     converse_timeout;        // 通话超时秒数
        } invited_ack;

        struct
        {
            char    callee_sn[64];                  // 被呼设备序列号
            int     callee_channel;                 // 被呼设备通道
            char    callee_user_id[64];             // 被呼小程序用户ID
            char    callee_web_username[64];        // 被呼Web登录用户名
            char    callee_web_sessionId[64];       // 被呼Web会话ID
            char    callee_api_userid[64];          // 被呼API小程序用户ID
            bool    able;                           // 是否可被呼
            char    unable_cause[128];              // 不能被呼的原因, 当able为false时使用
            int     ring_timeout;                   // 振铃超时秒数
            int     converse_timeout;               // 通话超时秒数
        } replied;

        struct
        {
            char    *url;                           // 快照下载URL
        } snapshotted;

        struct
        {
            char caller_media[16];                  // 主呼发送的媒体类型
            char callee_media[16];                  // 被呼发送的媒体类型
            char callee_ipc[32];                    // IPC ip地址
            bool reverse_underlying_call;           // 是否反向呼叫
            int  callee_channel;                    // 被呼设备通道, 部分设备使用
            bool no_other_function;                 // true: 无其他功能, 如：呼梯、消警、开锁 false: 使用默认设置的功能
        } accept;

        struct
        {
            char    caller_media[16];           // 主呼发送的媒体类型
            char    callee_media[16];           // 被呼发送的媒体类型
            bool    reverse_underlying_call;    // 是否反向呼叫
            char    accept_kind[16];            // 接听类型, 如app、door
            bool    can_open;                   // 是否可被开锁
            bool    can_liftcall;               // 是否可被呼梯
            bool    can_cancelalarm;            // 是否可被消警
            char    callee_sn[64];                   // 被呼设备序列号
            int     callee_channel;                 // 被呼设备通道
            char    callee_user_id[64];             // 被呼小程序用户ID
            char    callee_web_username[64];        // 被呼Web登录用户名
            char    callee_web_sessionId[64];       // 被呼Web会话ID
            char    callee_api_userid[64];          // 被呼API小程序用户ID
            char    *callee_alias;                  // 被呼的显示别名
            location_info_t *callee_location;                               // 被呼设备位置信息
            int     location_num;                                           // 被呼设备位置信息数量
        } accepted;

        struct
        {
            char    callee_sn[64];                  // 被呼设备序列号
            int     callee_channel;                 // 被呼设备通道
            char    callee_user_id[64];             // 被呼小程序用户ID
            char    callee_web_username[64];        // 被呼Web登录用户名
            char    callee_web_sessionId[64];       // 被呼Web会话ID
            char    callee_api_userid[64];          // 被呼API小程序用户ID
            bool    able;                           // 是否可以被接听
            char    unable_cause[128];              // 不能被接听的原因, 当able为false时使用
            bool    hangup_other_callees;           // 是否挂断其它被呼方
        } accepted_ack;

        struct
        {
            bool    able;                           // 是否可以被接听
            char    unable_cause[128];              // 不能被接听的原因, 当able为false时使用
            char    mode[16];                       // 呼叫模式 callX、voip、webrtc
            char    *opposite_txiot_productid;      // 对端设备腾讯连连产品ID
            char    *opposite_txiot_device_name;    // 对端设备腾讯连连设备名称
            char    *opposite_txiot_userid;         // 对端设备腾讯连连小程序用户ID
            char    *opposite_webrtc_license;       // 对端设备WebRTC授权码
            char    *webrtc_session_id;             // WebRTC会话ID
        } confirmed;

        struct
        {
            bool    entirely;                   // 是否把呼叫彻底挂断
            callee_dev_info_t *dev_info;        // 被呼叫轮次信息
            int     dev_num;                    // 被呼叫轮次数量
            char    *cause;                     // 挂断原因
        } hangup;

        struct
        {
            bool    entirely;                 // 是否把呼叫彻底挂断
            char    *cause;                   // 挂断原因
            char    callee_sn[64];                   // 被呼设备序列号
            char    callee_user_id[64];             // 被呼小程序用户ID
            char    callee_web_username[64];        // 被呼Web登录用户名
            char    callee_web_sessionId[64];       // 被呼Web会话ID
            char    callee_api_userid[64];          // 被呼API小程序用户ID
        } hungup;

        struct
        {
            int channel;
            int lock_number;
        } open_lock;

    } data;

} goblin_aiot_callx_data_t;

typedef struct aiot_callx_config
{
    bool can_open;              // 是否可被开锁
    bool can_liftcall;         // 是否可被呼梯
    bool can_cancelalarm;       // 是否可被消警
    char supported_modes[CALLX_MAX_SUPPORTED_MODES][16];    // 主呼支持的呼叫模式列表, 可为callX、voip、webrtc, 优先级依次从高到低
    char supported_medias[CALLX_MAX_SUPPORTED_MEDIAS][16];  // 主呼支持的媒体类型列表, 可为audio, none, video
    char required_medias[CALLX_MAX_SUPPORTED_MEDIAS][16];    // 主呼要求的接收媒体类型列表 可为audio, none, video
} aiot_callx_config_t;

GOBLIN_AIOT_API int goblin_aiot_set_callx_config(void *ctx, aiot_callx_config_t *config);


/***
 * aiot发送消息
 * @param ctx  aiot服务句柄
 * @param data 消息数据
 * @return 0:success
 */
GOBLIN_AIOT_API int goblin_aiot_send_callx_msg(void *ctx, goblin_aiot_callx_data_t *data);


#ifdef __cplusplus
}
#endif

#endif /* __GOBLIN_AIO_CALLX_H__ */
