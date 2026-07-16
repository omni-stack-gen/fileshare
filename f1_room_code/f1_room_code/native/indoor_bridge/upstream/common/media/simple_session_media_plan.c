/**
 * @file
 * @brief simple 层 session media plan 记录实现。
 */

#include "simple_session_media_plan.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

static int simple_session_media_plan_find_slot(const simple_session_media_plan_t *plan,
                                               uint64_t session_id,
                                               simple_session_media_kind_t media_kind)
{
    size_t i;

    if (!plan || session_id == 0 || media_kind == SIMPLE_SESSION_MEDIA_KIND_NONE)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        if (plan->slots[i].in_use &&
            plan->slots[i].record.session_id == session_id &&
            plan->slots[i].record.media_kind == media_kind)
        {
            return (int)i;
        }
    }

    return -ENOENT;
}

static int simple_session_media_plan_find_free_slot(const simple_session_media_plan_t *plan)
{
    size_t i;

    if (!plan)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        if (!plan->slots[i].in_use)
        {
            return (int)i;
        }
    }

    return -ENOSPC;
}

static int simple_session_media_plan_ensure(simple_session_media_plan_t *plan,
                                            uint64_t session_id,
                                            simple_session_media_kind_t media_kind)
{
    int slot;

    slot = simple_session_media_plan_find_slot(plan, session_id, media_kind);
    if (slot >= 0)
    {
        return slot;
    }
    if (slot != -ENOENT)
    {
        return slot;
    }

    slot = simple_session_media_plan_find_free_slot(plan);
    if (slot < 0)
    {
        return slot;
    }

    memset(&plan->slots[slot], 0, sizeof(plan->slots[slot]));
    plan->slots[slot].in_use = true;
    plan->slots[slot].record.session_id = session_id;
    plan->slots[slot].record.media_kind = media_kind;
    return slot;
}

static void simple_session_media_plan_fill_from_profile(
    simple_session_media_plan_record_t *record,
    uint8_t msg_type,
    const call_media_profile_t *profile,
    bool tx_profile)
{
    bool is_video;

    if (!record || !profile)
    {
        return;
    }

    is_video = record->media_kind == SIMPLE_SESSION_MEDIA_KIND_VIDEO;
    record->last_msg_type = msg_type;
    record->direction = is_video ? profile->video_dir : profile->audio_dir;
    record->codec = is_video ? profile->video_codec : profile->audio_codec;
    record->payload_type = is_video ? profile->video_pt : profile->audio_pt;
    if (tx_profile)
    {
        record->tx_profile_seen = true;
        record->local_profile_seen = true;
        record->local_profile_msg_type = msg_type;
        record->local_profile = *profile;
        record->has_declared_endpoint = is_video ?
                                        profile->has_video_endpoint :
                                        profile->has_audio_endpoint;
        if (record->has_declared_endpoint)
        {
            record->declared_endpoint = is_video ?
                                        profile->video_endpoint :
                                        profile->audio_endpoint;
        }
    }
    else
    {
        record->rx_profile_seen = true;
        record->peer_profile_seen = true;
        record->peer_profile_msg_type = msg_type;
        record->peer_profile = *profile;
        record->has_peer_endpoint = is_video ?
                                    profile->has_video_endpoint :
                                    profile->has_audio_endpoint;
        if (record->has_peer_endpoint)
        {
            record->peer_endpoint = is_video ?
                                    profile->video_endpoint :
                                    profile->audio_endpoint;
        }
    }
}

void simple_session_media_plan_init(simple_session_media_plan_t *plan)
{
    simple_session_media_plan_clear(plan);
}

void simple_session_media_plan_clear(simple_session_media_plan_t *plan)
{
    if (!plan)
    {
        return;
    }

    memset(plan, 0, sizeof(*plan));
}

int simple_session_media_plan_note_profile(simple_session_media_plan_t *plan,
                                           uint64_t session_id,
                                           uint8_t msg_type,
                                           const call_media_profile_t *profile,
                                           bool tx_profile)
{
    int ret = 0;

    if (!plan || !profile || session_id == 0)
    {
        return -EINVAL;
    }

    if ((profile->media_mask & CALL_MEDIA_MASK_VIDEO) != 0)
    {
        int slot = simple_session_media_plan_ensure(plan, session_id,
                                                    SIMPLE_SESSION_MEDIA_KIND_VIDEO);
        if (slot < 0)
        {
            return slot;
        }
        simple_session_media_plan_fill_from_profile(&plan->slots[slot].record,
                                                    msg_type, profile, tx_profile);
    }
    if ((profile->media_mask & CALL_MEDIA_MASK_AUDIO) != 0)
    {
        int slot = simple_session_media_plan_ensure(plan, session_id,
                                                    SIMPLE_SESSION_MEDIA_KIND_AUDIO);
        if (slot < 0)
        {
            ret = slot;
        }
        else
        {
            simple_session_media_plan_fill_from_profile(&plan->slots[slot].record,
                                                        msg_type, profile, tx_profile);
        }
    }

    return ret;
}

