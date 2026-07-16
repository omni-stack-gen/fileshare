/**
 * @file
 * @brief simple 示例应用公共模块 simple_session 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_SESSION_H__
#define __SIMPLE_SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief simple 呼叫/监视会话状态。
 */
typedef enum
{
    /** 当前没有呼叫或监视会话。 */
    SIMPLE_SESSION_IDLE = 0,

    /** 呼叫已收到振铃或正在等待对端响应。 */
    SIMPLE_SESSION_CALL_RINGING,

    /** 呼叫或监视仅启用视频，音频尚未建立。 */
    SIMPLE_SESSION_CALL_VIDEO_ONLY,

    /** 呼叫已经进入音视频活动状态。 */
    SIMPLE_SESSION_CALL_ACTIVE,

    /** 当前处于监视模式，不属于普通呼叫。 */
    SIMPLE_SESSION_MONITOR_ONLY,
} simple_session_state_t;

/**
 * @brief 将会话状态转换为诊断字符串。
 *
 * @param[in] state 会话状态枚举。
 * @return 静态字符串，调用方不需要释放。
 */
const char *simple_session_state_name(simple_session_state_t state);

#ifdef __cplusplus
}
#endif

#endif
