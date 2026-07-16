#include "simple_media_runtime.h"

#include "simple_log.h"
#include "simple_mem.h"

#include <errno.h>
#include <string.h>
#include <time.h>

typedef struct
{
    struct simple_media_runtime_debug_probe_state *probe;
    simple_media_runtime_debug_probe_link_t link;
    simple_media_runtime_debug_probe_media_t media;
} simple_media_runtime_debug_probe_stream_ctx_t;

struct simple_media_runtime_debug_probe_state
{
    mtrans_stream_rx_t udp_video_probe;
    mtrans_stream_rx_t udp_audio_probe;
    mtrans_stream_rx_t plc_video_probe;
    mtrans_stream_rx_t plc_audio_probe;
    simple_media_runtime_debug_probe_observer_fn observer;
    void *observer_ctx;
    simple_media_runtime_debug_probe_stream_ctx_t udp_video_ctx;
    simple_media_runtime_debug_probe_stream_ctx_t udp_audio_ctx;
    simple_media_runtime_debug_probe_stream_ctx_t plc_video_ctx;
    simple_media_runtime_debug_probe_stream_ctx_t plc_audio_ctx;
};

static void simple_media_runtime_lock(simple_media_runtime_t *runtime)
{
    if (runtime && runtime->cfg.lock)
    {
        pthread_mutex_lock(runtime->cfg.lock);
    }
}

static void simple_media_runtime_unlock(simple_media_runtime_t *runtime)
{
    if (runtime && runtime->cfg.lock)
    {
        pthread_mutex_unlock(runtime->cfg.lock);
    }
}

static void simple_media_runtime_lock_const(const simple_media_runtime_t *runtime)
{
    if (runtime && runtime->cfg.lock)
    {
        pthread_mutex_lock(runtime->cfg.lock);
    }
}

static void simple_media_runtime_unlock_const(const simple_media_runtime_t *runtime)
{
    if (runtime && runtime->cfg.lock)
    {
        pthread_mutex_unlock(runtime->cfg.lock);
    }
}

static void simple_media_runtime_notify_apply(
    simple_media_runtime_t *runtime,
    bool audio,
    const simple_media_endpoint_binding_t *binding,
    int ret,
    const char *reason)
{
    if (runtime && runtime->cfg.apply_observer)
    {
        runtime->cfg.apply_observer(runtime->cfg.apply_observer_ctx,
                                    audio,
                                    binding,
                                    ret,
                                    reason ? reason : "-");
    }
}

static void simple_media_runtime_copy_device_id(char *dst, size_t cap, const char *src)
{
    if (!dst || cap == 0)
    {
        return;
    }

    if (!src)
    {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

static bool simple_media_runtime_endpoint_type_is_bridge(simple_media_endpoint_type_t type);
static bool simple_media_runtime_has_streams_locked(const simple_media_runtime_t *runtime);
static int simple_media_runtime_build_bridge_config_from_descriptor_locked(
    const simple_media_runtime_t *runtime,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor,
    simple_media_config_t *cfg_out);

static void simple_media_runtime_direct_bridge_cleanup_record_locked(
    simple_media_runtime_bridge_record_t *record)
{
    mtrans_route_rule_t rule;

    if (!record || !record->owns_direct_resources)
    {
        return;
    }

    if (record->direct_bridge_ingress_ch && record->direct_bridge_egress_ch &&
        record->direct_bridge_reverse_added)
    {
        memset(&rule, 0, sizeof(rule));
        rule.src_channel = record->direct_bridge_egress_ch;
        rule.dst_channel = record->direct_bridge_ingress_ch;
        rule.route_group = record->direct_bridge_route_group;
        rule.flags = record->direct_bridge_reverse_flags;
        (void)mtrans_route_del_rule(&rule);
    }
    if (record->direct_bridge_ingress_ch && record->direct_bridge_egress_ch &&
        record->direct_bridge_forward_added)
    {
        memset(&rule, 0, sizeof(rule));
        rule.src_channel = record->direct_bridge_ingress_ch;
        rule.dst_channel = record->direct_bridge_egress_ch;
        rule.route_group = record->direct_bridge_route_group;
        rule.flags = record->direct_bridge_forward_flags;
        (void)mtrans_route_del_rule(&rule);
    }
    if (record->direct_bridge_ingress_ch)
    {
        mtrans_channel_destroy(record->direct_bridge_ingress_ch);
        record->direct_bridge_ingress_ch = NULL;
    }
    if (record->direct_bridge_egress_ch)
    {
        mtrans_channel_destroy(record->direct_bridge_egress_ch);
        record->direct_bridge_egress_ch = NULL;
    }
    record->direct_bridge_route_group = 0;
    record->direct_bridge_forward_flags = 0;
    record->direct_bridge_reverse_flags = 0;
    record->direct_bridge_forward_added = false;
    record->direct_bridge_reverse_added = false;
    record->owns_direct_resources = false;
}

static void simple_media_runtime_fill_udp_channel_cfg(
    mtrans_channel_config_t *cfg,
    bool tx_mode,
    const simple_media_config_t *media_cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->link_type = MTRANS_LINK_TYPE_UDP;
    cfg->link_profile = MTRANS_LINK_PROFILE_UDP;
    cfg->mtu = SIMPLE_UDP_RTP_MTU;
    cfg->proto = TRANSPORT_PROTO_UDP;
    cfg->interface = media_cfg->udp_interface;
    cfg->local_ep.addr = media_cfg->udp_local_addr;
    cfg->local_ep.port = SIMPLE_UDP_MEDIA_PORT;
    cfg->remote_ep.addr = media_cfg->udp_remote_addr;
    cfg->remote_ep.port = media_cfg->udp_remote_port;
    cfg->rtcp_sr_rr_disable = true;
    cfg->mcast_enabled = !tx_mode && media_cfg->udp_mcast_enabled;
    cfg->mcast_opt.group_addr = media_cfg->udp_mcast_group_addr;
    cfg->mcast_loop_disable = media_cfg->udp_mcast_loop_disable;
    cfg->rx_match_ep_enabled = media_cfg->udp_rx_match_ep_enabled;
    cfg->rx_match_ep.addr = media_cfg->udp_rx_match_addr;
    cfg->rx_match_ep.port = media_cfg->udp_rx_match_port;
}

static void simple_media_runtime_fill_plc_channel_cfg(
    mtrans_channel_config_t *cfg,
    const simple_media_config_t *media_cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->link_type = MTRANS_LINK_TYPE_PLC;
    cfg->link_profile = MTRANS_LINK_PROFILE_PLC;
    cfg->mtu = SIMPLE_PLC_RTP_MTU;
    cfg->proto = TRANSPORT_PROTO_PLC;
    cfg->local_ep.addr = media_cfg->plc_local_addr;
    cfg->local_ep.port = SIMPLE_PLC_MEDIA_PORT;
    cfg->remote_ep.addr = media_cfg->plc_remote_addr;
    cfg->remote_ep.port = media_cfg->plc_remote_port;
    cfg->rtcp_sr_rr_disable = true;
    cfg->rx_match_ep_enabled = media_cfg->plc_rx_match_ep_enabled;
    cfg->rx_match_ep.addr = media_cfg->plc_rx_match_addr;
    cfg->rx_match_ep.port = media_cfg->plc_rx_match_port;
    cfg->mcast_enabled = media_cfg->plc_mcast_enabled;
    cfg->mcast_opt.group_addr = media_cfg->plc_mcast_group_addr;
}

static int simple_media_runtime_add_media_route_locked(
    mtrans_channel_t src,
    mtrans_channel_t dst,
    uint32_t route_group,
    uint32_t flags)
{
    mtrans_route_rule_t rule;
    int ret;

    if (!src || !dst)
    {
        return -EINVAL;
    }
    memset(&rule, 0, sizeof(rule));
    rule.src_channel = src;
    rule.dst_channel = dst;
    rule.route_group = route_group;
    rule.flags = flags;
    ret = mtrans_route_add_rule(&rule);
    return ret < 0 ? ret : (ret == 0 ? 0 : -EIO);
}

static void simple_media_runtime_del_media_route_locked(
    mtrans_channel_t src,
    mtrans_channel_t dst,
    uint32_t route_group,
    uint32_t flags)
{
    mtrans_route_rule_t rule;

    if (!src || !dst)
    {
        return;
    }
    memset(&rule, 0, sizeof(rule));
    rule.src_channel = src;
    rule.dst_channel = dst;
    rule.route_group = route_group;
    rule.flags = flags;
    (void)mtrans_route_del_rule(&rule);
}

static void simple_media_runtime_media_bridge_cleanup_record_locked(
    simple_media_runtime_bridge_record_t *record)
{
    if (!record || !record->owns_media_resources)
    {
        return;
    }

    if (record->media_bridge_udp_to_video_plc_added)
    {
        simple_media_runtime_del_media_route_locked(
            record->media_bridge_udp_ch,
            record->media_bridge_video_plc_ch,
            record->media_bridge_route_group,
            record->media_bridge_udp_to_video_plc_flags);
    }
    if (record->media_bridge_plc_to_audio_udp_added)
    {
        simple_media_runtime_del_media_route_locked(
            record->media_bridge_plc_ch,
            record->media_bridge_audio_udp_ch,
            record->media_bridge_route_group,
            record->media_bridge_plc_to_audio_udp_flags);
    }
    if (record->media_bridge_plc_to_udp_added)
    {
        simple_media_runtime_del_media_route_locked(
            record->media_bridge_plc_ch,
            record->media_bridge_udp_ch,
            record->media_bridge_route_group,
            record->media_bridge_plc_to_udp_flags);
    }
    if (record->media_bridge_udp_to_plc_added)
    {
        simple_media_runtime_del_media_route_locked(
            record->media_bridge_udp_ch,
            record->media_bridge_plc_ch,
            record->media_bridge_route_group,
            record->media_bridge_udp_to_plc_flags);
    }

    if (record->media_bridge_audio_udp_ch)
    {
        mtrans_channel_destroy(record->media_bridge_audio_udp_ch);
        record->media_bridge_audio_udp_ch = NULL;
    }
    if (record->media_bridge_video_plc_ch)
    {
        mtrans_channel_destroy(record->media_bridge_video_plc_ch);
        record->media_bridge_video_plc_ch = NULL;
    }
    if (record->media_bridge_udp_ch)
    {
        mtrans_channel_destroy(record->media_bridge_udp_ch);
        record->media_bridge_udp_ch = NULL;
    }
    if (record->media_bridge_plc_ch)
    {
        mtrans_channel_destroy(record->media_bridge_plc_ch);
        record->media_bridge_plc_ch = NULL;
    }

    record->media_bridge_route_group = 0;
    record->media_bridge_udp_to_plc_flags = 0;
    record->media_bridge_plc_to_udp_flags = 0;
    record->media_bridge_udp_to_video_plc_flags = 0;
    record->media_bridge_plc_to_audio_udp_flags = 0;
    record->media_bridge_udp_to_plc_added = false;
    record->media_bridge_plc_to_udp_added = false;
    record->media_bridge_udp_to_video_plc_added = false;
    record->media_bridge_plc_to_audio_udp_added = false;
    record->owns_media_resources = false;
}

static int simple_media_runtime_prepare_media_bridge_locked(simple_media_runtime_t *runtime)
{
    simple_media_t *media = runtime ? runtime->cfg.media : NULL;

    if (!media || !media->configured)
    {
        return -EINVAL;
    }
    if (!media->channels_open)
    {
        return 0;
    }
    if (simple_media_runtime_has_streams_locked(runtime) ||
        (media->channel_cfg_valid &&
         simple_media_runtime_endpoint_type_is_bridge(media->channel_cfg.type)))
    {
        return -EBUSY;
    }
    if (media->audio_tx_ch)
    {
        mtrans_channel_destroy(media->audio_tx_ch);
        media->audio_tx_ch = NULL;
    }
    if (media->bridge_audio_udp_ch)
    {
        mtrans_channel_destroy(media->bridge_audio_udp_ch);
        media->bridge_audio_udp_ch = NULL;
    }
    if (media->bridge_video_plc_ch)
    {
        mtrans_channel_destroy(media->bridge_video_plc_ch);
        media->bridge_video_plc_ch = NULL;
    }
    if (media->udp_ch)
    {
        mtrans_channel_destroy(media->udp_ch);
        media->udp_ch = NULL;
    }
    if (media->plc_ch)
    {
        mtrans_channel_destroy(media->plc_ch);
        media->plc_ch = NULL;
    }
    media->channels_open = false;
    media->channel_cfg_valid = false;
    media->audio_tx_channel_cfg_valid = false;
    memset(&media->channel_cfg, 0, sizeof(media->channel_cfg));
    memset(&media->audio_tx_channel_cfg, 0, sizeof(media->audio_tx_channel_cfg));
    return 0;
}

static void simple_media_runtime_clear_pending_media_bridge_locked(
    simple_media_runtime_t *runtime)
{
    if (!runtime)
    {
        return;
    }
    runtime->has_pending_media_bridge = false;
    memset(&runtime->pending_media_bridge, 0, sizeof(runtime->pending_media_bridge));
}

static void simple_media_runtime_bridge_record_clear(
    simple_media_runtime_bridge_record_t *record)
{
    if (!record)
    {
        return;
    }
    memset(record, 0, sizeof(*record));
}

static const char *simple_media_runtime_bridge_kind_name(
    simple_media_runtime_bridge_kind_t kind)
{
    switch (kind)
    {
    case SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA:
        return "media";
    case SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_DIRECT:
        return "direct";
    case SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_NONE:
    default:
        return "none";
    }
}

static const char *simple_media_runtime_media_bridge_mode_name(
    simple_media_runtime_media_bridge_mode_t mode)
{
    switch (mode)
    {
    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_BIDIRECTIONAL:
        return "bidirectional";
    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_SPLIT:
        return "split";
    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_AUDIO_ONLY:
        return "audio_only";
    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_NONE:
    default:
        return "none";
    }
}

static void simple_media_runtime_fill_bridge_snapshot_locked(
    const simple_media_runtime_bridge_record_t *record,
    simple_media_runtime_bridge_snapshot_t *snapshot)
{
    if (!snapshot)
    {
        return;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    if (!record)
    {
        return;
    }
    snapshot->state = record->state;
    snapshot->has_media_descriptor = record->has_media_descriptor;
    if (record->has_media_descriptor)
    {
        snapshot->media_descriptor = record->media_descriptor;
    }
    snapshot->owns_media_resources = record->owns_media_resources;
    snapshot->media_udp_open = record->media_bridge_udp_ch != NULL;
    snapshot->media_plc_open = record->media_bridge_plc_ch != NULL;
    snapshot->media_video_plc_open = record->media_bridge_video_plc_ch != NULL;
    snapshot->media_audio_udp_open = record->media_bridge_audio_udp_ch != NULL;
    snapshot->media_route_group = record->media_bridge_route_group;
    snapshot->media_udp_to_plc_flags = record->media_bridge_udp_to_plc_flags;
    snapshot->media_plc_to_udp_flags = record->media_bridge_plc_to_udp_flags;
    snapshot->media_udp_to_video_plc_flags = record->media_bridge_udp_to_video_plc_flags;
    snapshot->media_plc_to_audio_udp_flags = record->media_bridge_plc_to_audio_udp_flags;
    snapshot->media_udp_to_plc_added = record->media_bridge_udp_to_plc_added;
    snapshot->media_plc_to_udp_added = record->media_bridge_plc_to_udp_added;
    snapshot->media_udp_to_video_plc_added = record->media_bridge_udp_to_video_plc_added;
    snapshot->media_plc_to_audio_udp_added = record->media_bridge_plc_to_audio_udp_added;
    snapshot->owns_direct_resources = record->owns_direct_resources;
    snapshot->direct_ingress_open = record->direct_bridge_ingress_ch != NULL;
    snapshot->direct_egress_open = record->direct_bridge_egress_ch != NULL;
    snapshot->direct_route_group = record->direct_bridge_route_group;
    snapshot->direct_forward_flags = record->direct_bridge_forward_flags;
    snapshot->direct_reverse_flags = record->direct_bridge_reverse_flags;
    snapshot->direct_forward_added = record->direct_bridge_forward_added;
    snapshot->direct_reverse_added = record->direct_bridge_reverse_added;
}

static simple_media_runtime_bridge_record_t *
simple_media_runtime_active_bridge_record_locked(simple_media_runtime_t *runtime)
{
    size_t i;

    if (!runtime)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active)
        {
            return &runtime->bridge_table[i];
        }
    }
    return NULL;
}

static const simple_media_runtime_bridge_record_t *
simple_media_runtime_active_bridge_record_const_locked(
    const simple_media_runtime_t *runtime)
{
    size_t i;

    if (!runtime)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active)
        {
            return &runtime->bridge_table[i];
        }
    }
    return NULL;
}

