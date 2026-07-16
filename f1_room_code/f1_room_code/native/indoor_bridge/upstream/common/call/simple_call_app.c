/**
 * @file
 * @brief simple 示例应用公共模块 simple_call_app 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_call_app.h"
#include "simple_log.h"
#include "simple_mem.h"
#include "simple_media_resource_policy.h"
#include "simple_session_manager.h"
#include "simple_session_media_plan.h"
#include "call_media_link.h"

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utils/util.h>

#ifndef SIMPLE_CALL_LOG_ACK_HEARTBEAT_EVENTS
#define SIMPLE_CALL_LOG_ACK_HEARTBEAT_EVENTS 0
#endif

struct simple_call_app
{
    simple_call_app_config_t cfg;
    simple_control_route_runtime_t route_runtime;
    simple_control_link_t link_cfg[SIMPLE_CALL_MAX_LINKS];
    call_flow_runtime_link_t runtime_links[SIMPLE_CALL_MAX_LINKS];
    transport_handle_t *transports[SIMPLE_CALL_MAX_LINKS];
    call_flow_runtime_ctx_t *runtime;
    simple_session_manager_t session_manager;
    simple_session_media_plan_t media_plan;
    simple_media_resource_policy_t resource_policy;
    uint32_t local_session_boot_nonce;
    uint32_t local_session_counter;
    pthread_mutex_t lock;
    bool lock_ready;
    bool running;
};

static int simple_call_send_with_msg_ids(simple_call_app_t *app, uint8_t msg_type,
                                         const char *route_callee_device_id,
                                         const char *payload_caller_device_id,
                                         const char *payload_callee_device_id,
                                         uint32_t txn_id, uint64_t session_id,
                                         const call_endpoint_t *target);
static int simple_call_send_with_msg_ids_ex(simple_call_app_t *app, uint8_t msg_type,
                                            const char *route_callee_device_id,
                                            const char *payload_caller_device_id,
                                            const char *payload_callee_device_id,
                                            uint32_t txn_id, uint64_t session_id,
                                            const call_endpoint_t *target,
                                            bool has_result_code,
                                            uint8_t result_code);
static bool simple_call_media_active(const call_media_profile_t *profile,
                                     uint8_t media_mask);
static int simple_call_copy_current_monitor_record(simple_call_app_t *app,
                                                   simple_session_record_t *record);

static int simple_call_copy_payload_string(char *dst, size_t dst_len, const char *src)
{
    size_t src_len;

    if (!dst || dst_len == 0 || !src)
    {
        return -EINVAL;
    }

    src_len = safe_strncpy(dst, src, dst_len);
    return src_len < dst_len ? 0 : -ENOSPC;
}

static void simple_call_fill_media_endpoint(call_media_endpoint_t *endpoint,
                                            uint8_t link_kind,
                                            uint8_t cast_mode,
                                            uint8_t source,
                                            uint32_t addr,
                                            uint16_t port)
{
    if (!endpoint)
    {
        return;
    }

    memset(endpoint, 0, sizeof(*endpoint));
    endpoint->link_kind = link_kind;
    endpoint->cast_mode = cast_mode;
    endpoint->source = source;
    endpoint->addr = addr;
    endpoint->port = port;
}

static bool simple_call_device_id_equal(const char *a, const char *b)
{
    return IS_VALID_STRING(a) && IS_VALID_STRING(b) && strcmp(a, b) == 0;
}

static const char *simple_call_role_name(simple_device_role_t role)
{
    switch (role)
    {
    case SIMPLE_DEVICE_MANAGE:
        return "manage";
    case SIMPLE_DEVICE_DOOR_MAIN:
        return "door_main";
    case SIMPLE_DEVICE_CONVERTER:
        return "converter";
    case SIMPLE_DEVICE_INDOOR:
        return "indoor";
    case SIMPLE_DEVICE_SECONDARY_CONFIRM:
        return "secondary_confirm";
    default:
        return "unknown";
    }
}

static const char *simple_call_link_name(call_link_kind_t link_kind)
{
    switch (link_kind)
    {
    case CALL_LINK_UDP:
        return "udp";
    case CALL_LINK_PLC:
        return "plc";
    default:
        return "unknown";
    }
}

static const char *simple_call_media_endpoint_link_name(uint8_t link_kind)
{
    switch (link_kind)
    {
    case CALL_MEDIA_ENDPOINT_LINK_UDP:
        return "udp";
    case CALL_MEDIA_ENDPOINT_LINK_PLC:
        return "plc";
    case CALL_MEDIA_ENDPOINT_LINK_NONE:
    default:
        return "none";
    }
}

static const char *simple_call_media_cast_name(uint8_t cast_mode)
{
    switch (cast_mode)
    {
    case CALL_MEDIA_CAST_UNICAST:
        return "unicast";
    case CALL_MEDIA_CAST_MULTICAST:
        return "multicast";
    case CALL_MEDIA_CAST_NONE:
    default:
        return "none";
    }
}

static const char *simple_call_media_endpoint_source_name(uint8_t source)
{
    switch (source)
    {
    case CALL_MEDIA_ENDPOINT_SOURCE_TOPOLOGY:
        return "topology";
    case CALL_MEDIA_ENDPOINT_SOURCE_BUSINESS:
        return "business";
    case CALL_MEDIA_ENDPOINT_SOURCE_STATIC:
        return "static";
    case CALL_MEDIA_ENDPOINT_SOURCE_SESSION:
        return "session";
    case CALL_MEDIA_ENDPOINT_SOURCE_NONE:
    default:
        return "none";
    }
}

static void simple_call_format_endpoint(const call_media_endpoint_t *endpoint,
                                        char *buf,
                                        size_t cap)
{
    if (!buf || cap == 0)
    {
        return;
    }
    if (!endpoint || endpoint->link_kind == CALL_MEDIA_ENDPOINT_LINK_NONE)
    {
        snprintf(buf, cap, "-");
        return;
    }

    snprintf(buf, cap, "%s/%s/%s/0x%08x:%u",
             simple_call_media_endpoint_link_name(endpoint->link_kind),
             simple_call_media_cast_name(endpoint->cast_mode),
             simple_call_media_endpoint_source_name(endpoint->source),
             endpoint->addr,
             endpoint->port);
}

static const char *simple_call_profile_seen_name(bool tx_seen, bool rx_seen)
{
    if (tx_seen && rx_seen)
    {
        return "tx+rx";
    }
    if (tx_seen)
    {
        return "tx";
    }
    if (rx_seen)
    {
        return "rx";
    }
    return "none";
}

static const char *simple_call_seen_result_name(bool seen, int ret)
{
    if (!seen)
    {
        return "none";
    }
    return ret == 0 ? "pass" : "fail";
}

static const char *simple_call_link_decision_result_name(
    const simple_session_media_plan_record_t *record)
{
    if (!record || !record->has_link_decision)
    {
        return "none";
    }
    if (record->link_decision_ret != 0)
    {
        return "fail";
    }
    return record->link_decision_enabled ? "pass" : "off";
}

static const char *simple_call_resource_result_name(
    const simple_session_media_plan_record_t *record)
{
    if (!record || !record->resource_checked)
    {
        return "none";
    }
    return record->resource_admitted ? "pass" : "deny";
}

static const char *simple_call_endpoint_stage_name(uint8_t stage)
{
    switch ((simple_media_endpoint_stage_t)stage)
    {
    case SIMPLE_MEDIA_ENDPOINT_STAGE_TX_PROFILE:
        return "tx_profile";
    case SIMPLE_MEDIA_ENDPOINT_STAGE_MEDIA_START:
        return "media_start";
    default:
        return "none";
    }
}

static void simple_call_format_rx_match(bool has_rx_match,
                                        uint32_t addr,
                                        uint16_t port,
                                        char *buf,
                                        size_t cap)
{
    if (!buf || cap == 0)
    {
        return;
    }
    if (!has_rx_match)
    {
        snprintf(buf, cap, "-");
        return;
    }
    snprintf(buf, cap, "0x%08x:%u", addr, port);
}

static void simple_call_format_media_plan_summary(
    const simple_call_app_t *app,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    char *buf,
    size_t cap)
{
    simple_session_media_plan_record_t record;
    char declared[96];
    char peer[96];
    char provider_declared[96];
    char provider_tx[96];
    char provider_rx[48];
    char apply_tx[96];
    char apply_rx[48];
    char bridge_plan_in[96];
    char bridge_plan_out[96];
    char bridge_plan_rx[48];
    char bridge_in[96];
    char bridge_out[96];
    char bridge_rx[48];
    int ret;

    if (!buf || cap == 0)
    {
        return;
    }
    buf[0] = '\0';
    if (!app || session_id == 0)
    {
        snprintf(buf, cap, "none");
        return;
    }

    ret = simple_session_media_plan_get(&app->media_plan,
                                        session_id, media_kind, &record);
    if (ret < 0)
    {
        snprintf(buf, cap, "none");
        return;
    }

    simple_call_format_endpoint(record.has_declared_endpoint ?
                                &record.declared_endpoint : NULL,
                                declared, sizeof(declared));
    simple_call_format_endpoint(record.has_peer_endpoint ?
                                &record.peer_endpoint : NULL,
                                peer, sizeof(peer));
    simple_call_format_endpoint(record.provider_has_declared_endpoint ?
                                &record.provider_declared_endpoint : NULL,
                                provider_declared, sizeof(provider_declared));
    simple_call_format_endpoint(record.provider_has_tx_remote_endpoint ?
                                &record.provider_tx_remote_endpoint : NULL,
                                provider_tx, sizeof(provider_tx));
    simple_call_format_rx_match(record.provider_has_rx_match_endpoint,
                                record.provider_rx_match_addr,
                                record.provider_rx_match_port,
                                provider_rx, sizeof(provider_rx));
    simple_call_format_endpoint(record.has_apply_tx_remote_endpoint ?
                                &record.apply_tx_remote_endpoint : NULL,
                                apply_tx, sizeof(apply_tx));
    simple_call_format_rx_match(record.has_apply_rx_match_endpoint,
                                record.apply_rx_match_addr,
                                record.apply_rx_match_port,
                                apply_rx, sizeof(apply_rx));
    simple_call_format_endpoint(record.has_bridge_plan_ingress_endpoint ?
                                &record.bridge_plan_ingress_endpoint : NULL,
                                bridge_plan_in, sizeof(bridge_plan_in));
    simple_call_format_endpoint(record.has_bridge_plan_egress_endpoint ?
                                &record.bridge_plan_egress_endpoint : NULL,
                                bridge_plan_out, sizeof(bridge_plan_out));
    simple_call_format_rx_match(record.has_bridge_plan_rx_match_endpoint,
                                record.bridge_plan_rx_match_addr,
                                record.bridge_plan_rx_match_port,
                                bridge_plan_rx, sizeof(bridge_plan_rx));
    simple_call_format_endpoint(record.has_bridge_ingress_endpoint ?
                                &record.bridge_ingress_endpoint : NULL,
                                bridge_in, sizeof(bridge_in));
    simple_call_format_endpoint(record.has_bridge_egress_endpoint ?
                                &record.bridge_egress_endpoint : NULL,
                                bridge_out, sizeof(bridge_out));
    simple_call_format_rx_match(record.has_bridge_rx_match_endpoint,
                                record.bridge_rx_match_addr,
                                record.bridge_rx_match_port,
                                bridge_rx, sizeof(bridge_rx));

    snprintf(buf, cap,
             "profile=%s shape=%s smask=0x%02x->0x%02x producer=%s dir=%s "
             "codec=%s pt=%u declared=%s peer=%s "
             "provider=%s stage=%s p_declared=%s p_tx=%s p_rx=%s "
             "link_decision=%s apply=%s tx=%s rx=%s bridge_plan=%s bp_in=%s "
             "bp_out=%s bp_rx=%s bridge=%s in=%s out=%s bridge_rx=%s "
             "resource=%s used=%u/%u",
             simple_call_profile_seen_name(record.tx_profile_seen,
                                           record.rx_profile_seen),
             simple_call_seen_result_name(record.profile_shape_seen,
                                          record.profile_shape_ret),
             record.profile_shape_seen ? record.profile_shape_before_mask : 0,
             record.profile_shape_seen ? record.profile_shape_after_mask : 0,
             simple_call_seen_result_name(record.profile_producer_seen,
                                          record.profile_producer_ret),
             call_proto_media_dir_name(record.direction),
             call_proto_media_codec_name(record.codec),
             record.payload_type,
             declared,
             peer,
             simple_call_seen_result_name(record.provider_seen, record.provider_ret),
             simple_call_endpoint_stage_name(record.provider_stage),
             provider_declared,
             provider_tx,
             provider_rx,
             simple_call_link_decision_result_name(&record),
             simple_call_seen_result_name(record.apply_seen, record.apply_ret),
             apply_tx,
             apply_rx,
             record.bridge_plan_seen ? "yes" : "none",
             bridge_plan_in,
             bridge_plan_out,
             bridge_plan_rx,
             simple_call_seen_result_name(record.bridge_seen, record.bridge_ret),
             bridge_in,
             bridge_out,
             bridge_rx,
             simple_call_resource_result_name(&record),
             record.resource_checked ? record.resource_used : 0U,
             record.resource_checked ? record.resource_limit : 0U);
}

static void simple_call_dump_session_record(void *user_ctx,
                                            const simple_session_record_t *record)
{
    const simple_call_app_t *app = (const simple_call_app_t *)user_ctx;
    char video[512];
    char audio[512];

    if (!record)
    {
        return;
    }

    SIMPLE_LOGI("session record: session=0x%" PRIx64
                " kind=%s state=%s txn=%u peer=%u caller=%s callee=%s "
                "source=%s domain=%s gateway=%s\n",
                record->session_id,
                simple_session_record_kind_name(record->kind),
                simple_session_record_state_name(record->state),
                record->txn_id,
                record->peer_id,
                record->caller_device_id[0] ? record->caller_device_id : "-",
                record->callee_device_id[0] ? record->callee_device_id : "-",
                record->logical_source_id[0] ? record->logical_source_id : "-",
                record->domain_id[0] ? record->domain_id : "-",
                record->gateway_id[0] ? record->gateway_id : "-");
    simple_call_format_media_plan_summary(app, record->session_id,
                                          SIMPLE_SESSION_MEDIA_KIND_VIDEO,
                                          video, sizeof(video));
    simple_call_format_media_plan_summary(app, record->session_id,
                                          SIMPLE_SESSION_MEDIA_KIND_AUDIO,
                                          audio, sizeof(audio));
    SIMPLE_LOGI("session media summary: session=0x%" PRIx64
                " video={%s} audio={%s}\n",
                record->session_id,
                video,
                audio);
}

static void simple_call_dump_session_release_record(
    void *user_ctx,
    const simple_session_record_t *record)
{
    (void)user_ctx;

    if (!record)
    {
        return;
    }

    SIMPLE_LOGI("session release record: session=0x%" PRIx64
                " kind=%s state=%s txn=%u peer=%u caller=%s callee=%s "
                "reason=%s\n",
                record->session_id,
                simple_session_record_kind_name(record->kind),
                simple_session_record_state_name(record->state),
                record->txn_id,
                record->peer_id,
                record->caller_device_id[0] ? record->caller_device_id : "-",
                record->callee_device_id[0] ? record->callee_device_id : "-",
                record->release_reason[0] ? record->release_reason : "-");
}

static void simple_call_dump_media_plan_record(
    void *user_ctx,
    const simple_session_media_plan_record_t *record)
{
    char declared[96];
    char peer[96];
    char provider_declared[96];
    char provider_tx[96];
    char provider_rx[48];
    char link_decision[96];
    char apply_tx[96];
    char bridge_plan_in[96];
    char bridge_plan_out[96];
    char bridge_in[96];
    char bridge_out[96];

    (void)user_ctx;

    if (!record)
    {
        return;
    }

    simple_call_format_endpoint(record->has_declared_endpoint ?
                                &record->declared_endpoint : NULL,
                                declared, sizeof(declared));
    simple_call_format_endpoint(record->has_peer_endpoint ?
                                &record->peer_endpoint : NULL,
                                peer, sizeof(peer));
    simple_call_format_endpoint(record->provider_has_declared_endpoint ?
                                &record->provider_declared_endpoint : NULL,
                                provider_declared, sizeof(provider_declared));
    simple_call_format_endpoint(record->provider_has_tx_remote_endpoint ?
                                &record->provider_tx_remote_endpoint : NULL,
                                provider_tx, sizeof(provider_tx));
    simple_call_format_rx_match(record->provider_has_rx_match_endpoint,
                                record->provider_rx_match_addr,
                                record->provider_rx_match_port,
                                provider_rx, sizeof(provider_rx));
    simple_call_format_endpoint(record->has_link_decision ?
                                &record->link_decision_endpoint : NULL,
                                link_decision, sizeof(link_decision));
    simple_call_format_endpoint(record->has_apply_tx_remote_endpoint ?
                                &record->apply_tx_remote_endpoint : NULL,
                                apply_tx, sizeof(apply_tx));
    simple_call_format_endpoint(record->has_bridge_plan_ingress_endpoint ?
                                &record->bridge_plan_ingress_endpoint : NULL,
                                bridge_plan_in, sizeof(bridge_plan_in));
    simple_call_format_endpoint(record->has_bridge_plan_egress_endpoint ?
                                &record->bridge_plan_egress_endpoint : NULL,
                                bridge_plan_out, sizeof(bridge_plan_out));
    simple_call_format_endpoint(record->has_bridge_ingress_endpoint ?
                                &record->bridge_ingress_endpoint : NULL,
                                bridge_in, sizeof(bridge_in));
    simple_call_format_endpoint(record->has_bridge_egress_endpoint ?
                                &record->bridge_egress_endpoint : NULL,
                                bridge_out, sizeof(bridge_out));

    SIMPLE_LOGI("media plan record: session=0x%" PRIx64
                " media=%s profile=%s shape=%s shape_ret=%d "
                "shape_mask=0x%02x->0x%02x shape_reason=%s "
                "producer=%s producer_ret=%d producer_resolver=%u "
                "producer_resolver_ret=%d producer_link_ret=%d producer_reason=%s "
                "msg=%s dir=%s codec=%s pt=%u "
                "declared=%s peer=%s provider=%s provider_ret=%d "
                "provider_stage=%s provider_declared=%s provider_tx=%s "
                "provider_rx=%s provider_reason=%s link_decision=%s link_decision_ret=%d "
                "apply=%s apply_ret=%d apply_tx=%s apply_rx=0x%08x:%u "
                "bridge_plan=%u bridge_plan_in=%s bridge_plan_out=%s "
                "bridge_plan_rx=0x%08x:%u bridge_plan_reason=%s "
                "bridge=%s bridge_ret=%d bridge_in=%s bridge_out=%s "
                "bridge_rx=0x%08x:%u resource=%s admitted=%u used=%u "
                "limit=%u resource_reason=%s runtime=%u runtime_msg=%s "
                "runtime_ret=%d runtime_reason=%s reason=%s\n",
                record->session_id,
                simple_session_media_kind_name(record->media_kind),
                simple_call_profile_seen_name(record->tx_profile_seen,
                                              record->rx_profile_seen),
                record->profile_shape_seen ? "seen" : "none",
                record->profile_shape_seen ? record->profile_shape_ret : 0,
                record->profile_shape_seen ? record->profile_shape_before_mask : 0,
                record->profile_shape_seen ? record->profile_shape_after_mask : 0,
                record->profile_shape_reason[0] ? record->profile_shape_reason : "-",
                record->profile_producer_seen ? "seen" : "none",
                record->profile_producer_seen ? record->profile_producer_ret : 0,
                record->profile_producer_endpoint_resolver_called ? 1U : 0U,
                record->profile_producer_endpoint_resolver_ret,
                record->profile_producer_link_decision_ret,
                record->profile_producer_reason[0] ?
                    record->profile_producer_reason : "-",
                call_proto_msg_type_name(record->last_msg_type),
                call_proto_media_dir_name(record->direction),
                call_proto_media_codec_name(record->codec),
                record->payload_type,
                declared,
                peer,
                record->provider_seen ? "seen" : "none",
                record->provider_seen ? record->provider_ret : 0,
                simple_call_endpoint_stage_name(record->provider_stage),
                provider_declared,
                provider_tx,
                provider_rx,
                record->provider_reason[0] ? record->provider_reason : "-",
                link_decision,
                record->has_link_decision ? record->link_decision_ret : 0,
                record->apply_seen ? "seen" : "none",
                record->apply_seen ? record->apply_ret : 0,
                apply_tx,
                record->has_apply_rx_match_endpoint ?
                    record->apply_rx_match_addr : 0,
                record->has_apply_rx_match_endpoint ?
                    record->apply_rx_match_port : 0,
                record->bridge_plan_seen ? 1U : 0U,
                bridge_plan_in,
                bridge_plan_out,
                record->has_bridge_plan_rx_match_endpoint ?
                    record->bridge_plan_rx_match_addr : 0,
                record->has_bridge_plan_rx_match_endpoint ?
                    record->bridge_plan_rx_match_port : 0,
                record->bridge_plan_reason[0] ? record->bridge_plan_reason : "-",
                record->bridge_seen ? "seen" : "none",
                record->bridge_seen ? record->bridge_ret : 0,
                bridge_in,
                bridge_out,
                record->has_bridge_rx_match_endpoint ?
                    record->bridge_rx_match_addr : 0,
                record->has_bridge_rx_match_endpoint ?
                    record->bridge_rx_match_port : 0,
                record->resource_checked ? "checked" : "none",
                record->resource_checked && record->resource_admitted ? 1U : 0U,
                record->resource_checked ? record->resource_used : 0U,
                record->resource_checked ? record->resource_limit : 0U,
                record->resource_reason[0] ? record->resource_reason : "-",
                record->runtime_active ? 1U : 0U,
                call_proto_msg_type_name(record->runtime_msg_type),
                record->runtime_ret,
                record->runtime_reason[0] ? record->runtime_reason : "-",
                record->reason[0] ? record->reason : "-");
}

static simple_session_record_kind_t simple_call_session_kind_for_msg(uint8_t msg_type)
{
    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_INVITE:
    case CALL_MSG_CALL_RING:
    case CALL_MSG_CALL_ACCEPT:
    case CALL_MSG_CALL_REJECT:
    case CALL_MSG_CALL_BYE:
        return SIMPLE_SESSION_RECORD_KIND_CALL;
    case CALL_MSG_MONITOR_START:
    case CALL_MSG_MONITOR_ACCEPT:
    case CALL_MSG_MONITOR_REJECT:
    case CALL_MSG_MONITOR_STOP:
        return SIMPLE_SESSION_RECORD_KIND_MONITOR;
    default:
        return SIMPLE_SESSION_RECORD_KIND_NONE;
    }
}

static simple_session_record_state_t simple_call_session_state_for_msg(uint8_t msg_type)
{
    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_INVITE:
    case CALL_MSG_MONITOR_START:
        return SIMPLE_SESSION_RECORD_STATE_CREATED;
    case CALL_MSG_CALL_RING:
        return SIMPLE_SESSION_RECORD_STATE_RINGING;
    case CALL_MSG_CALL_ACCEPT:
    case CALL_MSG_MONITOR_ACCEPT:
        return SIMPLE_SESSION_RECORD_STATE_ACCEPTED;
    default:
        return SIMPLE_SESSION_RECORD_STATE_ACTIVE;
    }
}

static uint64_t simple_call_session_id_for_msg(simple_call_app_t *app,
                                               uint8_t msg_type)
{
    simple_session_record_t record;
    simple_session_record_kind_t kind;
    uint64_t session_id = 0;

    if (!app)
    {
        return 0;
    }

    kind = simple_call_session_kind_for_msg(msg_type);
    pthread_mutex_lock(&app->lock);
    if (kind == SIMPLE_SESSION_RECORD_KIND_MONITOR &&
        simple_session_manager_get_current_monitor(&app->session_manager, &record) == 0)
    {
        session_id = record.session_id;
    }
    else if (kind == SIMPLE_SESSION_RECORD_KIND_CALL &&
             simple_session_manager_get_current_call(&app->session_manager, &record) == 0)
    {
        session_id = record.session_id;
    }
    pthread_mutex_unlock(&app->lock);

    return session_id;
}

static void simple_call_session_decision_log(uint64_t session_id,
                                             simple_session_record_kind_t kind,
                                             const char *caller_device_id,
                                             const char *callee_device_id,
                                             const simple_session_manager_admission_decision_t *decision)
{
    if (!decision)
    {
        return;
    }

    if (decision->admitted)
    {
        SIMPLE_LOGD("session admission: session=0x%" PRIx64
                    " kind=%s result=pass reason=%s caller=%s callee=%s\n",
                    session_id,
                    simple_session_record_kind_name(kind),
                    decision->reason[0] ? decision->reason : "-",
                    IS_VALID_STRING(caller_device_id) ? caller_device_id : "-",
                    IS_VALID_STRING(callee_device_id) ? callee_device_id : "-");
    }
    else
    {
        SIMPLE_LOGI("session admission: session=0x%" PRIx64
                    " kind=%s result=deny ret=%d reason=%s conflict=0x%" PRIx64
                    " caller=%s callee=%s\n",
                    session_id,
                    simple_session_record_kind_name(kind),
                    decision->ret,
                    decision->reason[0] ? decision->reason : "-",
                    decision->conflict_session_id,
                    IS_VALID_STRING(caller_device_id) ? caller_device_id : "-",
                    IS_VALID_STRING(callee_device_id) ? callee_device_id : "-");
    }
}

static int simple_call_session_reserve_locked(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    uint32_t txn_id,
    uint32_t peer_id,
    const char *caller_device_id,
    const char *callee_device_id,
    simple_session_record_state_t state,
    simple_session_manager_admission_decision_t *decision,
    bool *created_out)
{
    simple_session_record_kind_t kind;
    const char *logical_source_id;
    int ret;

    if (decision)
    {
        memset(decision, 0, sizeof(*decision));
        decision->ret = -EINVAL;
        decision->result_code = (uint8_t)CALL_RESULT_BAD_REQ;
        snprintf(decision->reason, sizeof(decision->reason), "%s", "invalid");
    }
    if (!app || session_id == 0)
    {
        return -EINVAL;
    }

    kind = simple_call_session_kind_for_msg(msg_type);
    if (kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return -EINVAL;
    }
    logical_source_id = IS_VALID_STRING(callee_device_id) ?
                        callee_device_id : caller_device_id;
    ret = simple_session_manager_reserve(&app->session_manager, kind, session_id,
                                         txn_id, peer_id,
                                         caller_device_id, callee_device_id,
                                         logical_source_id, NULL, NULL,
                                         state, decision, created_out);
    return ret;
}

static void simple_call_session_upsert(simple_call_app_t *app,
                                       uint8_t msg_type,
                                       uint64_t session_id,
                                       uint32_t txn_id,
                                       uint32_t peer_id,
                                       const char *caller_device_id,
                                       const char *callee_device_id,
                                       simple_session_record_state_t state)
{
    simple_session_record_kind_t kind;
    const char *merged_caller_device_id = caller_device_id;
    const char *merged_callee_device_id = callee_device_id;
    const char *merged_logical_source_id = NULL;
    char caller_storage[CALL_DEVICE_ID_MAX];
    char callee_storage[CALL_DEVICE_ID_MAX];
    char source_storage[CALL_DEVICE_ID_MAX];
    bool created = false;
    int ret;

    if (!app || session_id == 0)
    {
        return;
    }

    kind = simple_call_session_kind_for_msg(msg_type);
    if (kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    if (peer_id == 0 ||
        !IS_VALID_STRING(merged_caller_device_id) ||
        !IS_VALID_STRING(merged_callee_device_id))
    {
        simple_session_record_t existing;

        if (simple_session_manager_get(&app->session_manager, session_id,
                                       &existing) == 0)
        {
            if (peer_id == 0)
            {
                peer_id = existing.peer_id;
            }
            if (!IS_VALID_STRING(merged_caller_device_id) &&
                IS_VALID_STRING(existing.caller_device_id) &&
                simple_call_copy_payload_string(caller_storage,
                                                sizeof(caller_storage),
                                                existing.caller_device_id) == 0)
            {
                merged_caller_device_id = caller_storage;
            }
            if (!IS_VALID_STRING(merged_callee_device_id) &&
                IS_VALID_STRING(existing.callee_device_id) &&
                simple_call_copy_payload_string(callee_storage,
                                                sizeof(callee_storage),
                                                existing.callee_device_id) == 0)
            {
                merged_callee_device_id = callee_storage;
            }
            if (!IS_VALID_STRING(merged_logical_source_id) &&
                IS_VALID_STRING(existing.logical_source_id) &&
                simple_call_copy_payload_string(source_storage,
                                                sizeof(source_storage),
                                                existing.logical_source_id) == 0)
            {
                merged_logical_source_id = source_storage;
            }
        }
    }
    if (IS_VALID_STRING(merged_callee_device_id))
    {
        merged_logical_source_id = merged_callee_device_id;
    }
    else if (IS_VALID_STRING(merged_caller_device_id))
    {
        merged_logical_source_id = merged_caller_device_id;
    }
    ret = simple_session_manager_upsert(&app->session_manager, kind, session_id,
                                        txn_id, peer_id,
                                        merged_caller_device_id,
                                        merged_callee_device_id,
                                        merged_logical_source_id, NULL, NULL,
                                        state, &created);
    pthread_mutex_unlock(&app->lock);

    if (ret == 0)
    {
        SIMPLE_LOGD("session %s: session=0x%" PRIx64 " kind=%s state=%s "
                    "caller=%s callee=%s source=%s\n",
                    created ? "create" : "update",
                    session_id,
                    simple_session_record_kind_name(kind),
                    simple_session_record_state_name(state),
                    IS_VALID_STRING(merged_caller_device_id) ?
                    merged_caller_device_id : "-",
                    IS_VALID_STRING(merged_callee_device_id) ?
                    merged_callee_device_id : "-",
                    IS_VALID_STRING(merged_logical_source_id) ?
                    merged_logical_source_id : "-");
    }
    else
    {
        SIMPLE_LOGD("session manager update failed: session=0x%" PRIx64
                    " msg=%s ret=%d\n",
                    session_id, call_proto_msg_type_name(msg_type), ret);
    }
}

static void simple_call_stop_video_for_session(simple_call_app_t *app,
                                               uint64_t session_id,
                                               const char *reason);
static void simple_call_stop_audio_for_session(simple_call_app_t *app,
                                               uint64_t session_id,
                                               const char *reason);

static void simple_call_session_release(simple_call_app_t *app,
                                        uint64_t session_id,
                                        const char *reason)
{
    int ret;

    if (!app || session_id == 0)
    {
        return;
    }

    simple_call_stop_audio_for_session(app, session_id, reason);
    simple_call_stop_video_for_session(app, session_id, reason);

    pthread_mutex_lock(&app->lock);
    ret = simple_session_manager_release(&app->session_manager, session_id, reason);
    simple_session_media_plan_release(&app->media_plan, session_id, reason);
    pthread_mutex_unlock(&app->lock);

    if (ret == 0)
    {
        SIMPLE_LOGD("session release: session=0x%" PRIx64 " reason=%s\n",
                    session_id, reason ? reason : "-");
    }
}

static bool simple_call_mark_runtime_media_inactive(simple_call_app_t *app,
                                                    uint64_t session_id,
                                                    simple_session_media_kind_t media_kind,
                                                    uint8_t msg_type,
                                                    const char *reason)
{
    bool was_active;
    bool should_stop;

    if (!app || session_id == 0 || media_kind == SIMPLE_SESSION_MEDIA_KIND_NONE)
    {
        return false;
    }

    pthread_mutex_lock(&app->lock);
    was_active = simple_session_media_plan_runtime_active(&app->media_plan,
                                                          session_id,
                                                          media_kind);
    should_stop = was_active &&
        simple_session_media_plan_count_runtime_active_excluding(
            &app->media_plan, media_kind, session_id) == 0;
    if (was_active)
    {
        (void)simple_session_media_plan_note_runtime_active(
            &app->media_plan, session_id, media_kind, msg_type, false, 0,
            reason);
    }
    pthread_mutex_unlock(&app->lock);

    return should_stop;
}

static void simple_call_stop_runtime_media(simple_call_app_t *app,
                                           uint64_t session_id,
                                           simple_session_media_kind_t media_kind,
                                           const char *reason)
{
    bool should_stop;

    should_stop = simple_call_mark_runtime_media_inactive(
        app, session_id, media_kind, 0, reason);
    if (!should_stop || !app->cfg.media_ops)
    {
        return;
    }
    if (media_kind == SIMPLE_SESSION_MEDIA_KIND_VIDEO &&
        app->cfg.media_ops->stop_video)
    {
        app->cfg.media_ops->stop_video(app->cfg.media_ops->user_ctx);
    }
    else if (media_kind == SIMPLE_SESSION_MEDIA_KIND_AUDIO &&
             app->cfg.media_ops->stop_audio)
    {
        app->cfg.media_ops->stop_audio(app->cfg.media_ops->user_ctx);
    }
}

static void simple_call_stop_video_for_session(simple_call_app_t *app,
                                               uint64_t session_id,
                                               const char *reason)
{
    simple_call_stop_runtime_media(app, session_id,
                                   SIMPLE_SESSION_MEDIA_KIND_VIDEO, reason);
}

static void simple_call_stop_audio_for_session(simple_call_app_t *app,
                                               uint64_t session_id,
                                               const char *reason)
{
    simple_call_stop_runtime_media(app, session_id,
                                   SIMPLE_SESSION_MEDIA_KIND_AUDIO, reason);
}

static void simple_call_note_media_plan_profile(simple_call_app_t *app,
                                                uint64_t session_id,
                                                uint8_t msg_type,
                                                const call_media_profile_t *profile,
                                                bool tx_profile)
{
    int ret;

    if (!app || !profile || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    ret = tx_profile ?
          simple_session_media_plan_note_local_profile(&app->media_plan, session_id,
                                                       msg_type, profile) :
          simple_session_media_plan_note_peer_profile(&app->media_plan, session_id,
                                                      msg_type, profile);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGD("media plan profile: session=0x%" PRIx64 " dir=%s msg=%s "
                "mask=0x%02x result=%s ret=%d\n",
                session_id,
                tx_profile ? "local" : "peer",
                call_proto_msg_type_name(msg_type),
                profile->media_mask,
                ret == 0 ? "pass" : "fail",
                ret);
}

static const char *simple_call_profile_direction_name(
    simple_session_media_profile_direction_t direction)
{
    switch (direction)
    {
    case SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_LOCAL:
        return "local";
    case SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER:
        return "peer";
    case SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE:
    default:
        return "none";
    }
}

static bool simple_call_load_peer_media_profile_for_msg(simple_call_app_t *app,
                                                        uint64_t session_id,
                                                        uint8_t msg_type,
                                                        call_media_profile_t *profile_out)
{
    int ret;

    if (!app || !profile_out || session_id == 0)
    {
        return false;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_load_peer_profile_for_msg(
        &app->media_plan, session_id, msg_type, profile_out, NULL);
    pthread_mutex_unlock(&app->lock);

    return ret == 0;
}

static int simple_call_select_media_plan_profile_for_start(
    simple_call_app_t *app,
    uint64_t session_id,
    uint8_t trigger_msg_type,
    call_media_profile_t *profile_out,
    simple_session_media_plan_profile_selection_t *selection_out)
{
    int ret;

    if (!app || !profile_out || session_id == 0)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_select_profile_for_media_start(
        &app->media_plan, session_id, trigger_msg_type, profile_out, selection_out);
    pthread_mutex_unlock(&app->lock);

    return ret;
}

static void simple_call_note_media_plan_profile_selection(
    simple_call_app_t *app,
    uint64_t session_id,
    const simple_session_media_plan_profile_selection_t *selection,
    const call_media_profile_t *profile,
    int ret,
    const char *reason)
{
    int note_ret;

    if (!app || !selection || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    note_ret = simple_session_media_plan_note_profile_selection(
        &app->media_plan, session_id, selection, profile, ret, reason);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGD("media plan profile selection: session=0x%" PRIx64
                " trigger=%s profile_msg=%s dir=%s result=%s ret=%d "
                "note_ret=%d reason=%s\n",
                session_id,
                call_proto_msg_type_name(selection->trigger_msg_type),
                call_proto_msg_type_name(selection->profile_msg_type),
                simple_call_profile_direction_name(selection->direction),
                ret == 0 ? "pass" : "fail",
                ret,
                note_ret,
                reason ? reason : (selection->reason[0] ? selection->reason : "-"));
}

static void simple_call_note_media_plan_profile_shape(
    simple_call_app_t *app,
    uint64_t session_id,
    uint8_t msg_type,
    const simple_media_profile_shape_result_t *shape)
{
    int ret;

    if (!app || !shape || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_note_profile_shape(
        &app->media_plan,
        session_id,
        msg_type,
        shape->ret,
        shape->before_mask,
        shape->after_mask,
        shape->reason);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGD("media plan profile shape: session=0x%" PRIx64
                " msg=%s before=0x%02x after=0x%02x result=%s ret=%d reason=%s\n",
                session_id,
                call_proto_msg_type_name(msg_type),
                shape->before_mask,
                shape->after_mask,
                shape->ret == 0 ? "pass" : "fail",
                shape->ret,
                shape->reason[0] ? shape->reason : "-");
    if (ret < 0)
    {
        SIMPLE_LOGD("media plan profile shape note failed: session=0x%" PRIx64
                    " msg=%s ret=%d\n",
                    session_id, call_proto_msg_type_name(msg_type), ret);
    }
}

static void simple_call_note_media_plan_profile_producer(
    simple_call_app_t *app,
    uint64_t session_id,
    const simple_session_media_plan_profile_producer_result_t *result,
    const call_media_profile_t *profile)
{
    int ret;

    if (!app || !result || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_note_profile_producer(&app->media_plan,
                                                          session_id,
                                                          result, profile);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGD("media plan profile producer: session=0x%" PRIx64
                " msg=%s result=%s ret=%d resolver=%u resolver_ret=%d "
                "link_ret=%d note_ret=%d reason=%s\n",
                session_id,
                call_proto_msg_type_name(result->msg_type),
                result->ret == 0 ? "pass" : "fail",
                result->ret,
                result->endpoint_resolver_called ? 1U : 0U,
                result->endpoint_resolver_ret,
                result->link_decision_ret,
                ret,
                result->reason[0] ? result->reason : "-");
}

static void simple_call_note_media_plan_provider_bindings(
    simple_call_app_t *app,
    uint64_t session_id,
    simple_media_endpoint_stage_t stage,
    uint8_t msg_type,
    const call_media_profile_t *profile,
    const simple_media_endpoint_result_t *result,
    int ret,
    const char *reason)
{
    int note_ret = 0;

    if (!app || !profile || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    if (simple_call_media_active(profile, CALL_MEDIA_MASK_VIDEO))
    {
        const simple_media_endpoint_binding_t *binding = result ? &result->video : NULL;

        note_ret = simple_session_media_plan_note_provider_endpoint(
            &app->media_plan,
            session_id,
            SIMPLE_SESSION_MEDIA_KIND_VIDEO,
            (uint8_t)stage,
            msg_type,
            ret,
            binding ? &binding->declared_endpoint : NULL,
            binding ? binding->has_declared_endpoint : false,
            binding ? &binding->tx_remote_endpoint : NULL,
            binding ? binding->has_tx_remote_endpoint : false,
            binding ? binding->rx_match_addr : 0,
            binding ? binding->rx_match_port : 0,
            binding ? binding->has_rx_match_endpoint : false,
            reason);
    }
    if (simple_call_media_active(profile, CALL_MEDIA_MASK_AUDIO))
    {
        const simple_media_endpoint_binding_t *binding = result ? &result->audio : NULL;

        note_ret = simple_session_media_plan_note_provider_endpoint(
            &app->media_plan,
            session_id,
            SIMPLE_SESSION_MEDIA_KIND_AUDIO,
            (uint8_t)stage,
            msg_type,
            ret,
            binding ? &binding->declared_endpoint : NULL,
            binding ? binding->has_declared_endpoint : false,
            binding ? &binding->tx_remote_endpoint : NULL,
            binding ? binding->has_tx_remote_endpoint : false,
            binding ? binding->rx_match_addr : 0,
            binding ? binding->rx_match_port : 0,
            binding ? binding->has_rx_match_endpoint : false,
            reason);
    }
    pthread_mutex_unlock(&app->lock);

    if (stage == SIMPLE_MEDIA_ENDPOINT_STAGE_MEDIA_START || ret < 0)
    {
        SIMPLE_LOGI("media plan provider: session=0x%" PRIx64
                    " stage=%s msg=%s result=%s ret=%d note_ret=%d reason=%s\n",
                    session_id,
                    simple_call_endpoint_stage_name((uint8_t)stage),
                    call_proto_msg_type_name(msg_type),
                    ret == 0 ? "pass" : "fail",
                    ret,
                    note_ret,
                    reason ? reason : "-");
    }
    else
    {
        SIMPLE_LOGD("media plan provider: session=0x%" PRIx64
                    " stage=%s msg=%s result=%s ret=%d note_ret=%d reason=%s\n",
                    session_id,
                    simple_call_endpoint_stage_name((uint8_t)stage),
                    call_proto_msg_type_name(msg_type),
                    ret == 0 ? "pass" : "fail",
                    ret,
                    note_ret,
                    reason ? reason : "-");
    }
}

static int simple_call_check_resource_policy(simple_call_app_t *app,
                                             uint64_t session_id,
                                             uint8_t msg_type,
                                             const call_media_link_decision_t *decision,
                                             const char *stage,
                                             simple_media_resource_decision_t *decision_out)
{
    simple_media_resource_request_t request;
    simple_media_resource_decision_t resource_decision;
    unsigned int used;
    int ret;

    if (!app || !decision || !decision->video_enabled ||
        decision->video_endpoint.link_kind != CALL_MEDIA_ENDPOINT_LINK_PLC)
    {
        return 0;
    }

    memset(&request, 0, sizeof(request));
    request.session_id = session_id;
    request.session_kind = simple_call_session_kind_for_msg(msg_type);
    request.media_kind = SIMPLE_SESSION_MEDIA_KIND_VIDEO;
    request.link_kind = decision->video_endpoint.link_kind;

    pthread_mutex_lock(&app->lock);
    used = simple_session_media_plan_count_plc_video_excluding(&app->media_plan,
                                                               session_id);
    ret = simple_media_resource_policy_check(&app->resource_policy,
                                             &request, used, &resource_decision);
    if (ret == 0)
    {
        (void)simple_session_media_plan_note_resource(&app->media_plan,
                                                      session_id,
                                                      SIMPLE_SESSION_MEDIA_KIND_VIDEO,
                                                      resource_decision.admitted,
                                                      resource_decision.used,
                                                      resource_decision.limit,
                                                      resource_decision.reason);
    }
    pthread_mutex_unlock(&app->lock);

    if (decision_out)
    {
        *decision_out = resource_decision;
    }
    if (ret == 0 && !resource_decision.admitted)
    {
        SIMPLE_LOGI("resource deny: session=0x%" PRIx64
                    " stage=%s media=video link=plc used=%u limit=%u "
                    "reason=%s resource_policy=deny\n",
                    session_id, stage ? stage : "-",
                    resource_decision.used, resource_decision.limit,
                    resource_decision.reason);
        return resource_decision.ret < 0 ? resource_decision.ret : -EBUSY;
    }
    return ret;
}

static void simple_call_note_media_plan_link_decision(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    const call_media_link_decision_t *decision,
    int ret)
{
    int note_ret;

    if (!app || !decision || session_id == 0)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    note_ret = simple_session_media_plan_note_link_decision(&app->media_plan,
                                                               session_id,
                                                               msg_type,
                                                               decision,
                                                               ret);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGD("media plan link decision: session=0x%" PRIx64
                " msg=%s video=%u audio=%u result=%s ret=%d note_ret=%d "
                "stage=link_decision\n",
                session_id,
                call_proto_msg_type_name(msg_type),
                decision->video_enabled ? 1U : 0U,
                decision->audio_enabled ? 1U : 0U,
                ret == 0 ? "pass" : "fail",
                ret,
                note_ret);
}

static void simple_call_note_media_plan_apply(simple_call_app_t *app,
                                              uint8_t msg_type,
                                              uint64_t session_id,
                                              bool video,
                                              int ret,
                                              const char *reason)
{
    simple_session_media_kind_t media_kind;
    int note_ret;

    if (!app)
    {
        return;
    }

    if (session_id == 0)
    {
        return;
    }
    media_kind = video ? SIMPLE_SESSION_MEDIA_KIND_VIDEO : SIMPLE_SESSION_MEDIA_KIND_AUDIO;

    pthread_mutex_lock(&app->lock);
    note_ret = simple_session_media_plan_note_apply(&app->media_plan,
                                                    session_id, media_kind,
                                                    ret, reason);
    pthread_mutex_unlock(&app->lock);

    if (ret < 0)
    {
        SIMPLE_LOGI("media plan apply: session=0x%" PRIx64 " media=%s "
                    "result=%s ret=%d note_ret=%d reason=%s\n",
                    session_id,
                    simple_session_media_kind_name(media_kind),
                    ret == 0 ? "pass" : "fail",
                    ret,
                    note_ret,
                    reason ? reason : "-");
    }
    else
    {
        SIMPLE_LOGD("media plan apply: session=0x%" PRIx64 " media=%s "
                    "result=%s ret=%d note_ret=%d reason=%s\n",
                    session_id,
                    simple_session_media_kind_name(media_kind),
                    ret == 0 ? "pass" : "fail",
                    ret,
                    note_ret,
                    reason ? reason : "-");
    }
}

static void simple_call_note_media_plan_apply_binding(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    bool video,
    int ret,
    const char *reason,
    const simple_media_endpoint_binding_t *binding)
{
    simple_session_media_kind_t media_kind;
    int note_ret;

    if (!app)
    {
        return;
    }

    if (session_id == 0)
    {
        return;
    }
    media_kind = video ? SIMPLE_SESSION_MEDIA_KIND_VIDEO : SIMPLE_SESSION_MEDIA_KIND_AUDIO;

    pthread_mutex_lock(&app->lock);
    note_ret = simple_session_media_plan_note_apply_endpoint(
        &app->media_plan,
        session_id,
        media_kind,
        ret,
        binding ? &binding->tx_remote_endpoint : NULL,
        binding ? binding->has_tx_remote_endpoint : false,
        binding ? binding->rx_match_addr : 0,
        binding ? binding->rx_match_port : 0,
        binding ? binding->has_rx_match_endpoint : false,
        reason);
    pthread_mutex_unlock(&app->lock);

    SIMPLE_LOGI("media plan apply endpoint: session=0x%" PRIx64 " media=%s "
                "result=%s ret=%d note_ret=%d reason=%s\n",
                session_id,
                simple_session_media_kind_name(media_kind),
                ret == 0 ? "pass" : "fail",
                ret,
                note_ret,
                reason ? reason : "-");
}

static call_runtime_role_t simple_call_runtime_role(simple_device_role_t role)
{
    switch (role)
    {
    case SIMPLE_DEVICE_CONVERTER:
    case SIMPLE_DEVICE_DOOR_MAIN:
        return CALL_RUNTIME_ROLE_HYBRID;
    case SIMPLE_DEVICE_MANAGE:
    case SIMPLE_DEVICE_INDOOR:
    case SIMPLE_DEVICE_SECONDARY_CONFIRM:
    default:
        return CALL_RUNTIME_ROLE_PARTICIPANT;
    }
}

static int simple_call_probe_udp_exclusive(const char *interface,
                                           const transport_endpoint_t *ep)
{
    int fd;
    struct sockaddr_in addr;

    if (!ep)
    {
        return -EINVAL;
    }
    if (interface && strncmp(interface, "host", 4) == 0)
    {
        return 0;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        return -errno;
    }

    if (interface && interface[0] != '\0')
    {
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
        {
            int saved_errno = errno;

            close(fd);
            return -saved_errno;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ep->port);
    addr.sin_addr.s_addr = htonl(ep->addr == ADDR_ANY ? INADDR_ANY : ep->addr);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        int saved_errno = errno;

        close(fd);
        return -saved_errno;
    }

    close(fd);
    return 0;
}

static uint32_t simple_call_boot_nonce_for_role(simple_device_role_t role)
{
    switch (role)
    {
    case SIMPLE_DEVICE_DOOR_MAIN:
        return 0x58395353U; /* "X9SS" */
    case SIMPLE_DEVICE_CONVERTER:
        return 0x434F4E56U; /* "CONV" */
    case SIMPLE_DEVICE_INDOOR:
        return 0x494E4452U; /* "INDR" */
    case SIMPLE_DEVICE_SECONDARY_CONFIRM:
        return 0x53434F4EU; /* "SCON" */
    case SIMPLE_DEVICE_MANAGE:
    default:
        return 0x4D414E47U; /* "MANG" */
    }
}

