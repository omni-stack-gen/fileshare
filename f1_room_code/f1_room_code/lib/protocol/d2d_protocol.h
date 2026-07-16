#ifndef D2D_PROTOCOL_H
#define D2D_PROTOCOL_H

/**
 * @file d2d_protocol.h
 * @brief D2D protocol 对外协议模型与编解码接口。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief d2d 固定帧头魔术字。 */
#define D2D_HEAD_MAGIC 0x00D2F999u

/** @brief `msgId` 字段最大可打印字符数：3 位随机数 + 13 位毫秒时间戳。 */
#define D2D_MSG_ID_MAX_LEN 16

/** @brief `origAddr` 与 `destAddr` 字段最大可打印字符数。 */
#define D2D_ADDR_MAX_LEN 32

/**
 * @brief D2D protocol 日志级别。
 */
typedef enum d2d_log_level
{
    D2D_LOG_LEVEL_DEBUG = 1,
    D2D_LOG_LEVEL_INFO = 2,
    D2D_LOG_LEVEL_WARN = 3,
    D2D_LOG_LEVEL_ERROR = 4
} d2d_log_level_t;

/**
 * @brief D2D protocol 日志回调原型。
 */
typedef void (*d2d_log_func_t)(d2d_log_level_t level,
                               const char *file,
                               long line,
                               const char *func,
                               const char *format,
                               va_list ap);

/**
 * @brief D2D protocol 内存分配钩子集合。
 */
struct d2d_mem_hooks
{
    void *(*malloc)(size_t size, const char *file, int line);
    void *(*realloc)(void *ptr, size_t size, const char *file, int line);
    void (*free)(void *ptr, const char *file, int line);
};

void d2d_log_set_handler(d2d_log_func_t function);
void d2d_mem_register_hooks(const struct d2d_mem_hooks *hooks);
void d2d_mem_free(void *ptr);

/**
 * @brief D2D protocol 对外错误码枚举。
 */
typedef enum
{
    D2D_OK = 0,          /**< 成功。 */
    D2D_ERR_ARG = 1,     /**< 参数非法，例如空指针或缺少必填内容。 */
    D2D_ERR_NOMEM = 2,   /**< 内存申请失败。 */
    D2D_ERR_CMD = 3,     /**< 命令字未知或与当前接口预期不匹配。 */
    D2D_ERR_FORMAT = 4,  /**< 数据格式错误，例如字段缺失或类型错误。 */
    D2D_ERR_JSON = 5,    /**< JSON 解析失败。 */
    D2D_ERR_RANGE = 6,   /**< 字段值超出协议允许范围。 */
    D2D_ERR_MISMATCH = 7,/**< 外层头与内容、长度等信息不一致。 */
    D2D_ERR_STATE = 8    /**< 运行时状态不合法，例如句柄失效或发生重入调用。 */
} d2d_error_code_t;

/**
 * @brief d2d 外层报文类型枚举。
 */
typedef enum
{
    D2D_MESSAGE_REQUEST = 0,      /**< 请求报文。 */
    D2D_MESSAGE_REPLY_OK = 1,     /**< 成功应答报文，允许携带 content。 */
    D2D_MESSAGE_REPLY_FAIL = 2,   /**< 失败应答报文，不允许携带 content。 */
    D2D_MESSAGE_FORWARD_FAIL = 3  /**< 转发失败报文，不允许携带 content。 */
} d2d_message_type_t;

/**
 * @brief 协议中使用的设备类型枚举。
 */
typedef enum
{
    D2D_DEV_INDOOR = 1,           /**< 室内机。 */
    D2D_DEV_UNIT_DOOR = 2,        /**< 单元门口机。 */
    D2D_DEV_VILLA_DOOR = 3,       /**< 别墅门口机。 */
    D2D_DEV_GUARD = 6,            /**< 保安机。 */
    D2D_DEV_GATE = 7,             /**< 围墙机。 */
    D2D_DEV_CENTER = 8,           /**< 中心机。 */
    D2D_DEV_SIGNAL_EXTENDER = 500 /**< 二线信号扩展器。 */
} d2d_device_type_t;

/**
 * @brief 协议文档第 III-VI 节命令字枚举。
 */