static bool simple_media_runtime_bridge_table_has_active_locked(
    const simple_media_runtime_t *runtime)
{
    return simple_media_runtime_active_bridge_record_const_locked(runtime) != NULL;
}

static size_t simple_media_runtime_bridge_table_count_active_locked(
    const simple_media_runtime_t *runtime)
{
    size_t i;
    size_t count = 0;

    if (!runtime)
    {
        return 0;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active)
        {
            ++count;
        }
    }
    return count;
}

static bool simple_media_runtime_bridge_table_has_kind_active_locked(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_kind_t kind)
{
    size_t i;

    if (!runtime)
    {
        return false;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active &&
            runtime->bridge_table[i].state.kind == kind)
        {
            return true;
        }
    }
    return false;
}

static simple_media_runtime_bridge_record_t *
simple_media_runtime_bridge_table_find_session_locked(
    simple_media_runtime_t *runtime,
    uint64_t session_id)
{
    size_t i;

    if (!runtime || session_id == 0)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active &&
            runtime->bridge_table[i].state.session_id == session_id)
        {
            return &runtime->bridge_table[i];
        }
    }
    return NULL;
}

static const simple_media_runtime_bridge_record_t *
simple_media_runtime_bridge_table_find_session_const_locked(
    const simple_media_runtime_t *runtime,
    uint64_t session_id)
{
    size_t i;

    if (!runtime || session_id == 0)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active &&
            runtime->bridge_table[i].state.session_id == session_id)
        {
            return &runtime->bridge_table[i];
        }
    }
    return NULL;
}

static const simple_media_runtime_bridge_record_t *
simple_media_runtime_bridge_table_active_at_const_locked(
    const simple_media_runtime_t *runtime,
    size_t ordinal)
{
    size_t i;
    size_t active_ordinal = 0;

    if (!runtime)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active)
        {
            if (active_ordinal == ordinal)
            {
                return &runtime->bridge_table[i];
            }
            ++active_ordinal;
        }
    }
    return NULL;
}

static simple_media_runtime_bridge_record_t *
simple_media_runtime_bridge_table_alloc_locked(simple_media_runtime_t *runtime)
{
    size_t i;

    if (!runtime)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (!runtime->bridge_table[i].used)
        {
            return &runtime->bridge_table[i];
        }
    }
    return NULL;
}

static void simple_media_runtime_clear_bridge_table_locked(
    simple_media_runtime_t *runtime)
{
    size_t i;

    if (!runtime)
    {
        return;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        simple_media_runtime_media_bridge_cleanup_record_locked(&runtime->bridge_table[i]);
        simple_media_runtime_direct_bridge_cleanup_record_locked(&runtime->bridge_table[i]);
        simple_media_runtime_bridge_record_clear(&runtime->bridge_table[i]);
    }
}

static void simple_media_runtime_clear_pending_endpoint_locked(
    simple_media_runtime_t *runtime,
    bool audio)
{
    if (!runtime)
    {
        return;
    }
    if (audio)
    {
        runtime->has_pending_audio_endpoint = false;
        memset(&runtime->pending_audio_endpoint, 0, sizeof(runtime->pending_audio_endpoint));
    }
    else
    {
        runtime->has_pending_video_endpoint = false;
        memset(&runtime->pending_video_endpoint, 0, sizeof(runtime->pending_video_endpoint));
    }
}

static void simple_media_runtime_clear_active_endpoint_locked(
    simple_media_runtime_t *runtime,
    bool audio)
{
    if (!runtime)
    {
        return;
    }
    if (audio)
    {
        runtime->has_active_audio_endpoint = false;
        memset(&runtime->active_audio_endpoint, 0, sizeof(runtime->active_audio_endpoint));
    }
    else
    {
        runtime->has_active_video_endpoint = false;
        memset(&runtime->active_video_endpoint, 0, sizeof(runtime->active_video_endpoint));
    }
}

static void simple_media_runtime_restore_base_config_locked(
    simple_media_runtime_t *runtime)
{
    if (!runtime || !runtime->cfg.media || !runtime->has_base_config)
    {
        return;
    }
    runtime->cfg.media->base_cfg = runtime->base_config;
}

static bool simple_media_runtime_endpoint_type_is_bridge(simple_media_endpoint_type_t type)
{
    return type == SIMPLE_MEDIA_ENDPOINT_UDP_TO_PLC_BRIDGE ||
           type == SIMPLE_MEDIA_ENDPOINT_PLC_TO_UDP_BRIDGE;
}

static bool simple_media_runtime_has_streams_locked(const simple_media_runtime_t *runtime)
{
    const simple_media_t *media = runtime ? runtime->cfg.media : NULL;

    return media && (media->video_tx || media->video_rx ||
                     media->audio_tx || media->audio_rx);
}

static uint64_t simple_media_runtime_monotonic_ms(void)
{
    struct timespec ts;
    uint64_t now_ms;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 1;
    }
    now_ms = (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
    return now_ms != 0 ? now_ms : 1;
}

static bool simple_media_runtime_type_is_plc(simple_media_endpoint_type_t type);

static void simple_media_runtime_clear_video_feedback_locked(
    simple_media_runtime_t *runtime)
{
    if (!runtime)
    {
        return;
    }
    runtime->video_feedback_last_pli_ms = 0;
    runtime->video_feedback_last_fir_ms = 0;
}

static void simple_media_runtime_fill_endpoint_snapshot(
    const simple_media_config_t *cfg,
    simple_media_runtime_endpoint_snapshot_t *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    if (!cfg)
    {
        return;
    }

    snapshot->type = cfg->type;
    snapshot->udp_interface = cfg->udp_interface;
    snapshot->udp_remote_addr = cfg->udp_remote_addr;
    snapshot->udp_remote_port = (uint16_t)cfg->udp_remote_port;
    snapshot->udp_mcast_enabled = cfg->udp_mcast_enabled;
    snapshot->plc_local_addr = cfg->plc_local_addr;
    snapshot->plc_remote_addr = cfg->plc_remote_addr;
    snapshot->plc_remote_port = (uint16_t)cfg->plc_remote_port;
    snapshot->plc_rx_match_ep_enabled = cfg->plc_rx_match_ep_enabled;
    snapshot->plc_rx_match_addr = cfg->plc_rx_match_addr;
    snapshot->plc_rx_match_port = (uint16_t)cfg->plc_rx_match_port;
    snapshot->audio_tx_remote_ep_enabled = cfg->audio_tx_remote_ep_enabled;
    snapshot->audio_tx_remote_addr = cfg->audio_tx_remote_addr;
    snapshot->audio_tx_remote_port = (uint16_t)cfg->audio_tx_remote_port;
}

static int simple_media_runtime_copy_base_config_locked(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg_out)
{
    if (!runtime || !cfg_out)
    {
        return -EINVAL;
    }
    if (runtime->has_base_config)
    {
        *cfg_out = runtime->base_config;
        return 0;
    }
    if (runtime->cfg.media)
    {
        *cfg_out = runtime->cfg.media->base_cfg;
        return 0;
    }
    return -EINVAL;
}

static int simple_media_runtime_apply_video_binding_to_config(
    simple_media_config_t *cfg,
    const simple_media_endpoint_binding_t *binding)
{
    if (!cfg || !binding ||
        !binding->has_tx_remote_endpoint || !binding->has_rx_match_endpoint ||
        binding->tx_remote_endpoint.port == 0 || binding->rx_match_port == 0)
    {
        return -EINVAL;
    }

    if (binding->tx_remote_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC)
    {
        if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_TX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_PLC_TX;
        }
        else if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_RX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_PLC_RX;
        }
    }
    else if (binding->tx_remote_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_UDP)
    {
        if (cfg->type == SIMPLE_MEDIA_ENDPOINT_PLC_TX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_UDP_TX;
        }
        else if (cfg->type == SIMPLE_MEDIA_ENDPOINT_PLC_RX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_UDP_RX;
        }
    }

    if (simple_media_runtime_type_is_plc(cfg->type))
    {
        cfg->plc_remote_addr = binding->tx_remote_endpoint.addr;
        cfg->plc_remote_port = binding->tx_remote_endpoint.port;
        cfg->plc_rx_match_ep_enabled = true;
        cfg->plc_rx_match_addr = binding->rx_match_addr;
        cfg->plc_rx_match_port = binding->rx_match_port;
        cfg->plc_mcast_enabled =
            binding->tx_remote_endpoint.cast_mode == (uint8_t)CALL_MEDIA_CAST_MULTICAST;
        cfg->plc_mcast_group_addr = cfg->plc_mcast_enabled ?
                                    binding->tx_remote_endpoint.addr : 0;
        return 0;
    }

    if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_TX ||
        cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_RX)
    {
        cfg->udp_remote_addr = binding->tx_remote_endpoint.addr;
        cfg->udp_remote_port = binding->tx_remote_endpoint.port;
        cfg->udp_rx_match_ep_enabled = true;
        cfg->udp_rx_match_addr = binding->rx_match_addr;
        cfg->udp_rx_match_port = binding->rx_match_port;
        cfg->udp_mcast_enabled =
            binding->tx_remote_endpoint.cast_mode == (uint8_t)CALL_MEDIA_CAST_MULTICAST;
        cfg->udp_mcast_group_addr = cfg->udp_mcast_enabled ?
                                    binding->tx_remote_endpoint.addr : 0;
        return 0;
    }

    return -EINVAL;
}