int simple_session_media_plan_note_local_profile(simple_session_media_plan_t *plan,
                                                 uint64_t session_id,
                                                 uint8_t msg_type,
                                                 const call_media_profile_t *profile)
{
    return simple_session_media_plan_note_profile(plan, session_id, msg_type,
                                                  profile, true);
}

int simple_session_media_plan_note_peer_profile(simple_session_media_plan_t *plan,
                                                uint64_t session_id,
                                                uint8_t msg_type,
                                                const call_media_profile_t *profile)
{
    return simple_session_media_plan_note_profile(plan, session_id, msg_type,
                                                  profile, false);
}

int simple_session_media_plan_load_profile(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    simple_session_media_profile_direction_t direction,
    call_media_profile_t *profile_out)
{
    size_t i;

    if (!plan || !profile_out || session_id == 0 ||
        direction == SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        const simple_session_media_plan_record_t *record = &plan->slots[i].record;

        if (!plan->slots[i].in_use || record->session_id != session_id)
        {
            continue;
        }
        if (direction == SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_LOCAL &&
            record->local_profile_seen &&
            record->local_profile_msg_type == msg_type)
        {
            *profile_out = record->local_profile;
            return 0;
        }
        if (direction == SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER &&
            record->peer_profile_seen &&
            record->peer_profile_msg_type == msg_type)
        {
            *profile_out = record->peer_profile;
            return 0;
        }
    }

    return -ENOENT;
}

static uint8_t simple_session_media_plan_peer_fallback_msg(uint8_t msg_type)
{
    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_ACCEPT:
        return CALL_MSG_CALL_INVITE;
    case CALL_MSG_MONITOR_ACCEPT:
        return CALL_MSG_MONITOR_START;
    default:
        return msg_type;
    }
}

static uint8_t simple_session_media_plan_start_profile_msg(uint8_t trigger_msg_type)
{
    if (trigger_msg_type == CALL_MSG_CALL_RING)
    {
        return CALL_MSG_CALL_INVITE;
    }
    return trigger_msg_type;
}

static bool simple_session_media_plan_msg_has_profile(uint8_t msg_type)
{
    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_INVITE:
    case CALL_MSG_CALL_ACCEPT:
    case CALL_MSG_MONITOR_START:
    case CALL_MSG_MONITOR_ACCEPT:
        return true;
    default:
        return false;
    }
}

static uint8_t simple_session_media_plan_default_mask_for_msg(uint8_t msg_type)
{
    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_INVITE:
    case CALL_MSG_CALL_ACCEPT:
        return CALL_MEDIA_MASK_VIDEO | CALL_MEDIA_MASK_AUDIO;
    case CALL_MSG_MONITOR_START:
    case CALL_MSG_MONITOR_ACCEPT:
        return CALL_MEDIA_MASK_VIDEO;
    default:
        return 0;
    }
}

static void simple_session_media_plan_fill_selection(
    simple_session_media_plan_profile_selection_t *selection,
    uint8_t trigger_msg_type,
    uint8_t profile_msg_type,
    simple_session_media_profile_direction_t direction,
    const char *reason)
{
    if (!selection)
    {
        return;
    }

    memset(selection, 0, sizeof(*selection));
    selection->trigger_msg_type = trigger_msg_type;
    selection->profile_msg_type = profile_msg_type;
    selection->direction = direction;
    safe_strncpy(selection->reason, reason ? reason : "", sizeof(selection->reason));
}

int simple_session_media_plan_load_peer_profile_for_msg(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    call_media_profile_t *profile_out,
    simple_session_media_plan_profile_selection_t *selection_out)
{
    uint8_t fallback_msg;
    int ret;

    if (!plan || !profile_out || session_id == 0)
    {
        return -EINVAL;
    }

    ret = simple_session_media_plan_load_profile(
        plan, session_id, msg_type,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, profile_out);
    if (ret == 0)
    {
        simple_session_media_plan_fill_selection(
            selection_out, msg_type, msg_type,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, "peer_exact");
        return 0;
    }
    if (ret != -ENOENT)
    {
        return ret;
    }

    fallback_msg = simple_session_media_plan_peer_fallback_msg(msg_type);
    if (fallback_msg == msg_type)
    {
        simple_session_media_plan_fill_selection(
            selection_out, msg_type, msg_type,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE, "peer_missing");
        return -ENOENT;
    }

    ret = simple_session_media_plan_load_profile(
        plan, session_id, fallback_msg,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, profile_out);
    if (ret == 0)
    {
        simple_session_media_plan_fill_selection(
            selection_out, msg_type, fallback_msg,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, "peer_fallback");
        return 0;
    }

    simple_session_media_plan_fill_selection(
        selection_out, msg_type, fallback_msg,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE, "peer_missing");
    return ret;
}

