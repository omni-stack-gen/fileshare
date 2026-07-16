/**
 * @file
 * @brief simple 示例应用公共模块 simple_media 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_media.h"
#include "simple_log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void simple_media_on_video_frame(void *ctx, const uint8_t *frame,
                                        size_t len, uint32_t ts, int flags)
{
    simple_media_t *media = (simple_media_t *)ctx;

    if (!media || !media->active_video_codec_ops)
    {
        return;
    }

    (void)simple_codec_input_video(media->active_video_codec_ops, frame, len, ts, flags);
}

static void simple_media_on_audio_frame(void *ctx, const uint8_t *frame,
                                        size_t len, uint32_t ts, int flags)
{
    simple_media_t *media = (simple_media_t *)ctx;

    if (!media || !media->active_audio_codec_ops)
    {
        return;
    }

    (void)simple_codec_input_audio(media->active_audio_codec_ops, frame, len, ts, flags);
}

static void simple_media_on_stream_event(void *ctx, mtrans_stream_event_t event)
{
    simple_media_t *media = (simple_media_t *)ctx;

    if (!media || !media->active_video_codec_ops)
    {
        return;
    }

    simple_codec_on_stream_event(media->active_video_codec_ops, event);
}

void simple_media_default_config(simple_media_config_t *cfg,
                                 simple_media_endpoint_type_t type)
{
    if (!cfg)
    {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->type = type;
    cfg->udp_interface = "eth0";
    cfg->udp_local_addr = ADDR_ANY;
    cfg->udp_remote_addr = SIMPLE_UDP_MEDIA_GROUP_ADDR;
    cfg->udp_remote_port = SIMPLE_UDP_MEDIA_PORT;
    cfg->udp_mcast_enabled = true;
    cfg->udp_mcast_group_addr = SIMPLE_UDP_MEDIA_GROUP_ADDR;
    cfg->udp_mcast_loop_disable = true;
    /*
     * UDP 组播接收时，socket 实际看到的 src_ep 是发送端单播地址:10000，
     * 不是组播地址 239.10.10.10:10000。为了让 auto transport 能把收到的
     * 组播媒体正确分发到 channel，这里默认把接收匹配和发送目标解耦：
     * - remote_ep 继续保留组播地址，供 TX / reverse bridge 使用
     * - rx_match_ep 改为 ADDR_ANY:10000，匹配任意发送端的媒体上行
     */
    cfg->udp_rx_match_ep_enabled = true;
    cfg->udp_rx_match_addr = ADDR_ANY;
    cfg->udp_rx_match_port = SIMPLE_UDP_MEDIA_PORT;
    cfg->plc_local_addr = 0;
    cfg->plc_remote_addr = SIMPLE_PLC_MEDIA_GROUP_ADDR;
    cfg->plc_remote_port = SIMPLE_PLC_MEDIA_PORT;
    cfg->plc_rx_match_ep_enabled = true;
    cfg->plc_rx_match_addr = ADDR_ANY;
    cfg->plc_rx_match_port = SIMPLE_PLC_MEDIA_PORT;
    cfg->plc_mcast_enabled = true;
    cfg->plc_mcast_group_addr = SIMPLE_PLC_MEDIA_GROUP_ADDR;
    cfg->audio_tx_remote_ep_enabled = false;
    cfg->audio_tx_remote_addr = 0;
    cfg->audio_tx_remote_port = 0;
    cfg->bridge_split_plc_to_udp_enabled = false;
    cfg->bridge_audio_only_enabled = false;
    cfg->bridge_video_plc_remote_addr = 0;
    cfg->bridge_video_plc_remote_port = 0;
    cfg->bridge_audio_udp_remote_addr = 0;
    cfg->bridge_audio_udp_remote_port = 0;
    cfg->bidirectional_bridge = false;
    cfg->video_tx_enabled = false;
    cfg->video_rx_enabled = false;
    cfg->audio_tx_enabled = false;
    cfg->audio_rx_enabled = false;
    cfg->video_ssrc = 0;
    cfg->video_rx_ssrc = 0;
    cfg->audio_ssrc = 0;
    cfg->audio_rx_ssrc = 0;
}