static int simple_media_runtime_apply_audio_binding_to_config(
    simple_media_config_t *cfg,
    const simple_media_endpoint_binding_t *binding)
{
    if (!cfg || !binding ||
        !binding->has_tx_remote_endpoint || !binding->has_rx_match_endpoint ||
        binding->tx_remote_endpoint.port == 0 || binding->rx_match_port == 0)
    {
        return -EINVAL;
    }

    if (binding->tx_remote_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC)
    {
        if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_TX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_PLC_TX;
        }
        else if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_RX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_PLC_RX;
        }
    }
    else if (binding->tx_remote_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_UDP)
    {
        if (cfg->type == SIMPLE_MEDIA_ENDPOINT_PLC_TX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_UDP_TX;
        }
        else if (cfg->type == SIMPLE_MEDIA_ENDPOINT_PLC_RX)
        {
            cfg->type = SIMPLE_MEDIA_ENDPOINT_UDP_RX;
        }
    }

    cfg->audio_tx_remote_ep_enabled = true;
    cfg->audio_tx_remote_addr = binding->tx_remote_endpoint.addr;
    cfg->audio_tx_remote_port = binding->tx_remote_endpoint.port;

    if (simple_media_runtime_type_is_plc(cfg->type))
    {
        cfg->plc_rx_match_ep_enabled = true;
        cfg->plc_rx_match_addr = binding->rx_match_addr;
        cfg->plc_rx_match_port = binding->rx_match_port;
        return 0;
    }

    if (cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_TX ||
        cfg->type == SIMPLE_MEDIA_ENDPOINT_UDP_RX)
    {
        cfg->udp_rx_match_ep_enabled = true;
        cfg->udp_rx_match_addr = binding->rx_match_addr;
        cfg->udp_rx_match_port = binding->rx_match_port;
        return 0;
    }

    return -EINVAL;
}

static int simple_media_runtime_apply_audio_tx_binding_to_config(
    simple_media_config_t *cfg,
    const simple_media_endpoint_binding_t *binding)
{
    if (!cfg || !binding ||
        !binding->has_tx_remote_endpoint || !binding->has_rx_match_endpoint ||
        binding->tx_remote_endpoint.port == 0 || binding->rx_match_port == 0)
    {
        return -EINVAL;
    }

    cfg->audio_tx_remote_ep_enabled = true;
    cfg->audio_tx_remote_addr = binding->tx_remote_endpoint.addr;
    cfg->audio_tx_remote_port = binding->tx_remote_endpoint.port;
    return 0;
}

static int simple_media_runtime_apply_media_bridge_descriptor_to_config(
    simple_media_config_t *cfg,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor)
{
    if (!cfg || !descriptor ||
        !simple_media_runtime_endpoint_type_is_bridge(cfg->type))
    {
        return -EINVAL;
    }

    switch (descriptor->mode)
    {
    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_BIDIRECTIONAL:
        if (descriptor->udp_remote_port == 0 ||
            descriptor->udp_rx_match_port == 0 ||
            descriptor->plc_remote_port == 0 ||
            descriptor->plc_rx_match_port == 0)
        {
            return -EINVAL;
        }
        cfg->udp_remote_addr = descriptor->udp_remote_addr;
        cfg->udp_remote_port = descriptor->udp_remote_port;
        cfg->udp_mcast_enabled = true;
        cfg->udp_mcast_group_addr = descriptor->udp_remote_addr;
        cfg->bridge_split_plc_to_udp_enabled = false;
        cfg->bridge_audio_only_enabled = false;
        cfg->bridge_video_plc_remote_addr = 0;
        cfg->bridge_video_plc_remote_port = 0;
        cfg->bridge_audio_udp_remote_addr = 0;
        cfg->bridge_audio_udp_remote_port = 0;
        cfg->udp_rx_match_ep_enabled = true;
        cfg->udp_rx_match_addr = descriptor->udp_rx_match_addr;
        cfg->udp_rx_match_port = descriptor->udp_rx_match_port;
        cfg->plc_remote_addr = descriptor->plc_remote_addr;
        cfg->plc_remote_port = descriptor->plc_remote_port;
        cfg->plc_mcast_enabled = true;
        cfg->plc_mcast_group_addr = descriptor->plc_remote_addr;
        cfg->plc_rx_match_ep_enabled = true;
        cfg->plc_rx_match_addr = descriptor->plc_rx_match_addr;
        cfg->plc_rx_match_port = descriptor->plc_rx_match_port;
        return 0;

    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_SPLIT:
        if (descriptor->udp_remote_port == 0 ||
            descriptor->audio_udp_remote_port == 0 ||
            descriptor->video_plc_remote_port == 0 ||
            descriptor->udp_rx_match_port == 0 ||
            descriptor->plc_remote_port == 0 ||
            descriptor->plc_rx_match_port == 0)
        {
            return -EINVAL;
        }
        cfg->udp_remote_addr = descriptor->udp_remote_addr;
        cfg->udp_remote_port = descriptor->udp_remote_port;
        cfg->udp_mcast_enabled = true;
        cfg->udp_mcast_group_addr = descriptor->udp_remote_addr;
        cfg->bridge_split_plc_to_udp_enabled = true;
        cfg->bridge_audio_only_enabled = false;
        cfg->bridge_video_plc_remote_addr = descriptor->video_plc_remote_addr;
        cfg->bridge_video_plc_remote_port = descriptor->video_plc_remote_port;
        cfg->bridge_audio_udp_remote_addr = descriptor->audio_udp_remote_addr;
        cfg->bridge_audio_udp_remote_port = descriptor->audio_udp_remote_port;
        cfg->udp_rx_match_ep_enabled = true;
        cfg->udp_rx_match_addr = descriptor->udp_rx_match_addr;
        cfg->udp_rx_match_port = descriptor->udp_rx_match_port;
        cfg->plc_remote_addr = descriptor->plc_remote_addr;
        cfg->plc_remote_port = descriptor->plc_remote_port;
        cfg->plc_mcast_enabled = true;
        cfg->plc_rx_match_ep_enabled = true;
        cfg->plc_rx_match_addr = descriptor->plc_rx_match_addr;
        cfg->plc_rx_match_port = descriptor->plc_rx_match_port;
        return 0;

    case SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_AUDIO_ONLY:
        if (descriptor->udp_remote_port == 0 ||
            descriptor->udp_rx_match_port == 0 ||
            descriptor->plc_remote_port == 0 ||
            descriptor->plc_rx_match_port == 0)
        {
            return -EINVAL;
        }
        cfg->udp_remote_addr = descriptor->udp_remote_addr;
        cfg->udp_remote_port = descriptor->udp_remote_port;
        cfg->udp_mcast_enabled = false;
        cfg->udp_mcast_group_addr = 0;
        cfg->bridge_split_plc_to_udp_enabled = false;
        cfg->bridge_audio_only_enabled = true;
        cfg->bridge_video_plc_remote_addr = 0;
        cfg->bridge_video_plc_remote_port = 0;
        cfg->bridge_audio_udp_remote_addr = 0;
        cfg->bridge_audio_udp_remote_port = 0;
        cfg->udp_rx_match_ep_enabled = true;
        cfg->udp_rx_match_addr = descriptor->udp_rx_match_addr;
        cfg->udp_rx_match_port = descriptor->udp_rx_match_port;
        cfg->plc_remote_addr = descriptor->plc_remote_addr;
        cfg->plc_remote_port = descriptor->plc_remote_port;
        cfg->plc_mcast_enabled = false;
        cfg->plc_mcast_group_addr = 0;
        cfg->plc_rx_match_ep_enabled = true;
        cfg->plc_rx_match_addr = descriptor->plc_rx_match_addr;
        cfg->plc_rx_match_port = descriptor->plc_rx_match_port;
        return 0;

    default:
        return -EINVAL;
    }
}

static int simple_media_runtime_build_video_config_locked(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg_out,
    bool *uses_pending)
{
    int ret;

    if (uses_pending)
    {
        *uses_pending = false;
    }
    ret = simple_media_runtime_copy_base_config_locked(runtime, cfg_out);
    if (ret < 0)
    {
        return ret;
    }
    if (runtime->has_pending_video_endpoint)
    {
        ret = simple_media_runtime_apply_video_binding_to_config(
            cfg_out, &runtime->pending_video_endpoint);
        if (ret == 0 && uses_pending)
        {
            *uses_pending = true;
        }
        return ret;
    }
    if (runtime->has_active_video_endpoint)
    {
        return simple_media_runtime_apply_video_binding_to_config(
            cfg_out, &runtime->active_video_endpoint);
    }
    return 0;
}

static int simple_media_runtime_build_audio_config_locked(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg_out,
    bool *uses_pending)
{
    int ret;
    bool preserve_main_channel = false;

    if (uses_pending)
    {
        *uses_pending = false;
    }
    ret = simple_media_runtime_copy_base_config_locked(runtime, cfg_out);
    if (ret < 0)
    {
        return ret;
    }
    if (runtime->cfg.media->channels_open &&
        runtime->cfg.media->channel_cfg_valid &&
        !simple_media_runtime_endpoint_type_is_bridge(
            runtime->cfg.media->channel_cfg.type))
    {
        *cfg_out = runtime->cfg.media->channel_cfg;
        preserve_main_channel = true;
    }
    else if (runtime->has_active_video_endpoint)
    {
        ret = simple_media_runtime_apply_video_binding_to_config(
            cfg_out, &runtime->active_video_endpoint);
        if (ret < 0)
        {
            return ret;
        }
        preserve_main_channel = true;
    }
    if (runtime->has_pending_audio_endpoint)
    {
        ret = preserve_main_channel ?
            simple_media_runtime_apply_audio_tx_binding_to_config(
                cfg_out, &runtime->pending_audio_endpoint) :
            simple_media_runtime_apply_audio_binding_to_config(
                cfg_out, &runtime->pending_audio_endpoint);
        if (ret == 0 && uses_pending)
        {
            *uses_pending = true;
        }
        return ret;
    }
    if (runtime->has_active_audio_endpoint)
    {
        return preserve_main_channel ?
            simple_media_runtime_apply_audio_tx_binding_to_config(
                cfg_out, &runtime->active_audio_endpoint) :
            simple_media_runtime_apply_audio_binding_to_config(
                cfg_out, &runtime->active_audio_endpoint);
    }
    return 0;
}

static int simple_media_runtime_build_bridge_config_locked(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg_out)
{
    if (!runtime || !runtime->has_pending_media_bridge)
    {
        return -EINVAL;
    }
    return simple_media_runtime_build_bridge_config_from_descriptor_locked(
        runtime, &runtime->pending_media_bridge, cfg_out);
}

static int simple_media_runtime_build_bridge_config_from_descriptor_locked(
    const simple_media_runtime_t *runtime,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor,
    simple_media_config_t *cfg_out)
{
    int ret;

    if (!runtime || !descriptor || !cfg_out)
    {
        return -EINVAL;
    }
    ret = simple_media_runtime_copy_base_config_locked(runtime, cfg_out);
    if (ret < 0)
    {
        return ret;
    }
    return simple_media_runtime_apply_media_bridge_descriptor_to_config(
        cfg_out, descriptor);
}

