/**
 * @file call_media_link.h
 * @brief media_profile 建链前 dry-run 决策封装。
 */
#ifndef __CALL_MEDIA_LINK_H__
#define __CALL_MEDIA_LINK_H__

#include "mtrans/call/call_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 建链前决策日志回调。
 *
 * @param user_ctx 用户上下文。
 * @param line 单行日志，不含换行符。
 */
typedef void (*call_media_link_log_cb)(void *user_ctx, const char *line);

/**
 * @brief 建链前 dry-run 决策输入。
 *
 * 该输入只描述当前消息和 `media_profile`，不会携带额外 endpoint 输入。
 * endpoint 建链输入必须来自 `media_profile.video_endpoint/audio_endpoint`。
 */
typedef struct
{
    /** @brief 当前信令消息类型，见 `call_msg_type_t`。 */
    uint8_t msg_type;
    /** @brief 当前消息携带或本地生成的 `media_profile`。 */
    const call_media_profile_t *profile;
    /** @brief 可选日志前缀；为空时使用实现默认前缀。 */
    const char *log_prefix;
    /** @brief 可选日志回调。 */
    call_media_link_log_cb log_cb;
    /** @brief 日志回调上下文。 */
    void *log_user_ctx;
} call_media_link_decision_request_t;

/**
 * @brief 获取 endpoint 来源名称。
 *
 * @param source endpoint 来源，见 `call_media_endpoint_source_t`。
 * @return 字符串常量。
 */
const char *call_media_endpoint_source_name(uint8_t source);

/**
 * @brief 执行 media_profile 建链前 dry-run 决策并输出统一日志。
 *
 * 该函数只做 dry-run 决策，不启动媒体、不修改 endpoint。
 * 真实 TX/RX endpoint 由当前 `media_profile` 中的 endpoint 字段提供。
 *
 * @param cfg 决策输入，不可为 `NULL`。
 * @param decision_out 决策输出，不可为 `NULL`。
 * @return 成功返回 `0`；profile 非法返回 `-EINVAL`；活跃媒体缺 wire endpoint 返回
 *         `-ENOENT`；其它失败返回负值错误码。
 */
int call_media_profile_link_decide(
    const call_media_link_decision_request_t *cfg,
    call_media_link_decision_t *decision_out);

#ifdef __cplusplus
}
#endif

#endif
