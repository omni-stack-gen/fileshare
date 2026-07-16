/**
 * @file
 * @brief simple 示例应用公共模块 simple_session 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_session.h"

const char *simple_session_state_name(simple_session_state_t state)
{
    switch (state)
    {
    case SIMPLE_SESSION_IDLE:
        return "idle";
    case SIMPLE_SESSION_CALL_RINGING:
        return "call_ringing";
    case SIMPLE_SESSION_CALL_VIDEO_ONLY:
        return "call_video_only";
    case SIMPLE_SESSION_CALL_ACTIVE:
        return "call_active";
    case SIMPLE_SESSION_MONITOR_ONLY:
        return "monitor_only";
    default:
        return "unknown";
    }
}