static void simple_media_fill_udp_cfg(mtrans_channel_config_t *cfg,
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

static void simple_media_fill_plc_cfg(mtrans_channel_config_t *cfg,
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

static bool simple_media_is_udp_endpoint(simple_media_endpoint_type_t type)
{
    return type == SIMPLE_MEDIA_ENDPOINT_UDP_TX ||
           type == SIMPLE_MEDIA_ENDPOINT_UDP_RX;
}

static bool simple_media_is_plc_endpoint(simple_media_endpoint_type_t type)
{
    return type == SIMPLE_MEDIA_ENDPOINT_PLC_TX ||
           type == SIMPLE_MEDIA_ENDPOINT_PLC_RX;
}

static bool simple_media_is_bridge_endpoint(simple_media_endpoint_type_t type)
{
    return type == SIMPLE_MEDIA_ENDPOINT_UDP_TO_PLC_BRIDGE ||
           type == SIMPLE_MEDIA_ENDPOINT_PLC_TO_UDP_BRIDGE;
}

static bool simple_media_has_streams(const simple_media_t *media)
{
    return media && (media->video_tx || media->video_rx ||
                     media->audio_tx || media->audio_rx);
}

static bool simple_media_string_equal(const char *a, const char *b)
{
    if (a == b)
    {
        return true;
    }
    if (!a || !b)
    {
        return false;
    }
    return strcmp(a, b) == 0;
}

static bool simple_media_udp_channel_config_equal(
    const simple_media_config_t *a,
    const simple_media_config_t *b)
{
    return simple_media_string_equal(a->udp_interface, b->udp_interface) &&
           a->udp_local_addr == b->udp_local_addr &&
           a->udp_remote_addr == b->udp_remote_addr &&
           a->udp_remote_port == b->udp_remote_port &&
           a->udp_mcast_enabled == b->udp_mcast_enabled &&
           a->udp_mcast_group_addr == b->udp_mcast_group_addr &&
           a->udp_mcast_loop_disable == b->udp_mcast_loop_disable &&
           a->udp_rx_match_ep_enabled == b->udp_rx_match_ep_enabled &&
           a->udp_rx_match_addr == b->udp_rx_match_addr &&
           a->udp_rx_match_port == b->udp_rx_match_port;
}

static bool simple_media_plc_channel_config_equal(
    const simple_media_config_t *a,
    const simple_media_config_t *b)
{
    return a->plc_local_addr == b->plc_local_addr &&
           a->plc_remote_addr == b->plc_remote_addr &&
           a->plc_remote_port == b->plc_remote_port &&
           a->plc_rx_match_ep_enabled == b->plc_rx_match_ep_enabled &&
           a->plc_rx_match_addr == b->plc_rx_match_addr &&
           a->plc_rx_match_port == b->plc_rx_match_port &&
           a->plc_mcast_enabled == b->plc_mcast_enabled &&
           a->plc_mcast_group_addr == b->plc_mcast_group_addr;
}

static bool simple_media_main_channel_config_equal(
    const simple_media_config_t *a,
    const simple_media_config_t *b)
{
    if (!a || !b || a->type != b->type)
    {
        return false;
    }

    if (simple_media_is_udp_endpoint(a->type))
    {
        return simple_media_udp_channel_config_equal(a, b);
    }
    if (simple_media_is_plc_endpoint(a->type))
    {
        return simple_media_plc_channel_config_equal(a, b);
    }
    if (simple_media_is_bridge_endpoint(a->type))
    {
        return simple_media_udp_channel_config_equal(a, b) &&
               simple_media_plc_channel_config_equal(a, b) &&
               a->bridge_split_plc_to_udp_enabled == b->bridge_split_plc_to_udp_enabled &&
               a->bridge_audio_only_enabled == b->bridge_audio_only_enabled &&
               a->bridge_video_plc_remote_addr == b->bridge_video_plc_remote_addr &&
               a->bridge_video_plc_remote_port == b->bridge_video_plc_remote_port &&
               a->bridge_audio_udp_remote_addr == b->bridge_audio_udp_remote_addr &&
               a->bridge_audio_udp_remote_port == b->bridge_audio_udp_remote_port &&
               a->bidirectional_bridge == b->bidirectional_bridge;
    }

    return true;
}

static bool simple_media_audio_tx_channel_config_equal(
    const simple_media_config_t *a,
    const simple_media_config_t *b)
{
    if (!a || !b ||
        a->audio_tx_remote_ep_enabled != b->audio_tx_remote_ep_enabled)
    {
        return false;
    }
    if (!a->audio_tx_remote_ep_enabled)
    {
        return true;
    }
    if (a->type != b->type ||
        a->audio_tx_remote_addr != b->audio_tx_remote_addr ||
        a->audio_tx_remote_port != b->audio_tx_remote_port)
    {
        return false;
    }
    if (simple_media_is_udp_endpoint(a->type))
    {
        return simple_media_string_equal(a->udp_interface, b->udp_interface) &&
               a->udp_local_addr == b->udp_local_addr;
    }
    if (simple_media_is_plc_endpoint(a->type))
    {
        return a->plc_local_addr == b->plc_local_addr;
    }
    return false;
}

static void simple_media_note_channel_cfg(simple_media_t *media,
                                          const simple_media_config_t *cfg)
{
    if (!media || !cfg)
    {
        return;
    }
    media->channel_cfg = *cfg;
    media->channel_cfg_valid = true;
    media->channels_open = true;
}

static void simple_media_note_audio_tx_channel_cfg(
    simple_media_t *media,
    const simple_media_config_t *cfg)
{
    if (!media || !cfg)
    {
        return;
    }
    media->audio_tx_channel_cfg = *cfg;
    media->audio_tx_channel_cfg_valid = true;
}

static void simple_media_clear_audio_tx_channel(simple_media_t *media)
{
    if (!media)
    {
        return;
    }
    if (media->audio_tx_ch)
    {
        mtrans_channel_destroy(media->audio_tx_ch);
        media->audio_tx_ch = NULL;
    }
    media->audio_tx_channel_cfg_valid = false;
    memset(&media->audio_tx_channel_cfg, 0, sizeof(media->audio_tx_channel_cfg));
}

static void simple_media_log_bridge_rule_stats(const char *name,
                                               mtrans_channel_t src,
                                               mtrans_channel_t dst)
{
    mtrans_route_rule_stats_t stats;

    if (!name || !src || !dst)
    {
        return;
    }
    if (mtrans_route_get_rule_stats(src, dst, &stats) < 0)
    {
        return;
    }

    SIMPLE_LOGI("media bridge route: name=%s flags=0x%x forward=%llu/%llub "
                "drop_policy=%llu/%llub drop_send=%llu/%llub\n",
                name, stats.flags,
                (unsigned long long)stats.forwarded_pkts,
                (unsigned long long)stats.forwarded_bytes,
                (unsigned long long)stats.dropped_policy_pkts,
                (unsigned long long)stats.dropped_policy_bytes,
                (unsigned long long)stats.dropped_send_fail_pkts,
                (unsigned long long)stats.dropped_send_fail_bytes);
}

static void simple_media_log_bridge_stats(const simple_media_t *media)
{
    if (!media || !media->channel_cfg_valid ||
        !simple_media_is_bridge_endpoint(media->channel_cfg.type))
    {
        return;
    }

    if (!media->bridge_video_plc_ch && !media->bridge_audio_udp_ch)
    {
        simple_media_log_bridge_rule_stats("udp_to_plc",
                                           media->udp_ch, media->plc_ch);
        simple_media_log_bridge_rule_stats("plc_to_udp",
                                           media->plc_ch, media->udp_ch);
        return;
    }

    simple_media_log_bridge_rule_stats("udp_to_plc_video",
                                       media->udp_ch, media->bridge_video_plc_ch);
    simple_media_log_bridge_rule_stats("udp_to_plc_audio",
                                       media->udp_ch, media->plc_ch);
    simple_media_log_bridge_rule_stats("plc_to_udp_video",
                                       media->plc_ch, media->udp_ch);
    simple_media_log_bridge_rule_stats("plc_to_udp_audio",
                                       media->plc_ch, media->bridge_audio_udp_ch);
}

static void simple_media_close_channels(simple_media_t *media)
{
    if (!media)
    {
        return;
    }

    simple_media_log_bridge_stats(media);

    if (media->audio_tx_ch)
    {
        mtrans_channel_destroy(media->audio_tx_ch);
        media->audio_tx_ch = NULL;
    }
    media->audio_tx_channel_cfg_valid = false;
    memset(&media->audio_tx_channel_cfg, 0, sizeof(media->audio_tx_channel_cfg));
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
    memset(&media->channel_cfg, 0, sizeof(media->channel_cfg));
}

static void simple_media_close_idle_endpoint_channels(simple_media_t *media)
{
    if (!media || simple_media_has_streams(media) ||
        (media->channel_cfg_valid &&
         simple_media_is_bridge_endpoint(media->channel_cfg.type)))
    {
        return;
    }

    simple_media_close_channels(media);
}

static int simple_media_open_channels_with_config(simple_media_t *media,
                                                  const simple_media_config_t *cfg);

static int simple_media_open_audio_tx_channel(simple_media_t *media,
                                              const simple_media_config_t *cfg)
{
    mtrans_channel_config_t ch_cfg;

    if (!media || !cfg)
    {
        return -EINVAL;
    }

    if (!cfg->audio_tx_remote_ep_enabled)
    {
        if (media->audio_tx_ch && !media->audio_tx)
        {
            simple_media_clear_audio_tx_channel(media);
        }
        return 0;
    }

    if (media->audio_tx_ch)
    {
        if (media->audio_tx_channel_cfg_valid &&
            simple_media_audio_tx_channel_config_equal(&media->audio_tx_channel_cfg, cfg))
        {
            return 0;
        }
        if (media->audio_tx)
        {
            return -EBUSY;
        }
        simple_media_clear_audio_tx_channel(media);
    }

    if (cfg->audio_tx_remote_port == 0)
    {
        return -EINVAL;
    }

    if (simple_media_is_plc_endpoint(cfg->type) && media->plc_ch)
    {
        simple_media_fill_plc_cfg(&ch_cfg, cfg);
        ch_cfg.remote_ep.addr = cfg->audio_tx_remote_addr;
        ch_cfg.remote_ep.port = cfg->audio_tx_remote_port;
        ch_cfg.rx_match_ep_enabled = false;
        ch_cfg.mcast_enabled = false;
        media->audio_tx_ch = mtrans_channel_open(&ch_cfg);
        if (!media->audio_tx_ch)
        {
            return -EIO;
        }
        simple_media_note_audio_tx_channel_cfg(media, cfg);
        return 0;
    }

    if (simple_media_is_udp_endpoint(cfg->type) && media->udp_ch)
    {
        simple_media_fill_udp_cfg(&ch_cfg, true, cfg);
        ch_cfg.remote_ep.addr = cfg->audio_tx_remote_addr;
        ch_cfg.remote_ep.port = cfg->audio_tx_remote_port;
        ch_cfg.rx_match_ep_enabled = false;
        ch_cfg.mcast_enabled = false;
        media->audio_tx_ch = mtrans_channel_open(&ch_cfg);
        if (!media->audio_tx_ch)
        {
            return -EIO;
        }
        simple_media_note_audio_tx_channel_cfg(media, cfg);
        return 0;
    }

    return -EINVAL;
}

static int simple_media_prepare_open_channels(simple_media_t *media,
                                              const simple_media_config_t *cfg)
{
    if (!media || !cfg)
    {
        return -EINVAL;
    }

    if (!media->channels_open)
    {
        return 0;
    }

    if (media->channel_cfg_valid &&
        simple_media_main_channel_config_equal(&media->channel_cfg, cfg))
    {
        return 0;
    }

    if (simple_media_has_streams(media) ||
        (media->channel_cfg_valid &&
         simple_media_is_bridge_endpoint(media->channel_cfg.type)))
    {
        return -EBUSY;
    }

    simple_media_close_channels(media);
    return 0;
}

static int simple_media_open_channels_with_config(simple_media_t *media,
                                                  const simple_media_config_t *cfg)
{
    mtrans_channel_config_t udp_cfg;
    mtrans_channel_config_t audio_udp_cfg;
    mtrans_channel_config_t plc_cfg;
    mtrans_route_rule_t rule;
    int ret;

    if (!media || !media->configured || !cfg)
    {
        return -EINVAL;
    }

    ret = simple_media_prepare_open_channels(media, cfg);
    if (ret < 0)
    {
        return ret;
    }
    if (media->channels_open)
    {
        return simple_media_open_audio_tx_channel(media, cfg);
    }
    switch (cfg->type)
    {
    case SIMPLE_MEDIA_ENDPOINT_UDP_TX:
        simple_media_fill_udp_cfg(&udp_cfg, true, cfg);
        media->udp_ch = mtrans_channel_open(&udp_cfg);
        ret = simple_media_open_audio_tx_channel(media, cfg);
        if (!media->udp_ch || ret < 0)
        {
            simple_media_close_channels(media);
            return ret < 0 ? ret : -EIO;
        }
        simple_media_note_channel_cfg(media, cfg);
        return 0;

    case SIMPLE_MEDIA_ENDPOINT_UDP_RX:
        simple_media_fill_udp_cfg(&udp_cfg, false, cfg);
        media->udp_ch = mtrans_channel_open(&udp_cfg);
        ret = simple_media_open_audio_tx_channel(media, cfg);
        if (!media->udp_ch || ret < 0)
        {
            simple_media_close_channels(media);
            return ret < 0 ? ret : -EIO;
        }
        simple_media_note_channel_cfg(media, cfg);
        return 0;

    case SIMPLE_MEDIA_ENDPOINT_PLC_TX:
    case SIMPLE_MEDIA_ENDPOINT_PLC_RX:
        simple_media_fill_plc_cfg(&plc_cfg, cfg);
        media->plc_ch = mtrans_channel_open(&plc_cfg);
        ret = simple_media_open_audio_tx_channel(media, cfg);
        if (!media->plc_ch || ret < 0)
        {
            simple_media_close_channels(media);
            return ret < 0 ? ret : -EIO;
        }
        simple_media_note_channel_cfg(media, cfg);
        return 0;

    case SIMPLE_MEDIA_ENDPOINT_UDP_TO_PLC_BRIDGE:
        simple_media_fill_udp_cfg(&udp_cfg, false, cfg);
        simple_media_fill_plc_cfg(&plc_cfg, cfg);
        if (cfg->bidirectional_bridge && cfg->bridge_audio_only_enabled)
        {
            udp_cfg.mcast_enabled = false;
            udp_cfg.mcast_opt.group_addr = 0;
            plc_cfg.mcast_enabled = false;
            plc_cfg.mcast_opt.group_addr = 0;
        }
        else if (cfg->bidirectional_bridge && cfg->bridge_split_plc_to_udp_enabled)
        {
            plc_cfg.mcast_enabled = false;
            plc_cfg.mcast_opt.group_addr = 0;
        }
        media->udp_ch = mtrans_channel_open(&udp_cfg);
        media->plc_ch = mtrans_channel_open(&plc_cfg);
        if (!media->udp_ch || !media->plc_ch)
        {
            simple_media_close_channels(media);
            return -EIO;
        }
        if (cfg->bidirectional_bridge && cfg->bridge_audio_only_enabled)
        {
            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->udp_ch;
            rule.dst_channel = media->plc_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->plc_ch;
            rule.dst_channel = media->udp_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }
        }
        else if (cfg->bidirectional_bridge && cfg->bridge_split_plc_to_udp_enabled)
        {
            uint32_t video_plc_remote_addr = cfg->bridge_video_plc_remote_addr != 0 ?
                                             cfg->bridge_video_plc_remote_addr :
                                             cfg->plc_remote_addr;
            uint16_t video_plc_remote_port = cfg->bridge_video_plc_remote_port != 0 ?
                                             cfg->bridge_video_plc_remote_port :
                                             (uint16_t)cfg->plc_remote_port;
            if (video_plc_remote_port > 0xffU)
            {
                video_plc_remote_port = SIMPLE_PLC_MEDIA_PORT;
            }

            if (cfg->bridge_audio_udp_remote_port == 0 || video_plc_remote_port == 0)
            {
                simple_media_close_channels(media);
                return -EINVAL;
            }
            simple_media_fill_plc_cfg(&plc_cfg, cfg);
            plc_cfg.remote_ep.addr = video_plc_remote_addr;
            plc_cfg.remote_ep.port = video_plc_remote_port;
            plc_cfg.mcast_enabled = true;
            plc_cfg.mcast_opt.group_addr = video_plc_remote_addr;
            media->bridge_video_plc_ch = mtrans_channel_open(&plc_cfg);
            if (!media->bridge_video_plc_ch)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            simple_media_fill_udp_cfg(&audio_udp_cfg, true, cfg);
            audio_udp_cfg.remote_ep.addr = cfg->bridge_audio_udp_remote_addr;
            audio_udp_cfg.remote_ep.port = cfg->bridge_audio_udp_remote_port;
            audio_udp_cfg.rx_match_ep_enabled = false;
            audio_udp_cfg.mcast_enabled = false;
            media->bridge_audio_udp_ch = mtrans_channel_open(&audio_udp_cfg);
            if (!media->bridge_audio_udp_ch)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->udp_ch;
            rule.dst_channel = media->bridge_video_plc_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_VIDEO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->udp_ch;
            rule.dst_channel = media->plc_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->plc_ch;
            rule.dst_channel = media->udp_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_VIDEO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }

            memset(&rule, 0, sizeof(rule));
            rule.src_channel = media->plc_ch;
            rule.dst_channel = media->bridge_audio_udp_ch;
            rule.flags = MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY;
            if (mtrans_route_add_rule(&rule) != 0)
            {
                simple_media_close_channels(media);
                return -EIO;
            }
        }
        else if (mtrans_route_bridge_channels(media->udp_ch, media->plc_ch) != 0)
        {
            simple_media_close_channels(media);
            return -EIO;
        }
        else if (cfg->bidirectional_bridge &&
            mtrans_route_bridge_channels(media->plc_ch, media->udp_ch) != 0)
        {
            simple_media_close_channels(media);
            return -EIO;
        }
        ret = simple_media_open_audio_tx_channel(media, cfg);
        if (ret < 0)
        {
            simple_media_close_channels(media);
            return ret;
        }
        simple_media_note_channel_cfg(media, cfg);
        return 0;

    case SIMPLE_MEDIA_ENDPOINT_PLC_TO_UDP_BRIDGE:
        simple_media_fill_udp_cfg(&udp_cfg, true, cfg);
        simple_media_fill_plc_cfg(&plc_cfg, cfg);
        media->plc_ch = mtrans_channel_open(&plc_cfg);
        media->udp_ch = mtrans_channel_open(&udp_cfg);
        if (!media->udp_ch || !media->plc_ch)
        {
            simple_media_close_channels(media);
            return -EIO;
        }
        if (mtrans_route_bridge_channels(media->plc_ch, media->udp_ch) != 0)
        {
            simple_media_close_channels(media);
            return -EIO;
        }
        ret = simple_media_open_audio_tx_channel(media, cfg);
        if (ret < 0)
        {
            simple_media_close_channels(media);
            return ret;
        }
        simple_media_note_channel_cfg(media, cfg);
        return 0;

    default:
        return 0;
    }
}