int simple_session_media_plan_select_profile_for_media_start(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t trigger_msg_type,
    call_media_profile_t *profile_out,
    simple_session_media_plan_profile_selection_t *selection_out)
{
    uint8_t profile_msg_type;
    int ret;

    if (!plan || !profile_out || session_id == 0)
    {
        return -EINVAL;
    }

    profile_msg_type = simple_session_media_plan_start_profile_msg(trigger_msg_type);
    if (!simple_session_media_plan_msg_has_profile(profile_msg_type))
    {
        simple_session_media_plan_fill_selection(
            selection_out, trigger_msg_type, profile_msg_type,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE, "invalid_msg");
        return -EINVAL;
    }

    ret = simple_session_media_plan_load_profile(
        plan, session_id, profile_msg_type,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_LOCAL, profile_out);
    if (ret == 0)
    {
        simple_session_media_plan_fill_selection(
            selection_out, trigger_msg_type, profile_msg_type,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_LOCAL, "local");
        return 0;
    }
    if (ret != -ENOENT)
    {
        return ret;
    }

    ret = simple_session_media_plan_load_profile(
        plan, session_id, profile_msg_type,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, profile_out);
    if (ret == 0)
    {
        simple_session_media_plan_fill_selection(
            selection_out, trigger_msg_type, profile_msg_type,
            SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER, "peer");
        return 0;
    }

    simple_session_media_plan_fill_selection(
        selection_out, trigger_msg_type, profile_msg_type,
        SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE, "missing_profile");
    return ret;
}

static int simple_session_media_plan_note_profile_selection_one(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    const simple_session_media_plan_profile_selection_t *selection,
    int ret,
    const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.last_msg_type = selection->profile_msg_type;
    plan->slots[slot].record.profile_selection_seen = true;
    plan->slots[slot].record.profile_selection_ret = ret;
    plan->slots[slot].record.profile_selection_trigger_msg_type =
        selection->trigger_msg_type;
    plan->slots[slot].record.profile_selection_profile_msg_type =
        selection->profile_msg_type;
    plan->slots[slot].record.profile_selection_direction = selection->direction;
    safe_strncpy(plan->slots[slot].record.profile_selection_reason,
                 reason ? reason : selection->reason,
                 sizeof(plan->slots[slot].record.profile_selection_reason));
    return 0;
}

int simple_session_media_plan_note_profile_selection(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    const simple_session_media_plan_profile_selection_t *selection,
    const call_media_profile_t *profile,
    int ret,
    const char *reason)
{
    uint8_t mask;
    int note_ret = 0;

    if (!plan || !selection || session_id == 0)
    {
        return -EINVAL;
    }

    mask = profile ? profile->media_mask :
           simple_session_media_plan_default_mask_for_msg(selection->profile_msg_type);
    if ((mask & CALL_MEDIA_MASK_VIDEO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_selection_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_VIDEO, selection,
            ret, reason);
        if (note_ret < 0)
        {
            return note_ret;
        }
    }
    if ((mask & CALL_MEDIA_MASK_AUDIO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_selection_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_AUDIO, selection,
            ret, reason);
    }

    return note_ret;
}

static bool simple_session_media_plan_media_active(const call_media_profile_t *profile,
                                                   uint8_t media_mask)
{
    return profile && ((profile->media_mask & media_mask) != 0);
}