typedef enum
{
    D2D_CMD_NONE = 0,                       /**< 空命令，仅用于初始化占位。 */
    D2D_CMD_INDOOR_HEARTBEAT_REQ = 20010,  /**< 室内机心跳请求。 */
    D2D_CMD_INDOOR_HEARTBEAT_ACK = 20011,  /**< 室内机心跳应答。 */
    D2D_CMD_INDOOR_SYNC_REQ = 20014,       /**< 室内机同步请求。 */
    D2D_CMD_INDOOR_SYNC_ACK = 20015,       /**< 室内机同步应答。 */
    D2D_CMD_EXTENDER_HEARTBEAT_REQ = 20020,/**< 信号扩展器心跳请求。 */
    D2D_CMD_EXTENDER_HEARTBEAT_ACK = 20021,/**< 信号扩展器心跳应答。 */
    D2D_CMD_EXTENDER_SYNC_REQ = 20024,     /**< 信号扩展器同步请求。 */
    D2D_CMD_EXTENDER_SYNC_ACK = 20025,     /**< 信号扩展器同步应答。 */
    D2D_CMD_DOOR_HEARTBEAT_REQ = 20030,    /**< 门口机或围墙机心跳请求。 */
    D2D_CMD_DOOR_HEARTBEAT_ACK = 20031,    /**< 门口机或围墙机心跳应答。 */
    D2D_CMD_DOOR_SYNC_REQ = 20034,         /**< 门口机或围墙机同步请求。 */
    D2D_CMD_DOOR_SYNC_ACK = 20035,         /**< 门口机或围墙机同步应答。 */
    D2D_CMD_ADMIN_HEARTBEAT_REQ = 20040,   /**< 保安机或中心机心跳请求。 */
    D2D_CMD_ADMIN_HEARTBEAT_ACK = 20041,   /**< 保安机或中心机心跳应答。 */
    D2D_CMD_ADMIN_SYNC_REQ = 20044,        /**< 保安机或中心机同步请求。 */
    D2D_CMD_ADMIN_SYNC_ACK = 20045,        /**< 保安机或中心机同步应答。 */
    D2D_CMD_UNLOCK_REQ = 20130,            /**< 开锁请求。 */
    D2D_CMD_UNLOCK_ACK = 20131,            /**< 开锁应答。 */
    D2D_CMD_ELEVATOR_REQ = 20140,          /**< 呼梯请求。 */
    D2D_CMD_ELEVATOR_ACK = 20141,          /**< 呼梯应答。 */
    D2D_CMD_CALL_FORWARD_REQ = 20150,      /**< 查询允许呼叫或转呼请求。 */
    D2D_CMD_CALL_FORWARD_ACK = 20151,      /**< 查询允许呼叫或转呼应答。 */
    D2D_CMD_SET_UNLOCK_PASSWORD_REQ = 20160,/**< 设置开锁密码请求。 */
    D2D_CMD_SET_UNLOCK_PASSWORD_ACK = 20161,/**< 设置开锁密码应答。 */
    D2D_CMD_GET_UNLOCK_PASSWORD_REQ = 20162,/**< 获取开锁密码请求。 */
    D2D_CMD_GET_UNLOCK_PASSWORD_ACK = 20163,/**< 获取开锁密码应答。 */
    D2D_CMD_CLOUD_HEARTBEAT_REQ = 23010,   /**< 代理上云心跳请求。 */
    D2D_CMD_CLOUD_HEARTBEAT_ACK = 23011,   /**< 代理上云心跳应答。 */
    D2D_CMD_BIND_INFO_REQ = 23100,         /**< 手机绑定设备信息查询请求。 */
    D2D_CMD_BIND_INFO_ACK = 23101,         /**< 手机绑定设备信息查询应答。 */
    D2D_CMD_BIND_RESULT_REQ = 23104,       /**< 手机绑定结果通知请求。 */
    D2D_CMD_BIND_RESULT_ACK = 23105,       /**< 手机绑定结果通知应答。 */
    D2D_CMD_VILLA_HEARTBEAT_REQ = 25000,   /**< 二次确认机或小门口机心跳请求。 */
    D2D_CMD_VILLA_HEARTBEAT_ACK = 25001,   /**< 二次确认机或小门口机心跳应答。 */
    D2D_CMD_SET_VILLA_ID_REQ = 25010,      /**< 设置别墅机房号请求。 */
    D2D_CMD_SET_VILLA_ID_ACK = 25011,      /**< 设置别墅机房号应答。 */
    D2D_CMD_SET_VILLA_INFO_REQ = 25020,    /**< 设置别墅机信息请求。 */
    D2D_CMD_SET_VILLA_INFO_ACK = 25021,    /**< 设置别墅机信息应答。 */
    D2D_CMD_GET_VILLA_INFO_REQ = 25030,    /**< 获取别墅机信息请求。 */
    D2D_CMD_GET_VILLA_INFO_ACK = 25031,    /**< 获取别墅机信息应答。 */
    D2D_CMD_GET_VILLA_INFO2_REQ = 25032,   /**< 获取别墅机版本信息请求。 */
    D2D_CMD_GET_VILLA_INFO2_ACK = 25033,   /**< 获取别墅机版本信息应答。 */
    D2D_CMD_SET_CARD_ADD_STATE_REQ = 25040,/**< 发卡状态通知请求。 */
    D2D_CMD_SET_CARD_ADD_STATE_ACK = 25041,/**< 发卡状态通知应答。 */
    D2D_CMD_SET_CARD_DELETE_STATE_REQ = 25050,/**< 删卡状态通知请求。 */
    D2D_CMD_SET_CARD_DELETE_STATE_ACK = 25051,/**< 删卡状态通知应答。 */
    D2D_CMD_DELETE_ALL_CARDS_REQ = 25060,  /**< 删除全部卡片请求。 */
    D2D_CMD_DELETE_ALL_CARDS_ACK = 25061,  /**< 删除全部卡片应答。 */
    D2D_CMD_DELETE_CARDS_REQ = 25062,      /**< 删除指定卡片请求。 */
    D2D_CMD_DELETE_CARDS_ACK = 25063,      /**< 删除指定卡片应答。 */
    D2D_CMD_GET_ALL_CARDS_REQ = 25064,     /**< 获取全部卡片请求。 */
    D2D_CMD_GET_ALL_CARDS_ACK = 25065,     /**< 获取全部卡片应答。 */
    D2D_CMD_APP_UPGRADE_REQ = 27000,       /**< 下发应用升级包请求。 */
    D2D_CMD_APP_UPGRADE_ACK = 27001,       /**< 下发应用升级包应答。 */
    D2D_CMD_SYS_UPGRADE_REQ = 27010,       /**< 下发系统升级包请求。 */
    D2D_CMD_SYS_UPGRADE_ACK = 27011        /**< 下发系统升级包应答。 */
} d2d_cmd_t;

/**
 * @brief 可选整数字段包装结构。
 */
typedef struct
{
    bool present; /**< 是否携带该整数字段。 */
    int value;    /**< 整数字段值。 */
} d2d_optional_int_t;

/**
 * @brief 可选字符串字段包装结构。
 */
typedef struct
{
    bool present; /**< 是否携带该字符串字段。 */
    char *value;  /**< 字符串字段内容，允许为 `NULL` 表示 JSON null。 */
} d2d_optional_string_t;

/**
 * @brief 可选整型数组字段包装结构。
 */
typedef struct
{
    bool present;  /**< 是否携带该整型数组字段。 */
    size_t count;  /**< 数组元素个数。 */
    int *items;    /**< 数组元素首地址。 */
} d2d_int_array_t;

/**
 * @brief 可选字符串数组字段包装结构。
 */
typedef struct
{
    bool present; /**< 是否携带该字符串数组字段。 */
    size_t count; /**< 数组元素个数。 */
    char **items; /**< 字符串数组元素首地址。 */
} d2d_string_array_t;

typedef struct d2d_device_info d2d_device_info_t;

/**
 * @brief 同步消息中使用的设备信息数组包装结构。
 */