static int simple_media_runtime_build_current_config_locked(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg_out)
{
    const simple_media_runtime_bridge_record_t *record;
    int ret;

    ret = simple_media_runtime_copy_base_config_locked(runtime, cfg_out);
    if (ret < 0)
    {
        return ret;
    }
    record = simple_media_runtime_active_bridge_record_const_locked(runtime);
    if (record &&
        record->state.kind == SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA &&
        record->has_media_descriptor)
    {
        return simple_media_runtime_apply_media_bridge_descriptor_to_config(
            cfg_out, &record->media_descriptor);
    }
    if (runtime->has_active_video_endpoint)
    {
        ret = simple_media_runtime_apply_video_binding_to_config(
            cfg_out, &runtime->active_video_endpoint);
        if (ret < 0)
        {
            return ret;
        }
    }
    if (runtime->has_active_audio_endpoint)
    {
        ret = simple_media_runtime_apply_audio_binding_to_config(
            cfg_out, &runtime->active_audio_endpoint);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

static int simple_media_runtime_apply_media_bridge_descriptor(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor)
{
    simple_media_config_t cfg;
    int ret;

    if (!runtime || !runtime->cfg.media || !descriptor)
    {
        return -EINVAL;
    }
    if (runtime->bridge_releasing ||
        simple_media_runtime_bridge_table_has_active_locked(runtime))
    {
        return -EBUSY;
    }

    ret = simple_media_runtime_copy_base_config_locked(runtime, &cfg);
    if (ret == 0)
    {
        ret = simple_media_runtime_apply_media_bridge_descriptor_to_config(&cfg, descriptor);
    }
    if (ret == 0)
    {
        runtime->pending_media_bridge = *descriptor;
        runtime->has_pending_media_bridge = true;
    }
    else
    {
        simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    }
    return ret;
}

static bool simple_media_runtime_type_is_plc(simple_media_endpoint_type_t type)
{
    return type == SIMPLE_MEDIA_ENDPOINT_PLC_TX ||
           type == SIMPLE_MEDIA_ENDPOINT_PLC_RX;
}

static bool simple_media_runtime_audio_link_matches(
    const simple_media_runtime_t *runtime,
    const simple_media_endpoint_binding_t *binding)
{
    bool tx_is_plc;
    bool media_is_plc;

    if (!runtime || !binding || !binding->has_tx_remote_endpoint)
    {
        return true;
    }

    tx_is_plc = binding->tx_remote_endpoint.link_kind == CALL_MEDIA_ENDPOINT_LINK_PLC;
    media_is_plc = runtime->has_base_config ?
                   simple_media_runtime_type_is_plc(runtime->base_config.type) :
                   (runtime->cfg.media &&
                    simple_media_runtime_type_is_plc(runtime->cfg.media->base_cfg.type));
    return tx_is_plc == media_is_plc;
}

static int simple_media_runtime_ops_start_video(void *ctx)
{
    simple_media_runtime_t *runtime = (simple_media_runtime_t *)ctx;

    if (!runtime)
    {
        return -EINVAL;
    }
    if (runtime->cfg.start_video)
    {
        return runtime->cfg.start_video(runtime->cfg.delegate_ctx);
    }
    return simple_media_runtime_start_video(runtime);
}

static void simple_media_runtime_ops_stop_video(void *ctx)
{
    simple_media_runtime_t *runtime = (simple_media_runtime_t *)ctx;

    if (!runtime)
    {
        return;
    }
    if (runtime->cfg.stop_video)
    {
        runtime->cfg.stop_video(runtime->cfg.delegate_ctx);
        return;
    }
    simple_media_runtime_stop_video(runtime);
}

static int simple_media_runtime_ops_apply_video_endpoint(
    void *ctx,
    const simple_media_endpoint_binding_t *binding)
{
    return simple_media_runtime_apply_video_endpoint((simple_media_runtime_t *)ctx, binding);
}

static int simple_media_runtime_ops_apply_audio_endpoint(
    void *ctx,
    const simple_media_endpoint_binding_t *binding)
{
    return simple_media_runtime_apply_audio_endpoint((simple_media_runtime_t *)ctx, binding);
}

static int simple_media_runtime_ops_start_audio(void *ctx)
{
    simple_media_runtime_t *runtime = (simple_media_runtime_t *)ctx;

    if (!runtime)
    {
        return -EINVAL;
    }
    if (runtime->cfg.start_audio)
    {
        return runtime->cfg.start_audio(runtime->cfg.delegate_ctx);
    }
    return simple_media_runtime_start_audio(runtime);
}

static void simple_media_runtime_ops_stop_audio(void *ctx)
{
    simple_media_runtime_t *runtime = (simple_media_runtime_t *)ctx;

    if (!runtime)
    {
        return;
    }
    if (runtime->cfg.stop_audio)
    {
        runtime->cfg.stop_audio(runtime->cfg.delegate_ctx);
        return;
    }
    simple_media_runtime_stop_audio(runtime);
}

void simple_media_runtime_config_init(simple_media_runtime_config_t *cfg)
{
    if (cfg)
    {
        memset(cfg, 0, sizeof(*cfg));
        cfg->video_feedback_pli_min_interval_ms =
            SIMPLE_MEDIA_RUNTIME_DEFAULT_PLI_MIN_INTERVAL_MS;
        cfg->video_feedback_fir_min_interval_ms =
            SIMPLE_MEDIA_RUNTIME_DEFAULT_FIR_MIN_INTERVAL_MS;
    }
}

void simple_media_runtime_direct_bridge_config_init(
    simple_media_runtime_direct_bridge_config_t *cfg)
{
    if (cfg)
    {
        memset(cfg, 0, sizeof(*cfg));
    }
}

void simple_media_runtime_debug_probe_config_init(
    simple_media_runtime_debug_probe_config_t *cfg)
{
    if (cfg)
    {
        memset(cfg, 0, sizeof(*cfg));
    }
}

int simple_media_runtime_init(simple_media_runtime_t *runtime,
                              const simple_media_runtime_config_t *cfg)
{
    if (!runtime || !cfg || !cfg->media)
    {
        return -EINVAL;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->cfg = *cfg;
    runtime->owns_media = false;
    if (cfg->media)
    {
        runtime->base_config = cfg->media->base_cfg;
        runtime->has_base_config = true;
    }
    return 0;
}

int simple_media_runtime_open(simple_media_runtime_t *runtime,
                              const simple_media_runtime_config_t *cfg,
                              const simple_media_config_t *media_cfg)
{
    simple_media_runtime_config_t owned_cfg;
    int ret;

    if (!runtime || !cfg || !media_cfg)
    {
        return -EINVAL;
    }

    memset(runtime, 0, sizeof(*runtime));
    ret = simple_media_open(&runtime->owned_media, media_cfg);
    if (ret < 0)
    {
        memset(runtime, 0, sizeof(*runtime));
        return ret;
    }

    owned_cfg = *cfg;
    owned_cfg.media = &runtime->owned_media;
    runtime->cfg = owned_cfg;
    runtime->owns_media = true;
    runtime->base_config = *media_cfg;
    runtime->has_base_config = true;
    return 0;
}

void simple_media_runtime_close(simple_media_runtime_t *runtime)
{
    if (!runtime)
    {
        return;
    }

    simple_media_runtime_stop_debug_probe(runtime);
    while (simple_media_runtime_get_bridge_state(runtime, NULL))
    {
        (void)simple_media_runtime_release_bridge(runtime, 0, NULL);
    }
    if (runtime->owns_media)
    {
        simple_media_close(&runtime->owned_media);
    }
    simple_media_runtime_lock(runtime);
    simple_media_runtime_clear_bridge_table_locked(runtime);
    simple_media_runtime_unlock(runtime);
    memset(runtime, 0, sizeof(*runtime));
}

void simple_media_runtime_deinit(simple_media_runtime_t *runtime)
{
    simple_media_runtime_close(runtime);
}

int simple_media_runtime_fill_ops(simple_media_runtime_t *runtime, simple_media_ops_t *ops)
{
    if (!runtime || !runtime->cfg.media || !ops)
    {
        return -EINVAL;
    }

    memset(ops, 0, sizeof(*ops));
    ops->start_video = simple_media_runtime_ops_start_video;
    ops->stop_video = simple_media_runtime_ops_stop_video;
    ops->apply_video_endpoint = simple_media_runtime_ops_apply_video_endpoint;
    ops->apply_audio_endpoint = simple_media_runtime_ops_apply_audio_endpoint;
    ops->start_audio = simple_media_runtime_ops_start_audio;
    ops->stop_audio = simple_media_runtime_ops_stop_audio;
    ops->user_ctx = runtime;
    return 0;
}

#ifdef SIMPLE_MEDIA_RUNTIME_TESTING
/*
 * Test-only helpers for tests/simple_media_runtime_test.c.
 *
 * These functions are intentionally static and only compiled when the runtime
 * .c file is included by the unit test with SIMPLE_MEDIA_RUNTIME_TESTING set.
 * They are not production API and must not be used by examples/device code.
 */
static bool simple_media_runtime_test_has_media(const simple_media_runtime_t *runtime)
{
    return runtime && runtime->cfg.media;
}

static bool simple_media_runtime_test_copy_current_config(
    const simple_media_runtime_t *runtime,
    simple_media_config_t *cfg)
{
    if (cfg)
    {
        memset(cfg, 0, sizeof(*cfg));
    }
    if (!runtime || !runtime->cfg.media || !cfg)
    {
        return false;
    }
    *cfg = runtime->cfg.media->base_cfg;
    return true;
}

static void simple_media_runtime_test_set_streams(
    simple_media_runtime_t *runtime,
    mtrans_stream_tx_t video_tx,
    mtrans_stream_tx_t audio_tx,
    mtrans_stream_rx_t video_rx)
{
    if (!runtime || !runtime->cfg.media)
    {
        return;
    }
    runtime->cfg.media->video_tx = video_tx;
    runtime->cfg.media->audio_tx = audio_tx;
    runtime->cfg.media->video_rx = video_rx;
}

static void simple_media_runtime_test_set_video_rx(
    simple_media_runtime_t *runtime,
    mtrans_stream_rx_t video_rx)
{
    if (!runtime || !runtime->cfg.media)
    {
        return;
    }
    runtime->cfg.media->video_rx = video_rx;
}

static void simple_media_runtime_test_set_bridge_channels(
    simple_media_runtime_t *runtime,
    mtrans_channel_t udp_ch,
    mtrans_channel_t plc_ch)
{
    if (!runtime || !runtime->cfg.media)
    {
        return;
    }
    runtime->cfg.media->udp_ch = udp_ch;
    runtime->cfg.media->plc_ch = plc_ch;
}
#endif

const simple_media_config_t *simple_media_runtime_base_config(
    const simple_media_runtime_t *runtime)
{
    if (!runtime || !runtime->has_base_config)
    {
        return NULL;
    }
    return &runtime->base_config;
}

bool simple_media_runtime_get_base_endpoint_type(
    const simple_media_runtime_t *runtime,
    simple_media_endpoint_type_t *type)
{
    if (!runtime || !runtime->has_base_config)
    {
        return false;
    }
    if (type)
    {
        *type = runtime->base_config.type;
    }
    return true;
}

bool simple_media_runtime_base_bridge_capable(const simple_media_runtime_t *runtime)
{
    if (!runtime || !runtime->has_base_config)
    {
        return false;
    }
    return simple_media_runtime_endpoint_type_is_bridge(runtime->base_config.type);
}

bool simple_media_runtime_base_bridge_bidirectional(const simple_media_runtime_t *runtime)
{
    if (!simple_media_runtime_base_bridge_capable(runtime))
    {
        return false;
    }
    return runtime->base_config.bidirectional_bridge;
}

const char *simple_media_runtime_base_udp_interface(const simple_media_runtime_t *runtime)
{
    const char *ifname;

    if (!runtime || !runtime->has_base_config)
    {
        return NULL;
    }
    simple_media_runtime_lock_const(runtime);
    ifname = runtime->base_config.udp_interface;
    simple_media_runtime_unlock_const(runtime);
    return ifname;
}

const char *simple_media_runtime_current_udp_interface(const simple_media_runtime_t *runtime)
{
    const char *ifname;

    if (!runtime)
    {
        return NULL;
    }
    simple_media_runtime_lock_const(runtime);
    ifname = runtime->has_base_config ? runtime->base_config.udp_interface :
             (runtime->cfg.media ? runtime->cfg.media->base_cfg.udp_interface : NULL);
    simple_media_runtime_unlock_const(runtime);
    return ifname;
}

int simple_media_runtime_select_udp_interface_idle(simple_media_runtime_t *runtime,
                                                   const char *ifname)
{
    if (!runtime || !ifname)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (!runtime->cfg.media)
    {
        simple_media_runtime_unlock(runtime);
        return -EINVAL;
    }
    if (simple_media_runtime_has_streams_locked(runtime))
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    runtime->cfg.media->base_cfg.udp_interface = ifname;
    if (runtime->has_base_config)
    {
        runtime->base_config.udp_interface = ifname;
    }
    simple_media_runtime_unlock(runtime);
    return 0;
}

bool simple_media_runtime_get_endpoint_snapshot(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_endpoint_snapshot_source_t source,
    simple_media_runtime_endpoint_snapshot_t *snapshot)
{
    simple_media_config_t cfg;
    int ret = -EINVAL;

    if (snapshot)
    {
        memset(snapshot, 0, sizeof(*snapshot));
    }
    if (!runtime || !snapshot)
    {
        return false;
    }
    if (source != SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_CURRENT &&
        source != SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_BASE)
    {
        return false;
    }

    simple_media_runtime_lock_const(runtime);
    if (source == SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_CURRENT)
    {
        ret = simple_media_runtime_build_current_config_locked(runtime, &cfg);
    }
    else
    {
        ret = simple_media_runtime_copy_base_config_locked(runtime, &cfg);
    }
    if (ret == 0)
    {
        simple_media_runtime_fill_endpoint_snapshot(&cfg, snapshot);
    }
    simple_media_runtime_unlock_const(runtime);
    return ret == 0;
}

bool simple_media_runtime_video_tx_ready(const simple_media_runtime_t *runtime)
{
    bool ready;

    simple_media_runtime_lock_const(runtime);
    ready = runtime && runtime->cfg.media && runtime->cfg.media->video_tx;
    simple_media_runtime_unlock_const(runtime);
    return ready;
}

bool simple_media_runtime_audio_tx_ready(const simple_media_runtime_t *runtime)
{
    bool ready;

    simple_media_runtime_lock_const(runtime);
    ready = runtime && runtime->cfg.media && runtime->cfg.media->audio_tx;
    simple_media_runtime_unlock_const(runtime);
    return ready;
}

int simple_media_runtime_input_video(simple_media_runtime_t *runtime,
                                     const uint8_t *payload,
                                     size_t payload_len,
                                     uint32_t rtp_ts)
{
    int ret;

    if (!runtime || !payload || payload_len == 0)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (!runtime->cfg.media || !runtime->cfg.media->video_tx)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOENT;
    }
    ret = mtrans_stream_tx_input_video(runtime->cfg.media->video_tx,
                                       payload,
                                       payload_len,
                                       rtp_ts);
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_input_audio(simple_media_runtime_t *runtime,
                                     const uint8_t *payload,
                                     size_t payload_len,
                                     uint32_t rtp_ts)
{
    int ret;

    if (!runtime || !payload || payload_len == 0)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (!runtime->cfg.media || !runtime->cfg.media->audio_tx)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOENT;
    }
    ret = mtrans_stream_tx_input_audio(runtime->cfg.media->audio_tx,
                                       payload,
                                       payload_len,
                                       rtp_ts);
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_request_video_feedback(
    simple_media_runtime_t *runtime,
    simple_media_runtime_video_feedback_t type,
    const char *reason)
{
    uint64_t now_ms;
    uint64_t min_interval_ms;
    uint64_t *last_ms;
    int ret;

    (void)reason;

    if (!runtime)
    {
        return -EINVAL;
    }

    if (type != SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_PLI &&
        type != SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_FIR)
    {
        return -EINVAL;
    }

    now_ms = simple_media_runtime_monotonic_ms();
    simple_media_runtime_lock(runtime);
    if (!runtime->cfg.media || !runtime->cfg.media->video_rx)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOENT;
    }

    if (type == SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_PLI)
    {
        min_interval_ms = runtime->cfg.video_feedback_pli_min_interval_ms;
        last_ms = &runtime->video_feedback_last_pli_ms;
    }
    else
    {
        min_interval_ms = runtime->cfg.video_feedback_fir_min_interval_ms;
        last_ms = &runtime->video_feedback_last_fir_ms;
    }

    if (min_interval_ms != 0 && *last_ms != 0 && now_ms >= *last_ms &&
        now_ms - *last_ms < min_interval_ms)
    {
        simple_media_runtime_unlock(runtime);
        return -EAGAIN;
    }

    ret = type == SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_PLI ?
          mtrans_stream_rx_send_pli(runtime->cfg.media->video_rx) :
          mtrans_stream_rx_send_fir(runtime->cfg.media->video_rx);
    if (ret == 0)
    {
        *last_ms = now_ms;
    }
    else if (ret == -EAGAIN)
    {
        ret = -EIO;
    }
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_get_bridge_route_stats(
    simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_route_direction_t direction,
    mtrans_route_rule_stats_t *stats)
{
    const simple_media_runtime_bridge_record_t *record;
    mtrans_channel_t src;
    mtrans_channel_t dst;
    int ret;

    if (!runtime || !stats)
    {
        return -EINVAL;
    }
    if (direction != SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_UDP_TO_PLC &&
        direction != SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_PLC_TO_UDP)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    record = simple_media_runtime_active_bridge_record_const_locked(runtime);
    if (!record ||
        record->state.kind != SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA ||
        !record->owns_media_resources ||
        !record->media_bridge_udp_ch ||
        !record->media_bridge_plc_ch)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOENT;
    }
    if (direction == SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_UDP_TO_PLC)
    {
        src = record->media_bridge_udp_ch;
        dst = record->media_bridge_plc_ch;
    }
    else if (direction == SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_PLC_TO_UDP)
    {
        src = record->media_bridge_plc_ch;
        dst = record->media_bridge_udp_ch;
    }
    else
    {
        simple_media_runtime_unlock(runtime);
        return -EINVAL;
    }
    memset(stats, 0, sizeof(*stats));
    ret = mtrans_route_get_rule_stats(src, dst, stats);
    simple_media_runtime_unlock(runtime);
    return ret;
}

static void simple_media_runtime_dump_route_stats_locked(
    const char *name,
    mtrans_channel_t src,
    mtrans_channel_t dst)
{
    mtrans_route_rule_stats_t stats;
    int ret;

    if (!name || !src || !dst)
    {
        SIMPLE_LOGI("bridge_table route_stats name=%s stats=unavailable\n",
                    name ? name : "-");
        return;
    }
    memset(&stats, 0, sizeof(stats));
    ret = mtrans_route_get_rule_stats(src, dst, &stats);
    if (ret < 0)
    {
        SIMPLE_LOGI("bridge_table route_stats name=%s stats=unavailable ret=%d\n",
                    name,
                    ret);
        return;
    }
    SIMPLE_LOGI("bridge_table route_stats name=%s flags=0x%x forward=%llu/%llub "
                "drop_policy=%llu/%llub drop_send=%llu/%llub\n",
                name,
                stats.flags,
                (unsigned long long)stats.forwarded_pkts,
                (unsigned long long)stats.forwarded_bytes,
                (unsigned long long)stats.dropped_policy_pkts,
                (unsigned long long)stats.dropped_policy_bytes,
                (unsigned long long)stats.dropped_send_fail_pkts,
                (unsigned long long)stats.dropped_send_fail_bytes);
}

static void simple_media_runtime_dump_media_descriptor_locked(
    size_t ordinal,
    const simple_media_runtime_media_bridge_descriptor_t *desc)
{
    if (!desc)
    {
        SIMPLE_LOGI("bridge_table[%zu] media_descriptor=unavailable\n", ordinal);
        return;
    }
    SIMPLE_LOGI("bridge_table[%zu] media mode=%s udp=0x%08x:%u udp_match=0x%08x:%u "
                "plc=0x%08x:%u plc_match=0x%08x:%u video_plc=0x%08x:%u "
                "audio_udp=0x%08x:%u\n",
                ordinal,
                simple_media_runtime_media_bridge_mode_name(desc->mode),
                desc->udp_remote_addr,
                desc->udp_remote_port,
                desc->udp_rx_match_addr,
                desc->udp_rx_match_port,
                desc->plc_remote_addr,
                desc->plc_remote_port,
                desc->plc_rx_match_addr,
                desc->plc_rx_match_port,
                desc->video_plc_remote_addr,
                desc->video_plc_remote_port,
                desc->audio_udp_remote_addr,
                desc->audio_udp_remote_port);
}

static void simple_media_runtime_dump_bridge_record_locked(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_bridge_record_t *record,
    size_t ordinal)
{
    const simple_media_runtime_bridge_state_t *state;

    if (!record)
    {
        return;
    }
    (void)runtime;
    state = &record->state;
    SIMPLE_LOGI("bridge_table[%zu] kind=%s active=%d session=0x%llx video=%d audio=%d "
                "caller=%s callee=%s media_desc=%d media_owned=%d "
                "media_udp=%d media_plc=%d media_video_plc=%d media_audio_udp=%d "
                "direct_owned=%d "
                "direct_ingress=%d direct_egress=%d route_group=%u "
                "forward_added=%d reverse_added=%d forward_flags=0x%x reverse_flags=0x%x\n",
                ordinal,
                simple_media_runtime_bridge_kind_name(state->kind),
                state->active ? 1 : 0,
                (unsigned long long)state->session_id,
                state->video_active ? 1 : 0,
                state->audio_active ? 1 : 0,
                state->caller_device_id,
                state->callee_device_id,
                record->has_media_descriptor ? 1 : 0,
                record->owns_media_resources ? 1 : 0,
                record->media_bridge_udp_ch ? 1 : 0,
                record->media_bridge_plc_ch ? 1 : 0,
                record->media_bridge_video_plc_ch ? 1 : 0,
                record->media_bridge_audio_udp_ch ? 1 : 0,
                record->owns_direct_resources ? 1 : 0,
                record->direct_bridge_ingress_ch ? 1 : 0,
                record->direct_bridge_egress_ch ? 1 : 0,
                record->direct_bridge_route_group,
                record->direct_bridge_forward_added ? 1 : 0,
                record->direct_bridge_reverse_added ? 1 : 0,
                record->direct_bridge_forward_flags,
                record->direct_bridge_reverse_flags);
    if (state->kind == SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA)
    {
        if (record->has_media_descriptor)
        {
            simple_media_runtime_dump_media_descriptor_locked(
                ordinal,
                &record->media_descriptor);
        }
        if (record->owns_media_resources &&
            record->media_bridge_udp_ch && record->media_bridge_plc_ch)
        {
            if (record->media_bridge_udp_to_video_plc_added)
            {
                simple_media_runtime_dump_route_stats_locked(
                    "media_udp_to_plc_video",
                    record->media_bridge_udp_ch,
                    record->media_bridge_video_plc_ch);
            }
            if (record->media_bridge_udp_to_plc_added)
            {
                simple_media_runtime_dump_route_stats_locked(
                    "media_udp_to_plc",
                    record->media_bridge_udp_ch,
                    record->media_bridge_plc_ch);
            }
            if (record->media_bridge_plc_to_udp_added)
            {
                simple_media_runtime_dump_route_stats_locked(
                    "media_plc_to_udp",
                    record->media_bridge_plc_ch,
                    record->media_bridge_udp_ch);
            }
            if (record->media_bridge_plc_to_audio_udp_added)
            {
                simple_media_runtime_dump_route_stats_locked(
                    "media_plc_to_udp_audio",
                    record->media_bridge_plc_ch,
                    record->media_bridge_audio_udp_ch);
            }
        }
        else
        {
            SIMPLE_LOGI("bridge_table[%zu] media route_stats=unavailable\n", ordinal);
        }
    }
    else if (state->kind == SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_DIRECT)
    {
        simple_media_runtime_dump_route_stats_locked(
            "direct_forward",
            record->direct_bridge_ingress_ch,
            record->direct_bridge_egress_ch);
        if (record->direct_bridge_reverse_added)
        {
            simple_media_runtime_dump_route_stats_locked(
                "direct_reverse",
                record->direct_bridge_egress_ch,
                record->direct_bridge_ingress_ch);
        }
    }
}

static void simple_media_runtime_debug_probe_destroy_streams(
    simple_media_runtime_debug_probe_state_t *probe)
{
    if (!probe)
    {
        return;
    }
    if (probe->udp_video_probe)
    {
        mtrans_stream_rx_destroy(probe->udp_video_probe);
        probe->udp_video_probe = NULL;
    }
    if (probe->udp_audio_probe)
    {
        mtrans_stream_rx_destroy(probe->udp_audio_probe);
        probe->udp_audio_probe = NULL;
    }
    if (probe->plc_video_probe)
    {
        mtrans_stream_rx_destroy(probe->plc_video_probe);
        probe->plc_video_probe = NULL;
    }
    if (probe->plc_audio_probe)
    {
        mtrans_stream_rx_destroy(probe->plc_audio_probe);
        probe->plc_audio_probe = NULL;
    }
}

static simple_media_runtime_debug_probe_rtcp_type_t
simple_media_runtime_debug_probe_map_rtcp_type(mtrans_rtcp_event_type_t type)
{
    switch (type)
    {
    case MTRANS_RTCP_EVENT_NACK:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_NACK;
    case MTRANS_RTCP_EVENT_PLI:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_PLI;
    case MTRANS_RTCP_EVENT_FIR:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_FIR;
    case MTRANS_RTCP_EVENT_RR:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_RR;
    case MTRANS_RTCP_EVENT_SR:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_SR;
    case MTRANS_RTCP_EVENT_AFB:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_AFB;
    case MTRANS_RTCP_EVENT_UNKNOWN:
    default:
        return SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_UNKNOWN;
    }
}

static void simple_media_runtime_debug_probe_emit_frame(
    simple_media_runtime_debug_probe_stream_ctx_t *stream_ctx,
    size_t len,
    uint32_t ts,
    int flags)
{
    simple_media_runtime_debug_probe_event_t event;
    simple_media_runtime_debug_probe_state_t *probe;

    if (!stream_ctx || !stream_ctx->probe || !stream_ctx->probe->observer)
    {
        return;
    }

    probe = stream_ctx->probe;
    memset(&event, 0, sizeof(event));
    event.event = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_EVENT_FRAME;
    event.link = stream_ctx->link;
    event.media = stream_ctx->media;
    event.len = len;
    event.ts = ts;
    event.flags = flags;
    event.reason = "frame";
    probe->observer(probe->observer_ctx, &event);
}

static void simple_media_runtime_debug_probe_emit_rtcp(
    simple_media_runtime_debug_probe_stream_ctx_t *stream_ctx,
    const mtrans_rtcp_event_t *rtcp)
{
    simple_media_runtime_debug_probe_event_t event;
    simple_media_runtime_debug_probe_state_t *probe;

    if (!stream_ctx || !stream_ctx->probe || !stream_ctx->probe->observer || !rtcp)
    {
        return;
    }

    probe = stream_ctx->probe;
    memset(&event, 0, sizeof(event));
    event.event = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_EVENT_RTCP;
    event.link = stream_ctx->link;
    event.media = stream_ctx->media;
    event.rtcp_type = simple_media_runtime_debug_probe_map_rtcp_type(rtcp->type);
    event.rtcp_packet_type = rtcp->rtcp_type;
    event.rtcp_fmt = rtcp->fmt;
    event.sender_ssrc = rtcp->sender_ssrc;
    event.media_ssrc = rtcp->media_ssrc;
    event.nack_count = rtcp->nack_count;
    event.rr_fraction_lost = rtcp->rr_fraction_lost;
    event.rr_jitter = rtcp->rr_jitter;
    event.remb_bps = rtcp->remb_bps;
    event.reason = "rtcp";
    probe->observer(probe->observer_ctx, &event);
}

static void simple_media_runtime_debug_probe_on_video_frame(
    void *ctx,
    const uint8_t *frame_data,
    size_t len,
    uint32_t ts,
    int flags)
{
    (void)frame_data;
    simple_media_runtime_debug_probe_emit_frame(
        (simple_media_runtime_debug_probe_stream_ctx_t *)ctx,
        len, ts, flags);
}

static void simple_media_runtime_debug_probe_on_audio_frame(
    void *ctx,
    const uint8_t *frame_data,
    size_t len,
    uint32_t ts,
    int flags)
{
    (void)frame_data;
    simple_media_runtime_debug_probe_emit_frame(
        (simple_media_runtime_debug_probe_stream_ctx_t *)ctx,
        len, ts, flags);
}

static void simple_media_runtime_debug_probe_on_rtcp_event(
    void *ctx,
    const mtrans_rtcp_event_t *event)
{
    simple_media_runtime_debug_probe_emit_rtcp(
        (simple_media_runtime_debug_probe_stream_ctx_t *)ctx,
        event);
}

static void simple_media_runtime_debug_probe_free(
    simple_media_runtime_debug_probe_state_t *probe)
{
    simple_media_runtime_debug_probe_destroy_streams(probe);
    SIMPLE_FREE(probe);
}

static void simple_media_runtime_debug_probe_wait_start(simple_media_runtime_t *runtime)
{
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = 1000000L;
    for (;;)
    {
        bool starting;

        simple_media_runtime_lock(runtime);
        starting = runtime && runtime->debug_probe_starting;
        simple_media_runtime_unlock(runtime);
        if (!starting)
        {
            return;
        }
        (void)nanosleep(&delay, NULL);
    }
}

bool simple_media_runtime_debug_probe_active(const simple_media_runtime_t *runtime)
{
    bool active;

    simple_media_runtime_lock_const(runtime);
    active = runtime && runtime->debug_probe;
    simple_media_runtime_unlock_const(runtime);
    return active;
}

void simple_media_runtime_stop_debug_probe(simple_media_runtime_t *runtime)
{
    simple_media_runtime_debug_probe_state_t *probe;

    if (!runtime)
    {
        return;
    }

    simple_media_runtime_debug_probe_wait_start(runtime);
    simple_media_runtime_lock(runtime);
    probe = runtime->debug_probe;
    runtime->debug_probe = NULL;
    simple_media_runtime_unlock(runtime);

    simple_media_runtime_debug_probe_free(probe);
}

int simple_media_runtime_start_debug_probe(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_debug_probe_config_t *cfg)
{
    simple_media_runtime_debug_probe_state_t *probe;
    mtrans_media_rx_params_t params;
    mtrans_stream_rx_callbacks_t udp_video_cbs;
    mtrans_stream_rx_callbacks_t udp_audio_cbs;
    mtrans_stream_rx_callbacks_t plc_video_cbs;
    mtrans_stream_rx_callbacks_t plc_audio_cbs;
    const simple_media_runtime_bridge_record_t *record;
    mtrans_channel_t udp_ch;
    mtrans_channel_t plc_ch;

    if (!runtime || !cfg)
    {
        return -EINVAL;
    }

    probe = (simple_media_runtime_debug_probe_state_t *)SIMPLE_CALLOC(1, sizeof(*probe));
    if (!probe)
    {
        return -ENOMEM;
    }
    probe->observer = cfg->observer;
    probe->observer_ctx = cfg->observer_ctx;
    probe->udp_video_ctx.probe = probe;
    probe->udp_video_ctx.link = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_UDP;
    probe->udp_video_ctx.media = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_VIDEO;
    probe->udp_audio_ctx.probe = probe;
    probe->udp_audio_ctx.link = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_UDP;
    probe->udp_audio_ctx.media = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_AUDIO;
    probe->plc_video_ctx.probe = probe;
    probe->plc_video_ctx.link = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_PLC;
    probe->plc_video_ctx.media = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_VIDEO;
    probe->plc_audio_ctx.probe = probe;
    probe->plc_audio_ctx.link = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_PLC;
    probe->plc_audio_ctx.media = SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_AUDIO;

    memset(&params, 0, sizeof(params));
    params.jitter_buffer_ms = cfg->jitter_buffer_ms;
    params.jitter_wallclock_timeout_ms = cfg->jitter_wallclock_timeout_ms;
    params.jitter_max_queue_packets = cfg->jitter_max_queue_packets;
    memset(&udp_video_cbs, 0, sizeof(udp_video_cbs));
    memset(&udp_audio_cbs, 0, sizeof(udp_audio_cbs));
    memset(&plc_video_cbs, 0, sizeof(plc_video_cbs));
    memset(&plc_audio_cbs, 0, sizeof(plc_audio_cbs));
    udp_video_cbs.on_video_frame_ready = simple_media_runtime_debug_probe_on_video_frame;
    udp_video_cbs.on_rtcp_event = simple_media_runtime_debug_probe_on_rtcp_event;
    udp_audio_cbs.on_audio_frame_ready = simple_media_runtime_debug_probe_on_audio_frame;
    udp_audio_cbs.on_rtcp_event = simple_media_runtime_debug_probe_on_rtcp_event;
    plc_video_cbs.on_video_frame_ready = simple_media_runtime_debug_probe_on_video_frame;
    plc_video_cbs.on_rtcp_event = simple_media_runtime_debug_probe_on_rtcp_event;
    plc_audio_cbs.on_audio_frame_ready = simple_media_runtime_debug_probe_on_audio_frame;
    plc_audio_cbs.on_rtcp_event = simple_media_runtime_debug_probe_on_rtcp_event;

    simple_media_runtime_lock(runtime);
    if (runtime->debug_probe || runtime->debug_probe_starting)
    {
        simple_media_runtime_unlock(runtime);
        simple_media_runtime_debug_probe_free(probe);
        return -EBUSY;
    }
    record = simple_media_runtime_active_bridge_record_const_locked(runtime);
    if (record &&
        record->state.kind == SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA &&
        record->owns_media_resources)
    {
        udp_ch = record->media_bridge_udp_ch;
        plc_ch = record->media_bridge_plc_ch;
    }
    else if (runtime->cfg.media)
    {
        udp_ch = runtime->cfg.media->udp_ch;
        plc_ch = runtime->cfg.media->plc_ch;
    }
    else
    {
        udp_ch = NULL;
        plc_ch = NULL;
    }
    if (!udp_ch || !plc_ch)
    {
        simple_media_runtime_unlock(runtime);
        simple_media_runtime_debug_probe_free(probe);
        return -ENOENT;
    }
    runtime->debug_probe_starting = true;
    simple_media_runtime_unlock(runtime);

    probe->udp_video_probe = mtrans_stream_rx_create(
        udp_ch, MTRANS_MEDIA_TYPE_VIDEO_H264, &params,
        &udp_video_cbs, &probe->udp_video_ctx);
    if (!probe->udp_video_probe)
    {
        goto fail;
    }
    probe->udp_audio_probe = mtrans_stream_rx_create(
        udp_ch, MTRANS_MEDIA_TYPE_AUDIO_OPUS, &params,
        &udp_audio_cbs, &probe->udp_audio_ctx);
    if (!probe->udp_audio_probe)
    {
        goto fail;
    }
    probe->plc_video_probe = mtrans_stream_rx_create(
        plc_ch, MTRANS_MEDIA_TYPE_VIDEO_H264, &params,
        &plc_video_cbs, &probe->plc_video_ctx);
    if (!probe->plc_video_probe)
    {
        goto fail;
    }
    probe->plc_audio_probe = mtrans_stream_rx_create(
        plc_ch, MTRANS_MEDIA_TYPE_AUDIO_OPUS, &params,
        &plc_audio_cbs, &probe->plc_audio_ctx);
    if (!probe->plc_audio_probe)
    {
        goto fail;
    }

    simple_media_runtime_lock(runtime);
    runtime->debug_probe_starting = false;
    runtime->debug_probe = probe;
    simple_media_runtime_unlock(runtime);
    return 0;

fail:
    simple_media_runtime_debug_probe_free(probe);
    simple_media_runtime_lock(runtime);
    runtime->debug_probe_starting = false;
    simple_media_runtime_unlock(runtime);
    return -EIO;
}

int simple_media_runtime_start_video(simple_media_runtime_t *runtime)
{
    simple_media_config_t cfg;
    bool uses_pending = false;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (simple_media_runtime_bridge_table_has_kind_active_locked(
            runtime, SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA))
    {
        ret = -EBUSY;
    }
    else
    {
        ret = simple_media_runtime_build_video_config_locked(runtime, &cfg, &uses_pending);
    }
    if (ret == 0)
    {
        ret = simple_media_start_video_with_config(runtime->cfg.media, &cfg);
    }
    if (ret == 0 && uses_pending)
    {
        runtime->active_video_endpoint = runtime->pending_video_endpoint;
        runtime->has_active_video_endpoint = true;
        simple_media_runtime_clear_pending_endpoint_locked(runtime, false);
    }
    simple_media_runtime_unlock(runtime);
    return ret;
}

void simple_media_runtime_stop_video(simple_media_runtime_t *runtime)
{
    if (!runtime || !runtime->cfg.media)
    {
        return;
    }
    simple_media_runtime_lock(runtime);
    simple_media_stop_video(runtime->cfg.media);
    simple_media_runtime_clear_pending_endpoint_locked(runtime, false);
    simple_media_runtime_clear_active_endpoint_locked(runtime, false);
    simple_media_runtime_clear_video_feedback_locked(runtime);
    simple_media_runtime_unlock(runtime);
}

int simple_media_runtime_start_audio(simple_media_runtime_t *runtime)
{
    simple_media_config_t cfg;
    bool uses_pending = false;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }
    simple_media_runtime_lock(runtime);
    if (simple_media_runtime_bridge_table_has_kind_active_locked(
            runtime, SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA))
    {
        ret = -EBUSY;
    }
    else
    {
        ret = simple_media_runtime_build_audio_config_locked(runtime, &cfg, &uses_pending);
    }
    if (ret == 0)
    {
        ret = simple_media_start_audio_with_config(runtime->cfg.media, &cfg);
    }
    if (ret == 0 && uses_pending)
    {
        runtime->active_audio_endpoint = runtime->pending_audio_endpoint;
        runtime->has_active_audio_endpoint = true;
        simple_media_runtime_clear_pending_endpoint_locked(runtime, true);
    }
    simple_media_runtime_unlock(runtime);
    return ret;
}

void simple_media_runtime_stop_audio(simple_media_runtime_t *runtime)
{
    if (!runtime || !runtime->cfg.media)
    {
        return;
    }
    simple_media_runtime_lock(runtime);
    simple_media_stop_audio(runtime->cfg.media);
    simple_media_runtime_clear_pending_endpoint_locked(runtime, true);
    simple_media_runtime_clear_active_endpoint_locked(runtime, true);
    simple_media_runtime_unlock(runtime);
}

int simple_media_runtime_apply_video_endpoint(
    simple_media_runtime_t *runtime,
    const simple_media_endpoint_binding_t *binding)
{
    simple_media_config_t cfg;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }
    if (!binding || !binding->has_tx_remote_endpoint || !binding->has_rx_match_endpoint)
    {
        return 0;
    }

    simple_media_runtime_lock(runtime);
    if (runtime->cfg.media->video_tx || runtime->cfg.media->video_rx)
    {
        ret = -EBUSY;
    }
    else
    {
        ret = simple_media_runtime_copy_base_config_locked(runtime, &cfg);
        if (ret == 0)
        {
            ret = simple_media_runtime_apply_video_binding_to_config(&cfg, binding);
        }
        if (ret == 0)
        {
            runtime->pending_video_endpoint = *binding;
            runtime->has_pending_video_endpoint = true;
        }
        else
        {
            simple_media_runtime_clear_pending_endpoint_locked(runtime, false);
        }
    }
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_apply_audio_endpoint(
    simple_media_runtime_t *runtime,
    const simple_media_endpoint_binding_t *binding)
{
    simple_media_config_t cfg;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }
    if (!binding || !binding->has_tx_remote_endpoint || !binding->has_rx_match_endpoint)
    {
        return 0;
    }

    simple_media_runtime_lock(runtime);
    if (runtime->cfg.media->audio_tx || runtime->cfg.media->audio_rx)
    {
        ret = -EBUSY;
        simple_media_runtime_unlock(runtime);
        simple_media_runtime_notify_apply(runtime,
                                          true,
                                          binding,
                                          ret,
                                          "busy");
        return ret;
    }
    if (runtime->cfg.audio_link_match_required &&
        !simple_media_runtime_audio_link_matches(runtime, binding))
    {
        simple_media_runtime_unlock(runtime);
        simple_media_runtime_notify_apply(runtime,
                                          true,
                                          binding,
                                          -EINVAL,
                                          "link_mismatch");
        return -EINVAL;
    }

    ret = simple_media_runtime_copy_base_config_locked(runtime, &cfg);
    if (ret == 0)
    {
        ret = simple_media_runtime_apply_audio_binding_to_config(&cfg, binding);
    }
    if (ret == 0)
    {
        runtime->pending_audio_endpoint = *binding;
        runtime->has_pending_audio_endpoint = true;
    }
    else
    {
        simple_media_runtime_clear_pending_endpoint_locked(runtime, true);
    }
    simple_media_runtime_unlock(runtime);
    simple_media_runtime_notify_apply(runtime,
                                      true,
                                      binding,
                                      ret,
                                      ret == 0 ? "ok" : "apply");
    return ret;
}