static bool simple_session_media_plan_declared_endpoint_valid(
    const call_media_endpoint_t *endpoint)
{
    if (!endpoint)
    {
        return false;
    }
    if (endpoint->link_kind != (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP &&
        endpoint->link_kind != (uint8_t)CALL_MEDIA_ENDPOINT_LINK_PLC)
    {
        return false;
    }
    if (endpoint->cast_mode != (uint8_t)CALL_MEDIA_CAST_UNICAST &&
        endpoint->cast_mode != (uint8_t)CALL_MEDIA_CAST_MULTICAST)
    {
        return false;
    }
    return endpoint->source == (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_TOPOLOGY ||
           endpoint->source == (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_BUSINESS ||
           endpoint->source == (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_STATIC ||
           endpoint->source == (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_SESSION;
}

static void simple_session_media_plan_apply_declared_bindings(
    call_media_profile_t *profile,
    const simple_media_endpoint_result_t *endpoints)
{
    if (!profile || !endpoints)
    {
        return;
    }

    profile->has_video_endpoint = false;
    memset(&profile->video_endpoint, 0, sizeof(profile->video_endpoint));
    profile->has_audio_endpoint = false;
    memset(&profile->audio_endpoint, 0, sizeof(profile->audio_endpoint));

    if (simple_session_media_plan_media_active(profile, CALL_MEDIA_MASK_VIDEO) &&
        endpoints->video.has_declared_endpoint)
    {
        profile->has_video_endpoint = true;
        profile->video_endpoint = endpoints->video.declared_endpoint;
    }
    if (simple_session_media_plan_media_active(profile, CALL_MEDIA_MASK_AUDIO) &&
        endpoints->audio.has_declared_endpoint)
    {
        profile->has_audio_endpoint = true;
        profile->audio_endpoint = endpoints->audio.declared_endpoint;
    }
}

static void simple_session_media_plan_fill_endpoint_request(
    const simple_session_media_plan_profile_producer_request_t *request,
    const call_media_profile_t *profile,
    simple_media_endpoint_request_t *endpoint_request)
{
    memset(endpoint_request, 0, sizeof(*endpoint_request));
    endpoint_request->stage = SIMPLE_MEDIA_ENDPOINT_STAGE_TX_PROFILE;
    endpoint_request->msg_type = request->msg_type;
    endpoint_request->profile = profile;
    endpoint_request->session_id = request->session_id;
    endpoint_request->peer_id = request->peer_id;
    if (request->self_device_id)
    {
        snprintf(endpoint_request->self_device_id,
                 sizeof(endpoint_request->self_device_id),
                 "%s", request->self_device_id);
    }
    if (request->caller_device_id)
    {
        snprintf(endpoint_request->caller_device_id,
                 sizeof(endpoint_request->caller_device_id),
                 "%s", request->caller_device_id);
    }
    if (request->callee_device_id)
    {
        snprintf(endpoint_request->callee_device_id,
                 sizeof(endpoint_request->callee_device_id),
                 "%s", request->callee_device_id);
    }
    if (request->has_offer_profile && request->offer_profile)
    {
        endpoint_request->peer_profile_storage = *request->offer_profile;
        endpoint_request->peer_profile = &endpoint_request->peer_profile_storage;
    }
}

static void simple_session_media_plan_producer_set_result(
    simple_session_media_plan_profile_producer_result_t *result,
    uint8_t msg_type,
    int ret,
    const char *reason)
{
    if (!result)
    {
        return;
    }

    result->ret = ret;
    result->msg_type = msg_type;
    safe_strncpy(result->reason, reason ? reason : "", sizeof(result->reason));
}

static int simple_session_media_plan_validate_producer_endpoints(
    const call_media_profile_t *profile,
    const simple_media_endpoint_result_t *endpoints,
    const char **reason_out)
{
    if (simple_session_media_plan_media_active(profile, CALL_MEDIA_MASK_VIDEO))
    {
        if (!endpoints->video.has_declared_endpoint)
        {
            *reason_out = "missing_video_declared";
            return -ENOENT;
        }
        if (!simple_session_media_plan_declared_endpoint_valid(
                &endpoints->video.declared_endpoint))
        {
            *reason_out = "invalid_video_declared";
            return -EINVAL;
        }
    }
    if (simple_session_media_plan_media_active(profile, CALL_MEDIA_MASK_AUDIO))
    {
        if (!endpoints->audio.has_declared_endpoint)
        {
            *reason_out = "missing_audio_declared";
            return -ENOENT;
        }
        if (!simple_session_media_plan_declared_endpoint_valid(
                &endpoints->audio.declared_endpoint))
        {
            *reason_out = "invalid_audio_declared";
            return -EINVAL;
        }
    }

    return 0;
}

int simple_session_media_plan_prepare_tx_profile(
    const simple_session_media_plan_profile_producer_request_t *request,
    call_media_profile_t *profile,
    simple_session_media_plan_profile_producer_result_t *result)
{
    simple_media_profile_shape_request_t shape_request;
    simple_media_endpoint_request_t endpoint_request;
    simple_media_endpoint_result_t endpoints;
    call_media_link_decision_t link_decision;
    const char *reason = "produced";
    int ret;

    if (result)
    {
        memset(result, 0, sizeof(*result));
        result->link_decision_ret = -ENOENT;
    }
    if (!request || !profile || !request->endpoint_resolver)
    {
        simple_session_media_plan_producer_set_result(
            result, request ? request->msg_type : 0, -EINVAL, "invalid");
        return -EINVAL;
    }

    memset(&shape_request, 0, sizeof(shape_request));
    shape_request.msg_type = request->msg_type;
    shape_request.session_id = request->session_id;
    shape_request.caller_device_id = request->caller_device_id;
    shape_request.callee_device_id = request->callee_device_id;
    shape_request.peer_id = request->peer_id;
    shape_request.tx_policy = request->tx_profile_shape_policy;
    shape_request.tx_policy_ctx = request->tx_profile_shape_policy_ctx;
    shape_request.offer_profile = request->offer_profile;
    shape_request.has_offer_profile = request->has_offer_profile;
    ret = simple_media_profile_shape_prepare(&shape_request, profile,
                                             result ? &result->shape : NULL);
    if (ret < 0)
    {
        reason = result && result->shape.reason[0] ? result->shape.reason : "shape";
        simple_session_media_plan_producer_set_result(result, request->msg_type,
                                                      ret, reason);
        return ret;
    }

    memset(&endpoint_request, 0, sizeof(endpoint_request));
    simple_session_media_plan_fill_endpoint_request(request, profile,
                                                    &endpoint_request);
    memset(&endpoints, 0, sizeof(endpoints));
    if (result)
    {
        result->endpoint_resolver_called = true;
    }
    ret = request->endpoint_resolver(request->endpoint_resolver_ctx,
                                     &endpoint_request, &endpoints);
    if (result)
    {
        result->endpoint_resolver_ret = ret;
    }
    if (ret < 0)
    {
        simple_session_media_plan_producer_set_result(result, request->msg_type,
                                                      ret, "endpoint_resolver");
        return ret;
    }

    ret = simple_session_media_plan_validate_producer_endpoints(profile,
                                                               &endpoints,
                                                               &reason);
    if (ret < 0)
    {
        simple_session_media_plan_producer_set_result(result, request->msg_type,
                                                      ret, reason);
        return ret;
    }

    simple_session_media_plan_apply_declared_bindings(profile, &endpoints);
    memset(&link_decision, 0, sizeof(link_decision));
    ret = call_proto_media_profile_decide_link(request->msg_type, profile,
                                               &link_decision);
    if (result)
    {
        result->link_decision = link_decision;
        result->link_decision_ret = ret;
    }
    if (ret < 0)
    {
        simple_session_media_plan_producer_set_result(result, request->msg_type,
                                                      ret, "link_decision");
        return ret;
    }

    simple_session_media_plan_producer_set_result(result, request->msg_type,
                                                  0, reason);
    return 0;
}

static int simple_session_media_plan_note_profile_producer_one(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    const simple_session_media_plan_profile_producer_result_t *result)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.last_msg_type = result->msg_type;
    plan->slots[slot].record.profile_producer_seen = true;
    plan->slots[slot].record.profile_producer_ret = result->ret;
    plan->slots[slot].record.profile_producer_msg_type = result->msg_type;
    plan->slots[slot].record.profile_producer_endpoint_resolver_called =
        result->endpoint_resolver_called;
    plan->slots[slot].record.profile_producer_endpoint_resolver_ret =
        result->endpoint_resolver_ret;
    plan->slots[slot].record.profile_producer_link_decision_ret =
        result->link_decision_ret;
    safe_strncpy(plan->slots[slot].record.profile_producer_reason,
                 result->reason,
                 sizeof(plan->slots[slot].record.profile_producer_reason));
    return 0;
}

int simple_session_media_plan_note_profile_producer(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    const simple_session_media_plan_profile_producer_result_t *result,
    const call_media_profile_t *profile)
{
    uint8_t mask;
    int note_ret = 0;

    if (!plan || !result || session_id == 0)
    {
        return -EINVAL;
    }

    mask = profile ? profile->media_mask :
           (uint8_t)(result->shape.before_mask | result->shape.after_mask);
    if (mask == 0)
    {
        mask = simple_session_media_plan_default_mask_for_msg(result->msg_type);
    }
    if ((mask & CALL_MEDIA_MASK_VIDEO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_producer_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_VIDEO, result);
        if (note_ret < 0)
        {
            return note_ret;
        }
    }
    if ((mask & CALL_MEDIA_MASK_AUDIO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_producer_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_AUDIO, result);
    }

    return note_ret;
}

static int simple_session_media_plan_note_profile_shape_one(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    uint8_t msg_type,
    int ret,
    uint8_t before_mask,
    uint8_t after_mask,
    const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.last_msg_type = msg_type;
    plan->slots[slot].record.profile_shape_seen = true;
    plan->slots[slot].record.profile_shape_ret = ret;
    plan->slots[slot].record.profile_shape_before_mask = before_mask;
    plan->slots[slot].record.profile_shape_after_mask = after_mask;
    safe_strncpy(plan->slots[slot].record.profile_shape_reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.profile_shape_reason));
    return 0;
}

int simple_session_media_plan_note_profile_shape(simple_session_media_plan_t *plan,
                                                 uint64_t session_id,
                                                 uint8_t msg_type,
                                                 int ret,
                                                 uint8_t before_mask,
                                                 uint8_t after_mask,
                                                 const char *reason)
{
    uint8_t mask = (uint8_t)(before_mask | after_mask);
    int note_ret = 0;

    if (!plan || session_id == 0)
    {
        return -EINVAL;
    }

    if ((mask & CALL_MEDIA_MASK_VIDEO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_shape_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_VIDEO, msg_type,
            ret, before_mask, after_mask, reason);
        if (note_ret < 0)
        {
            return note_ret;
        }
    }
    if ((mask & CALL_MEDIA_MASK_AUDIO) != 0)
    {
        note_ret = simple_session_media_plan_note_profile_shape_one(
            plan, session_id, SIMPLE_SESSION_MEDIA_KIND_AUDIO, msg_type,
            ret, before_mask, after_mask, reason);
    }

    return note_ret;
}

int simple_session_media_plan_note_provider_endpoint(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    uint8_t stage,
    uint8_t msg_type,
    int ret,
    const call_media_endpoint_t *declared_endpoint,
    bool has_declared_endpoint,
    const call_media_endpoint_t *tx_remote_endpoint,
    bool has_tx_remote_endpoint,
    uint32_t rx_match_addr,
    uint16_t rx_match_port,
    bool has_rx_match_endpoint,
    const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.last_msg_type = msg_type;
    plan->slots[slot].record.provider_seen = true;
    plan->slots[slot].record.provider_stage = stage;
    plan->slots[slot].record.provider_ret = ret;
    plan->slots[slot].record.provider_has_declared_endpoint = has_declared_endpoint;
    if (has_declared_endpoint && declared_endpoint)
    {
        plan->slots[slot].record.provider_declared_endpoint = *declared_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.provider_declared_endpoint, 0,
               sizeof(plan->slots[slot].record.provider_declared_endpoint));
    }
    plan->slots[slot].record.provider_has_tx_remote_endpoint = has_tx_remote_endpoint;
    if (has_tx_remote_endpoint && tx_remote_endpoint)
    {
        plan->slots[slot].record.provider_tx_remote_endpoint = *tx_remote_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.provider_tx_remote_endpoint, 0,
               sizeof(plan->slots[slot].record.provider_tx_remote_endpoint));
    }
    plan->slots[slot].record.provider_has_rx_match_endpoint = has_rx_match_endpoint;
    plan->slots[slot].record.provider_rx_match_addr = has_rx_match_endpoint ?
                                                      rx_match_addr : 0;
    plan->slots[slot].record.provider_rx_match_port = has_rx_match_endpoint ?
                                                      rx_match_port : 0;
    safe_strncpy(plan->slots[slot].record.provider_reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.provider_reason));
    return 0;
}