typedef struct
{
    bool present;           /**< 是否携带设备信息数组。 */
    size_t count;           /**< 设备信息条目数。 */
    d2d_device_info_t *items; /**< 设备信息数组首地址。 */
} d2d_device_info_array_t;

/**
 * @brief 同步消息中使用的单个设备信息结构。
 */
struct d2d_device_info
{
    char *id;                       /**< 设备标识，文档中通常为房号、MAC 或逻辑 ID。 */
    int dev_type;                   /**< 设备类型，取值参考 `d2d_device_type_t`。 */
    char *dev_model;                /**< 设备型号字符串。 */
    d2d_optional_string_t ip;       /**< 设备 IP 地址。 */
    d2d_optional_string_t mac;      /**< 设备 MAC 地址。 */
    d2d_optional_string_t sys_ver;  /**< 系统版本号。 */
    d2d_optional_string_t app_ver;  /**< 应用版本号。 */
    d2d_optional_int_t net_type;    /**< 网络类型。 */
    d2d_optional_int_t state;       /**< 设备状态。 */
    d2d_device_info_array_t villas; /**< 下级别墅机设备列表。 */
};

/**
 * @brief 卡片信息结构。
 */
typedef struct
{
    d2d_optional_string_t serial; /**< 卡片序列号。 */
    char *num;                    /**< 卡号。 */
} d2d_card_info_t;

/**
 * @brief 卡片信息数组包装结构。
 */
typedef struct
{
    bool present;          /**< 是否携带卡片数组。 */
    size_t count;          /**< 卡片条目数量。 */
    d2d_card_info_t *items; /**< 卡片数组首地址。 */
} d2d_card_info_array_t;

/**
 * @brief 门锁配置项结构。
 */
typedef struct
{
    d2d_optional_int_t door_id;      /**< 门编号。 */
    d2d_optional_int_t lock_switch;  /**< 锁控开关状态。 */
    d2d_optional_int_t lock_time;    /**< 开锁保持时间。 */
    d2d_optional_int_t menic_switch; /**< 门磁开关状态。 */
    d2d_optional_int_t menic_time;   /**< 门磁延时时间。 */
} d2d_door_lock_t;

/**
 * @brief 门锁配置数组包装结构。
 */
typedef struct
{
    bool present;          /**< 是否携带门锁配置数组。 */
    size_t count;          /**< 门锁配置条目数。 */
    d2d_door_lock_t *items; /**< 门锁配置数组首地址。 */
} d2d_door_lock_array_t;

/**
 * @brief 别墅机基础配置结构。
 */
typedef struct
{
    bool present;                   /**< 是否携带基础配置对象。 */
    d2d_optional_int_t sound_call;  /**< 呼叫音量。 */
    d2d_optional_int_t sound_talk;  /**< 通话音量。 */
} d2d_villa_base_t;

/**
 * @brief 别墅机入户配置结构。
 */
typedef struct
{
    bool present;                    /**< 是否携带入户配置对象。 */
    d2d_door_lock_array_t door_lock; /**< 门锁配置数组。 */
} d2d_villa_entrance_t;

/**
 * @brief 别墅机 AI 配置结构。
 */
typedef struct
{
    bool present;                /**< 是否携带 AI 配置对象。 */
    d2d_optional_int_t motion;   /**< 移动侦测配置。 */
} d2d_villa_ai_t;

/**
 * @brief 各类心跳请求共用的消息体结构。
 */
typedef struct
{
    int dev_type;                  /**< 心跳上报设备类型。 */
    char *dev_model;               /**< 心跳上报设备型号。 */
    char *id;                      /**< 心跳上报设备标识。 */
    d2d_optional_string_t ip;      /**< 设备 IP 地址。 */
    d2d_optional_string_t mac;     /**< 设备 MAC 地址。 */
    d2d_optional_string_t sys_ver; /**< 系统版本号。 */
    d2d_optional_string_t app_ver; /**< 应用版本号。 */
    d2d_optional_int_t net_type;   /**< 网络类型。 */
    d2d_optional_string_t sync_ver;/**< 同步版本号，可为 JSON null。 */
} d2d_heartbeat_t;

/**
 * @brief 带同步标记的应答消息体结构。
 */
typedef struct
{
    int result;                    /**< 处理结果，通常参考文档错误码表。 */
    d2d_optional_int_t dev_type;   /**< 应答设备类型。 */
    d2d_optional_string_t dev_model; /**< 应答设备型号。 */
    d2d_optional_int_t sync;       /**< 是否需要继续发起同步。 */
} d2d_ack_sync_t;

/**
 * @brief 仅包含结果码的简单消息体结构。
 */
typedef struct
{
    int result; /**< 结果码。 */
} d2d_simple_result_t;

/**
 * @brief 同步请求消息体结构。
 */
typedef struct
{
    char *id;                       /**< 发起同步的设备标识。 */
    d2d_optional_string_t sync_ver; /**< 当前同步版本号。 */
    d2d_device_info_array_t entries;/**< 需要同步的设备条目列表。 */
} d2d_sync_request_t;

/**
 * @brief 仅包含 `id` 字段的请求消息体结构。
 */
typedef struct
{
    char *id; /**< 请求目标设备标识。 */
} d2d_id_request_t;

/**
 * @brief 开锁请求消息体结构。
 */
typedef struct
{
    char *id;                 /**< 开锁目标设备标识。 */
    d2d_optional_int_t place; /**< 开锁位置或门位编号。 */
} d2d_unlock_request_t;

/**
 * @brief 呼梯参数结构。
 */
typedef struct
{
    int type;                     /**< 呼梯类型。 */
    d2d_int_array_t floor;        /**< 目标楼层列表。 */
    d2d_optional_int_t floor_start; /**< 起始楼层。 */
    d2d_int_array_t floor_end;    /**< 终点楼层列表。 */
    d2d_optional_int_t direction; /**< 呼梯方向。 */
} d2d_elevator_info_t;

/**
 * @brief 呼梯请求消息体结构。
 */
typedef struct
{
    char *id;                    /**< 电梯设备标识。 */
    d2d_elevator_info_t elevator;/**< 呼梯参数对象。 */
} d2d_elevator_request_t;

