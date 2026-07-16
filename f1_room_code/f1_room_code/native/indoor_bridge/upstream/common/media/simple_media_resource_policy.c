/**
 * @file
 * @brief simple 层媒体资源准入策略实现。
 */

#include "simple_media_resource_policy.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void simple_media_resource_policy_reason(simple_media_resource_decision_t *decision,
                                                const char *reason)
{
    if (!decision)
    {
        return;
    }

    snprintf(decision->reason, sizeof(decision->reason), "%s", reason ? reason : "-");
}

void simple_media_resource_policy_init(simple_media_resource_policy_t *policy)
{
    if (!policy)
    {
        return;
    }

    memset(policy, 0, sizeof(*policy));
    policy->plc_video_max = SIMPLE_MEDIA_RESOURCE_POLICY_DEFAULT_PLC_VIDEO_MAX;
}

void simple_media_resource_policy_set_plc_video_max(simple_media_resource_policy_t *policy,
                                                    unsigned int max_count)
{
    if (!policy)
    {
        return;
    }

    policy->plc_video_max = max_count;
}

int simple_media_resource_policy_check(const simple_media_resource_policy_t *policy,
                                       const simple_media_resource_request_t *request,
                                       unsigned int current_plc_video_count,
                                       simple_media_resource_decision_t *decision)
{
    if (!policy || !request || !decision)
    {
        return -EINVAL;
    }

    memset(decision, 0, sizeof(*decision));
    decision->admitted = true;
    decision->ret = 0;
    decision->result_code = (uint8_t)CALL_RESULT_OK;
    decision->used = current_plc_video_count;
    decision->limit = policy->plc_video_max;
    simple_media_resource_policy_reason(decision, "not_limited");

    if (request->media_kind != SIMPLE_SESSION_MEDIA_KIND_VIDEO ||
        request->link_kind != CALL_MEDIA_ENDPOINT_LINK_PLC)
    {
        return 0;
    }

    if (current_plc_video_count >= policy->plc_video_max)
    {
        decision->admitted = false;
        decision->ret = -EBUSY;
        decision->result_code = (uint8_t)CALL_RESULT_BUSY;
        simple_media_resource_policy_reason(decision, "plc_video_limit");
    }
    else if (request->session_kind == SIMPLE_SESSION_RECORD_KIND_CALL)
    {
        simple_media_resource_policy_reason(decision, "call_priority");
    }
    else
    {
        simple_media_resource_policy_reason(decision, "monitor_admitted");
    }

    return 0;
}