int simple_session_media_plan_note_link_decision(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    const call_media_link_decision_t *decision,
    int ret)
{
    int slot;

    if (!plan || !decision || session_id == 0)
    {
        return -EINVAL;
    }

    if (decision->video_enabled || decision->video_endpoint.link_kind != 0)
    {
        slot = simple_session_media_plan_ensure(plan, session_id,
                                                SIMPLE_SESSION_MEDIA_KIND_VIDEO);
        if (slot < 0)
        {
            return slot;
        }
        plan->slots[slot].record.last_msg_type = msg_type;
        plan->slots[slot].record.has_link_decision = true;
        plan->slots[slot].record.link_decision_ret = ret;
        plan->slots[slot].record.link_decision_enabled = decision->video_enabled;
        plan->slots[slot].record.direction = decision->video_dir;
        plan->slots[slot].record.codec = decision->video_codec;
        plan->slots[slot].record.payload_type = decision->video_pt;
        plan->slots[slot].record.link_decision_endpoint = decision->video_endpoint;
    }

    if (decision->audio_enabled || decision->audio_endpoint.link_kind != 0)
    {
        slot = simple_session_media_plan_ensure(plan, session_id,
                                                SIMPLE_SESSION_MEDIA_KIND_AUDIO);
        if (slot < 0)
        {
            return slot;
        }
        plan->slots[slot].record.last_msg_type = msg_type;
        plan->slots[slot].record.has_link_decision = true;
        plan->slots[slot].record.link_decision_ret = ret;
        plan->slots[slot].record.link_decision_enabled = decision->audio_enabled;
        plan->slots[slot].record.direction = decision->audio_dir;
        plan->slots[slot].record.codec = decision->audio_codec;
        plan->slots[slot].record.payload_type = decision->audio_pt;
        plan->slots[slot].record.link_decision_endpoint = decision->audio_endpoint;
    }

    return 0;
}