/**
 * @brief 允许呼叫或转呼查询结果消息体结构。
 */
typedef struct
{
    int result;                       /**< 查询结果码。 */
    int state;                        /**< 是否允许呼叫或转呼的状态值。 */
    d2d_optional_string_t transfer_id;/**< 转呼目标设备标识。 */
} d2d_call_forward_result_t;

/**
 * @brief 设置开锁密码请求消息体结构。
 */
typedef struct
{
    char *id;  /**< 需要设置密码的设备标识。 */
    char *pwd; /**< 要写入的开锁密码。 */
} d2d_password_request_t;

/**
 * @brief 获取开锁密码结果消息体结构。
 */
typedef struct
{
    int result; /**< 查询结果码。 */
    char *id;   /**< 返回密码所属设备标识。 */
    char *pwd;  /**< 返回的开锁密码。 */
} d2d_password_result_t;

/**
 * @brief 手机绑定设备信息查询结果消息体结构。
 */
typedef struct
{
    int result; /**< 查询结果码。 */
    char *id;   /**< 被绑定设备标识。 */
    char *mac;  /**< 被绑定设备 MAC。 */
    char *sn;   /**< 被绑定设备序列号。 */
} d2d_bind_info_result_t;

/**
 * @brief 手机绑定结果通知消息体结构。
 */
typedef struct
{
    int bind; /**< 绑定结果，通常 `1` 为绑定成功，`0` 为解绑或失败。 */
} d2d_bind_result_request_t;

/**
 * @brief 设置别墅机房号请求消息体结构。
 */
typedef struct
{
    char *id;       /**< 当前设备标识。 */
    char *villa_id; /**< 要设置的别墅机房号。 */
} d2d_set_villa_id_request_t;

/**
 * @brief 设置别墅机信息请求消息体结构。
 */
typedef struct
{
    char *id;                    /**< 目标别墅机标识。 */
    d2d_villa_base_t base;       /**< 基础配置。 */
    d2d_villa_entrance_t entrance; /**< 入户配置。 */
    d2d_villa_ai_t ai;           /**< AI 配置。 */
} d2d_set_villa_info_request_t;

/**
 * @brief 获取别墅机信息结果消息体结构。
 */
typedef struct
{
    int result;                    /**< 查询结果码。 */
    d2d_villa_base_t base;         /**< 基础配置结果。 */
    d2d_villa_entrance_t entrance; /**< 入户配置结果。 */
    d2d_villa_ai_t ai;             /**< AI 配置结果。 */
} d2d_get_villa_info_result_t;

/**
 * @brief 获取别墅机版本信息结果消息体结构。
 */
typedef struct
{
    int result;   /**< 查询结果码。 */
    char *id;     /**< 设备标识。 */
    char *sys_ver;/**< 系统版本号。 */
    char *app_ver;/**< 应用版本号。 */
} d2d_villa_info2_result_t;

/**
 * @brief 发卡或删卡状态通知请求消息体结构。
 */
typedef struct
{
    char *id;  /**< 目标设备标识。 */
    int state; /**< 当前状态值。 */
} d2d_state_request_t;

/**
 * @brief 删除指定卡片请求消息体结构。
 */
typedef struct
{
    char *id;               /**< 目标设备标识。 */
    d2d_string_array_t nums;/**< 待删除卡号列表。 */
} d2d_delete_cards_request_t;

/**
 * @brief 获取全部卡片结果消息体结构。
 */
typedef struct
{
    int result;                 /**< 查询结果码。 */
    d2d_card_info_array_t cards;/**< 卡片列表。 */
} d2d_get_cards_result_t;

/**
 * @brief 应用升级请求消息体结构。
 */
typedef struct
{
    char *file_dev_model;              /**< 升级包适配的设备型号。 */
    d2d_optional_string_t file_app_type;/**< 应用升级包类型。 */
    char *file_app_ver;                /**< 应用升级版本号。 */
    char *file_name;                   /**< 升级文件名。 */
    int file_size;                     /**< 升级文件大小。 */
    char *file_md5;                    /**< 升级文件 MD5。 */
} d2d_app_upgrade_request_t;

/**
 * @brief 系统升级请求消息体结构。
 */
typedef struct
{
    char *file_dev_model;              /**< 升级包适配的设备型号。 */
    d2d_optional_string_t file_sys_type;/**< 系统升级包类型。 */
    char *file_sys_ver;                /**< 系统升级版本号。 */
    char *file_name;                   /**< 升级文件名。 */
    int file_size;                     /**< 升级文件大小。 */
    char *file_md5;                    /**< 升级文件 MD5。 */
} d2d_sys_upgrade_request_t;

/**
 * @brief d2d 外层固定头结构。
 */
typedef struct
{
    uint32_t head;                        /**< 固定帧头魔术字。 */
    /**
     * @brief 消息 ID，按定长字段编码。
     *
     * 按二线对讲 PLC 设备通讯协议规则，取 16 位可打印数字字符串：
     * 前 3 位为随机数，后 13 位为毫秒时间戳。请求和对应回复必须使用相同
     * `msgId`，用于表示同一条业务链路。
     */
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];
    char orig_addr[D2D_ADDR_MAX_LEN + 1];/**< 源地址。 */
    char dest_addr[D2D_ADDR_MAX_LEN + 1];/**< 目的地址。 */
    int32_t cmd;                         /**< 协议命令字。 */
    int32_t dev_type;                    /**< 设备类型。 */
    int32_t message_type;                /**< 报文方向与结果类型。 */
    int32_t length;                      /**< JSON content 字节长度。 */
} d2d_header_t;

/**
 * @brief 所有协议消息体的联合体。
 */