int simple_media_runtime_apply_bridge_endpoint(simple_media_runtime_t *runtime,
                                               uint32_t udp_remote_addr,
                                               uint16_t udp_remote_port,
                                               uint32_t udp_rx_match_addr,
                                               uint16_t udp_rx_match_port,
                                               uint32_t plc_remote_addr,
                                               uint16_t plc_remote_port,
                                               uint32_t plc_rx_match_addr,
                                               uint16_t plc_rx_match_port)
{
    simple_media_runtime_media_bridge_descriptor_t descriptor;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.mode = SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_BIDIRECTIONAL;
    descriptor.udp_remote_addr = udp_remote_addr;
    descriptor.udp_remote_port = udp_remote_port;
    descriptor.udp_rx_match_addr = udp_rx_match_addr;
    descriptor.udp_rx_match_port = udp_rx_match_port;
    descriptor.plc_remote_addr = plc_remote_addr;
    descriptor.plc_remote_port = plc_remote_port;
    descriptor.plc_rx_match_addr = plc_rx_match_addr;
    descriptor.plc_rx_match_port = plc_rx_match_port;

    simple_media_runtime_lock(runtime);
    ret = simple_media_runtime_apply_media_bridge_descriptor(runtime, &descriptor);
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_apply_bridge_split_endpoint(simple_media_runtime_t *runtime,
                                                     uint32_t video_udp_remote_addr,
                                                     uint16_t video_udp_remote_port,
                                                     uint32_t audio_udp_remote_addr,
                                                     uint16_t audio_udp_remote_port,
                                                     uint32_t video_plc_remote_addr,
                                                     uint16_t video_plc_remote_port,
                                                     uint32_t udp_rx_match_addr,
                                                     uint16_t udp_rx_match_port,
                                                     uint32_t plc_remote_addr,
                                                     uint16_t plc_remote_port,
                                                     uint32_t plc_rx_match_addr,
                                                     uint16_t plc_rx_match_port)
{
    simple_media_runtime_media_bridge_descriptor_t descriptor;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.mode = SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_SPLIT;
    descriptor.udp_remote_addr = video_udp_remote_addr;
    descriptor.udp_remote_port = video_udp_remote_port;
    descriptor.audio_udp_remote_addr = audio_udp_remote_addr;
    descriptor.audio_udp_remote_port = audio_udp_remote_port;
    descriptor.video_plc_remote_addr = video_plc_remote_addr;
    descriptor.video_plc_remote_port = video_plc_remote_port;
    descriptor.udp_rx_match_addr = udp_rx_match_addr;
    descriptor.udp_rx_match_port = udp_rx_match_port;
    descriptor.plc_remote_addr = plc_remote_addr;
    descriptor.plc_remote_port = plc_remote_port;
    descriptor.plc_rx_match_addr = plc_rx_match_addr;
    descriptor.plc_rx_match_port = plc_rx_match_port;

    simple_media_runtime_lock(runtime);
    ret = simple_media_runtime_apply_media_bridge_descriptor(runtime, &descriptor);
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_apply_bridge_audio_only_endpoint(simple_media_runtime_t *runtime,
                                                          uint32_t udp_remote_addr,
                                                          uint16_t udp_remote_port,
                                                          uint32_t udp_rx_match_addr,
                                                          uint16_t udp_rx_match_port,
                                                          uint32_t plc_remote_addr,
                                                          uint16_t plc_remote_port,
                                                          uint32_t plc_rx_match_addr,
                                                          uint16_t plc_rx_match_port)
{
    simple_media_runtime_media_bridge_descriptor_t descriptor;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.mode = SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_AUDIO_ONLY;
    descriptor.udp_remote_addr = udp_remote_addr;
    descriptor.udp_remote_port = udp_remote_port;
    descriptor.udp_rx_match_addr = udp_rx_match_addr;
    descriptor.udp_rx_match_port = udp_rx_match_port;
    descriptor.plc_remote_addr = plc_remote_addr;
    descriptor.plc_remote_port = plc_remote_port;
    descriptor.plc_rx_match_addr = plc_rx_match_addr;
    descriptor.plc_rx_match_port = plc_rx_match_port;

    simple_media_runtime_lock(runtime);
    ret = simple_media_runtime_apply_media_bridge_descriptor(runtime, &descriptor);
    simple_media_runtime_unlock(runtime);
    return ret;
}

static int simple_media_runtime_start_media_bridge_descriptor_locked(
    simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_record_t *record,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor)
{
    simple_media_config_t cfg;
    mtrans_channel_config_t udp_cfg;
    mtrans_channel_config_t audio_udp_cfg;
    mtrans_channel_config_t plc_cfg;
    uint32_t video_plc_remote_addr;
    uint16_t video_plc_remote_port;
    int ret;

    if (!runtime || !runtime->cfg.media || !record || !descriptor)
    {
        return -EINVAL;
    }

    ret = simple_media_runtime_build_bridge_config_from_descriptor_locked(
        runtime, descriptor, &cfg);
    if (ret < 0)
    {
        return ret;
    }
    ret = simple_media_runtime_prepare_media_bridge_locked(runtime);
    if (ret < 0)
    {
        return ret;
    }

    record->owns_media_resources = true;
    record->media_descriptor = *descriptor;
    record->has_media_descriptor = true;
    record->media_bridge_route_group = 0;

    switch (cfg.type)
    {
    case SIMPLE_MEDIA_ENDPOINT_UDP_TO_PLC_BRIDGE:
        simple_media_runtime_fill_udp_channel_cfg(&udp_cfg, false, &cfg);
        simple_media_runtime_fill_plc_channel_cfg(&plc_cfg, &cfg);
        if (cfg.bidirectional_bridge && cfg.bridge_audio_only_enabled)
        {
            udp_cfg.mcast_enabled = false;
            udp_cfg.mcast_opt.group_addr = 0;
            plc_cfg.mcast_enabled = false;
            plc_cfg.mcast_opt.group_addr = 0;
        }
        else if (cfg.bidirectional_bridge && cfg.bridge_split_plc_to_udp_enabled)
        {
            plc_cfg.mcast_enabled = false;
            plc_cfg.mcast_opt.group_addr = 0;
        }
        record->media_bridge_udp_ch = mtrans_channel_open(&udp_cfg);
        record->media_bridge_plc_ch = mtrans_channel_open(&plc_cfg);
        if (!record->media_bridge_udp_ch || !record->media_bridge_plc_ch)
        {
            ret = -EIO;
            goto fail;
        }

        if (cfg.bidirectional_bridge && cfg.bridge_audio_only_enabled)
        {
            record->media_bridge_udp_to_plc_flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            record->media_bridge_plc_to_udp_flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_udp_ch,
                record->media_bridge_plc_ch,
                record->media_bridge_route_group,
                record->media_bridge_udp_to_plc_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_udp_to_plc_added = true;
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_plc_ch,
                record->media_bridge_udp_ch,
                record->media_bridge_route_group,
                record->media_bridge_plc_to_udp_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_plc_to_udp_added = true;
        }
        else if (cfg.bidirectional_bridge && cfg.bridge_split_plc_to_udp_enabled)
        {
            video_plc_remote_addr = cfg.bridge_video_plc_remote_addr != 0 ?
                                    cfg.bridge_video_plc_remote_addr :
                                    cfg.plc_remote_addr;
            video_plc_remote_port = cfg.bridge_video_plc_remote_port != 0 ?
                                    (uint16_t)cfg.bridge_video_plc_remote_port :
                                    (uint16_t)cfg.plc_remote_port;
            if (video_plc_remote_port > 0xffU)
            {
                video_plc_remote_port = SIMPLE_PLC_MEDIA_PORT;
            }

            if (cfg.bridge_audio_udp_remote_port == 0 || video_plc_remote_port == 0)
            {
                ret = -EINVAL;
                goto fail;
            }
            simple_media_runtime_fill_plc_channel_cfg(&plc_cfg, &cfg);
            plc_cfg.remote_ep.addr = video_plc_remote_addr;
            plc_cfg.remote_ep.port = video_plc_remote_port;
            /*
             * video_plc_ch 只作为 UDP -> PLC 的视频发送通道使用，反向接收仍走
             * media_bridge_plc_ch。这里不订阅 PLC 组播，避免底层 multicast ctrl
             * 失败导致桥接通道反复 open/close。
             */
            plc_cfg.mcast_enabled = false;
            plc_cfg.mcast_opt.group_addr = 0;
            record->media_bridge_video_plc_ch = mtrans_channel_open(&plc_cfg);
            if (!record->media_bridge_video_plc_ch)
            {
                ret = -EIO;
                goto fail;
            }

            simple_media_runtime_fill_udp_channel_cfg(&audio_udp_cfg, true, &cfg);
            audio_udp_cfg.remote_ep.addr = cfg.bridge_audio_udp_remote_addr;
            audio_udp_cfg.remote_ep.port = cfg.bridge_audio_udp_remote_port;
            audio_udp_cfg.rx_match_ep_enabled = false;
            audio_udp_cfg.mcast_enabled = false;
            record->media_bridge_audio_udp_ch = mtrans_channel_open(&audio_udp_cfg);
            if (!record->media_bridge_audio_udp_ch)
            {
                ret = -EIO;
                goto fail;
            }

            record->media_bridge_udp_to_video_plc_flags =
                MTRANS_ROUTE_FLAG_MEDIA_VIDEO_ONLY;
            record->media_bridge_udp_to_plc_flags =
                MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            record->media_bridge_plc_to_udp_flags =
                MTRANS_ROUTE_FLAG_MEDIA_VIDEO_ONLY;
            record->media_bridge_plc_to_audio_udp_flags =
                MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;

            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_udp_ch,
                record->media_bridge_video_plc_ch,
                record->media_bridge_route_group,
                record->media_bridge_udp_to_video_plc_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_udp_to_video_plc_added = true;
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_udp_ch,
                record->media_bridge_plc_ch,
                record->media_bridge_route_group,
                record->media_bridge_udp_to_plc_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_udp_to_plc_added = true;
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_plc_ch,
                record->media_bridge_udp_ch,
                record->media_bridge_route_group,
                record->media_bridge_plc_to_udp_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_plc_to_udp_added = true;
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_plc_ch,
                record->media_bridge_audio_udp_ch,
                record->media_bridge_route_group,
                record->media_bridge_plc_to_audio_udp_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_plc_to_audio_udp_added = true;
        }
        else
        {
            ret = simple_media_runtime_add_media_route_locked(
                record->media_bridge_udp_ch,
                record->media_bridge_plc_ch,
                record->media_bridge_route_group,
                record->media_bridge_udp_to_plc_flags);
            if (ret < 0)
            {
                goto fail;
            }
            record->media_bridge_udp_to_plc_added = true;
            if (cfg.bidirectional_bridge)
            {
                ret = simple_media_runtime_add_media_route_locked(
                    record->media_bridge_plc_ch,
                    record->media_bridge_udp_ch,
                    record->media_bridge_route_group,
                    record->media_bridge_plc_to_udp_flags);
                if (ret < 0)
                {
                    goto fail;
                }
                record->media_bridge_plc_to_udp_added = true;
            }
        }
        return 0;

    case SIMPLE_MEDIA_ENDPOINT_PLC_TO_UDP_BRIDGE:
        simple_media_runtime_fill_udp_channel_cfg(&udp_cfg, true, &cfg);
        simple_media_runtime_fill_plc_channel_cfg(&plc_cfg, &cfg);
        record->media_bridge_plc_ch = mtrans_channel_open(&plc_cfg);
        record->media_bridge_udp_ch = mtrans_channel_open(&udp_cfg);
        if (!record->media_bridge_udp_ch || !record->media_bridge_plc_ch)
        {
            ret = -EIO;
            goto fail;
        }
        ret = simple_media_runtime_add_media_route_locked(
            record->media_bridge_plc_ch,
            record->media_bridge_udp_ch,
            record->media_bridge_route_group,
            record->media_bridge_plc_to_udp_flags);
        if (ret < 0)
        {
            goto fail;
        }
        record->media_bridge_plc_to_udp_added = true;
        return 0;

    default:
        ret = -EINVAL;
        goto fail;
    }

fail:
    simple_media_runtime_media_bridge_cleanup_record_locked(record);
    return ret;
}

static int simple_media_runtime_start_pending_media_bridge_locked(
    simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_record_t *record)
{
    if (!runtime || !runtime->has_pending_media_bridge)
    {
        return -EINVAL;
    }
    return simple_media_runtime_start_media_bridge_descriptor_locked(
        runtime, record, &runtime->pending_media_bridge);
}

int simple_media_runtime_start_bridge(simple_media_runtime_t *runtime)
{
    simple_media_runtime_bridge_record_t *record;
    simple_media_runtime_bridge_state_t state;
    int ret;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (runtime->bridge_releasing ||
        simple_media_runtime_bridge_table_has_active_locked(runtime))
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    record = simple_media_runtime_bridge_table_alloc_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOSPC;
    }
    simple_media_runtime_bridge_record_clear(record);
    record->used = true;
    ret = simple_media_runtime_start_pending_media_bridge_locked(runtime, record);
    if (ret == 0)
    {
        memset(&state, 0, sizeof(state));
        state.active = true;
        state.kind = SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA;
        state.video_active =
            record->media_descriptor.mode != SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_AUDIO_ONLY;
        state.audio_active = true;
        record->state = state;
        simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    }
    else
    {
        simple_media_runtime_media_bridge_cleanup_record_locked(record);
        simple_media_runtime_bridge_record_clear(record);
        simple_media_runtime_restore_base_config_locked(runtime);
    }
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_stop_bridge(simple_media_runtime_t *runtime)
{
    simple_media_runtime_bridge_record_t *record;

    if (!runtime || !runtime->cfg.media)
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    record = simple_media_runtime_active_bridge_record_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return 0;
    }
    simple_media_runtime_unlock(runtime);
    return simple_media_runtime_release_bridge(runtime, 0, NULL);
}

int simple_media_runtime_start_bridge_session(simple_media_runtime_t *runtime,
                                              uint64_t session_id,
                                              const char *caller_device_id,
                                              const char *callee_device_id,
                                              bool video_active,
                                              bool audio_active)
{
    simple_media_runtime_bridge_record_t *record;
    simple_media_runtime_bridge_state_t state;
    int ret;

    if (!runtime || session_id == 0 || (!video_active && !audio_active))
    {
        return -EINVAL;
    }
    simple_media_runtime_lock(runtime);
    if (runtime->bridge_releasing)
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    if (simple_media_runtime_bridge_table_find_session_locked(runtime, session_id))
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    if (!runtime->has_pending_media_bridge)
    {
        simple_media_runtime_unlock(runtime);
        return -EINVAL;
    }
    record = simple_media_runtime_bridge_table_alloc_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOSPC;
    }
    simple_media_runtime_bridge_record_clear(record);
    record->used = true;
    ret = simple_media_runtime_start_pending_media_bridge_locked(runtime, record);
    if (ret < 0)
    {
        simple_media_runtime_media_bridge_cleanup_record_locked(record);
        simple_media_runtime_bridge_record_clear(record);
        simple_media_runtime_restore_base_config_locked(runtime);
        simple_media_runtime_unlock(runtime);
        return ret;
    }
    memset(&state, 0, sizeof(state));
    state.active = true;
    state.kind = SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA;
    state.session_id = session_id;
    state.video_active = video_active;
    state.audio_active = audio_active;
    simple_media_runtime_copy_device_id(state.caller_device_id,
                                        sizeof(state.caller_device_id),
                                        caller_device_id);
    simple_media_runtime_copy_device_id(state.callee_device_id,
                                        sizeof(state.callee_device_id),
                                        callee_device_id);
    record->state = state;
    simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    simple_media_runtime_unlock(runtime);
    return 0;
}