int simple_session_media_plan_note_apply(simple_session_media_plan_t *plan,
                                         uint64_t session_id,
                                         simple_session_media_kind_t media_kind,
                                         int ret,
                                         const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.apply_seen = true;
    plan->slots[slot].record.apply_ret = ret;
    safe_strncpy(plan->slots[slot].record.reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.reason));
    return 0;
}

int simple_session_media_plan_note_apply_endpoint(simple_session_media_plan_t *plan,
                                                  uint64_t session_id,
                                                  simple_session_media_kind_t media_kind,
                                                  int ret,
                                                  const call_media_endpoint_t *tx_remote_endpoint,
                                                  bool has_tx_remote_endpoint,
                                                  uint32_t rx_match_addr,
                                                  uint16_t rx_match_port,
                                                  bool has_rx_match_endpoint,
                                                  const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.apply_seen = true;
    plan->slots[slot].record.apply_ret = ret;
    plan->slots[slot].record.has_apply_tx_remote_endpoint = has_tx_remote_endpoint;
    if (has_tx_remote_endpoint && tx_remote_endpoint)
    {
        plan->slots[slot].record.apply_tx_remote_endpoint = *tx_remote_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.apply_tx_remote_endpoint, 0,
               sizeof(plan->slots[slot].record.apply_tx_remote_endpoint));
    }
    plan->slots[slot].record.has_apply_rx_match_endpoint = has_rx_match_endpoint;
    plan->slots[slot].record.apply_rx_match_addr = has_rx_match_endpoint ?
                                                   rx_match_addr : 0;
    plan->slots[slot].record.apply_rx_match_port = has_rx_match_endpoint ?
                                                   rx_match_port : 0;
    safe_strncpy(plan->slots[slot].record.reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.reason));
    return 0;
}

int simple_session_media_plan_note_bridge(simple_session_media_plan_t *plan,
                                          uint64_t session_id,
                                          simple_session_media_kind_t media_kind,
                                          int ret,
                                          const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.bridge_seen = true;
    plan->slots[slot].record.bridge_ret = ret;
    safe_strncpy(plan->slots[slot].record.reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.reason));
    return 0;
}