typedef union
{
    d2d_heartbeat_t heartbeat;                       /**< 心跳类消息体。 */
    d2d_ack_sync_t ack_sync;                        /**< 带同步标记的应答消息体。 */
    d2d_simple_result_t simple_result;              /**< 仅含结果码的消息体。 */
    d2d_sync_request_t sync_request;                /**< 同步请求消息体。 */
    d2d_id_request_t id_request;                    /**< 仅含 `id` 的请求消息体。 */
    d2d_unlock_request_t unlock_request;            /**< 开锁请求消息体。 */
    d2d_elevator_request_t elevator_request;        /**< 呼梯请求消息体。 */
    d2d_call_forward_result_t call_forward_result;  /**< 呼叫/转呼查询结果消息体。 */
    d2d_password_request_t password_request;        /**< 设置密码请求消息体。 */
    d2d_password_result_t password_result;          /**< 获取密码结果消息体。 */
    d2d_bind_info_result_t bind_info_result;        /**< 绑定设备信息结果消息体。 */
    d2d_bind_result_request_t bind_result_request;  /**< 绑定结果通知消息体。 */
    d2d_set_villa_id_request_t set_villa_id_request;/**< 设置别墅机房号消息体。 */
    d2d_set_villa_info_request_t set_villa_info_request;/**< 设置别墅机信息消息体。 */
    d2d_get_villa_info_result_t get_villa_info_result;/**< 获取别墅机信息结果消息体。 */
    d2d_villa_info2_result_t villa_info2_result;    /**< 获取别墅机版本结果消息体。 */
    d2d_state_request_t state_request;              /**< 状态通知请求消息体。 */
    d2d_delete_cards_request_t delete_cards_request;/**< 删除卡片请求消息体。 */
    d2d_get_cards_result_t get_cards_result;        /**< 获取卡片列表结果消息体。 */
    d2d_app_upgrade_request_t app_upgrade_request;  /**< 应用升级请求消息体。 */
    d2d_sys_upgrade_request_t sys_upgrade_request;  /**< 系统升级请求消息体。 */
} d2d_message_body_t;

typedef struct d2d_runtime d2d_runtime_t;
typedef struct d2d_runtime_event d2d_runtime_event_t;
typedef struct d2d_adapter d2d_adapter_t;
typedef struct d2d_adapter_event d2d_adapter_event_t;

/**
 * @brief d2d 完整消息对象结构。
 */
typedef struct
{
    d2d_header_t header;       /**< 外层固定头。 */
    d2d_message_body_t body;   /**< 当前命令对应的消息体。 */
} d2d_message_t;

/**
 * @brief 运行时请求句柄类型。
 */
typedef uint64_t d2d_request_handle_t;

/**
 * @brief 运行时透传的链路提示结构。
 */
typedef struct
{
    uintptr_t opaque;               /**< 由上层自定义的透传上下文标识。 */
    int link_kind;                  /**< 链路类型，通常用于 adapter 内部标识目标链路。 */
    uint32_t endpoint_addr;        /**< 目标或来源端点地址，主机字节序。 */
    uint16_t endpoint_port;        /**< 目标或来源端点端口，主机字节序。 */
    uint16_t reserved;             /**< 预留字段，当前固定为 0。 */
} d2d_route_hint_t;

/**
 * @brief 传输链路类型枚举。
 */
typedef enum
{
    D2D_LINK_ETH = 1, /**< 以太网控制链路。 */
    D2D_LINK_PLC = 2  /**< PLC 控制链路。 */
} d2d_link_kind_t;

/**
 * @brief 入站请求处理模式枚举。
 */
typedef enum
{
    D2D_REQUEST_MODE_NONE = 0, /**< 尚未声明处理模式，仅用于内部初始态。 */
    D2D_REQUEST_MODE_LOCAL = 1,/**< 按本地处理路径执行，失败或超时回 `messageType=2`。 */
    D2D_REQUEST_MODE_FORWARD = 2/**< 按转发处理路径执行，失败或超时回 `messageType=3`。 */
} d2d_request_mode_t;

/**
 * @brief 入站请求失败原因枚举。
 */
typedef enum
{
    D2D_REQUEST_FAIL_UNSPECIFIED = 0, /**< 未指定失败原因。 */
    D2D_REQUEST_FAIL_REJECTED = 1,    /**< 上层主动拒绝处理。 */
    D2D_REQUEST_FAIL_UNAVAILABLE = 2, /**< 目标资源不可用，例如设备不存在或链路不可达。 */
    D2D_REQUEST_FAIL_TIMEOUT = 3      /**< 处理超时。 */
} d2d_request_fail_reason_t;

/**
 * @brief 运行时事件类型枚举。
 */
typedef enum
{
    D2D_RUNTIME_EVENT_TX_FRAME = 1,       /**< 需要上层发送一条完整 d2d 二进制帧。 */
    D2D_RUNTIME_EVENT_REQUEST_IN = 2,     /**< 收到入站请求，等待上层异步处理。 */
    D2D_RUNTIME_EVENT_REQUEST_DONE = 3,   /**< 某条出站请求收到成功应答。 */
    D2D_RUNTIME_EVENT_REQUEST_FAIL = 4,   /**< 某条出站请求收到失败或转发失败应答。 */
    D2D_RUNTIME_EVENT_REQUEST_TIMEOUT = 5 /**< 某条出站请求发生超时。 */
} d2d_runtime_event_type_t;

/**
 * @brief 发送帧事件数据结构。
 */
typedef struct
{
    uint8_t *frame;                        /**< 待发送帧缓冲区，回调接收后由上层负责调用 `d2d_mem_free()` 释放。 */
    size_t frame_len;                      /**< 待发送帧长度，单位为字节。 */
    d2d_cmd_t cmd;                         /**< 关联命令字。 */
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];   /**< 关联消息 ID。 */
} d2d_runtime_tx_frame_event_t;

/**
 * @brief 入站请求事件数据结构。
 */
typedef struct
{
    d2d_request_handle_t handle;        /**< 状态机分配的入站请求句柄。 */
    const d2d_message_t *message;       /**< 原始请求消息快照，仅在当前事件回调期间有效。 */
} d2d_runtime_request_in_event_t;

/**
 * @brief 出站请求成功完成事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    const d2d_message_t *request;        /**< 原始请求消息快照，仅在当前事件回调期间有效。 */
    const d2d_message_t *reply;          /**< 成功应答消息快照，仅在当前事件回调期间有效。 */
} d2d_runtime_request_done_event_t;