int simple_media_runtime_start_bridge_session_with_descriptor(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor,
    uint64_t session_id,
    const char *caller_device_id,
    const char *callee_device_id,
    bool video_active,
    bool audio_active)
{
    simple_media_runtime_bridge_record_t *record;
    simple_media_runtime_bridge_state_t state;
    int ret;

    if (!runtime || !descriptor || session_id == 0 || (!video_active && !audio_active))
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (runtime->bridge_releasing)
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    if (simple_media_runtime_bridge_table_find_session_locked(runtime, session_id))
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    record = simple_media_runtime_bridge_table_alloc_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOSPC;
    }
    simple_media_runtime_bridge_record_clear(record);
    record->used = true;
    simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    ret = simple_media_runtime_start_media_bridge_descriptor_locked(runtime,
                                                                    record,
                                                                    descriptor);
    if (ret < 0)
    {
        simple_media_runtime_media_bridge_cleanup_record_locked(record);
        simple_media_runtime_bridge_record_clear(record);
        simple_media_runtime_restore_base_config_locked(runtime);
        simple_media_runtime_unlock(runtime);
        return ret;
    }
    memset(&state, 0, sizeof(state));
    state.active = true;
    state.kind = SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA;
    state.session_id = session_id;
    state.video_active = video_active;
    state.audio_active = audio_active;
    simple_media_runtime_copy_device_id(state.caller_device_id,
                                        sizeof(state.caller_device_id),
                                        caller_device_id);
    simple_media_runtime_copy_device_id(state.callee_device_id,
                                        sizeof(state.callee_device_id),
                                        callee_device_id);
    record->state = state;
    simple_media_runtime_unlock(runtime);
    return 0;
}