static uint32_t simple_call_hash_device_id(const char *device_id)
{
    uint32_t hash = 2166136261U;
    const unsigned char *p = (const unsigned char *)device_id;

    if (!p)
    {
        return 0;
    }

    while (*p)
    {
        hash ^= (uint32_t)(*p++);
        hash *= 16777619U;
    }

    return hash;
}

static bool simple_call_route_lookup_cb(void *user_ctx, const char *callee_device_id,
                                        call_endpoint_t *next_hop)
{
    simple_call_app_t *app = (simple_call_app_t *)user_ctx;
    bool found = false;

    if (!app || !next_hop || !IS_VALID_STRING(callee_device_id))
    {
        return false;
    }

    pthread_mutex_lock(&app->lock);
    found = simple_control_route_runtime_lookup(&app->route_runtime,
                                                callee_device_id,
                                                next_hop);
    pthread_mutex_unlock(&app->lock);

    return found;
}

static bool simple_call_local_id_match_cb(void *user_ctx, const char *callee_device_id)
{
    simple_call_app_t *app = (simple_call_app_t *)user_ctx;

    return app && simple_call_device_id_equal(callee_device_id, app->cfg.self_device_id);
}

static bool simple_call_media_snapshot_cb(void *user_ctx, uint64_t session_id,
                                          uint8_t *media_state_out)
{
    simple_call_app_t *app = (simple_call_app_t *)user_ctx;

    if (!app || !media_state_out)
    {
        return false;
    }

    pthread_mutex_lock(&app->lock);
    *media_state_out = simple_session_media_plan_media_state(&app->media_plan,
                                                             session_id);
    pthread_mutex_unlock(&app->lock);
    return true;
}

