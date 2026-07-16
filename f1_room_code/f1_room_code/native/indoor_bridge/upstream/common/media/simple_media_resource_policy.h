/**
 * @file
 * @brief simple 层媒体资源准入策略。
 */

#ifndef __SIMPLE_MEDIA_RESOURCE_POLICY_H__
#define __SIMPLE_MEDIA_RESOURCE_POLICY_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_proto.h"
#include "simple_session_manager.h"
#include "simple_session_media_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 默认允许的 PLC video 最大并发路数。 */
#define SIMPLE_MEDIA_RESOURCE_POLICY_DEFAULT_PLC_VIDEO_MAX 2U
/** @brief 资源策略 reason 字符串缓冲区长度，单位为字节。 */
#define SIMPLE_MEDIA_RESOURCE_POLICY_REASON_MAX 64

/**
 * @brief 媒体资源策略配置。
 */
typedef struct
{
    /** 允许的 PLC video 最大并发路数。 */
    unsigned int plc_video_max;
} simple_media_resource_policy_t;

/**
 * @brief 媒体资源准入请求。
 */
typedef struct
{
    /** session id。 */
    uint64_t session_id;
    /** session 类型。 */
    simple_session_record_kind_t session_kind;
    /** 媒体类型。 */
    simple_session_media_kind_t media_kind;
    /** call media endpoint link kind。 */
    uint8_t link_kind;
} simple_media_resource_request_t;

/**
 * @brief 媒体资源准入决策。
 */
typedef struct
{
    /** true 表示允许启动媒体。 */
    bool admitted;
    /** 决策返回值，0 表示允许，负值表示拒绝或错误。 */
    int ret;
    /** 映射到 CALL/MONITOR result_code 的结果码。 */
    uint8_t result_code;
    /** 当前已使用资源数量。 */
    unsigned int used;
    /** 当前资源上限。 */
    unsigned int limit;
    /** 决策原因字符串。 */
    char reason[SIMPLE_MEDIA_RESOURCE_POLICY_REASON_MAX];
} simple_media_resource_decision_t;

/** @brief 初始化资源策略默认值。 */
void simple_media_resource_policy_init(simple_media_resource_policy_t *policy);

/** @brief 设置允许的 PLC video 最大并发路数。 */
void simple_media_resource_policy_set_plc_video_max(simple_media_resource_policy_t *policy,
                                                    unsigned int max_count);

/**
 * @brief 检查一次媒体资源准入。
 *
 * 当前第一轮真实 deny 只覆盖 PLC video 路数限制。
 *
 * @param[in] policy 策略实例，不能为 NULL。
 * @param[in] request 准入请求，不能为 NULL。
 * @param[in] current_plc_video_count 当前已占用 PLC video 路数。
 * @param[out] decision 决策输出，不能为 NULL。
 *
 * @retval 0 检查完成；是否准入由 decision->admitted 表示。
 * @retval -EINVAL 参数非法。
 * @note 资源不足时本函数仍返回 0，并通过 decision->admitted=false、
 *       decision->ret=-EBUSY 和 reason 表达拒绝。
 */
int simple_media_resource_policy_check(const simple_media_resource_policy_t *policy,
                                       const simple_media_resource_request_t *request,
                                       unsigned int current_plc_video_count,
                                       simple_media_resource_decision_t *decision);

#ifdef __cplusplus
}
#endif

#endif