int simple_session_media_plan_note_bridge_plan(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    const simple_media_bridge_plan_t *bridge_plan)
{
    int slot;

    if (!bridge_plan)
    {
        return -EINVAL;
    }

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.bridge_plan_seen = true;
    plan->slots[slot].record.has_bridge_plan_ingress_endpoint =
        bridge_plan->has_ingress_endpoint;
    if (bridge_plan->has_ingress_endpoint)
    {
        plan->slots[slot].record.bridge_plan_ingress_endpoint =
            bridge_plan->ingress_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.bridge_plan_ingress_endpoint, 0,
               sizeof(plan->slots[slot].record.bridge_plan_ingress_endpoint));
    }
    plan->slots[slot].record.has_bridge_plan_egress_endpoint =
        bridge_plan->has_egress_endpoint;
    if (bridge_plan->has_egress_endpoint)
    {
        plan->slots[slot].record.bridge_plan_egress_endpoint =
            bridge_plan->egress_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.bridge_plan_egress_endpoint, 0,
               sizeof(plan->slots[slot].record.bridge_plan_egress_endpoint));
    }
    plan->slots[slot].record.has_bridge_plan_rx_match_endpoint =
        bridge_plan->has_rx_match_endpoint;
    plan->slots[slot].record.bridge_plan_rx_match_addr =
        bridge_plan->has_rx_match_endpoint ? bridge_plan->rx_match_addr : 0;
    plan->slots[slot].record.bridge_plan_rx_match_port =
        bridge_plan->has_rx_match_endpoint ? bridge_plan->rx_match_port : 0;
    safe_strncpy(plan->slots[slot].record.bridge_plan_reason,
                 bridge_plan->reason,
                 sizeof(plan->slots[slot].record.bridge_plan_reason));
    return 0;
}

int simple_session_media_plan_note_bridge_endpoint(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    int ret,
    const call_media_endpoint_t *ingress_endpoint,
    bool has_ingress_endpoint,
    const call_media_endpoint_t *egress_endpoint,
    bool has_egress_endpoint,
    uint32_t rx_match_addr,
    uint16_t rx_match_port,
    bool has_rx_match_endpoint,
    const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.bridge_seen = true;
    plan->slots[slot].record.bridge_ret = ret;
    plan->slots[slot].record.has_bridge_ingress_endpoint = has_ingress_endpoint;
    if (has_ingress_endpoint && ingress_endpoint)
    {
        plan->slots[slot].record.bridge_ingress_endpoint = *ingress_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.bridge_ingress_endpoint, 0,
               sizeof(plan->slots[slot].record.bridge_ingress_endpoint));
    }
    plan->slots[slot].record.has_bridge_egress_endpoint = has_egress_endpoint;
    if (has_egress_endpoint && egress_endpoint)
    {
        plan->slots[slot].record.bridge_egress_endpoint = *egress_endpoint;
    }
    else
    {
        memset(&plan->slots[slot].record.bridge_egress_endpoint, 0,
               sizeof(plan->slots[slot].record.bridge_egress_endpoint));
    }
    plan->slots[slot].record.has_bridge_rx_match_endpoint = has_rx_match_endpoint;
    plan->slots[slot].record.bridge_rx_match_addr = has_rx_match_endpoint ?
                                                   rx_match_addr : 0;
    plan->slots[slot].record.bridge_rx_match_port = has_rx_match_endpoint ?
                                                   rx_match_port : 0;
    safe_strncpy(plan->slots[slot].record.reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.reason));
    return 0;
}

int simple_session_media_plan_note_resource(simple_session_media_plan_t *plan,
                                            uint64_t session_id,
                                            simple_session_media_kind_t media_kind,
                                            bool admitted,
                                            unsigned int used,
                                            unsigned int limit,
                                            const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.resource_checked = true;
    plan->slots[slot].record.resource_admitted = admitted;
    plan->slots[slot].record.resource_used = used;
    plan->slots[slot].record.resource_limit = limit;
    safe_strncpy(plan->slots[slot].record.resource_reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.resource_reason));
    return 0;
}