static void simple_call_release_endpoint_bindings(simple_call_app_t *app,
                                                  uint64_t session_id,
                                                  const char *reason)
{
    if (!app || !app->cfg.endpoint_release || session_id == 0)
    {
        return;
    }

    app->cfg.endpoint_release(app->cfg.endpoint_release_ctx, session_id, reason);
}

static uint64_t simple_call_next_local_session_id(simple_call_app_t *app)
{
    if (!app || app->local_session_boot_nonce == 0)
    {
        return 0;
    }

    app->local_session_counter++;
    if (app->local_session_counter == 0)
    {
        app->local_session_counter = 0x80000001U;
    }
    return ((uint64_t)app->local_session_boot_nonce << 32) |
           (uint64_t)app->local_session_counter;
}

static bool simple_call_media_active(const call_media_profile_t *profile, uint8_t media_mask)
{
    return profile && ((profile->media_mask & media_mask) != 0);
}

static bool simple_call_declared_endpoint_valid(const call_media_endpoint_t *endpoint)
{
    if (!endpoint)
    {
        return false;
    }
    if (endpoint->addr == ADDR_ANY || endpoint->port == 0)
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

static void simple_call_fill_static_binding(simple_media_endpoint_binding_t *binding,
                                            bool video)
{
    uint16_t port = video ? SIMPLE_UDP_MEDIA_PORT : (uint16_t)(SIMPLE_UDP_MEDIA_PORT + 2U);

    if (!binding)
    {
        return;
    }

    memset(binding, 0, sizeof(*binding));
    binding->has_declared_endpoint = true;
    simple_call_fill_media_endpoint(&binding->declared_endpoint,
                                    (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP,
                                    (uint8_t)CALL_MEDIA_CAST_MULTICAST,
                                    (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_STATIC,
                                    SIMPLE_UDP_MEDIA_GROUP_ADDR,
                                    port);
}

static void simple_call_fill_static_bindings(const call_media_profile_t *profile,
                                             simple_media_endpoint_result_t *result)
{
    if (!profile || !result)
    {
        return;
    }

    if (simple_call_media_active(profile, CALL_MEDIA_MASK_VIDEO) &&
        !result->video.has_declared_endpoint)
    {
        simple_call_fill_static_binding(&result->video, true);
    }
    if (simple_call_media_active(profile, CALL_MEDIA_MASK_AUDIO) &&
        !result->audio.has_declared_endpoint)
    {
        simple_call_fill_static_binding(&result->audio, false);
    }
}

static int simple_call_build_endpoint_request(simple_call_app_t *app,
                                              simple_media_endpoint_stage_t stage,
                                              uint8_t msg_type,
                                              const call_media_profile_t *profile,
                                              const char *caller_device_id,
                                              const char *callee_device_id,
                                              uint64_t session_id,
                                              uint32_t peer_id,
                                              simple_media_endpoint_request_t *request)
{
    if (!app || !request)
    {
        return -EINVAL;
    }

    memset(request, 0, sizeof(*request));
    request->stage = stage;
    request->msg_type = msg_type;
    request->profile = profile;
    request->session_id = session_id;
    request->peer_id = peer_id;
    (void)simple_call_copy_payload_string(request->self_device_id,
                                          sizeof(request->self_device_id),
                                          app->cfg.self_device_id);
    if (IS_VALID_STRING(caller_device_id))
    {
        (void)simple_call_copy_payload_string(request->caller_device_id,
                                             sizeof(request->caller_device_id),
                                             caller_device_id);
    }
    if (IS_VALID_STRING(callee_device_id))
    {
        (void)simple_call_copy_payload_string(request->callee_device_id,
                                             sizeof(request->callee_device_id),
                                             callee_device_id);
    }

    if ((!IS_VALID_STRING(request->caller_device_id) ||
         !IS_VALID_STRING(request->callee_device_id) ||
         request->session_id == 0 || request->peer_id == 0) && app->lock_ready)
    {
        bool is_monitor =
            simple_call_session_kind_for_msg(msg_type) == SIMPLE_SESSION_RECORD_KIND_MONITOR;
        simple_session_record_t record;
        bool has_record = false;

        pthread_mutex_lock(&app->lock);
        if (request->session_id == 0)
        {
            if (is_monitor &&
                simple_session_manager_get_current_monitor(&app->session_manager,
                                                           &record) == 0)
            {
                request->session_id = record.session_id;
                has_record = true;
            }
            else if (!is_monitor &&
                     simple_session_manager_get_current_call(&app->session_manager,
                                                             &record) == 0)
            {
                request->session_id = record.session_id;
                has_record = true;
            }
        }
        if (!has_record &&
            request->session_id != 0 &&
            simple_session_manager_get(&app->session_manager,
                                       request->session_id, &record) == 0)
        {
            has_record = true;
        }
        if (!IS_VALID_STRING(request->caller_device_id))
        {
            if (has_record)
            {
                (void)simple_call_copy_payload_string(request->caller_device_id,
                                                     sizeof(request->caller_device_id),
                                                     record.caller_device_id);
            }
        }
        if (!IS_VALID_STRING(request->callee_device_id))
        {
            if (has_record)
            {
                (void)simple_call_copy_payload_string(request->callee_device_id,
                                                     sizeof(request->callee_device_id),
                                                     record.callee_device_id);
            }
        }
        if (request->peer_id == 0)
        {
            request->peer_id = has_record ? record.peer_id : 0;
        }
        pthread_mutex_unlock(&app->lock);
    }
    if (request->session_id != 0 &&
        simple_call_load_peer_media_profile_for_msg(app, request->session_id,
                                                    msg_type,
                                                    &request->peer_profile_storage))
    {
        request->peer_profile = &request->peer_profile_storage;
    }

    return 0;
}

static int simple_call_resolve_endpoint_request(
    simple_call_app_t *app,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result)
{
    uint64_t note_session_id;
    int ret;

    if (!app || !request || !request->profile || !result)
    {
        return -EINVAL;
    }

    memset(result, 0, sizeof(*result));
    note_session_id = request->session_id != 0 ?
                      request->session_id :
                      simple_call_session_id_for_msg(app, request->msg_type);
    if (app->cfg.endpoint_provider)
    {
        ret = app->cfg.endpoint_provider(app->cfg.endpoint_provider_ctx,
                                         request, result);
        if (ret < 0)
        {
            simple_call_note_media_plan_provider_bindings(
                app, note_session_id, request->stage, request->msg_type,
                request->profile, result, ret, "provider");
            return ret;
        }
    }
    else
    {
        simple_call_fill_static_bindings(request->profile, result);
    }

    if (simple_call_media_active(request->profile, CALL_MEDIA_MASK_VIDEO))
    {
        if (!result->video.has_declared_endpoint)
        {
            simple_call_note_media_plan_provider_bindings(
                app, note_session_id, request->stage, request->msg_type,
                request->profile, result, -ENOENT, "missing_video_declared");
            return -ENOENT;
        }
        if (!simple_call_declared_endpoint_valid(&result->video.declared_endpoint))
        {
            simple_call_note_media_plan_provider_bindings(
                app, note_session_id, request->stage, request->msg_type,
                request->profile, result, -EINVAL, "invalid_video_declared");
            return -EINVAL;
        }
    }
    if (simple_call_media_active(request->profile, CALL_MEDIA_MASK_AUDIO))
    {
        if (!result->audio.has_declared_endpoint)
        {
            simple_call_note_media_plan_provider_bindings(
                app, note_session_id, request->stage, request->msg_type,
                request->profile, result, -ENOENT, "missing_audio_declared");
            return -ENOENT;
        }
        if (!simple_call_declared_endpoint_valid(&result->audio.declared_endpoint))
        {
            simple_call_note_media_plan_provider_bindings(
                app, note_session_id, request->stage, request->msg_type,
                request->profile, result, -EINVAL, "invalid_audio_declared");
            return -EINVAL;
        }
    }

    simple_call_note_media_plan_provider_bindings(
        app, note_session_id, request->stage, request->msg_type,
        request->profile, result, 0, "resolved");
    return 0;
}

static int simple_call_resolve_endpoint_bindings(simple_call_app_t *app,
                                                 simple_media_endpoint_stage_t stage,
                                                 uint8_t msg_type,
                                                 const call_media_profile_t *profile,
                                                 const char *caller_device_id,
                                                 const char *callee_device_id,
                                                 uint64_t session_id,
                                                 uint32_t peer_id,
                                                 simple_media_endpoint_result_t *result)
{
    simple_media_endpoint_request_t request;
    int ret;

    if (!app || !profile || !result)
    {
        return -EINVAL;
    }

    ret = simple_call_build_endpoint_request(app, stage, msg_type, profile,
                                             caller_device_id, callee_device_id,
                                             session_id, peer_id, &request);
    if (ret < 0)
    {
        uint64_t note_session_id = session_id != 0 ?
                                   session_id :
                                   simple_call_session_id_for_msg(app, msg_type);

        simple_call_note_media_plan_provider_bindings(app, note_session_id, stage,
                                                      msg_type, profile, NULL, ret,
                                                      "build_request");
        return ret;
    }

    return simple_call_resolve_endpoint_request(app, &request, result);
}

static void simple_call_apply_declared_bindings(call_media_profile_t *profile,
                                                const simple_media_endpoint_result_t *result)
{
    if (!profile || !result)
    {
        return;
    }

    profile->has_video_endpoint = false;
    memset(&profile->video_endpoint, 0, sizeof(profile->video_endpoint));
    profile->has_audio_endpoint = false;
    memset(&profile->audio_endpoint, 0, sizeof(profile->audio_endpoint));

    if (simple_call_media_active(profile, CALL_MEDIA_MASK_VIDEO) &&
        result->video.has_declared_endpoint)
    {
        profile->has_video_endpoint = true;
        profile->video_endpoint = result->video.declared_endpoint;
    }
    if (simple_call_media_active(profile, CALL_MEDIA_MASK_AUDIO) &&
        result->audio.has_declared_endpoint)
    {
        profile->has_audio_endpoint = true;
        profile->audio_endpoint = result->audio.declared_endpoint;
    }
}

static int simple_call_media_profile_for_decision(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    call_media_profile_t *profile_out)
{
    simple_session_media_plan_profile_selection_t selection;
    uint8_t mode = 0;
    int ret;

    if (!app || !profile_out)
    {
        return -EINVAL;
    }
    if (!call_proto_media_profile_mode_for_msg(msg_type, &mode))
    {
        return -EINVAL;
    }

    memset(&selection, 0, sizeof(selection));
    selection.trigger_msg_type = msg_type;
    selection.profile_msg_type = msg_type;
    selection.direction = SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE;
    snprintf(selection.reason, sizeof(selection.reason), "%s", "missing_profile");
    ret = session_id != 0 ?
          simple_call_select_media_plan_profile_for_start(app, session_id, msg_type,
                                                          profile_out, &selection) :
          -ENOENT;
    if (ret == 0)
    {
        simple_call_note_media_plan_profile_selection(app, session_id, &selection,
                                                      profile_out, 0, selection.reason);
        return 0;
    }
    if (ret != -ENOENT)
    {
        return ret;
    }

    simple_call_note_media_plan_profile_selection(app, session_id, &selection,
                                                  NULL, -ENOENT,
                                                  "missing_profile");
    SIMPLE_LOGI("media profile missing: session=0x%" PRIx64
                " msg=%s result=FAIL ret=%d reason=missing_profile\n",
                session_id, call_proto_msg_type_name(msg_type), -ENOENT);
    return -ENOENT;
}

static void simple_call_link_decision_log(void *user_ctx, const char *line)
{
    (void)user_ctx;
    SIMPLE_LOGI("%s\n", line ? line : "-");
}

static void simple_call_apply_link_profile_policy(simple_call_app_t *app,
                                                     uint8_t msg_type,
                                                     call_media_profile_t *profile)
{
    if (!app || !profile)
    {
        return;
    }
    if (app->cfg.link_profile_policy)
    {
        app->cfg.link_profile_policy(app->cfg.link_profile_policy_ctx,
                                        msg_type, profile);
    }
}

static int simple_call_prepare_profile_for_link_decision(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    call_media_profile_t *profile_out,
    bool apply_profile_policy,
    bool clear_endpoint_without_ops,
    bool *missing_profile_out)
{
    call_media_profile_t profile;
    int ret;

    if (missing_profile_out)
    {
        *missing_profile_out = false;
    }
    if (!app)
    {
        return -EINVAL;
    }
    if (!profile_out)
    {
        return -EINVAL;
    }
    ret = simple_call_media_profile_for_decision(
        app, msg_type, session_id, &profile);
    if (ret < 0)
    {
        if (ret == -ENOENT && missing_profile_out)
        {
            *missing_profile_out = true;
        }
        return ret;
    }
    if (apply_profile_policy)
    {
        simple_call_apply_link_profile_policy(app, msg_type, &profile);
    }

    if (clear_endpoint_without_ops &&
        (profile.media_mask & CALL_MEDIA_MASK_VIDEO) != 0 &&
        (!app->cfg.media_ops || !app->cfg.media_ops->start_video))
    {
        profile.has_video_endpoint = false;
    }
    if (clear_endpoint_without_ops &&
        (profile.media_mask & CALL_MEDIA_MASK_AUDIO) != 0 &&
        (!app->cfg.media_ops || !app->cfg.media_ops->start_audio))
    {
        profile.has_audio_endpoint = false;
    }

    *profile_out = profile;
    return 0;
}

static int simple_call_run_link_decision(simple_call_app_t *app,
                                            uint8_t msg_type,
                                            uint64_t session_id,
                                            const call_media_profile_t *profile,
                                            call_media_link_decision_t *decision_out)
{
    call_media_link_decision_request_t cfg;
    call_media_link_decision_t decision;
    int ret;

    if (!app || !profile || !decision_out)
    {
        return -EINVAL;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.msg_type = msg_type;
    cfg.profile = profile;
    cfg.log_prefix = "media profile link decision:";
    cfg.log_cb = simple_call_link_decision_log;
    cfg.log_user_ctx = app;

    ret = call_media_profile_link_decide(&cfg, &decision);
    simple_call_note_media_plan_link_decision(app, msg_type, session_id,
                                                 &decision, ret);
    *decision_out = decision;
    return ret;
}

static int simple_call_link_decide_before_media_start(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    call_media_link_decision_t *decision_out,
    call_media_profile_t *profile_out,
    const char *resource_stage,
    bool *missing_profile_out)
{
    call_media_profile_t profile;
    call_media_link_decision_t decision;
    int ret;

    ret = simple_call_prepare_profile_for_link_decision(app, msg_type, session_id,
                                                           &profile, true, true,
                                                           missing_profile_out);
    if (ret < 0)
    {
        return ret;
    }

    ret = simple_call_run_link_decision(app, msg_type, session_id,
                                           &profile, &decision);
    if (ret == 0)
    {
        ret = simple_call_check_resource_policy(app, session_id, msg_type, &decision,
                                                resource_stage, NULL);
    }
    if (ret == 0 && decision_out)
    {
        *decision_out = decision;
    }
    if (ret == 0 && profile_out)
    {
        *profile_out = profile;
    }
    return ret;
}

static int simple_call_check_resource_before_response(simple_call_app_t *app,
                                                      uint8_t msg_type,
                                                      uint64_t session_id)
{
    call_media_link_decision_t decision;
    call_media_profile_t profile;
    bool missing_profile = false;
    int ret;

    memset(&decision, 0, sizeof(decision));
    memset(&profile, 0, sizeof(profile));
    ret = simple_call_link_decide_before_media_start(app, msg_type, session_id,
                                                        &decision, &profile,
                                                        "protocol",
                                                        &missing_profile);
    if (ret == -EBUSY || (ret == -ENOENT && missing_profile))
    {
        return ret;
    }
    if (ret < 0)
    {
        SIMPLE_LOGD("resource precheck ignored: session=0x%" PRIx64
                    " msg=%s ret=%d\n",
                    session_id, call_proto_msg_type_name(msg_type), ret);
    }
    return 0;
}

static uint8_t simple_call_reject_result_for_precheck(int ret)
{
    return ret == -EBUSY ? (uint8_t)CALL_RESULT_BUSY :
                           (uint8_t)CALL_RESULT_BAD_REQ;
}

static const char *simple_call_release_reason_for_precheck(int ret)
{
    return ret == -EBUSY ? "resource_denied" : "missing_profile";
}

typedef struct
{
    call_media_link_decision_t link_decision;
    int link_decision_ret;
    bool link_decision_ready;
    simple_media_endpoint_binding_t video_binding;
    simple_media_endpoint_binding_t audio_binding;
    bool has_video_binding;
    bool has_audio_binding;
    int provider_ret;
    bool provider_resolved;
} simple_call_start_decision_t;

static int simple_call_start_decision_capture_binding(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    const call_media_profile_t *profile,
    bool video,
    simple_call_start_decision_t *decision);
static int simple_call_apply_endpoint_validate_for_start(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    bool video,
    const simple_media_endpoint_binding_t *binding);
static int simple_call_apply_endpoint_invoke_for_start(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    bool video,
    const simple_media_endpoint_binding_t *binding);

static int simple_call_apply_endpoint_after_link_decision(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    const call_media_profile_t *profile,
    simple_call_start_decision_t *decision,
    bool video)
{
    const simple_media_endpoint_binding_t *binding;
    int ret;

    if (!app || !profile || !decision || !app->cfg.endpoint_provider)
    {
        return 0;
    }
    if (video ? !decision->link_decision.video_enabled :
                !decision->link_decision.audio_enabled)
    {
        return 0;
    }

    ret = simple_call_start_decision_capture_binding(app, msg_type, session_id,
                                                        profile, video, decision);
    if (ret < 0)
    {
        return ret;
    }
    if (!simple_call_media_active(profile, video ? CALL_MEDIA_MASK_VIDEO :
                                                   CALL_MEDIA_MASK_AUDIO))
    {
        return 0;
    }

    binding = video ? &decision->video_binding : &decision->audio_binding;
    if (video ? !decision->has_video_binding : !decision->has_audio_binding)
    {
        return 0;
    }

    ret = simple_call_apply_endpoint_validate_for_start(app, msg_type,
                                                        session_id, video,
                                                        binding);
    if (ret < 0)
    {
        return ret;
    }
    return simple_call_apply_endpoint_invoke_for_start(app, msg_type, session_id,
                                                       video, binding);
}

static int simple_call_start_decision_capture_binding(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    const call_media_profile_t *profile,
    bool video,
    simple_call_start_decision_t *decision)
{
    simple_media_endpoint_result_t endpoints;
    int ret;

    if (!app || !profile || !decision)
    {
        return -EINVAL;
    }
    if (decision->provider_resolved)
    {
        return decision->provider_ret;
    }

    ret = simple_call_resolve_endpoint_bindings(app,
                                                SIMPLE_MEDIA_ENDPOINT_STAGE_MEDIA_START,
                                                msg_type, profile, NULL, NULL,
                                                session_id, 0, &endpoints);
    decision->provider_resolved = true;
    decision->provider_ret = ret;
    if (ret < 0)
    {
        simple_call_note_media_plan_apply(app, msg_type, session_id,
                                          video, ret, "resolve_endpoint");
        return ret;
    }

    if (simple_call_media_active(profile, CALL_MEDIA_MASK_VIDEO))
    {
        decision->video_binding = endpoints.video;
        decision->has_video_binding = true;
    }
    if (simple_call_media_active(profile, CALL_MEDIA_MASK_AUDIO))
    {
        decision->audio_binding = endpoints.audio;
        decision->has_audio_binding = true;
    }
    return 0;
}

static int simple_call_apply_endpoint_validate_for_start(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    bool video,
    const simple_media_endpoint_binding_t *binding)
{
    if (!binding || !binding->has_tx_remote_endpoint ||
        !binding->has_rx_match_endpoint)
    {
        return 0;
    }
    if (binding->tx_remote_endpoint.port == 0 || binding->rx_match_port == 0)
    {
        simple_call_note_media_plan_apply_binding(app, msg_type, session_id, video,
                                                  -EINVAL, "invalid_endpoint",
                                                  binding);
        return -EINVAL;
    }
    if (!app->cfg.media_ops)
    {
        simple_call_note_media_plan_apply(app, msg_type, session_id,
                                          video, -EINVAL, "missing_media_ops");
        return -EINVAL;
    }
    return 0;
}

static int simple_call_apply_endpoint_invoke_for_start(
    simple_call_app_t *app,
    uint8_t msg_type,
    uint64_t session_id,
    bool video,
    const simple_media_endpoint_binding_t *binding)
{
    int ret;

    if (!binding || !binding->has_tx_remote_endpoint ||
        !binding->has_rx_match_endpoint)
    {
        return 0;
    }
    if (video)
    {
        if (!app->cfg.media_ops->apply_video_endpoint)
        {
            return 0;
        }
        ret = app->cfg.media_ops->apply_video_endpoint(app->cfg.media_ops->user_ctx,
                                                       binding);
        simple_call_note_media_plan_apply_binding(app, msg_type, session_id, video,
                                                  ret, "apply_video_endpoint",
                                                  binding);
        return ret;
    }

    if (!app->cfg.media_ops->apply_audio_endpoint)
    {
        return 0;
    }
    ret = app->cfg.media_ops->apply_audio_endpoint(app->cfg.media_ops->user_ctx,
                                                   binding);
    simple_call_note_media_plan_apply_binding(app, msg_type, session_id, video,
                                              ret, "apply_audio_endpoint",
                                              binding);
    return ret;
}

static uint32_t simple_call_peer_from_payload(const call_flow_runtime_inbound_event_t *in)
{
    if (!in)
    {
        return 0;
    }

    return in->src_ep.addr;
}

typedef struct
{
    uint8_t trigger_msg_type;
    uint8_t profile_msg_type;
    uint64_t session_id;
    simple_session_media_kind_t media_kind;
    bool video;
    call_media_profile_t profile;
    simple_call_start_decision_t start_decision;
    bool decision_allows_media;
    bool session_active;
    bool start_needed;
} simple_call_media_start_context_t;

static void simple_call_media_start_context_init(simple_call_media_start_context_t *ctx,
                                                 uint8_t trigger_msg_type,
                                                 uint64_t session_id,
                                                 simple_session_media_kind_t media_kind)
{
    if (!ctx)
    {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->trigger_msg_type = trigger_msg_type;
    ctx->profile_msg_type = trigger_msg_type;
    ctx->session_id = session_id;
    ctx->media_kind = media_kind;
    ctx->video = media_kind == SIMPLE_SESSION_MEDIA_KIND_VIDEO;
    if (ctx->video && trigger_msg_type == CALL_MSG_CALL_RING)
    {
        ctx->profile_msg_type = CALL_MSG_CALL_INVITE;
    }
}

static int simple_call_media_start_prepare_video_ring(
    simple_call_app_t *app,
    simple_call_media_start_context_t *ctx)
{
    int ret;

    ret = simple_call_prepare_profile_for_link_decision(
        app, ctx->profile_msg_type, ctx->session_id, &ctx->profile,
        false, false, NULL);
    if (ret < 0)
    {
        return ret;
    }
    ret = simple_call_run_link_decision(app, ctx->profile_msg_type,
                                           ctx->session_id, &ctx->profile,
                                           &ctx->start_decision.link_decision);
    ctx->start_decision.link_decision_ret = ret;
    ctx->start_decision.link_decision_ready = true;
    ctx->decision_allows_media = ret == 0 &&
        ctx->start_decision.link_decision.video_enabled;
    if (ret < 0)
    {
        return ret;
    }
    return simple_call_check_resource_policy(app, ctx->session_id,
                                             ctx->profile_msg_type,
                                             &ctx->start_decision.link_decision,
                                             "start", NULL);
}

static int simple_call_media_start_prepare_profile(
    simple_call_app_t *app,
    simple_call_media_start_context_t *ctx)
{
    int ret;

    if (!app || !ctx)
    {
        return -EINVAL;
    }
    if (ctx->profile_msg_type == 0)
    {
        return 0;
    }
    if (ctx->video && ctx->trigger_msg_type == CALL_MSG_CALL_RING)
    {
        return simple_call_media_start_prepare_video_ring(app, ctx);
    }

    ret = simple_call_prepare_profile_for_link_decision(
        app, ctx->profile_msg_type, ctx->session_id, &ctx->profile,
        true, true, NULL);
    if (ret < 0)
    {
        return ret;
    }
    ret = simple_call_run_link_decision(app, ctx->profile_msg_type,
                                           ctx->session_id, &ctx->profile,
                                           &ctx->start_decision.link_decision);
    ctx->start_decision.link_decision_ret = ret;
    ctx->start_decision.link_decision_ready = true;
    ctx->decision_allows_media = ret == 0 &&
        (ctx->video ? ctx->start_decision.link_decision.video_enabled :
                      ctx->start_decision.link_decision.audio_enabled);
    if (ret < 0)
    {
        return ret;
    }
    return simple_call_check_resource_policy(app, ctx->session_id,
                                             ctx->profile_msg_type,
                                             &ctx->start_decision.link_decision,
                                             "start", NULL);
}

static bool simple_call_media_start_decision_enabled(
    const simple_call_media_start_context_t *ctx)
{
    if (!ctx || ctx->trigger_msg_type == 0)
    {
        return true;
    }
    if (!ctx->start_decision.link_decision_ready ||
        ctx->start_decision.link_decision_ret < 0)
    {
        return false;
    }
    return ctx->decision_allows_media;
}

static void simple_call_media_start_load_runtime_state(
    simple_call_app_t *app,
    simple_call_media_start_context_t *ctx)
{
    if (!app || !ctx)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    ctx->session_active = simple_session_media_plan_runtime_active(
        &app->media_plan, ctx->session_id, ctx->media_kind);
    if (ctx->video)
    {
        ctx->start_needed = !ctx->session_active &&
            simple_session_media_plan_count_runtime_active(
                &app->media_plan, ctx->media_kind) == 0;
    }
    else
    {
        ctx->start_needed = !ctx->session_active &&
            simple_session_media_plan_count_runtime_active(
                &app->media_plan, ctx->media_kind) == 0;
    }
    pthread_mutex_unlock(&app->lock);
}

static int simple_call_media_start_apply_endpoint_if_needed(
    simple_call_app_t *app,
    simple_call_media_start_context_t *ctx)
{
    if (!app || !ctx || ctx->profile_msg_type == 0 || !ctx->start_needed)
    {
        return 0;
    }
    return simple_call_apply_endpoint_after_link_decision(app, ctx->profile_msg_type,
                                                        ctx->session_id,
                                                        &ctx->profile,
                                                        &ctx->start_decision,
                                                        ctx->video);
}

static int simple_call_media_start_ops_if_needed(
    simple_call_app_t *app,
    const simple_call_media_start_context_t *ctx)
{
    if (!app || !ctx || !ctx->start_needed || !app->cfg.media_ops)
    {
        return 0;
    }
    if (ctx->video)
    {
        if (!app->cfg.media_ops->start_video)
        {
            return 0;
        }
        return app->cfg.media_ops->start_video(app->cfg.media_ops->user_ctx);
    }

    if (!app->cfg.media_ops->start_audio)
    {
        return 0;
    }
    return app->cfg.media_ops->start_audio(app->cfg.media_ops->user_ctx);
}

static void simple_call_media_start_rollback_ops(
    simple_call_app_t *app,
    const simple_call_media_start_context_t *ctx)
{
    if (!app || !ctx || !ctx->start_needed || !app->cfg.media_ops)
    {
        return;
    }
    if (ctx->video && app->cfg.media_ops->stop_video)
    {
        app->cfg.media_ops->stop_video(app->cfg.media_ops->user_ctx);
    }
    else if (!ctx->video && app->cfg.media_ops->stop_audio)
    {
        app->cfg.media_ops->stop_audio(app->cfg.media_ops->user_ctx);
    }
}

static int simple_call_media_start_commit_runtime_active(
    simple_call_app_t *app,
    const simple_call_media_start_context_t *ctx)
{
    const char *reason;
    int note_ret;

    if (!app || !ctx)
    {
        return -EINVAL;
    }

    reason = ctx->start_needed ?
             (ctx->video ? "start_video" : "start_audio") :
             (ctx->video ? "shared_video" : "shared_audio");

    pthread_mutex_lock(&app->lock);
    note_ret = simple_session_media_plan_note_runtime_active(
        &app->media_plan, ctx->session_id, ctx->media_kind,
        ctx->trigger_msg_type, true, 0, reason);
    pthread_mutex_unlock(&app->lock);
    if (note_ret < 0)
    {
        simple_call_media_start_rollback_ops(app, ctx);
    }
    SIMPLE_LOGD("media runtime active: session=0x%" PRIx64 " media=%s "
                "msg=%s active=1 ret=%d reason=%s\n",
                ctx->session_id,
                simple_session_media_kind_name(ctx->media_kind),
                call_proto_msg_type_name(ctx->trigger_msg_type),
                note_ret,
                reason);
    return note_ret;
}

static void simple_call_start_video_for_msg(simple_call_app_t *app,
                                            uint8_t trigger_msg_type,
                                            uint64_t session_id)
{
    simple_call_media_start_context_t ctx;
    int ret;

    simple_call_media_start_context_init(&ctx, trigger_msg_type, session_id,
                                         SIMPLE_SESSION_MEDIA_KIND_VIDEO);
    ret = simple_call_media_start_prepare_profile(app, &ctx);
    if (ret < 0)
    {
        return;
    }
    if (!simple_call_media_start_decision_enabled(&ctx))
    {
        return;
    }

    simple_call_media_start_load_runtime_state(app, &ctx);
    if (ctx.session_active)
    {
        return;
    }

    ret = simple_call_media_start_apply_endpoint_if_needed(app, &ctx);
    if (ret < 0)
    {
        return;
    }
    ret = simple_call_media_start_ops_if_needed(app, &ctx);
    if (ret < 0)
    {
        return;
    }
    (void)simple_call_media_start_commit_runtime_active(app, &ctx);
}

static int simple_call_start_audio_for_msg(simple_call_app_t *app,
                                           uint8_t trigger_msg_type,
                                           uint64_t session_id)
{
    simple_call_media_start_context_t ctx;
    int ret;

    simple_call_media_start_context_init(&ctx, trigger_msg_type, session_id,
                                         SIMPLE_SESSION_MEDIA_KIND_AUDIO);
    simple_call_media_start_load_runtime_state(app, &ctx);
    if (ctx.session_active)
    {
        return 0;
    }

    ret = simple_call_media_start_prepare_profile(app, &ctx);
    if (ret < 0)
    {
        return ret;
    }
    if (!simple_call_media_start_decision_enabled(&ctx))
    {
        return 0;
    }

    if (!ctx.start_needed)
    {
        return simple_call_media_start_commit_runtime_active(app, &ctx);
    }

    if (app->cfg.media_ops && app->cfg.media_ops->start_audio)
    {
        ret = simple_call_media_start_apply_endpoint_if_needed(app, &ctx);
        if (ret < 0)
        {
            return ret;
        }
        ret = simple_call_media_start_ops_if_needed(app, &ctx);
        if (ret < 0)
        {
            return ret;
        }
    }

    return simple_call_media_start_commit_runtime_active(app, &ctx);
}

typedef struct
{
    uint64_t ids[SIMPLE_SESSION_MANAGER_MAX_SESSIONS + 2];
    size_t count;
} simple_call_session_id_list_t;

static void simple_call_collect_session_id(void *user_ctx,
                                           const simple_session_record_t *record)
{
    simple_call_session_id_list_t *list = (simple_call_session_id_list_t *)user_ctx;
    size_t i;

    if (!list || !record || record->session_id == 0)
    {
        return;
    }
    for (i = 0; i < list->count; i++)
    {
        if (list->ids[i] == record->session_id)
        {
            return;
        }
    }
    if (list->count < (sizeof(list->ids) / sizeof(list->ids[0])))
    {
        list->ids[list->count++] = record->session_id;
    }
}

static void simple_call_force_idle(simple_call_app_t *app)
{
    simple_call_session_id_list_t sessions;
    size_t i;

    if (!app)
    {
        return;
    }

    memset(&sessions, 0, sizeof(sessions));
    pthread_mutex_lock(&app->lock);
    simple_session_manager_dump(&app->session_manager,
                                simple_call_collect_session_id, &sessions);
    pthread_mutex_unlock(&app->lock);

    for (i = 0; i < sessions.count; i++)
    {
        simple_call_release_endpoint_bindings(app, sessions.ids[i], "force_idle");
        simple_call_session_release(app, sessions.ids[i], "force_idle");
    }
}

static bool simple_call_load_call_invite_offer(simple_call_app_t *app,
                                               uint64_t session_id,
                                               call_media_profile_t *offer_out)
{
    return simple_call_load_peer_media_profile_for_msg(app, session_id,
                                                       CALL_MSG_CALL_ACCEPT,
                                                       offer_out);
}

static int simple_call_profile_producer_endpoint_resolver(
    void *user_ctx,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result)
{
    return simple_call_resolve_endpoint_request((simple_call_app_t *)user_ctx,
                                                request, result);
}

static int simple_call_prepare_tx_media_profile(simple_call_app_t *app,
                                                uint8_t msg_type,
                                                const char *caller_device_id,
                                                const char *callee_device_id,
                                                uint64_t session_id,
                                                uint32_t peer_id,
                                                call_media_profile_t *profile)
{
    simple_session_media_plan_profile_producer_request_t producer_request;
    simple_session_media_plan_profile_producer_result_t producer_result;
    call_media_profile_t offer_profile;
    bool has_offer_profile;
    int ret;

    if (!app || !profile)
    {
        return -EINVAL;
    }

    memset(&producer_request, 0, sizeof(producer_request));
    has_offer_profile = simple_call_load_call_invite_offer(app, session_id,
                                                           &offer_profile);
    producer_request.msg_type = msg_type;
    producer_request.session_id = session_id;
    producer_request.self_device_id = app->cfg.self_device_id;
    producer_request.caller_device_id = caller_device_id;
    producer_request.callee_device_id = callee_device_id;
    producer_request.peer_id = peer_id;
    producer_request.tx_profile_shape_policy = app->cfg.tx_profile_shape_policy;
    producer_request.tx_profile_shape_policy_ctx = app->cfg.tx_profile_shape_policy_ctx;
    producer_request.offer_profile = has_offer_profile ? &offer_profile : NULL;
    producer_request.has_offer_profile = has_offer_profile;
    producer_request.endpoint_resolver = simple_call_profile_producer_endpoint_resolver;
    producer_request.endpoint_resolver_ctx = app;
    ret = simple_session_media_plan_prepare_tx_profile(&producer_request,
                                                       profile, &producer_result);
    simple_call_note_media_plan_profile_shape(app, session_id, msg_type,
                                              &producer_result.shape);
    simple_call_note_media_plan_profile_producer(app, session_id,
                                                 &producer_result, profile);
    return ret;
}

static int simple_call_send_with_msg(simple_call_app_t *app, uint8_t msg_type,
                                     const char *callee_device_id, uint32_t txn_id,
                                     uint64_t session_id,
                                     const call_endpoint_t *target)
{
    return simple_call_send_with_msg_ids(app, msg_type, callee_device_id,
                                         app ? app->cfg.self_device_id : NULL,
                                         callee_device_id, txn_id, session_id, target);
}

static bool simple_call_msg_should_send_media_profile(uint8_t msg_type)
{
    return call_proto_msg_requires_media_profile(msg_type);
}

static void simple_call_resolve_payload_ids(simple_call_app_t *app,
                                            const call_payload_t *payload,
                                            char *caller_device_id_out,
                                            size_t caller_device_id_out_len,
                                            char *callee_device_id_out,
                                            size_t callee_device_id_out_len)
{
    if (!caller_device_id_out || caller_device_id_out_len == 0 ||
        !callee_device_id_out || callee_device_id_out_len == 0)
    {
        return;
    }

    caller_device_id_out[0] = '\0';
    callee_device_id_out[0] = '\0';

    if (payload)
    {
        if (payload->has_caller_device_id)
        {
            (void)simple_call_copy_payload_string(caller_device_id_out,
                                                  caller_device_id_out_len,
                                                  payload->caller_device_id);
        }
        if (payload->has_callee_device_id)
        {
            (void)simple_call_copy_payload_string(callee_device_id_out,
                                                  callee_device_id_out_len,
                                                  payload->callee_device_id);
        }
    }

    if (!IS_VALID_STRING(caller_device_id_out) && app)
    {
        (void)simple_call_copy_payload_string(caller_device_id_out,
                                              caller_device_id_out_len,
                                              app->cfg.self_device_id);
    }
    if (!IS_VALID_STRING(callee_device_id_out) && app)
    {
        (void)simple_call_copy_payload_string(callee_device_id_out,
                                              callee_device_id_out_len,
                                              app->cfg.self_device_id);
    }
}

static bool simple_call_inbound_is_local(const simple_call_app_t *app,
                                         const char *caller_device_id,
                                         const char *callee_device_id)
{
    if (!app || !IS_VALID_STRING(app->cfg.self_device_id))
    {
        return false;
    }

    return simple_call_device_id_equal(caller_device_id, app->cfg.self_device_id) ||
           simple_call_device_id_equal(callee_device_id, app->cfg.self_device_id);
}

typedef struct
{
    call_send_params_t params;
    call_payload_t payload;
    call_media_profile_t profile;
    bool use_profile;
    uint32_t endpoint_peer_id;
} simple_call_tx_context_t;

static int simple_call_tx_context_init(simple_call_tx_context_t *tx,
                                       uint8_t msg_type,
                                       const char *route_callee_device_id,
                                       const char *payload_caller_device_id,
                                       const char *payload_callee_device_id,
                                       uint32_t txn_id,
                                       uint64_t session_id)
{
    int ret;

    if (!tx)
    {
        return -EINVAL;
    }

    memset(tx, 0, sizeof(*tx));
    if (IS_VALID_STRING(route_callee_device_id))
    {
        tx->params.has_callee_device_id = true;
        ret = simple_call_copy_payload_string(tx->params.callee_device_id,
                                              sizeof(tx->params.callee_device_id),
                                              route_callee_device_id);
        if (ret < 0)
        {
            return ret;
        }
    }
    tx->params.ack_required = msg_type != CALL_MSG_CALL_BYE &&
                              msg_type != CALL_MSG_MONITOR_STOP;
    tx->params.txn_id = txn_id;
    tx->params.session_id = session_id;

    if (IS_VALID_STRING(payload_caller_device_id))
    {
        tx->payload.has_caller_device_id = true;
        ret = simple_call_copy_payload_string(tx->payload.caller_device_id,
                                              sizeof(tx->payload.caller_device_id),
                                              payload_caller_device_id);
        if (ret < 0)
        {
            return ret;
        }
    }
    if (IS_VALID_STRING(payload_callee_device_id))
    {
        tx->payload.has_callee_device_id = true;
        ret = simple_call_copy_payload_string(tx->payload.callee_device_id,
                                              sizeof(tx->payload.callee_device_id),
                                              payload_callee_device_id);
        if (ret < 0)
        {
            return ret;
        }
    }
    tx->params.payload = &tx->payload;
    tx->use_profile = simple_call_msg_should_send_media_profile(msg_type);
    return 0;
}

static void simple_call_tx_context_resolve_next_hop(simple_call_app_t *app,
                                                    simple_call_tx_context_t *tx,
                                                    const char *route_callee_device_id,
                                                    const call_endpoint_t *target)
{
    if (!app || !tx)
    {
        return;
    }

    if (target)
    {
        tx->params.has_target_override = true;
        tx->params.target_override = *target;
        return;
    }

    if (IS_VALID_STRING(route_callee_device_id) &&
        simple_call_route_lookup_cb(app, route_callee_device_id, &tx->params.ingress_ep))
    {
        tx->params.has_ingress_ep = true;
    }
}

static int simple_call_tx_context_prepare_media_profile(
    simple_call_app_t *app,
    simple_call_tx_context_t *tx,
    uint8_t msg_type,
    const char *payload_caller_device_id,
    const char *payload_callee_device_id,
    uint64_t session_id)
{
    int ret;

    if (!app || !tx)
    {
        return -EINVAL;
    }
    if (!tx->use_profile)
    {
        return 0;
    }

    if (tx->params.has_target_override)
    {
        tx->endpoint_peer_id = tx->params.target_override.addr;
    }
    else if (tx->params.has_ingress_ep)
    {
        tx->endpoint_peer_id = tx->params.ingress_ep.addr;
    }

    ret = simple_call_prepare_tx_media_profile(app, msg_type,
                                               payload_caller_device_id,
                                               payload_callee_device_id,
                                               session_id, tx->endpoint_peer_id,
                                               &tx->profile);
    if (ret < 0)
    {
        SIMPLE_LOGI("simple_call tx media_profile failed: msg=%s ret=%d\n",
                    call_proto_msg_type_name(msg_type), ret);
        return ret;
    }

    tx->params.media_profile = &tx->profile;
    SIMPLE_LOGD("simple_call tx media_profile: session=0x%" PRIx64
                " mode=%s mask=0x%02x video=%ux%u@%u %uk ep={link=%u,cast=%u,source=%u,0x%x:%u} "
                "audio=%uHz/%uch %ums %uk ep={link=%u,cast=%u,source=%u,0x%x:%u}\n",
                session_id,
                call_proto_media_profile_mode_name(tx->profile.mode),
                tx->profile.media_mask,
                tx->profile.video_width,
                tx->profile.video_height,
                tx->profile.video_fps,
                tx->profile.video_bitrate_kbps,
                tx->profile.video_endpoint.link_kind,
                tx->profile.video_endpoint.cast_mode,
                tx->profile.video_endpoint.source,
                tx->profile.video_endpoint.addr,
                tx->profile.video_endpoint.port,
                tx->profile.audio_sample_rate,
                tx->profile.audio_channels,
                tx->profile.audio_ptime_ms,
                tx->profile.audio_bitrate_kbps,
                tx->profile.audio_endpoint.link_kind,
                tx->profile.audio_endpoint.cast_mode,
                tx->profile.audio_endpoint.source,
                tx->profile.audio_endpoint.addr,
                tx->profile.audio_endpoint.port);
    return 0;
}

static void simple_call_tx_context_apply_result_code(simple_call_tx_context_t *tx,
                                                     uint8_t msg_type,
                                                     bool has_result_code,
                                                     uint8_t result_code)
{
    if (!tx)
    {
        return;
    }
    if (msg_type == CALL_MSG_CALL_REJECT ||
        msg_type == CALL_MSG_MONITOR_REJECT)
    {
        tx->payload.has_result_code = true;
        tx->payload.result_code = has_result_code ? result_code :
                                  (uint8_t)CALL_RESULT_BUSY;
    }
}

static void simple_call_tx_context_log_route(uint8_t msg_type,
                                             const simple_call_tx_context_t *tx)
{
    if (!tx)
    {
        return;
    }

    if (tx->params.has_target_override)
    {
        SIMPLE_LOGI("simple_call send: msg=%s target={link=%u,addr=0x%x,port=%u}\n",
                    call_proto_msg_type_name(msg_type),
                    tx->params.target_override.link_kind,
                    tx->params.target_override.addr,
                    tx->params.target_override.port);
    }
    else if (tx->params.has_ingress_ep)
    {
        SIMPLE_LOGD("simple_call send: msg=%s next_hop={link=%u,addr=0x%x,port=%u}\n",
                    call_proto_msg_type_name(msg_type),
                    tx->params.ingress_ep.link_kind,
                    tx->params.ingress_ep.addr,
                    tx->params.ingress_ep.port);
    }
}

static int simple_call_tx_dispatch(simple_call_app_t *app,
                                   uint8_t msg_type,
                                   const simple_call_tx_context_t *tx)
{
    if (!app || !app->runtime || !tx)
    {
        return -EINVAL;
    }

    switch ((call_msg_type_t)msg_type)
    {
    case CALL_MSG_CALL_INVITE:
        return call_flow_runtime_invite(app->runtime, &tx->params, 0);
    case CALL_MSG_CALL_RING:
        return call_flow_runtime_ring(app->runtime, &tx->params, 0);
    case CALL_MSG_CALL_ACCEPT:
        return call_flow_runtime_accept(app->runtime, &tx->params, 0);
    case CALL_MSG_CALL_REJECT:
        return call_flow_runtime_reject(app->runtime, &tx->params, 0);
    case CALL_MSG_CALL_BYE:
        return call_flow_runtime_bye(app->runtime, &tx->params, 0);
    case CALL_MSG_MONITOR_START:
        return call_flow_runtime_monitor_start(app->runtime, &tx->params, 0);
    case CALL_MSG_MONITOR_ACCEPT:
        return call_flow_runtime_monitor_accept(app->runtime, &tx->params, 0);
    case CALL_MSG_MONITOR_REJECT:
        return call_flow_runtime_monitor_reject(app->runtime, &tx->params, 0);
    case CALL_MSG_MONITOR_STOP:
        return call_flow_runtime_monitor_stop(app->runtime, &tx->params, 0);
    default:
        return call_flow_runtime_send_custom(app->runtime, msg_type,
                                             &tx->params, 0);
    }
}

static void simple_call_tx_commit_success(simple_call_app_t *app,
                                          uint8_t msg_type,
                                          const simple_call_tx_context_t *tx,
                                          const char *payload_caller_device_id,
                                          const char *payload_callee_device_id,
                                          uint32_t txn_id,
                                          uint64_t session_id)
{
    if (!app || !tx)
    {
        return;
    }

    if (tx->use_profile)
    {
        simple_call_note_media_plan_profile(app, session_id, msg_type,
                                            &tx->profile, true);
    }
    if (simple_call_session_kind_for_msg(msg_type) != SIMPLE_SESSION_RECORD_KIND_NONE &&
        msg_type != CALL_MSG_CALL_BYE &&
        msg_type != CALL_MSG_CALL_REJECT &&
        msg_type != CALL_MSG_MONITOR_STOP &&
        msg_type != CALL_MSG_MONITOR_REJECT)
    {
        simple_call_session_upsert(app, msg_type, session_id, txn_id,
                                   tx->endpoint_peer_id,
                                   payload_caller_device_id,
                                   payload_callee_device_id,
                                   simple_call_session_state_for_msg(msg_type));
    }
}

static int simple_call_send_with_msg_ids_ex(simple_call_app_t *app, uint8_t msg_type,
                                            const char *route_callee_device_id,
                                            const char *payload_caller_device_id,
                                            const char *payload_callee_device_id,
                                            uint32_t txn_id, uint64_t session_id,
                                            const call_endpoint_t *target,
                                            bool has_result_code,
                                            uint8_t result_code)
{
    simple_call_tx_context_t tx;
    int ret;

    if (!app || !app->runtime)
    {
        return -EINVAL;
    }

    ret = simple_call_tx_context_init(&tx, msg_type, route_callee_device_id,
                                      payload_caller_device_id,
                                      payload_callee_device_id, txn_id,
                                      session_id);
    if (ret < 0)
    {
        return ret;
    }

    simple_call_tx_context_resolve_next_hop(app, &tx, route_callee_device_id,
                                            target);
    ret = simple_call_tx_context_prepare_media_profile(app, &tx, msg_type,
                                                       payload_caller_device_id,
                                                       payload_callee_device_id,
                                                       session_id);
    if (ret < 0)
    {
        return ret;
    }

    simple_call_tx_context_apply_result_code(&tx, msg_type,
                                             has_result_code, result_code);
    simple_call_tx_context_log_route(msg_type, &tx);
    ret = simple_call_tx_dispatch(app, msg_type, &tx);

    if (ret == 0)
    {
        simple_call_tx_commit_success(app, msg_type, &tx,
                                      payload_caller_device_id,
                                      payload_callee_device_id,
                                      txn_id, session_id);
    }
    return ret;
}

static int simple_call_send_with_msg_ids(simple_call_app_t *app, uint8_t msg_type,
                                         const char *route_callee_device_id,
                                         const char *payload_caller_device_id,
                                         const char *payload_callee_device_id,
                                         uint32_t txn_id, uint64_t session_id,
                                         const call_endpoint_t *target)
{
    return simple_call_send_with_msg_ids_ex(app, msg_type, route_callee_device_id,
                                            payload_caller_device_id,
                                            payload_callee_device_id,
                                            txn_id, session_id, target,
                                            false, 0);
}

static int simple_call_send_public_ipc_monitor(simple_call_app_t *app,
                                               const char *unit_device_id,
                                               const char *ipc_ip,
                                               const char *rtsp_url,
                                               const char *ipc_username,
                                               const char *ipc_password)
{
    call_send_params_t params;
    call_payload_t payload;
    call_media_profile_t profile;
    simple_session_manager_admission_decision_t decision;
    uint64_t session_id;
    bool created = false;
    int ret;

    if (!app || !app->runtime || !IS_VALID_STRING(unit_device_id) ||
        (!IS_VALID_STRING(ipc_ip) && !IS_VALID_STRING(rtsp_url)))
    {
        return -EINVAL;
    }

    memset(&params, 0, sizeof(params));
    memset(&payload, 0, sizeof(payload));
    params.has_callee_device_id = true;
    ret = simple_call_copy_payload_string(params.callee_device_id,
                                         sizeof(params.callee_device_id),
                                         unit_device_id);
    if (ret < 0)
    {
        return ret;
    }
    params.ack_required = true;

    if (IS_VALID_STRING(app->cfg.self_device_id))
    {
        payload.has_caller_device_id = true;
        ret = simple_call_copy_payload_string(payload.caller_device_id,
                                             sizeof(payload.caller_device_id),
                                             app->cfg.self_device_id);
        if (ret < 0)
        {
            return ret;
        }
    }
    payload.has_callee_device_id = true;
    ret = simple_call_copy_payload_string(payload.callee_device_id,
                                         sizeof(payload.callee_device_id),
                                         unit_device_id);
    if (ret < 0)
    {
        return ret;
    }
    payload.has_monitor_source_type = true;
    payload.monitor_source_type = (uint8_t)CALL_MONITOR_SOURCE_PUBLIC_IPC;
    if (IS_VALID_STRING(ipc_ip))
    {
        ret = simple_call_copy_payload_string(payload.ipc_ip, sizeof(payload.ipc_ip), ipc_ip);
        if (ret < 0)
        {
            return ret;
        }
        payload.has_ipc_ip = true;
    }
    if (IS_VALID_STRING(rtsp_url))
    {
        ret = simple_call_copy_payload_string(payload.rtsp_url, sizeof(payload.rtsp_url),
                                             rtsp_url);
        if (ret < 0)
        {
            return ret;
        }
        payload.has_rtsp_url = true;
    }
    if (IS_VALID_STRING(ipc_username))
    {
        ret = simple_call_copy_payload_string(payload.ipc_username,
                                             sizeof(payload.ipc_username), ipc_username);
        if (ret < 0)
        {
            return ret;
        }
        payload.has_ipc_username = true;
    }
    if (IS_VALID_STRING(ipc_password))
    {
        ret = simple_call_copy_payload_string(payload.ipc_password,
                                             sizeof(payload.ipc_password), ipc_password);
        if (ret < 0)
        {
            return ret;
        }
        payload.has_ipc_password = true;
    }

    params.payload = &payload;
    if (simple_call_route_lookup_cb(app, unit_device_id, &params.ingress_ep))
    {
        params.has_ingress_ep = true;
    }

    pthread_mutex_lock(&app->lock);
    session_id = simple_call_next_local_session_id(app);
    ret = simple_call_session_reserve_locked(app, CALL_MSG_MONITOR_START,
                                             session_id, 0,
                                             params.has_ingress_ep ?
                                             params.ingress_ep.addr : 0,
                                             app->cfg.self_device_id,
                                             unit_device_id,
                                             SIMPLE_SESSION_RECORD_STATE_CREATED,
                                             &decision, &created);
    pthread_mutex_unlock(&app->lock);
    simple_call_session_decision_log(session_id, SIMPLE_SESSION_RECORD_KIND_MONITOR,
                                     app->cfg.self_device_id, unit_device_id,
                                     &decision);
    if (ret < 0)
    {
        return ret;
    }

    ret = simple_call_prepare_tx_media_profile(app, CALL_MSG_MONITOR_START,
                                               app->cfg.self_device_id,
                                               unit_device_id,
                                               session_id,
                                               params.has_ingress_ep ?
                                               params.ingress_ep.addr : 0,
                                               &profile);
    if (ret < 0)
    {
        SIMPLE_LOGI("simple_call tx media_profile failed: msg=%s ret=%d\n",
               call_proto_msg_type_name(CALL_MSG_MONITOR_START), ret);
        simple_call_release_endpoint_bindings(app, session_id,
                                              "MONITOR_START_PROFILE_FAIL");
        simple_call_session_release(app, session_id, "MONITOR_START_PROFILE_FAIL");
        return ret;
    }
    params.media_profile = &profile;
    params.session_id = session_id;

    SIMPLE_LOGI("simple_call send: msg=%s source=public_ipc unit=%s ipc_ip=%s "
           "rtsp_url=%s user=%s password_present=%u prefer=%s\n",
           call_proto_msg_type_name(CALL_MSG_MONITOR_START),
           unit_device_id,
           payload.has_ipc_ip ? payload.ipc_ip : "-",
           payload.has_rtsp_url ? payload.rtsp_url : "-",
           payload.has_ipc_username ? payload.ipc_username : "-",
           payload.has_ipc_password ? 1U : 0U,
           payload.has_ipc_ip ? "ipc_ip" : "rtsp_url");
    if (params.has_ingress_ep)
    {
        SIMPLE_LOGD("simple_call send: msg=%s next_hop={link=%u,addr=0x%x,port=%u}\n",
               call_proto_msg_type_name(CALL_MSG_MONITOR_START),
               params.ingress_ep.link_kind,
               params.ingress_ep.addr,
               params.ingress_ep.port);
    }

    ret = call_flow_runtime_monitor_start(app->runtime, &params, 0);
    if (ret == 0)
    {
        simple_call_note_media_plan_profile(app, session_id,
                                              CALL_MSG_MONITOR_START, &profile, true);
        simple_call_session_upsert(app, CALL_MSG_MONITOR_START,
                                          session_id, params.txn_id,
                                          params.has_ingress_ep ? params.ingress_ep.addr : 0,
                                          app->cfg.self_device_id, unit_device_id,
                                          SIMPLE_SESSION_RECORD_STATE_CREATED);
    }
    else
    {
        simple_call_release_endpoint_bindings(app, session_id, "MONITOR_START_FAIL");
        simple_call_session_release(app, session_id, "MONITOR_START_FAIL");
    }
    return ret;
}

static void simple_call_log_public_ipc_monitor(simple_call_app_t *app,
                                               const call_payload_t *payload,
                                               const char *caller_device_id,
                                               const char *callee_device_id)
{
    if (!app || !payload ||
        !payload->has_monitor_source_type ||
        payload->monitor_source_type != (uint8_t)CALL_MONITOR_SOURCE_PUBLIC_IPC)
    {
        return;
    }

    SIMPLE_LOGI("public ipc monitor: role=%s caller=%s callee=%s "
           "source=public_ipc ipc_ip=%s rtsp_url=%s user=%s password_present=%u "
           "prefer=%s\n",
           simple_call_role_name(app->cfg.role),
           IS_VALID_STRING(caller_device_id) ? caller_device_id : "-",
           IS_VALID_STRING(callee_device_id) ? callee_device_id : "-",
           payload->has_ipc_ip ? payload->ipc_ip : "-",
           payload->has_rtsp_url ? payload->rtsp_url : "-",
           payload->has_ipc_username ? payload->ipc_username : "-",
           payload->has_ipc_password ? 1U : 0U,
           payload->has_ipc_ip ? "onvif_or_local" : "rtsp_url");
}

typedef struct
{
    const call_flow_runtime_inbound_event_t *in;
    uint32_t peer_id;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
    bool local_session_msg;
} simple_call_inbound_context_t;

static void simple_call_inbound_context_init(simple_call_app_t *app,
                                             const call_flow_runtime_inbound_event_t *in,
                                             simple_call_inbound_context_t *ctx)
{
    if (!app || !in || !ctx)
    {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->in = in;
    ctx->peer_id = simple_call_peer_from_payload(in);
    simple_call_resolve_payload_ids(app, &in->payload,
                                    ctx->caller_device_id, sizeof(ctx->caller_device_id),
                                    ctx->callee_device_id, sizeof(ctx->callee_device_id));
    ctx->local_session_msg =
        simple_call_inbound_is_local(app, ctx->caller_device_id, ctx->callee_device_id);
    if (in->payload.has_media_profile)
    {
        simple_call_note_media_plan_profile(app, in->header.session_id,
                                            in->header.msg_type,
                                            &in->payload.media_profile, false);
    }
}

static bool simple_call_inbound_targets_self(simple_call_app_t *app,
                                             const simple_call_inbound_context_t *ctx)
{
    if (!app || !ctx || !ctx->in)
    {
        return false;
    }
    return ctx->in->payload.has_callee_device_id &&
           simple_call_device_id_equal(ctx->in->payload.callee_device_id,
                                       app->cfg.self_device_id);
}

static void simple_call_handle_session_state_timeout(
    simple_call_app_t *app,
    const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;

    if (!app || !ctx || !in)
    {
        return;
    }
    if (in->header.msg_type == CALL_MSG_SESSION_STATE &&
        in->payload.has_session_state &&
        in->payload.session_state == (uint8_t)CALL_SESSION_STATE_TIMEOUT &&
        ctx->local_session_msg)
    {
        simple_call_session_release(app, in->header.session_id, "timeout");
        simple_call_force_idle(app);
    }
}

static void simple_call_preempt_monitor_for_call(simple_call_app_t *app,
                                                 const char *reason)
{
    simple_session_record_t record;
    int ret;

    if (!app)
    {
        return;
    }

    ret = simple_call_copy_current_monitor_record(app, &record);
    if (ret < 0)
    {
        return;
    }

    SIMPLE_LOGI("simple_call preempt monitor for call: session=0x%" PRIx64
                " reason=%s\n",
                record.session_id, reason ? reason : "-");
    simple_call_release_endpoint_bindings(app, record.session_id, reason);
    simple_call_session_release(app, record.session_id, reason);
}

static bool simple_call_external_call_busy(simple_call_app_t *app)
{
    if (!app || !app->cfg.external_call_busy)
    {
        return false;
    }
    return app->cfg.external_call_busy(app->cfg.user_ctx);
}

static void simple_call_handle_call_invite(simple_call_app_t *app,
                                           const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;
    simple_session_manager_admission_decision_t decision;
    bool created = false;
    int admit_ret;

    if (!app || !ctx || !in || !simple_call_inbound_targets_self(app, ctx))
    {
        return;
    }

    if (simple_call_external_call_busy(app))
    {
        SIMPLE_LOGI("simple_call incoming busy: session=0x%" PRIx64
                    " reason=external_call_busy caller=%s callee=%s\n",
                    in->header.session_id,
                    IS_VALID_STRING(ctx->caller_device_id) ?
                        ctx->caller_device_id : "-",
                    IS_VALID_STRING(ctx->callee_device_id) ?
                        ctx->callee_device_id : "-");
        (void)simple_call_send_with_msg_ids_ex(
            app, CALL_MSG_CALL_REJECT, NULL,
            ctx->caller_device_id, ctx->callee_device_id,
            in->header.txn_id, in->header.session_id,
            &in->src_ep, true, (uint8_t)CALL_RESULT_BUSY);
        return;
    }

    pthread_mutex_lock(&app->lock);
    admit_ret = simple_call_session_reserve_locked(
        app, in->header.msg_type, in->header.session_id,
        in->header.txn_id, ctx->peer_id,
        ctx->caller_device_id, ctx->callee_device_id,
        SIMPLE_SESSION_RECORD_STATE_CREATED,
        &decision, &created);
    pthread_mutex_unlock(&app->lock);
    simple_call_session_decision_log(in->header.session_id,
                                     SIMPLE_SESSION_RECORD_KIND_CALL,
                                     ctx->caller_device_id, ctx->callee_device_id,
                                     &decision);
    if (admit_ret < 0)
    {
        (void)simple_call_send_with_msg_ids_ex(
            app, CALL_MSG_CALL_REJECT, NULL,
            ctx->caller_device_id, ctx->callee_device_id,
            in->header.txn_id, in->header.session_id,
            &in->src_ep, true, decision.result_code);
        return;
    }

    simple_call_preempt_monitor_for_call(app, "CALL_INVITE_PREEMPT_MONITOR");

    if (app->cfg.role == SIMPLE_DEVICE_INDOOR ||
        app->cfg.role == SIMPLE_DEVICE_MANAGE ||
        app->cfg.role == SIMPLE_DEVICE_DOOR_MAIN ||
        app->cfg.role == SIMPLE_DEVICE_SECONDARY_CONFIRM)
    {
        int resource_ret = simple_call_check_resource_before_response(
            app, in->header.msg_type, in->header.session_id);

        if (resource_ret < 0)
        {
            uint8_t result_code = simple_call_reject_result_for_precheck(resource_ret);
            const char *release_reason =
                simple_call_release_reason_for_precheck(resource_ret);

            simple_call_release_endpoint_bindings(app, in->header.session_id,
                                                  release_reason);
            simple_call_session_release(app, in->header.session_id,
                                        release_reason);
            (void)simple_call_send_with_msg_ids_ex(
                app, CALL_MSG_CALL_REJECT, NULL,
                ctx->caller_device_id, ctx->callee_device_id,
                in->header.txn_id, in->header.session_id,
                &in->src_ep, true, result_code);
            return;
        }
        /*
         * RING 是业务请求，不只是一个 ACK。它用于通知主叫端：
         * 被叫已经进入待接听阶段，此时可以先开启仅视频链路。
         */
        (void)simple_call_send_with_msg_ids(app, CALL_MSG_CALL_RING, NULL,
                                            ctx->caller_device_id,
                                            ctx->callee_device_id,
                                            in->header.txn_id,
                                            in->header.session_id,
                                            &in->src_ep);
    }
}

static void simple_call_handle_call_ring(simple_call_app_t *app,
                                         const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;

    if (!app || !ctx || !in || !ctx->local_session_msg)
    {
        return;
    }
    simple_call_session_upsert(app, in->header.msg_type,
                               in->header.session_id,
                               in->header.txn_id, ctx->peer_id,
                               ctx->caller_device_id, ctx->callee_device_id,
                               SIMPLE_SESSION_RECORD_STATE_RINGING);
}

static void simple_call_handle_call_accept(simple_call_app_t *app,
                                           const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;

    if (!app || !ctx || !in || !ctx->local_session_msg)
    {
        return;
    }
    simple_call_session_upsert(app, in->header.msg_type,
                               in->header.session_id,
                               in->header.txn_id, ctx->peer_id,
                               ctx->caller_device_id, ctx->callee_device_id,
                               SIMPLE_SESSION_RECORD_STATE_ACTIVE);
}

static void simple_call_handle_call_terminal(simple_call_app_t *app,
                                             const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;
    uint64_t release_session_id;

    if (!app || !ctx || !in || !ctx->local_session_msg)
    {
        return;
    }
    release_session_id = in->header.session_id;

    simple_call_release_endpoint_bindings(app, release_session_id,
                                          call_proto_msg_type_name(in->header.msg_type));
    simple_call_session_release(app, release_session_id,
                                call_proto_msg_type_name(in->header.msg_type));
}

static void simple_call_handle_monitor_start(simple_call_app_t *app,
                                             const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;
    simple_session_manager_admission_decision_t decision;
    bool created = false;
    int admit_ret;

    if (!app || !ctx || !in || !simple_call_inbound_targets_self(app, ctx))
    {
        return;
    }

    simple_call_log_public_ipc_monitor(app, &in->payload,
                                       ctx->caller_device_id, ctx->callee_device_id);

    /*
     * 监视刻意复用同一个固定视频组播地址，因此它可以与通话并存，
     * 只共享视频资源而不占用当前通话的音频资源。
     */
    pthread_mutex_lock(&app->lock);
    admit_ret = simple_call_session_reserve_locked(
        app, in->header.msg_type, in->header.session_id,
        in->header.txn_id, ctx->peer_id,
        ctx->caller_device_id, ctx->callee_device_id,
        SIMPLE_SESSION_RECORD_STATE_CREATED,
        &decision, &created);
    pthread_mutex_unlock(&app->lock);
    simple_call_session_decision_log(in->header.session_id,
                                     SIMPLE_SESSION_RECORD_KIND_MONITOR,
                                     ctx->caller_device_id, ctx->callee_device_id,
                                     &decision);
    if (admit_ret < 0)
    {
        (void)simple_call_send_with_msg_ids(app, CALL_MSG_MONITOR_REJECT, NULL,
                                            ctx->caller_device_id,
                                            ctx->callee_device_id,
                                            in->header.txn_id,
                                            in->header.session_id,
                                            &in->src_ep);
        return;
    }

    if (app->cfg.role == SIMPLE_DEVICE_INDOOR ||
        app->cfg.role == SIMPLE_DEVICE_DOOR_MAIN ||
        app->cfg.role == SIMPLE_DEVICE_SECONDARY_CONFIRM)
    {
        int resource_ret = simple_call_check_resource_before_response(
            app, in->header.msg_type, in->header.session_id);

        if (resource_ret < 0)
        {
            uint8_t result_code = simple_call_reject_result_for_precheck(resource_ret);
            const char *release_reason =
                simple_call_release_reason_for_precheck(resource_ret);

            simple_call_release_endpoint_bindings(app, in->header.session_id,
                                                  release_reason);
            simple_call_session_release(app, in->header.session_id,
                                        release_reason);
            (void)simple_call_send_with_msg_ids_ex(
                app, CALL_MSG_MONITOR_REJECT, NULL,
                ctx->caller_device_id, ctx->callee_device_id,
                in->header.txn_id, in->header.session_id,
                &in->src_ep, true, result_code);
            return;
        }
        (void)simple_call_send_with_msg_ids(app, CALL_MSG_MONITOR_ACCEPT, NULL,
                                            ctx->caller_device_id,
                                            ctx->callee_device_id,
                                            in->header.txn_id,
                                            in->header.session_id,
                                            &in->src_ep);
    }
}

static void simple_call_handle_monitor_accept(simple_call_app_t *app,
                                              const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;

    if (!app || !ctx || !in || !ctx->local_session_msg)
    {
        return;
    }
    simple_call_session_upsert(app, in->header.msg_type,
                               in->header.session_id,
                               in->header.txn_id, ctx->peer_id,
                               ctx->caller_device_id, ctx->callee_device_id,
                               SIMPLE_SESSION_RECORD_STATE_ACTIVE);
}

static void simple_call_handle_monitor_terminal(simple_call_app_t *app,
                                                const simple_call_inbound_context_t *ctx)
{
    const call_flow_runtime_inbound_event_t *in = ctx ? ctx->in : NULL;
    uint64_t release_session_id;

    if (!app || !ctx || !in || !ctx->local_session_msg)
    {
        return;
    }
    release_session_id = in->header.session_id;

    simple_call_release_endpoint_bindings(app, release_session_id,
                                          call_proto_msg_type_name(in->header.msg_type));
    simple_call_session_release(app, release_session_id,
                                call_proto_msg_type_name(in->header.msg_type));
}

static void simple_call_handle_inbound(simple_call_app_t *app,
                                       const call_flow_runtime_inbound_event_t *in)
{
    simple_call_inbound_context_t ctx;

    if (!app || !in || in->result_code != CALL_RESULT_OK ||
        in->decision == CALL_RUNTIME_DISPATCH_DROP)
    {
        return;
    }

    simple_call_inbound_context_init(app, in, &ctx);
    switch ((call_msg_type_t)in->header.msg_type)
    {
    case CALL_MSG_SESSION_STATE:
        simple_call_handle_session_state_timeout(app, &ctx);
        break;
    case CALL_MSG_CALL_INVITE:
        simple_call_handle_call_invite(app, &ctx);
        break;
    case CALL_MSG_CALL_RING:
        simple_call_handle_call_ring(app, &ctx);
        break;
    case CALL_MSG_CALL_ACCEPT:
        simple_call_handle_call_accept(app, &ctx);
        break;
    case CALL_MSG_CALL_BYE:
    case CALL_MSG_CALL_REJECT:
        simple_call_handle_call_terminal(app, &ctx);
        break;
    case CALL_MSG_MONITOR_START:
        simple_call_handle_monitor_start(app, &ctx);
        break;
    case CALL_MSG_MONITOR_ACCEPT:
        simple_call_handle_monitor_accept(app, &ctx);
        break;
    case CALL_MSG_MONITOR_STOP:
    case CALL_MSG_MONITOR_REJECT:
        simple_call_handle_monitor_terminal(app, &ctx);
        break;
    default:
        break;
    }
}

static void simple_call_handle_response_reject_or_error(
    simple_call_app_t *app,
    const call_flow_runtime_response_event_t *rsp)
{
    simple_session_record_t record;
    uint64_t release_session_id;

    if (!app || !rsp)
    {
        return;
    }

    SIMPLE_LOGI("simple_call: request rejected msg=%s result=%u\n",
                call_proto_msg_type_name(rsp->header.msg_type),
                rsp->payload.has_result_code ? rsp->payload.result_code : 0);
    pthread_mutex_lock(&app->lock);
    if (rsp->header.session_id != 0)
    {
        release_session_id = rsp->header.session_id;
    }
    else if (simple_session_manager_get_current_call(&app->session_manager,
                                                     &record) == 0)
    {
        release_session_id = record.session_id;
    }
    else
    {
        release_session_id = 0;
    }
    pthread_mutex_unlock(&app->lock);
    if (release_session_id != 0)
    {
        simple_call_release_endpoint_bindings(
            app, release_session_id, call_proto_msg_type_name(rsp->header.msg_type));
        simple_call_session_release(app, release_session_id,
                                    call_proto_msg_type_name(rsp->header.msg_type));
    }
}

static void simple_call_handle_response_call_ring_ack(
    simple_call_app_t *app,
    const call_flow_runtime_response_event_t *rsp)
{
    if (!app || !rsp)
    {
        return;
    }
    if (rsp->payload.has_ref_msg_type &&
        rsp->payload.ref_msg_type == CALL_MSG_CALL_RING)
    {
        simple_call_start_video_for_msg(app, CALL_MSG_CALL_RING,
                                        rsp->header.session_id);
    }
}

static void simple_call_handle_response(simple_call_app_t *app,
                                        const call_flow_runtime_response_event_t *rsp)
{
    if (!app || !rsp)
    {
        return;
    }

    if (rsp->header.msg_type == CALL_MSG_CALL_REJECT ||
        rsp->header.msg_type == CALL_MSG_ERROR)
    {
        simple_call_handle_response_reject_or_error(app, rsp);
    }
    else if (rsp->header.msg_type == CALL_MSG_ACK)
    {
        simple_call_handle_response_call_ring_ack(app, rsp);
    }
}

static const char *simple_runtime_error_stage_name(call_flow_runtime_error_stage_t stage)
{
    switch (stage)
    {
    case CALL_FLOW_RT_ERR_STAGE_SEND:
        return "send";
    case CALL_FLOW_RT_ERR_STAGE_HANDLE_PACKET:
        return "handle_packet";
    case CALL_FLOW_RT_ERR_STAGE_TICK:
        return "tick";
    case CALL_FLOW_RT_ERR_STAGE_INTERNAL:
        return "internal";
    default:
        return "unknown";
    }
}

static const char *simple_dispatch_decision_name(call_runtime_dispatch_decision_t decision)
{
    switch (decision)
    {
    case CALL_RUNTIME_DISPATCH_DROP:
        return "drop";
    case CALL_RUNTIME_DISPATCH_FORWARD:
        return "forward";
    case CALL_RUNTIME_DISPATCH_SEND_ACK:
        return "local_ack";
    case CALL_RUNTIME_DISPATCH_SEND_ERROR:
        return "local_error";
    case CALL_RUNTIME_DISPATCH_TXN_RETRY:
        return "txn_retry";
    case CALL_RUNTIME_DISPATCH_TXN_TIMEOUT:
        return "txn_timeout";
    default:
        return "unknown";
    }
}

static bool simple_call_should_log_runtime_msg(uint8_t msg_type)
{
#if SIMPLE_CALL_LOG_ACK_HEARTBEAT_EVENTS
    (void)msg_type;
    return true;
#else
    return call_proto_should_log_msg(msg_type);
#endif
}

static void simple_call_log_inbound_event(const call_flow_runtime_inbound_event_t *in)
{
    if (!in || !simple_call_should_log_runtime_msg(in->header.msg_type))
    {
        return;
    }

    SIMPLE_LOGI("simple_call event: type=inbound msg=%s result=%u decision=%s "
                "src={link=%u,addr=0x%x,port=%u} caller=%s callee=%s "
                "session=0x%" PRIx64 " txn=%u\n",
                call_proto_msg_type_name(in->header.msg_type),
                in->result_code,
                simple_dispatch_decision_name(in->decision),
                in->src_ep.link_kind,
                in->src_ep.addr,
                in->src_ep.port,
                in->payload.has_caller_device_id ?
                in->payload.caller_device_id : "-",
                in->payload.has_callee_device_id ?
                in->payload.callee_device_id : "-",
                in->header.session_id,
                in->header.txn_id);
}

static void simple_call_log_response_event(const call_flow_runtime_response_event_t *rsp)
{
    if (!rsp || !simple_call_should_log_runtime_msg(rsp->header.msg_type))
    {
        return;
    }

    SIMPLE_LOGI("simple_call event: type=response msg=%s result=%u "
                "src={link=%u,addr=0x%x,port=%u} ref=%s session=0x%" PRIx64
                " txn=%u\n",
                call_proto_msg_type_name(rsp->header.msg_type),
                rsp->payload.has_result_code ?
                rsp->payload.result_code :
                rsp->result_code,
                rsp->src_ep.link_kind,
                rsp->src_ep.addr,
                rsp->src_ep.port,
                rsp->payload.has_ref_msg_type ?
                call_proto_msg_type_name(rsp->payload.ref_msg_type) : "-",
                rsp->header.session_id,
                rsp->header.txn_id);
}

static void simple_call_handle_runtime_inbound_event(
    simple_call_app_t *app,
    const call_flow_runtime_inbound_event_t *in)
{
    if (!app || !in)
    {
        return;
    }

    simple_call_log_inbound_event(in);
    if (app->cfg.inbound_observer)
    {
        app->cfg.inbound_observer(app->cfg.user_ctx, in);
    }
    simple_call_handle_inbound(app, in);
}

static void simple_call_handle_runtime_response_event(
    simple_call_app_t *app,
    const call_flow_runtime_response_event_t *rsp)
{
    if (!app || !rsp)
    {
        return;
    }

    simple_call_log_response_event(rsp);
    simple_call_handle_response(app, rsp);
}

static void simple_call_handle_runtime_media_update(
    simple_call_app_t *app,
    const call_runtime_media_update_event_t *media_update)
{
    if (!app || !media_update)
    {
        return;
    }

    SIMPLE_LOGI("simple_call event: type=media_update video=%u audio=%u\n",
                media_update->video_active ? 1U : 0U,
                media_update->audio_active ? 1U : 0U);
    if (media_update->video_active)
    {
        simple_call_start_video_for_msg(app, media_update->trigger_msg_type,
                                        media_update->session_id);
    }
    if (media_update->audio_active)
    {
        (void)simple_call_start_audio_for_msg(app, media_update->trigger_msg_type,
                                              media_update->session_id);
    }
    if (!media_update->video_active &&
        !media_update->audio_active)
    {
        simple_call_stop_audio_for_session(app, media_update->session_id,
                                           "media_update");
        simple_call_stop_video_for_session(app, media_update->session_id,
                                           "media_update");
    }
}

static void simple_call_handle_runtime_session_error(
    simple_call_app_t *app,
    const call_flow_runtime_session_error_event_t *session_error)
{
    if (!app || !session_error)
    {
        return;
    }

    SIMPLE_LOGI("simple_call session_error: msg=%s txn=%u session=0x%" PRIx64
                " retry=%u result=%u\n",
                call_proto_msg_type_name(session_error->msg_type),
                session_error->txn_id,
                session_error->session_id,
                session_error->retry_count,
                session_error->result_code);
    simple_call_release_endpoint_bindings(app, session_error->session_id,
                                          "session_error");
    simple_call_force_idle(app);
}

static void simple_call_log_runtime_error(
    const call_flow_runtime_error_event_t *runtime_error)
{
    if (!runtime_error)
    {
        return;
    }

    SIMPLE_LOGI("simple_call runtime_error: stage=%s err=%d msg=%s txn=%u "
                "ep={link=%u,addr=0x%x,port=%u}\n",
                simple_runtime_error_stage_name(runtime_error->stage),
                runtime_error->error_code,
                call_proto_msg_type_name(runtime_error->msg_type),
                runtime_error->txn_id,
                runtime_error->ep.link_kind,
                runtime_error->ep.addr,
                runtime_error->ep.port);
}

static void simple_call_runtime_on_event(void *user_ctx,
                                         const call_flow_runtime_event_t *event)
{
    simple_call_app_t *app = (simple_call_app_t *)user_ctx;

    if (!app || !event)
    {
        return;
    }

    switch (event->type)
    {
    case CALL_FLOW_RT_EVENT_INBOUND_MESSAGE:
        simple_call_handle_runtime_inbound_event(app, &event->u.inbound);
        break;

    case CALL_FLOW_RT_EVENT_RESPONSE_MESSAGE:
        simple_call_handle_runtime_response_event(app, &event->u.response);
        break;

    case CALL_FLOW_RT_EVENT_MEDIA_UPDATE:
        simple_call_handle_runtime_media_update(app, &event->u.media_update);
        break;

    case CALL_FLOW_RT_EVENT_SESSION_ERROR:
        simple_call_handle_runtime_session_error(app, &event->u.session_error);
        break;

    case CALL_FLOW_RT_EVENT_RUNTIME_ERROR:
        simple_call_log_runtime_error(&event->u.runtime_error);
        break;

    default:
        break;
    }
}

int simple_call_app_init(simple_call_app_t **app_out, const simple_call_app_config_t *cfg)
{
    simple_call_app_t *app;
    call_flow_runtime_config_t rt_cfg;
    simple_control_route_runtime_config_t route_cfg;
    size_t i;
    int ret;

    if (!app_out || !cfg || !cfg->links || cfg->link_count == 0)
    {
        return -EINVAL;
    }
    if (cfg->route_count > SIMPLE_CALL_MAX_ROUTES ||
        cfg->link_count > SIMPLE_CALL_MAX_LINKS)
    {
        return -E2BIG;
    }
    app = (simple_call_app_t *)SIMPLE_CALLOC(1, sizeof(*app));
    if (!app)
    {
        return -ENOMEM;
    }

    app->cfg = *cfg;
    app->cfg.routes = NULL;
    app->cfg.route_count = 0;
    memset(&route_cfg, 0, sizeof(route_cfg));
    route_cfg.legacy_fallback_enabled = !cfg->disable_route_fallback;
    simple_control_route_runtime_init(&app->route_runtime, &route_cfg);
    ret = simple_control_route_runtime_load_bootstrap(&app->route_runtime,
                                                      cfg->routes,
                                                      cfg->route_count);
    if (ret != 0)
    {
        SIMPLE_FREE(app);
        return ret;
    }
    memcpy(app->link_cfg, cfg->links, cfg->link_count * sizeof(cfg->links[0]));
    app->cfg.links = app->link_cfg;
    simple_session_manager_init(&app->session_manager);
    simple_session_media_plan_init(&app->media_plan);
    simple_media_resource_policy_init(&app->resource_policy);

    SIMPLE_LOGI("simple_call init: role=%s self_device_id=%s links=%zu routes=%zu\n",
           simple_call_role_name(cfg->role),
           cfg->self_device_id ? cfg->self_device_id : "<null>",
           cfg->link_count,
           cfg->route_count);
    for (i = 0; i < cfg->link_count; i++)
    {
        SIMPLE_LOGI("simple_call link[%zu]: kind=%s proto=%u if=%s local={0x%x:%u}\n",
               i,
               simple_call_link_name(app->link_cfg[i].link_kind),
               app->link_cfg[i].proto,
               app->link_cfg[i].interface ? app->link_cfg[i].interface : "<null>",
               app->link_cfg[i].local_ep.addr,
               app->link_cfg[i].local_ep.port);
    }
    for (i = 0; i < cfg->route_count; i++)
    {
        SIMPLE_LOGI("simple_call route[%zu]: callee=%s -> {link=%s,addr=0x%x,port=%u}\n",
               i,
               cfg->routes[i].callee_device_id,
               simple_call_link_name(cfg->routes[i].next_hop.link_kind),
               cfg->routes[i].next_hop.addr,
               cfg->routes[i].next_hop.port);
    }

    ret = pthread_mutex_init(&app->lock, NULL);
    if (ret != 0)
    {
        SIMPLE_FREE(app);
        return -ret;
    }
    app->lock_ready = true;

    for (i = 0; i < cfg->link_count; i++)
    {
        transport_endpoint_t ep = app->link_cfg[i].local_ep;

        if (app->link_cfg[i].proto == TRANSPORT_PROTO_UDP)
        {
            ret = simple_call_probe_udp_exclusive(app->link_cfg[i].interface, &ep);
            if (ret < 0)
            {
                SIMPLE_LOGI("simple_call udp probe failed: if=%s local={0x%x:%u} err=%d (%s)\n",
                       app->link_cfg[i].interface ? app->link_cfg[i].interface : "<null>",
                       ep.addr, ep.port, -ret, strerror(-ret));
                simple_call_app_deinit(app);
                return ret;
            }
        }

        app->transports[i] = transport_open(app->link_cfg[i].proto,
                                            app->link_cfg[i].interface,
                                            &ep);
        if (!app->transports[i])
        {
            simple_call_app_deinit(app);
            return -EIO;
        }
        app->runtime_links[i].link_kind = app->link_cfg[i].link_kind;
        app->runtime_links[i].transport = app->transports[i];
    }

    memset(&rt_cfg, 0, sizeof(rt_cfg));
    rt_cfg.self_device_id = cfg->self_device_id;
    rt_cfg.monitor_limit = 8;
    rt_cfg.boot_nonce = cfg->boot_nonce != 0 ?
                         cfg->boot_nonce :
                         (simple_call_boot_nonce_for_role(cfg->role) ^
                          simple_call_hash_device_id(cfg->self_device_id));
    app->local_session_boot_nonce = rt_cfg.boot_nonce;
    app->local_session_counter = 0x80000000U;
    rt_cfg.strict_mode = true;
    rt_cfg.role = simple_call_runtime_role(cfg->role);
    rt_cfg.conflict_policy = cfg->role == SIMPLE_DEVICE_CONVERTER ?
                             CALL_RUNTIME_CONFLICT_RELAY_FIRST :
                             CALL_RUNTIME_CONFLICT_LOCAL_FIRST;
    rt_cfg.local_id_match = simple_call_local_id_match_cb;
    rt_cfg.local_id_match_ctx = app;
    rt_cfg.next_hop_lookup = simple_call_route_lookup_cb;
    rt_cfg.next_hop_lookup_ctx = app;
    rt_cfg.log_level = CALL_INFO;
    rt_cfg.log_handler = simple_example_call_log_handler;
    rt_cfg.mem_hooks = simple_example_call_mem_hooks();
    rt_cfg.links = app->runtime_links;
    rt_cfg.link_count = cfg->link_count;
    rt_cfg.on_event = simple_call_runtime_on_event;
    rt_cfg.get_media_state_snapshot = simple_call_media_snapshot_cb;
    rt_cfg.user_ctx = app;
#if defined(SIMPLE_CALL_APP_TESTING) && defined(CALL_FLOW_RUNTIME_TESTING)
    rt_cfg.test_hb_period_ms = cfg->test_hb_period_ms;
    rt_cfg.test_hb_miss_limit = cfg->test_hb_miss_limit;
#endif

    ret = call_flow_runtime_init(&app->runtime, &rt_cfg);
    if (ret < 0)
    {
        simple_call_app_deinit(app);
        return ret;
    }

    *app_out = app;
    return 0;
}

int simple_call_app_start(simple_call_app_t *app)
{
    int ret;

    if (!app || !app->runtime)
    {
        return -EINVAL;
    }

    ret = call_flow_runtime_start(app->runtime);
    if (ret == 0)
    {
        app->running = true;
        SIMPLE_LOGI("simple_call runtime started: role=%s self_device_id=%s\n",
               simple_call_role_name(app->cfg.role), app->cfg.self_device_id);
    }
    return ret;
}

void simple_call_app_stop(simple_call_app_t *app)
{
    if (!app || !app->runtime || !app->running)
    {
        return;
    }

    (void)call_flow_runtime_stop(app->runtime);
    app->running = false;
    simple_call_force_idle(app);
}

void simple_call_app_deinit(simple_call_app_t *app)
{
    size_t i;

    if (!app)
    {
        return;
    }

    simple_call_app_stop(app);
    if (app->runtime)
    {
        call_flow_runtime_deinit(app->runtime);
        app->runtime = NULL;
    }
    for (i = 0; i < app->cfg.link_count && i < SIMPLE_CALL_MAX_LINKS; i++)
    {
        if (app->transports[i])
        {
            transport_close(app->transports[i]);
            app->transports[i] = NULL;
        }
    }
    if (app->lock_ready)
    {
        pthread_mutex_destroy(&app->lock);
    }
    SIMPLE_FREE(app);
}

int simple_call_apply_route_fact(simple_call_app_t *app,
                                 const simple_control_route_fact_t *fact)
{
    int ret;

    if (!app || !fact)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_control_route_runtime_apply_fact(&app->route_runtime, fact);
    pthread_mutex_unlock(&app->lock);
    if (ret == 0)
    {
        SIMPLE_LOGI("simple_call route_%s: source=%s callee=%s -> "
                    "{link=%s,addr=0x%x,port=%u} observed_ms=%" PRIu64
                    " ttl_ms=%u\n",
                    fact->online ? "upsert" : "remove",
                    simple_control_route_source_name(fact->source),
                    fact->callee_device_id ? fact->callee_device_id : "-",
                    fact->online ? simple_call_link_name(fact->next_hop.link_kind) : "-",
                    fact->online ? fact->next_hop.addr : 0,
                    fact->online ? fact->next_hop.port : 0,
                    fact->online ? fact->observed_ms : 0,
                    fact->online ? fact->ttl_ms : 0);
    }
    return ret;
}

int simple_call_apply_route_online(simple_call_app_t *app,
                                   simple_control_route_source_t source,
                                   const char *callee_device_id,
                                   const call_endpoint_t *next_hop,
                                   uint64_t observed_ms,
                                   uint32_t ttl_ms)
{
    simple_control_route_fact_t fact;

    if (!app || !IS_VALID_STRING(callee_device_id) || !next_hop)
    {
        return -EINVAL;
    }

    memset(&fact, 0, sizeof(fact));
    fact.source = source;
    fact.callee_device_id = callee_device_id;
    fact.online = true;
    fact.next_hop = *next_hop;
    fact.observed_ms = observed_ms;
    fact.ttl_ms = ttl_ms;
    return simple_call_apply_route_fact(app, &fact);
}

int simple_call_apply_route_offline(simple_call_app_t *app,
                                    simple_control_route_source_t source,
                                    const char *callee_device_id)
{
    simple_control_route_fact_t fact;

    if (!app || !IS_VALID_STRING(callee_device_id))
    {
        return -EINVAL;
    }

    memset(&fact, 0, sizeof(fact));
    fact.source = source;
    fact.callee_device_id = callee_device_id;
    fact.online = false;
    return simple_call_apply_route_fact(app, &fact);
}

size_t simple_call_expire_route_facts(simple_call_app_t *app, uint64_t now_ms)
{
    size_t expired;

    if (!app)
    {
        return 0;
    }

    pthread_mutex_lock(&app->lock);
    expired = simple_control_route_runtime_expire(&app->route_runtime, now_ms);
    pthread_mutex_unlock(&app->lock);
    if (expired > 0)
    {
        SIMPLE_LOGI("simple_call route_expire: now_ms=%" PRIu64 " expired=%zu\n",
                    now_ms,
                    expired);
    }
    return expired;
}

static int simple_call_copy_current_call_record(simple_call_app_t *app,
                                                simple_session_record_t *record)
{
    int ret;

    if (!app || !record)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_manager_get_current_call(&app->session_manager, record);
    pthread_mutex_unlock(&app->lock);
    return ret;
}

static int simple_call_copy_current_monitor_record(simple_call_app_t *app,
                                                   simple_session_record_t *record)
{
    int ret;

    if (!app || !record)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_manager_get_current_monitor(&app->session_manager, record);
    pthread_mutex_unlock(&app->lock);
    return ret;
}

int simple_call_start(simple_call_app_t *app, const char *callee_device_id)
{
    simple_session_manager_admission_decision_t decision;
    call_endpoint_t next_hop;
    bool created = false;
    uint64_t session_id;
    int ret;

    if (!app || !IS_VALID_STRING(callee_device_id))
    {
        return -EINVAL;
    }
    if (!simple_call_route_lookup_cb(app, callee_device_id, &next_hop))
    {
        return -ENOENT;
    }

    pthread_mutex_lock(&app->lock);
    session_id = simple_call_next_local_session_id(app);
    ret = simple_call_session_reserve_locked(app, CALL_MSG_CALL_INVITE,
                                             session_id, 0, next_hop.addr,
                                             app->cfg.self_device_id,
                                             callee_device_id,
                                             SIMPLE_SESSION_RECORD_STATE_CREATED,
                                             &decision, &created);
    pthread_mutex_unlock(&app->lock);
    simple_call_session_decision_log(session_id, SIMPLE_SESSION_RECORD_KIND_CALL,
                                     app->cfg.self_device_id, callee_device_id,
                                     &decision);

    if (ret < 0)
    {
        return ret;
    }
    if (session_id == 0)
    {
        simple_call_session_release(app, session_id, "CALL_INVITE_INVALID_SESSION");
        return -EINVAL;
    }

    ret = simple_call_send_with_msg(app, CALL_MSG_CALL_INVITE,
                                    callee_device_id, 0, session_id, NULL);
    if (ret < 0)
    {
        simple_call_release_endpoint_bindings(app, session_id, "CALL_INVITE_FAIL");
        simple_call_session_release(app, session_id, "CALL_INVITE_FAIL");
    }
    return ret;
}

int simple_call_accept(simple_call_app_t *app)
{
    simple_session_record_t record;
    uint32_t peer_id;
    const char *route_peer_device_id = NULL;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
    uint32_t txn_id;
    uint64_t session_id;
    int ret;

    if (!app)
    {
        return -EINVAL;
    }

    ret = simple_call_copy_current_call_record(app, &record);
    if (ret < 0)
    {
        return ret;
    }
    peer_id = record.peer_id;
    (void)simple_call_copy_payload_string(caller_device_id, sizeof(caller_device_id),
                                          record.caller_device_id);
    (void)simple_call_copy_payload_string(callee_device_id, sizeof(callee_device_id),
                                          record.callee_device_id);
    txn_id = record.txn_id;
    session_id = record.session_id;

    if (peer_id == 0 || session_id == 0)
    {
        return -EINVAL;
    }

    if (simple_call_device_id_equal(caller_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = callee_device_id;
    }
    else if (simple_call_device_id_equal(callee_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = caller_device_id;
    }

    ret = simple_call_send_with_msg_ids(app, CALL_MSG_CALL_ACCEPT, route_peer_device_id,
                                        caller_device_id, callee_device_id,
                                        txn_id, session_id, NULL);
    if (ret == 0)
    {
        simple_call_start_video_for_msg(app, CALL_MSG_CALL_ACCEPT, session_id);
        ret = simple_call_start_audio_for_msg(app, CALL_MSG_CALL_ACCEPT, session_id);
    }
    return ret;
}

int simple_call_reject(simple_call_app_t *app)
{
    simple_session_record_t record;
    const char *route_peer_device_id = NULL;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
    uint32_t txn_id;
    uint64_t session_id;
    int ret;

    if (!app)
    {
        return -EINVAL;
    }

    ret = simple_call_copy_current_call_record(app, &record);
    if (ret < 0)
    {
        return ret;
    }
    (void)simple_call_copy_payload_string(caller_device_id, sizeof(caller_device_id),
                                          record.caller_device_id);
    (void)simple_call_copy_payload_string(callee_device_id, sizeof(callee_device_id),
                                          record.callee_device_id);
    txn_id = record.txn_id;
    session_id = record.session_id;

    if (simple_call_device_id_equal(caller_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = callee_device_id;
    }
    else if (simple_call_device_id_equal(callee_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = caller_device_id;
    }

    ret = simple_call_send_with_msg_ids(app, CALL_MSG_CALL_REJECT, route_peer_device_id,
                                        caller_device_id, callee_device_id,
                                        txn_id, session_id, NULL);
    simple_call_release_endpoint_bindings(app, session_id, "CALL_REJECT");
    simple_call_session_release(app, session_id, "CALL_REJECT");
    return ret;
}

int simple_call_hangup(simple_call_app_t *app)
{
    simple_session_record_t record;
    const char *route_peer_device_id = NULL;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
    uint32_t txn_id;
    uint64_t session_id;
    int ret;

    if (!app)
    {
        return -EINVAL;
    }

    ret = simple_call_copy_current_call_record(app, &record);
    if (ret < 0)
    {
        return ret;
    }
    (void)simple_call_copy_payload_string(caller_device_id, sizeof(caller_device_id),
                                          record.caller_device_id);
    (void)simple_call_copy_payload_string(callee_device_id, sizeof(callee_device_id),
                                          record.callee_device_id);
    txn_id = record.txn_id;
    session_id = record.session_id;

    if (simple_call_device_id_equal(caller_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = callee_device_id;
    }
    else if (simple_call_device_id_equal(callee_device_id, app->cfg.self_device_id))
    {
        route_peer_device_id = caller_device_id;
    }

    ret = simple_call_send_with_msg_ids(app, CALL_MSG_CALL_BYE, route_peer_device_id,
                                        caller_device_id, callee_device_id,
                                        txn_id, session_id, NULL);
    simple_call_release_endpoint_bindings(app, session_id, "CALL_BYE");
    simple_call_session_release(app, session_id, "CALL_BYE");
    return ret;
}

int simple_monitor_start(simple_call_app_t *app, const char *target_device_id)
{
    simple_session_manager_admission_decision_t decision;
    call_endpoint_t next_hop;
    uint64_t session_id;
    bool created = false;
    int ret;

    if (!app || !IS_VALID_STRING(target_device_id))
    {
        return -EINVAL;
    }
    if (!simple_call_route_lookup_cb(app, target_device_id, &next_hop))
    {
        return -ENOENT;
    }

    pthread_mutex_lock(&app->lock);
    session_id = simple_call_next_local_session_id(app);
    ret = simple_call_session_reserve_locked(app, CALL_MSG_MONITOR_START,
                                             session_id, 0, next_hop.addr,
                                             app->cfg.self_device_id,
                                             target_device_id,
                                             SIMPLE_SESSION_RECORD_STATE_CREATED,
                                             &decision, &created);
    pthread_mutex_unlock(&app->lock);
    simple_call_session_decision_log(session_id, SIMPLE_SESSION_RECORD_KIND_MONITOR,
                                     app->cfg.self_device_id, target_device_id,
                                     &decision);
    if (ret < 0)
    {
        return ret;
    }

    ret = simple_call_send_with_msg(app, CALL_MSG_MONITOR_START,
                                    target_device_id, 0, session_id, NULL);
    if (ret < 0)
    {
        simple_call_release_endpoint_bindings(app, session_id, "MONITOR_START_FAIL");
        simple_call_session_release(app, session_id, "MONITOR_START_FAIL");
    }
    return ret;
}

int simple_monitor_start_public_ipc(simple_call_app_t *app,
                                    const char *unit_device_id,
                                    const char *ipc_ip,
                                    const char *rtsp_url,
                                    const char *ipc_username,
                                    const char *ipc_password)
{
    return simple_call_send_public_ipc_monitor(app, unit_device_id, ipc_ip, rtsp_url,
                                               ipc_username, ipc_password);
}

int simple_monitor_stop_session(simple_call_app_t *app, uint64_t session_id)
{
    simple_session_record_t record;
    uint32_t peer_id;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
    int ret;

    if (!app || session_id == 0)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&app->lock);
    ret = simple_session_manager_get(&app->session_manager, session_id, &record);
    pthread_mutex_unlock(&app->lock);
    if (ret < 0 || record.kind != SIMPLE_SESSION_RECORD_KIND_MONITOR)
    {
        return ret < 0 ? ret : -EINVAL;
    }

    peer_id = record.peer_id;
    (void)simple_call_copy_payload_string(caller_device_id, sizeof(caller_device_id),
                                          record.caller_device_id);
    (void)simple_call_copy_payload_string(callee_device_id, sizeof(callee_device_id),
                                          record.callee_device_id);

    simple_call_release_endpoint_bindings(app, session_id, "MONITOR_STOP");
    simple_call_session_release(app, session_id, "MONITOR_STOP");

    if (peer_id == 0)
    {
        return -EINVAL;
    }

    return simple_call_send_with_msg_ids(app, CALL_MSG_MONITOR_STOP,
                                         callee_device_id, caller_device_id, callee_device_id,
                                         0, session_id, NULL);
}

int simple_monitor_stop(simple_call_app_t *app)
{
    simple_session_record_t record;
    int ret;

    ret = simple_call_copy_current_monitor_record(app, &record);
    if (ret < 0)
    {
        return ret;
    }

    return simple_monitor_stop_session(app, record.session_id);
}

simple_session_state_t simple_call_get_state(simple_call_app_t *app)
{
    simple_session_state_t state;

    if (!app)
    {
        return SIMPLE_SESSION_IDLE;
    }

    pthread_mutex_lock(&app->lock);
    state = simple_session_manager_public_state(&app->session_manager);
    pthread_mutex_unlock(&app->lock);
    return state;
}

void simple_call_dump_diagnostics(simple_call_app_t *app)
{
    simple_control_route_runtime_entry_t route_entries[SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS];
    size_t session_count;
    size_t release_count;
    size_t media_count;
    size_t route_count;
    size_t route_snapshot_count;
    size_t i;
    unsigned int plc_video_count;
    unsigned int plc_video_limit;
    simple_session_state_t state;

    if (!app)
    {
        return;
    }

    pthread_mutex_lock(&app->lock);
    state = simple_session_manager_public_state(&app->session_manager);
    session_count = simple_session_manager_count(&app->session_manager);
    release_count = simple_session_manager_release_count(&app->session_manager);
    media_count = simple_session_media_plan_count(&app->media_plan);
    plc_video_count = simple_session_media_plan_count_plc_video(&app->media_plan);
    plc_video_limit = app->resource_policy.plc_video_max;
    route_count = simple_control_route_runtime_count(&app->route_runtime);
    route_snapshot_count =
        simple_control_route_runtime_snapshot(&app->route_runtime,
                                              route_entries,
                                              SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS);

    SIMPLE_LOGI("call diagnostics: role=%s state=%s sessions=%zu releases=%zu "
                "media_plans=%zu route_facts=%zu plc_video=%u/%u media_plan=active "
                "session_manager=decision resource_policy=deny\n",
                simple_call_role_name(app->cfg.role),
                simple_session_state_name(state),
                session_count,
                release_count,
                media_count,
                route_count,
                plc_video_count,
                plc_video_limit);
    for (i = 0; i < route_snapshot_count; ++i)
    {
        const simple_control_route_runtime_entry_t *entry = &route_entries[i];

        SIMPLE_LOGI("call route_fact[%zu]: source=%s callee=%s "
                    "next_hop={link=%s,addr=0x%x,port=%u} observed_ms=%" PRIu64
                    " ttl_ms=%u seq=%u\n",
                    i,
                    simple_control_route_source_name(entry->source),
                    entry->callee_device_id,
                    simple_call_link_name(entry->next_hop.link_kind),
                    entry->next_hop.addr,
                    entry->next_hop.port,
                    entry->observed_ms,
                    entry->ttl_ms,
                    entry->seq);
    }
    simple_session_manager_dump(&app->session_manager,
                                simple_call_dump_session_record,
                                app);
    simple_session_media_plan_dump(&app->media_plan,
                                   simple_call_dump_media_plan_record,
                                   NULL);
    simple_session_manager_dump_releases(&app->session_manager,
                                         simple_call_dump_session_release_record,
                                         NULL);
    pthread_mutex_unlock(&app->lock);
}

int simple_call_note_media_bridge_plan(simple_call_app_t *app,
                                       uint64_t session_id,
                                       bool video,
                                       const simple_media_bridge_plan_t *bridge_plan)
{
    simple_session_media_kind_t media_kind;
    int ret;

    if (!app || session_id == 0 || !bridge_plan)
    {
        return -EINVAL;
    }

    media_kind = video ? SIMPLE_SESSION_MEDIA_KIND_VIDEO : SIMPLE_SESSION_MEDIA_KIND_AUDIO;

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_note_bridge_plan(&app->media_plan,
                                                     session_id,
                                                     media_kind,
                                                     bridge_plan);
    pthread_mutex_unlock(&app->lock);
    return ret;
}

int simple_call_get_media_bridge_plan(simple_call_app_t *app,
                                      uint64_t session_id,
                                      bool video,
                                      simple_media_bridge_plan_t *bridge_plan)
{
    simple_session_media_kind_t media_kind;
    simple_session_media_plan_record_t record;
    int ret;

    if (!app || session_id == 0 || !bridge_plan)
    {
        return -EINVAL;
    }

    media_kind = video ? SIMPLE_SESSION_MEDIA_KIND_VIDEO : SIMPLE_SESSION_MEDIA_KIND_AUDIO;

    pthread_mutex_lock(&app->lock);
    ret = simple_session_media_plan_get(&app->media_plan,
                                        session_id,
                                        media_kind,
                                        &record);
    pthread_mutex_unlock(&app->lock);
    if (ret < 0)
    {
        return ret;
    }
    if (!record.bridge_plan_seen)
    {
        return -ENOENT;
    }

    memset(bridge_plan, 0, sizeof(*bridge_plan));
    bridge_plan->has_ingress_endpoint = record.has_bridge_plan_ingress_endpoint;
    bridge_plan->ingress_endpoint = record.bridge_plan_ingress_endpoint;
    bridge_plan->has_egress_endpoint = record.has_bridge_plan_egress_endpoint;
    bridge_plan->egress_endpoint = record.bridge_plan_egress_endpoint;
    bridge_plan->has_rx_match_endpoint = record.has_bridge_plan_rx_match_endpoint;
    bridge_plan->rx_match_addr = record.bridge_plan_rx_match_addr;
    bridge_plan->rx_match_port = record.bridge_plan_rx_match_port;
    snprintf(bridge_plan->reason, sizeof(bridge_plan->reason), "%s",
             record.bridge_plan_reason[0] ? record.bridge_plan_reason : "-");
    return 0;
}

void simple_call_note_media_bridge_result(simple_call_app_t *app,
                                          uint64_t session_id,
                                          bool video,
                                          const simple_call_media_bridge_result_t *result)
{
    simple_session_media_kind_t media_kind;

    if (!app || session_id == 0 || !result)
    {
        return;
    }

    media_kind = video ? SIMPLE_SESSION_MEDIA_KIND_VIDEO : SIMPLE_SESSION_MEDIA_KIND_AUDIO;

    pthread_mutex_lock(&app->lock);
    (void)simple_session_media_plan_note_bridge_endpoint(
        &app->media_plan,
        session_id,
        media_kind,
        result->ret,
        result->has_ingress_endpoint ? &result->ingress_endpoint : NULL,
        result->has_ingress_endpoint,
        result->has_egress_endpoint ? &result->egress_endpoint : NULL,
        result->has_egress_endpoint,
        result->rx_match_addr,
        result->rx_match_port,
        result->has_rx_match_endpoint,
        result->reason);
    pthread_mutex_unlock(&app->lock);
}