int simple_media_runtime_start_direct_bridge_session(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_direct_bridge_config_t *bridge_cfg,
    uint64_t session_id,
    const char *caller_device_id,
    const char *callee_device_id,
    bool video_active,
    bool audio_active)
{
    mtrans_route_rule_t rule;
    simple_media_runtime_bridge_record_t *record;
    simple_media_runtime_bridge_state_t state;
    int ret = 0;

    if (!runtime || !bridge_cfg || session_id == 0 || (!video_active && !audio_active))
    {
        return -EINVAL;
    }

    simple_media_runtime_lock(runtime);
    if (runtime->bridge_releasing)
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    if (simple_media_runtime_bridge_table_find_session_locked(runtime, session_id))
    {
        simple_media_runtime_unlock(runtime);
        return -EBUSY;
    }
    record = simple_media_runtime_bridge_table_alloc_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return -ENOSPC;
    }
    simple_media_runtime_bridge_record_clear(record);
    record->used = true;
    record->owns_direct_resources = true;

    simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    simple_media_runtime_restore_base_config_locked(runtime);

    record->direct_bridge_ingress_ch = mtrans_channel_open(&bridge_cfg->ingress_channel);
    record->direct_bridge_egress_ch = mtrans_channel_open(&bridge_cfg->egress_channel);
    if (!record->direct_bridge_ingress_ch || !record->direct_bridge_egress_ch)
    {
        ret = -EIO;
        goto fail;
    }

    record->direct_bridge_route_group = bridge_cfg->route_group;
    record->direct_bridge_forward_flags = bridge_cfg->forward_flags;
    record->direct_bridge_reverse_flags = bridge_cfg->reverse_flags;

    memset(&rule, 0, sizeof(rule));
    rule.src_channel = record->direct_bridge_ingress_ch;
    rule.dst_channel = record->direct_bridge_egress_ch;
    rule.route_group = bridge_cfg->route_group;
    rule.flags = bridge_cfg->forward_flags;
    ret = mtrans_route_add_rule(&rule);
    if (ret != 0)
    {
        ret = ret < 0 ? ret : -EIO;
        goto fail;
    }
    record->direct_bridge_forward_added = true;

    if (bridge_cfg->bidirectional)
    {
        memset(&rule, 0, sizeof(rule));
        rule.src_channel = record->direct_bridge_egress_ch;
        rule.dst_channel = record->direct_bridge_ingress_ch;
        rule.route_group = bridge_cfg->route_group;
        rule.flags = bridge_cfg->reverse_flags;
        ret = mtrans_route_add_rule(&rule);
        if (ret != 0)
        {
            ret = ret < 0 ? ret : -EIO;
            goto fail;
        }
        record->direct_bridge_reverse_added = true;
    }

    memset(&state, 0, sizeof(state));
    state.kind = SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_DIRECT;
    state.active = true;
    state.session_id = session_id;
    state.video_active = video_active;
    state.audio_active = audio_active;
    simple_media_runtime_copy_device_id(state.caller_device_id,
                                        sizeof(state.caller_device_id),
                                        caller_device_id);
    simple_media_runtime_copy_device_id(state.callee_device_id,
                                        sizeof(state.callee_device_id),
                                        callee_device_id);
    record->state = state;
    simple_media_runtime_clear_pending_media_bridge_locked(runtime);
    simple_media_runtime_unlock(runtime);
    return 0;