/**
 * @brief 出站请求失败完成事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    d2d_message_type_t reply_type;       /**< 实际收到的失败应答类型，仅允许 `2` 或 `3`。 */
    const d2d_message_t *request;        /**< 原始请求消息快照，仅在当前事件回调期间有效。 */
    const d2d_message_t *reply;          /**< 失败应答消息快照，仅在当前事件回调期间有效。 */
} d2d_runtime_request_fail_event_t;

/**
 * @brief 出站请求超时事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    const d2d_message_t *request;        /**< 原始请求消息快照，仅在当前事件回调期间有效。 */
} d2d_runtime_request_timeout_event_t;

/**
 * @brief 运行时事件负载联合体。
 */
typedef union
{
    d2d_runtime_tx_frame_event_t tx_frame;             /**< 发送帧事件负载。 */
    d2d_runtime_request_in_event_t request_in;         /**< 入站请求事件负载。 */
    d2d_runtime_request_done_event_t request_done;     /**< 请求成功事件负载。 */
    d2d_runtime_request_fail_event_t request_fail;     /**< 请求失败事件负载。 */
    d2d_runtime_request_timeout_event_t request_timeout;/**< 请求超时事件负载。 */
} d2d_runtime_event_data_t;

/**
 * @brief 运行时事件对象结构。
 */
struct d2d_runtime_event
{
    d2d_runtime_event_type_t type; /**< 事件类型。 */
    uint64_t now_ms;               /**< 触发事件时使用的毫秒时间戳。 */
    d2d_route_hint_t route_hint;   /**< 与事件关联的链路提示，仅透传给上层。 */
    d2d_runtime_event_data_t data; /**< 事件负载。 */
};

/**
 * @brief 运行时事件回调函数类型。
 *
 * 事件回调为同步调用，且不允许在回调内部重入同一个 `d2d_runtime_t`
 * 实例的任意公开接口。`TX_FRAME` 事件中的 `frame` 所有权会转移给上层，
 * 上层发送完成后必须调用 `d2d_mem_free()` 释放。
 *
 * @param[in] user_data 创建运行时时传入的用户上下文。
 * @param[in] event 当前事件对象，仅在回调期间有效。
 */
typedef void (*d2d_runtime_event_fn_t)(void *user_data, const d2d_runtime_event_t *event);

/**
 * @brief 运行时配置结构。
 */
typedef struct
{
    int local_dev_type;                         /**< 本端设备类型，用于生成失败或转发失败应答头。 */
    uint32_t heartbeat_timeout_ms;              /**< 心跳类请求超时时间，单位为毫秒。 */
    uint32_t sync_timeout_ms;                   /**< 同步类请求超时时间，单位为毫秒。 */
    uint32_t request_timeout_ms;                /**< 普通业务请求超时时间，单位为毫秒。 */
    uint32_t inbound_timeout_ms;                /**< 入站异步处理超时时间，单位为毫秒。 */
    bool disable_auto_follow_up_sync;           /**< 是否禁用 ACK sync=1 的自动同步请求。 */
    d2d_runtime_event_fn_t event_cb;            /**< 运行时事件回调，不能为空。 */
    void *user_data;                            /**< 透传给事件回调的用户上下文。 */
} d2d_runtime_config_t;

/**
 * @brief adapter 对外业务事件类型枚举。
 */
typedef enum
{
    D2D_ADAPTER_EVENT_REQUEST_IN = 1,     /**< 收到一条入站业务请求。 */
    D2D_ADAPTER_EVENT_REQUEST_DONE = 2,   /**< 一条出站业务请求收到成功应答。 */
    D2D_ADAPTER_EVENT_REQUEST_FAIL = 3,   /**< 一条出站业务请求收到失败应答。 */
    D2D_ADAPTER_EVENT_REQUEST_TIMEOUT = 4,/**< 一条出站业务请求超时。 */
    D2D_ADAPTER_EVENT_LINK_ERROR = 5      /**< 某条链路在收发或线程驱动中发生错误。 */
} d2d_adapter_event_type_t;

/**
 * @brief adapter 链路配置结构。
 */
typedef struct
{
    d2d_link_kind_t link_kind;         /**< 链路类型，单个 adapter 内要求唯一。 */
    transport_handle_t *transport;     /**< 已打开的传输句柄，由调用方创建并持有。 */
    transport_endpoint_t local_endpoint;/**< 当前链路的本地监听端点。 */
    const char *name;                  /**< 可选链路名称，仅用于日志与调试。 */
    void *user_data;                   /**< 可选链路私有数据，原样透传到事件。 */
} d2d_adapter_link_t;

/**
 * @brief adapter 出站请求目标结构。
 */
typedef struct
{
    d2d_link_kind_t link_kind;          /**< 要发送到的目标链路。 */
    transport_endpoint_t remote_endpoint;/**< 目标传输端点。 */
    uintptr_t opaque;                   /**< 上层自定义透传字段。 */
} d2d_adapter_request_t;

/**
 * @brief adapter 入站请求事件数据结构。
 */
typedef struct
{
    d2d_request_handle_t handle;        /**< 入站请求句柄。 */
    d2d_link_kind_t ingress_link;       /**< 收到该请求的入口链路。 */
    transport_endpoint_t src_endpoint;  /**< 入口链路上的源传输端点。 */
    const d2d_message_t *message;       /**< 解码后的请求消息，仅在当前回调期间有效。 */
} d2d_adapter_request_in_event_t;

/**
 * @brief adapter 出站成功事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];/**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;              /**< 原请求命令字。 */
    d2d_link_kind_t target_link;        /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                   /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;       /**< 原请求消息，仅在当前回调期间有效。 */
    const d2d_message_t *reply;         /**< 成功应答消息，仅在当前回调期间有效。 */
} d2d_adapter_request_done_event_t;

/**
 * @brief adapter 出站失败事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];/**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;              /**< 原请求命令字。 */
    d2d_message_type_t reply_type;      /**< 失败应答类型。 */
    d2d_link_kind_t target_link;        /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                   /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;       /**< 原请求消息，仅在当前回调期间有效。 */
    const d2d_message_t *reply;         /**< 失败应答消息，仅在当前回调期间有效。 */
} d2d_adapter_request_fail_event_t;

