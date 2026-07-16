/**
 * @file
 * @brief simple 示例应用公共 media_profile shape helper。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_MEDIA_PROFILE_SHAPE_H__
#define __SIMPLE_MEDIA_PROFILE_SHAPE_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief profile shape result reason 字符串缓冲区长度，单位为字节。 */
#define SIMPLE_MEDIA_PROFILE_SHAPE_REASON_MAX 64

/**
 * @brief TX media_profile shape 策略输入。
 */
typedef struct
{
    /** 即将发送的 call 消息类型。 */
    uint8_t msg_type;
    /** 当前 session id。 */
    uint64_t session_id;
    /** caller device_id。 */
    const char *caller_device_id;
    /** callee device_id。 */
    const char *callee_device_id;
    /** 对端 numeric peer id。 */
    uint32_t peer_id;
} simple_tx_profile_shape_request_t;

/**
 * @brief TX media_profile 发送前形态策略。
 *
 * 回调在 endpoint provider 分配前执行，可按业务策略裁剪 media_mask/dir/endpoint。
 * 返回负数会终止本次消息发送。
 */
typedef int (*simple_tx_profile_shape_policy_t)(
    void *user_ctx,
    const simple_tx_profile_shape_request_t *request,
    call_media_profile_t *profile);

/**
 * @brief media_profile shape 完整请求。
 */
typedef struct
{
    /** 即将发送的 call 消息类型。 */
    uint8_t msg_type;
    /** 当前 session id。 */
    uint64_t session_id;
    /** caller device_id。 */
    const char *caller_device_id;
    /** callee device_id。 */
    const char *callee_device_id;
    /** 对端 numeric peer id。 */
    uint32_t peer_id;
    /** 可选 TX shape policy。 */
    simple_tx_profile_shape_policy_t tx_policy;
    /** 透传给 tx_policy 的用户上下文。 */
    void *tx_policy_ctx;
    /** 可选对端 offer profile，用于 answer 裁剪。 */
    const call_media_profile_t *offer_profile;
    /** true 表示 offer_profile 有效。 */
    bool has_offer_profile;
} simple_media_profile_shape_request_t;

/**
 * @brief media_profile shape 结果。
 */
typedef struct
{
    /** shape 返回值，0 表示成功。 */
    int ret;
    /** shape 前 media_mask。 */
    uint8_t before_mask;
    /** shape 后 media_mask。 */
    uint8_t after_mask;
    /** offer media_mask；无 offer 时为 0。 */
    uint8_t offer_mask;
    /** true 表示 answer 根据 offer 裁剪。 */
    bool answer_shaped_from_offer;
    /** shape 结果原因。 */
    char reason[SIMPLE_MEDIA_PROFILE_SHAPE_REASON_MAX];
} simple_media_profile_shape_result_t;

/** @brief 按消息类型填充默认 media_profile。 */
void simple_media_profile_shape_fill_default(call_media_profile_t *profile,
                                             uint8_t msg_type);

/**
 * @brief 生成发送前 media_profile shape。
 *
 * @param[in] request shape 请求，不能为 NULL。
 * @param[in,out] profile 待填充/裁剪的 profile，不能为 NULL。
 * @param[out] result shape 结果，可为 NULL。
 *
 * @retval 0 shape 成功。
 * @retval -EINVAL 参数非法。
 * @return 其它负值表示 TX shape policy 或内部策略拒绝。
 */
int simple_media_profile_shape_prepare(
    const simple_media_profile_shape_request_t *request,
    call_media_profile_t *profile,
    simple_media_profile_shape_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __SIMPLE_MEDIA_PROFILE_SHAPE_H__ */