fail:
    simple_media_runtime_direct_bridge_cleanup_record_locked(record);
    simple_media_runtime_bridge_record_clear(record);
    simple_media_runtime_unlock(runtime);
    return ret;
}

int simple_media_runtime_release_bridge(simple_media_runtime_t *runtime,
                                        uint64_t session_id,
                                        simple_media_runtime_bridge_state_t *released)
{
    simple_media_runtime_bridge_record_t *record;
    simple_media_runtime_bridge_state_t old_state;
    int ret;

    if (released)
    {
        memset(released, 0, sizeof(*released));
    }
    if (!runtime)
    {
        return -EINVAL;
    }
    simple_media_runtime_lock(runtime);
    record = session_id == 0 ?
             simple_media_runtime_active_bridge_record_locked(runtime) :
             simple_media_runtime_bridge_table_find_session_locked(runtime, session_id);
    if (!record)
    {
        simple_media_runtime_unlock(runtime);
        return 0;
    }

    old_state = record->state;
    if (old_state.kind == SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_DIRECT)
    {
        simple_media_runtime_direct_bridge_cleanup_record_locked(record);
        simple_media_runtime_bridge_record_clear(record);
        simple_media_runtime_unlock(runtime);
        ret = 0;
    }
    else
    {
        runtime->bridge_releasing = true;
        simple_media_runtime_clear_pending_media_bridge_locked(runtime);
        simple_media_runtime_media_bridge_cleanup_record_locked(record);
        simple_media_runtime_bridge_record_clear(record);
        simple_media_runtime_restore_base_config_locked(runtime);
        runtime->bridge_releasing = false;
        simple_media_runtime_unlock(runtime);
        ret = 0;
    }
    if (released)
    {
        *released = old_state;
    }
    return ret;
}

bool simple_media_runtime_get_bridge_state(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_state_t *state)
{
    const simple_media_runtime_bridge_record_t *record;

    if (state)
    {
        memset(state, 0, sizeof(*state));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_active_bridge_record_const_locked(runtime);
    if (!record)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    if (state)
    {
        *state = record->state;
    }
    simple_media_runtime_unlock_const(runtime);
    return true;
}

bool simple_media_runtime_get_media_bridge_descriptor(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_media_bridge_descriptor_t *descriptor)
{
    const simple_media_runtime_bridge_record_t *record;

    if (descriptor)
    {
        memset(descriptor, 0, sizeof(*descriptor));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_active_bridge_record_const_locked(runtime);
    if (!record ||
        record->state.kind != SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA ||
        !record->has_media_descriptor)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    if (descriptor)
    {
        *descriptor = record->media_descriptor;
    }
    simple_media_runtime_unlock_const(runtime);
    return true;
}

size_t simple_media_runtime_get_bridge_record_count(
    const simple_media_runtime_t *runtime)
{
    size_t count;

    simple_media_runtime_lock_const(runtime);
    count = simple_media_runtime_bridge_table_count_active_locked(runtime);
    simple_media_runtime_unlock_const(runtime);
    return count;
}

bool simple_media_runtime_get_bridge_state_at(
    const simple_media_runtime_t *runtime,
    size_t ordinal,
    simple_media_runtime_bridge_state_t *state)
{
    const simple_media_runtime_bridge_record_t *record;

    if (state)
    {
        memset(state, 0, sizeof(*state));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_bridge_table_active_at_const_locked(runtime, ordinal);
    if (!record)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    if (state)
    {
        *state = record->state;
    }
    simple_media_runtime_unlock_const(runtime);
    return true;
}

bool simple_media_runtime_get_bridge_state_for_session(
    const simple_media_runtime_t *runtime,
    uint64_t session_id,
    simple_media_runtime_bridge_state_t *state)
{
    const simple_media_runtime_bridge_record_t *record;

    if (state)
    {
        memset(state, 0, sizeof(*state));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_bridge_table_find_session_const_locked(runtime,
                                                                         session_id);
    if (!record)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    if (state)
    {
        *state = record->state;
    }
    simple_media_runtime_unlock_const(runtime);
    return true;
}

bool simple_media_runtime_get_bridge_snapshot_at(
    const simple_media_runtime_t *runtime,
    size_t ordinal,
    simple_media_runtime_bridge_snapshot_t *snapshot)
{
    const simple_media_runtime_bridge_record_t *record;

    if (snapshot)
    {
        memset(snapshot, 0, sizeof(*snapshot));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_bridge_table_active_at_const_locked(runtime, ordinal);
    if (!record)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    simple_media_runtime_fill_bridge_snapshot_locked(record, snapshot);
    simple_media_runtime_unlock_const(runtime);
    return true;
}

bool simple_media_runtime_get_bridge_snapshot_for_session(
    const simple_media_runtime_t *runtime,
    uint64_t session_id,
    simple_media_runtime_bridge_snapshot_t *snapshot)
{
    const simple_media_runtime_bridge_record_t *record;

    if (snapshot)
    {
        memset(snapshot, 0, sizeof(*snapshot));
    }
    simple_media_runtime_lock_const(runtime);
    record = simple_media_runtime_bridge_table_find_session_const_locked(runtime,
                                                                         session_id);
    if (!record)
    {
        simple_media_runtime_unlock_const(runtime);
        return false;
    }
    simple_media_runtime_fill_bridge_snapshot_locked(record, snapshot);
    simple_media_runtime_unlock_const(runtime);
    return true;
}

void simple_media_runtime_dump_bridge_table(simple_media_runtime_t *runtime,
                                            const char *reason)
{
    size_t i;
    size_t ordinal = 0;
    size_t active_count;

    simple_media_runtime_lock(runtime);
    active_count = simple_media_runtime_bridge_table_count_active_locked(runtime);
    SIMPLE_LOGI("bridge_table reason=%s active=%zu capacity=%u\n",
                reason ? reason : "-",
                active_count,
                (unsigned)SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY);
    if (!runtime || active_count == 0)
    {
        simple_media_runtime_unlock(runtime);
        return;
    }
    for (i = 0; i < SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY; ++i)
    {
        if (runtime->bridge_table[i].used &&
            runtime->bridge_table[i].state.active)
        {
            simple_media_runtime_dump_bridge_record_locked(runtime,
                                                           &runtime->bridge_table[i],
                                                           ordinal);
            ++ordinal;
        }
    }
    simple_media_runtime_unlock(runtime);
}