/**
 * @brief adapter 出站超时事件数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];/**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;              /**< 原请求命令字。 */
    d2d_link_kind_t target_link;        /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                   /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;       /**< 原请求消息，仅在当前回调期间有效。 */
} d2d_adapter_request_timeout_event_t;

/**
 * @brief adapter 链路错误事件数据结构。
 */
typedef struct
{
    d2d_link_kind_t link_kind;          /**< 出错链路类型。 */
    transport_endpoint_t local_endpoint;/**< 本地链路端点。 */
    transport_endpoint_t remote_endpoint;/**< 关联的远端链路端点，未知时为全 0。 */
    uintptr_t opaque;                   /**< 关联请求透传的上下文标识，未知时为 0。 */
    int error_code;                     /**< 错误码，通常为 transport 返回值或内部负值错误码。 */
    const char *operation;              /**< 出错操作名称，仅在当前回调期间有效。 */
    void *link_user_data;               /**< 对应链路的用户私有数据。 */
} d2d_adapter_link_error_event_t;

/**
 * @brief adapter 事件负载联合体。
 */
typedef union
{
    d2d_adapter_request_in_event_t request_in;         /**< 入站请求事件负载。 */
    d2d_adapter_request_done_event_t request_done;     /**< 出站成功事件负载。 */
    d2d_adapter_request_fail_event_t request_fail;     /**< 出站失败事件负载。 */
    d2d_adapter_request_timeout_event_t request_timeout;/**< 出站超时事件负载。 */
    d2d_adapter_link_error_event_t link_error;         /**< 链路错误事件负载。 */
} d2d_adapter_event_data_t;

/**
 * @brief adapter 事件对象结构。
 */
struct d2d_adapter_event
{
    d2d_adapter_event_type_t type; /**< 事件类型。 */
    uint64_t now_ms;               /**< 事件触发时间戳，单位为毫秒。 */
    d2d_adapter_event_data_t data; /**< 事件负载。 */
};

/**
 * @brief adapter 事件回调函数类型。
 *
 * 该回调由 adapter 内部线程同步调用。事件内的消息对象仅在本次回调期间有效。
 * 上层如果需要跨回调保存消息内容，应自行复制。回调内部允许调用
 * `d2d_adapter_begin_request()` / `d2d_adapter_complete_request()` /
 * `d2d_adapter_fail_request()`，这些接口只负责线程安全入队。
 *
 * @param[in] user_data 创建 adapter 时传入的用户上下文。
 * @param[in] event 当前 adapter 事件对象。
 */
typedef void (*d2d_adapter_event_fn_t)(void *user_data, const d2d_adapter_event_t *event);

/**
 * @brief adapter 配置结构。
 */
typedef struct
{
    int local_dev_type;                    /**< 本端设备类型，传给内部 runtime。 */
    uint32_t heartbeat_timeout_ms;         /**< 心跳类请求超时时间。 */
    uint32_t sync_timeout_ms;              /**< 同步类请求超时时间。 */
    uint32_t request_timeout_ms;           /**< 普通业务请求超时时间。 */
    uint32_t inbound_timeout_ms;           /**< 入站异步处理超时时间。 */
    uint32_t poll_wait_ms;                 /**< 内部线程 select 等待时间，单位为毫秒。 */
    bool disable_auto_follow_up_sync;      /**< 是否禁用 ACK sync=1 的自动同步请求。 */
    const d2d_adapter_link_t *links;       /**< 链路配置数组。 */
    size_t link_count;                     /**< 链路数量。 */
    d2d_adapter_event_fn_t event_cb;       /**< 业务事件回调。 */
    void *user_data;                       /**< 业务事件回调用户上下文。 */
} d2d_adapter_config_t;

/**
 * @brief 按指定命令初始化消息对象。
 *
 * @param[out] message 待初始化的消息对象。
 * @param[in] cmd 协议命令字。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_message_init(d2d_message_t *message, d2d_cmd_t cmd);

/**
 * @brief 释放消息对象持有的动态内存字段。
 *
 * @param[in,out] message 待释放的消息对象。
 */
void d2d_message_free(d2d_message_t *message);

/**
 * @brief 将消息体编码为 JSON 字符串。
 *
 * 返回的缓冲区由 D2D protocol 申请，调用方必须通过 `d2d_mem_free()` 释放，
 * 不应直接调用 libc `free()`。
 *
 * @param[in] message 待编码的消息对象。
 * @param[out] json_out 编码后的 JSON 缓冲区指针。
 * @param[out] json_len 编码后的 JSON 长度，不包含结尾 `\0`。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_encode_content_json(const d2d_message_t *message, char **json_out, size_t *json_len);

/**
 * @brief 将 JSON 字符串解码为消息对象。
 *
 * @param[out] message 输出消息对象。
 * @param[in] cmd 期望解析出的命令字。
 * @param[in] json 输入 JSON 缓冲区。
 * @param[in] json_len 输入 JSON 长度，单位为字节。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_decode_content_json(d2d_message_t *message, d2d_cmd_t cmd, const char *json, size_t json_len);

/**
 * @brief 将完整消息编码为 d2d 二进制帧。
 *
 * 返回的缓冲区由 D2D protocol 申请，调用方必须通过 `d2d_mem_free()` 释放。
 *
 * @param[in] message 待编码的消息对象。
 * @param[out] frame_out 编码后的帧缓冲区。
 * @param[out] frame_len 编码后的帧长度。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_encode_frame(const d2d_message_t *message, uint8_t **frame_out, size_t *frame_len);

/**
 * @brief 将 d2d 二进制帧解码为消息对象。
 *
 * @param[out] message 输出消息对象。
 * @param[in] frame 输入原始帧缓冲区。
 * @param[in] frame_len 输入原始帧长度。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_decode_frame(d2d_message_t *message, const uint8_t *frame, size_t frame_len);

/**
 * @brief 按协议规则校验消息对象是否合法。
 *
 * @param[in] message 待校验的消息对象。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_validate_message(const d2d_message_t *message);

/**
 * @brief 创建轻量运行时实例。
 *
 * 运行时实例不承诺多线程并发安全，默认由单一 IO 线程拥有和驱动。
 *
 * @param[in] config 运行时配置。
 * @param[out] out 输出运行时实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_create(const d2d_runtime_config_t *config, d2d_runtime_t **out);

/**
 * @brief 销毁轻量运行时实例并释放内部资源。
 *
 * @param[in,out] runtime 待销毁的运行时实例。
 */