int simple_media_open(simple_media_t *media, const simple_media_config_t *cfg)
{
    if (!media || !cfg)
    {
        return -EINVAL;
    }

    memset(media, 0, sizeof(*media));
    media->base_cfg = *cfg;
    media->configured = true;

    return 0;
}

int simple_media_start_bridge_with_config(simple_media_t *media,
                                          const simple_media_config_t *cfg)
{
    if (!media || !media->configured || !cfg ||
        !simple_media_is_bridge_endpoint(cfg->type))
    {
        return -EINVAL;
    }

    return simple_media_open_channels_with_config(media, cfg);
}

int simple_media_stop_bridge(simple_media_t *media)
{
    if (!media || !media->configured)
    {
        return -EINVAL;
    }
    if (!media->channel_cfg_valid ||
        !simple_media_is_bridge_endpoint(media->channel_cfg.type))
    {
        return 0;
    }

    simple_media_close_channels(media);
    return 0;
}

void simple_media_close(simple_media_t *media)
{
    if (!media)
    {
        return;
    }

    simple_media_stop_video(media);
    simple_media_stop_audio(media);
    simple_media_close_channels(media);
    memset(media, 0, sizeof(*media));
}

int simple_media_start_video_with_config(simple_media_t *media,
                                         const simple_media_config_t *cfg)
{
    mtrans_media_tx_params_t tx_params;
    mtrans_media_rx_params_t rx_params;
    mtrans_stream_rx_callbacks_t rx_cbs;
    mtrans_channel_t ch = NULL;
    bool video_rx_codec_started = false;
    int ret;

    if (!media || !cfg)
    {
        return -EINVAL;
    }

    if (media->video_tx || media->video_rx ||
        (!cfg->video_tx_enabled && !cfg->video_rx_enabled))
    {
        return 0;
    }

    ret = simple_media_open_channels_with_config(media, cfg);
    if (ret < 0)
    {
        return ret;
    }
    media->active_video_codec_ops = cfg->codec_ops;

    if (cfg->video_tx_enabled)
    {
        ch = media->udp_ch ? media->udp_ch : media->plc_ch;
        if (!ch)
        {
            simple_media_close_idle_endpoint_channels(media);
            media->active_video_codec_ops = NULL;
            return -EINVAL;
        }
        memset(&tx_params, 0, sizeof(tx_params));
        tx_params.ssrc = cfg->video_ssrc;
        tx_params.target_bitrate_bps = 100000;
        media->video_tx = mtrans_stream_tx_create(ch,
                                                  MTRANS_MEDIA_TYPE_VIDEO_H264,
                                                  &tx_params);
        if (!media->video_tx)
        {
            simple_media_close_idle_endpoint_channels(media);
            media->active_video_codec_ops = NULL;
            return -EIO;
        }
    }

    if (cfg->video_rx_enabled)
    {
        ch = media->udp_ch ? media->udp_ch : media->plc_ch;
        if (!ch)
        {
            if (media->video_tx)
            {
                mtrans_stream_tx_destroy(media->video_tx);
                media->video_tx = NULL;
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_video_codec_ops = NULL;
            return -EINVAL;
        }

        /*
         * RX stream 创建后，transport recv 线程可能立刻把已到达的媒体包分发进
         * on_video_frame_ready；因此解码侧必须先进入 running 状态，避免首帧
         * 先到、decoder 还没启动导致 indoor 回调里看到 dec->running == false。
         */
        if (simple_codec_start_video_rx(cfg->codec_ops) < 0)
        {
            if (media->video_tx)
            {
                mtrans_stream_tx_destroy(media->video_tx);
                media->video_tx = NULL;
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_video_codec_ops = NULL;
            return -EIO;
        }

        video_rx_codec_started = true;

        memset(&rx_params, 0, sizeof(rx_params));
        rx_params.expected_ssrc = cfg->video_rx_ssrc;
        rx_params.jitter_buffer_ms = 200;
        memset(&rx_cbs, 0, sizeof(rx_cbs));
        rx_cbs.on_video_frame_ready = simple_media_on_video_frame;
        rx_cbs.on_stream_event = simple_media_on_stream_event;
        media->video_rx = mtrans_stream_rx_create(ch, MTRANS_MEDIA_TYPE_VIDEO_H264,
                                                  &rx_params, &rx_cbs, media);

        if (!media->video_rx)
        {
            if (video_rx_codec_started)
            {
                simple_codec_stop_video_rx(cfg->codec_ops);
                video_rx_codec_started = false;
            }
            if (media->video_tx)
            {
                mtrans_stream_tx_destroy(media->video_tx);
                media->video_tx = NULL;
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_video_codec_ops = NULL;
            return -EIO;
        }
    }

    if (media->video_tx)
    {
        if (simple_codec_start_video_tx(cfg->codec_ops) < 0)
        {
            if (media->video_rx)
            {
                mtrans_stream_rx_destroy(media->video_rx);
                media->video_rx = NULL;
                simple_codec_stop_video_rx(cfg->codec_ops);
            }
            if (media->video_tx)
            {
                mtrans_stream_tx_destroy(media->video_tx);
                media->video_tx = NULL;
            }
            media->active_video_codec_ops = NULL;
            simple_media_close_idle_endpoint_channels(media);
            return -EIO;
        }
    }

    return 0;
}

void simple_media_stop_video(simple_media_t *media)
{
    if (!media)
    {
        return;
    }

    if (media->video_tx)
    {
        simple_codec_stop_video_tx(media->active_video_codec_ops);
        mtrans_stream_tx_destroy(media->video_tx);
        media->video_tx = NULL;
    }
    if (media->video_rx)
    {
        mtrans_stream_rx_destroy(media->video_rx);
        media->video_rx = NULL;
        simple_codec_stop_video_rx(media->active_video_codec_ops);
    }
    media->active_video_codec_ops = NULL;
    simple_media_close_idle_endpoint_channels(media);
}

int simple_media_start_audio_with_config(simple_media_t *media,
                                         const simple_media_config_t *cfg)
{
    mtrans_media_tx_params_t tx_params;
    mtrans_media_rx_params_t rx_params;
    mtrans_stream_rx_callbacks_t rx_cbs;
    mtrans_channel_t ch = NULL;
    bool audio_rx_codec_started = false;
    int ret;

    if (!media || !cfg)
    {
        return -EINVAL;
    }

    if (media->audio_tx || media->audio_rx ||
        (!cfg->audio_tx_enabled && !cfg->audio_rx_enabled))
    {
        return 0;
    }

    ret = simple_media_open_channels_with_config(media, cfg);
    if (ret < 0)
    {
        return ret;
    }

    ret = simple_media_open_audio_tx_channel(media, cfg);
    if (ret < 0)
    {
        simple_media_close_idle_endpoint_channels(media);
        return ret;
    }
    media->active_audio_codec_ops = cfg->codec_ops;

    if (cfg->audio_rx_enabled)
    {
        /*
         * RX stream 创建后可能立刻收到媒体包，因此先启动 decoder，
         * 避免 on_audio_frame_ready 进入时解码侧还未 ready。
         */
        ret = simple_codec_start_audio_rx(cfg->codec_ops);
        if (ret < 0)
        {
            simple_media_close_idle_endpoint_channels(media);
            media->active_audio_codec_ops = NULL;
            return ret;
        }
        audio_rx_codec_started = true;
    }

    if (cfg->audio_tx_enabled)
    {
        ch = media->audio_tx_ch ? media->audio_tx_ch :
             (media->udp_ch ? media->udp_ch : media->plc_ch);
        if (!ch)
        {
            if (audio_rx_codec_started)
            {
                simple_codec_stop_audio_rx(cfg->codec_ops);
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_audio_codec_ops = NULL;
            return -EINVAL;
        }
        memset(&tx_params, 0, sizeof(tx_params));
        tx_params.ssrc = cfg->audio_ssrc;
        tx_params.target_bitrate_bps = 64000;
        media->audio_tx = mtrans_stream_tx_create(ch,
                                                  MTRANS_MEDIA_TYPE_AUDIO_OPUS,
                                                  &tx_params);
        if (!media->audio_tx)
        {
            if (audio_rx_codec_started)
            {
                simple_codec_stop_audio_rx(cfg->codec_ops);
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_audio_codec_ops = NULL;
            return -EIO;
        }
    }

    if (cfg->audio_rx_enabled)
    {
        ch = media->udp_ch ? media->udp_ch : media->plc_ch;
        if (!ch)
        {
            if (media->audio_tx)
            {
                mtrans_stream_tx_destroy(media->audio_tx);
                media->audio_tx = NULL;
            }
            if (audio_rx_codec_started)
            {
                simple_codec_stop_audio_rx(cfg->codec_ops);
            }
            media->active_audio_codec_ops = NULL;
            simple_media_close_idle_endpoint_channels(media);
            return -EINVAL;
        }

        memset(&rx_params, 0, sizeof(rx_params));
        rx_params.expected_ssrc = cfg->audio_rx_ssrc;
        rx_params.jitter_buffer_ms = 300;
        memset(&rx_cbs, 0, sizeof(rx_cbs));
        rx_cbs.on_audio_frame_ready = simple_media_on_audio_frame;
        media->audio_rx = mtrans_stream_rx_create(ch, MTRANS_MEDIA_TYPE_AUDIO_OPUS,
                                                  &rx_params, &rx_cbs, media);
        if (!media->audio_rx)
        {
            if (media->audio_tx)
            {
                mtrans_stream_tx_destroy(media->audio_tx);
                media->audio_tx = NULL;
            }
            if (audio_rx_codec_started)
            {
                simple_codec_stop_audio_rx(cfg->codec_ops);
            }
            simple_media_close_idle_endpoint_channels(media);
            media->active_audio_codec_ops = NULL;
            return -EIO;
        }
    }

    if (cfg->audio_tx_enabled)
    {
        /*
         * Capture callback may immediately push encoded frames, so start the
         * encoder only after audio_tx has been created.
         */
        ret = simple_codec_start_audio_tx(cfg->codec_ops);
        if (ret < 0)
        {
            simple_media_stop_audio(media);
            return ret;
        }
    }

    return 0;
}

void simple_media_stop_audio(simple_media_t *media)
{
    if (!media)
    {
        return;
    }

    if (media->audio_tx)
    {
        simple_codec_stop_audio_tx(media->active_audio_codec_ops);
        mtrans_stream_tx_destroy(media->audio_tx);
        media->audio_tx = NULL;
    }
    if (media->audio_rx)
    {
        mtrans_stream_rx_destroy(media->audio_rx);
        media->audio_rx = NULL;
        simple_codec_stop_audio_rx(media->active_audio_codec_ops);
    }
    media->active_audio_codec_ops = NULL;
    simple_media_close_idle_endpoint_channels(media);
}