int simple_session_media_plan_note_runtime_active(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    uint8_t msg_type,
    bool active,
    int ret,
    const char *reason)
{
    int slot;

    slot = simple_session_media_plan_ensure(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    plan->slots[slot].record.runtime_active = active;
    plan->slots[slot].record.runtime_msg_type = msg_type;
    plan->slots[slot].record.runtime_ret = ret;
    safe_strncpy(plan->slots[slot].record.runtime_reason,
                 reason ? reason : "",
                 sizeof(plan->slots[slot].record.runtime_reason));
    return 0;
}

bool simple_session_media_plan_runtime_active(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind)
{
    int slot;

    slot = simple_session_media_plan_find_slot(plan, session_id, media_kind);
    if (slot < 0)
    {
        return false;
    }
    return plan->slots[slot].record.runtime_active;
}

unsigned int simple_session_media_plan_count_runtime_active_excluding(
    const simple_session_media_plan_t *plan,
    simple_session_media_kind_t media_kind,
    uint64_t session_id)
{
    size_t i;
    unsigned int count = 0;

    if (!plan || media_kind == SIMPLE_SESSION_MEDIA_KIND_NONE)
    {
        return 0;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        const simple_session_media_plan_record_t *record = &plan->slots[i].record;

        if (!plan->slots[i].in_use ||
            record->media_kind != media_kind ||
            record->session_id == session_id ||
            !record->runtime_active)
        {
            continue;
        }
        count++;
    }

    return count;
}

unsigned int simple_session_media_plan_count_runtime_active(
    const simple_session_media_plan_t *plan,
    simple_session_media_kind_t media_kind)
{
    return simple_session_media_plan_count_runtime_active_excluding(
        plan, media_kind, 0);
}

uint8_t simple_session_media_plan_media_state(
    const simple_session_media_plan_t *plan,
    uint64_t session_id)
{
    if (!plan)
    {
        return (uint8_t)CALL_MEDIA_STATE_STOPPED;
    }
    if (session_id != 0)
    {
        if (simple_session_media_plan_runtime_active(
                plan, session_id, SIMPLE_SESSION_MEDIA_KIND_VIDEO) ||
            simple_session_media_plan_runtime_active(
                plan, session_id, SIMPLE_SESSION_MEDIA_KIND_AUDIO))
        {
            return (uint8_t)CALL_MEDIA_STATE_ACTIVE;
        }
        return (uint8_t)CALL_MEDIA_STATE_STOPPED;
    }
    if (simple_session_media_plan_count_runtime_active(
            plan, SIMPLE_SESSION_MEDIA_KIND_VIDEO) > 0 ||
        simple_session_media_plan_count_runtime_active(
            plan, SIMPLE_SESSION_MEDIA_KIND_AUDIO) > 0)
    {
        return (uint8_t)CALL_MEDIA_STATE_ACTIVE;
    }
    return (uint8_t)CALL_MEDIA_STATE_STOPPED;
}

void simple_session_media_plan_release(simple_session_media_plan_t *plan,
                                       uint64_t session_id,
                                       const char *reason)
{
    size_t i;

    if (!plan || session_id == 0)
    {
        return;
    }

    (void)reason;
    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        if (plan->slots[i].in_use &&
            plan->slots[i].record.session_id == session_id)
        {
            memset(&plan->slots[i], 0, sizeof(plan->slots[i]));
        }
    }
}

int simple_session_media_plan_get(const simple_session_media_plan_t *plan,
                                  uint64_t session_id,
                                  simple_session_media_kind_t media_kind,
                                  simple_session_media_plan_record_t *record_out)
{
    int slot;

    if (!record_out)
    {
        return -EINVAL;
    }

    slot = simple_session_media_plan_find_slot(plan, session_id, media_kind);
    if (slot < 0)
    {
        return slot;
    }

    *record_out = plan->slots[slot].record;
    return 0;
}

size_t simple_session_media_plan_count(const simple_session_media_plan_t *plan)
{
    size_t i;
    size_t count = 0;

    if (!plan)
    {
        return 0;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        if (plan->slots[i].in_use)
        {
            count++;
        }
    }

    return count;
}

static bool simple_session_media_plan_record_uses_plc_video(
    const simple_session_media_plan_record_t *record)
{
    if (!record || record->media_kind != SIMPLE_SESSION_MEDIA_KIND_VIDEO)
    {
        return false;
    }

    return (record->has_link_decision &&
            record->link_decision_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC) ||
           (record->has_declared_endpoint &&
            record->declared_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC) ||
           (record->has_peer_endpoint &&
            record->peer_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC);
}

unsigned int simple_session_media_plan_count_plc_video_excluding(
    const simple_session_media_plan_t *plan,
    uint64_t session_id)
{
    size_t i;
    unsigned int count = 0;

    if (!plan)
    {
        return 0;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        const simple_session_media_plan_record_t *record = &plan->slots[i].record;

        if (!plan->slots[i].in_use ||
            record->session_id == session_id)
        {
            continue;
        }
        if (simple_session_media_plan_record_uses_plc_video(record))
        {
            count++;
        }
    }

    return count;
}

unsigned int simple_session_media_plan_count_plc_video(
    const simple_session_media_plan_t *plan)
{
    return simple_session_media_plan_count_plc_video_excluding(plan, 0);
}

void simple_session_media_plan_dump(const simple_session_media_plan_t *plan,
                                    simple_session_media_plan_dump_cb cb,
                                    void *user_ctx)
{
    size_t i;

    if (!plan || !cb)
    {
        return;
    }

    for (i = 0; i < SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES; i++)
    {
        if (plan->slots[i].in_use)
        {
            cb(user_ctx, &plan->slots[i].record);
        }
    }
}

const char *simple_session_media_kind_name(simple_session_media_kind_t kind)
{
    switch (kind)
    {
    case SIMPLE_SESSION_MEDIA_KIND_VIDEO:
        return "video";
    case SIMPLE_SESSION_MEDIA_KIND_AUDIO:
        return "audio";
    case SIMPLE_SESSION_MEDIA_KIND_NONE:
    default:
        return "none";
    }
}