void d2d_runtime_destroy(d2d_runtime_t *runtime);

/**
 * @brief 推进运行时的毫秒时间并执行超时扫描。
 *
 * 该接口只负责扫描超时，不负责周期性发心跳或重试。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] now_ms 当前毫秒时间戳。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_tick(d2d_runtime_t *runtime, uint64_t now_ms);

/**
 * @brief 发送一条出站业务请求。
 *
 * 调用方只需提供业务请求内容与基础头字段，运行时会生成 `msgId`、
 * 建立 `pending` 记录并派发 `TX_FRAME` 事件。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] request 业务请求消息，要求 `messageType=0`。
 * @param[in] route_hint 透传给事件的链路提示，可为 `NULL`。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_send_request(d2d_runtime_t *runtime,
                             const d2d_message_t *request,
                             const d2d_route_hint_t *route_hint);

/**
 * @brief 输入一条入站原始帧。
 *
 * 运行时会完成解码、分类、结束出站 `pending`、必要时生成 follow-up
 * 请求，并通过事件回调向上层派发结果。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] frame 入站原始帧。
 * @param[in] frame_len 入站帧长度。
 * @param[in] route_hint 透传给事件的链路提示，可为 `NULL`。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_input_frame(d2d_runtime_t *runtime,
                            const uint8_t *frame,
                            size_t frame_len,
                            const d2d_route_hint_t *route_hint);

/**
 * @brief 声明入站请求的处理模式。
 *
 * 上层在收到 `REQUEST_IN` 事件后，应先调用本接口声明按本地处理还是按转发
 * 处理，再在后续时机调用 `d2d_runtime_complete_request()` 或
 * `d2d_runtime_fail_request()` 回提结果。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] mode 处理模式。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_begin_request(d2d_runtime_t *runtime,
                              d2d_request_handle_t handle,
                              d2d_request_mode_t mode);

/**
 * @brief 回提出站成功应答内容。
 *
 * 上层需构造完整成功应答消息体，运行时会补齐关联头字段并派发 `TX_FRAME`
 * 事件。应答命令字必须与原请求匹配。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reply 成功应答消息。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_complete_request(d2d_runtime_t *runtime,
                                 d2d_request_handle_t handle,
                                 const d2d_message_t *reply);

/**
 * @brief 回提出站失败结果。
 *
 * 运行时会根据此前声明的处理模式自动生成 `messageType=2` 或
 * `messageType=3` 的失败应答，并派发 `TX_FRAME` 事件。
 *
 * @param[in,out] runtime 运行时实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reason 失败原因，仅用于日志与内部状态记录，不影响线上帧结构。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_runtime_fail_request(d2d_runtime_t *runtime,
                             d2d_request_handle_t handle,
                             d2d_request_fail_reason_t reason);

/**
 * @brief 创建 transport adapter 实例。
 *
 * adapter 自带内部工作线程，但创建时不会立即启动。调用方需先准备好所有
 * `transport_handle_t`，再通过 `d2d_adapter_start()` 启动。
 *
 * @param[in] config adapter 配置。
 * @param[out] out 输出 adapter 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_create(const d2d_adapter_config_t *config, d2d_adapter_t **out);

/**
 * @brief 启动 adapter 内部线程。
 *
 * @param[in,out] adapter adapter 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_start(d2d_adapter_t *adapter);

/**
 * @brief 停止 adapter 内部线程。
 *
 * 该接口会等待内部线程退出，但不会关闭外部传入的 `transport_handle_t`。
 *
 * @param[in,out] adapter adapter 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_stop(d2d_adapter_t *adapter);

/**
 * @brief 销毁 adapter 实例并释放内部资源。
 *
 * 销毁前若线程仍在运行，会先执行一次 stop。外部 transport 句柄仍由调用方
 * 自行关闭和释放。
 *
 * @param[in,out] adapter adapter 实例。
 */
void d2d_adapter_destroy(d2d_adapter_t *adapter);

/**
 * @brief 线程安全地提交一条出站业务请求。
 *
 * 请求会先入队，再由 adapter 内部线程驱动 runtime 和底层 transport。
 *
 * @param[in,out] adapter adapter 实例。
 * @param[in] request 业务请求消息。
 * @param[in] tx 出站目标配置。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_send_request(d2d_adapter_t *adapter,
                             const d2d_message_t *request,
                             const d2d_adapter_request_t *tx);

/**
 * @brief 线程安全地声明入站请求的处理模式。
 *
 * @param[in,out] adapter adapter 实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] mode 处理模式。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_begin_request(d2d_adapter_t *adapter,
                              d2d_request_handle_t handle,
                              d2d_request_mode_t mode);

/**
 * @brief 线程安全地回提出站成功应答。
 *
 * @param[in,out] adapter adapter 实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reply 成功应答消息。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_complete_request(d2d_adapter_t *adapter,
                                 d2d_request_handle_t handle,
                                 const d2d_message_t *reply);

/**
 * @brief 线程安全地回提出站失败结果。
 *
 * @param[in,out] adapter adapter 实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reason 失败原因。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_adapter_fail_request(d2d_adapter_t *adapter,
                             d2d_request_handle_t handle,
                             d2d_request_fail_reason_t reason);

/**
 * @brief 将命令字转换为调试用短字符串。
 *
 * @param[in] cmd 协议命令字。
 * @return 返回静态只读字符串。
 */
const char *d2d_cmd_to_string(d2d_cmd_t cmd);

/**
 * @brief 将负值错误码转换为文本描述。
 *
 * @param[in] error_code 对外 API 返回的负值错误码。
 * @return 返回静态只读字符串。
 */
const char *d2d_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif
