/**
 * @file
 * @brief simple 示例应用公共模块 simple_topology_audio 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_topology_audio.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

#include "simple_log.h"

static void simple_topology_audio_log_cb(void *user_ctx, const char *line)
{
    (void)user_ctx;

    if (line)
    {
        SIMPLE_LOGI("%s\n", line);
    }
}

int simple_topology_audio_init(simple_topology_audio_t *ctx,
                               const char *node_name,
                               const char *self_device_id)
{
    if (!ctx || !node_name || !self_device_id)
    {
        return -EINVAL;
    }

    memset(ctx, 0, sizeof(*ctx));
    safe_strncpy(ctx->node_name, node_name, sizeof(ctx->node_name));
    safe_strncpy(ctx->self_device_id, self_device_id, sizeof(ctx->self_device_id));
    return 0;
}

void simple_topology_audio_deinit(simple_topology_audio_t *ctx)
{
    if (!ctx)
    {
        return;
    }

    if (ctx->topology)
    {
        topology_destroy(ctx->topology);
        ctx->topology = NULL;
    }
    ctx->enabled = false;
}

int simple_topology_audio_resolve_binding(
    simple_topology_audio_t *ctx,
    const simple_media_endpoint_request_t *request,
    topology_link_kind_t expected_link_kind,
    uint16_t expected_rx_port,
    topology_audio_endpoint_result_t *audio_out,
    topology_endpoint_t *local_endpoint_out)
{
    simple_topology_profile_prepare_audio_t cfg;
    int ret;

    if (!ctx || !ctx->enabled || !ctx->topology)
    {
        return -ENOENT;
    }
    if (!request || !audio_out || !local_endpoint_out)
    {
        return -EINVAL;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.node_name = ctx->node_name;
    cfg.topology = ctx->topology;
    cfg.local_device_id = request->self_device_id;
    cfg.caller_device_id = request->caller_device_id;
    cfg.callee_device_id = request->callee_device_id;
    cfg.session_id = request->session_id;
    cfg.msg_type = request->msg_type;
    cfg.expected_tx_link_kind = expected_link_kind;
    cfg.validate_rx_port = expected_rx_port != 0;
    cfg.expected_rx_port = expected_rx_port;
    cfg.log_cb = simple_topology_audio_log_cb;
    cfg.audio_out = audio_out;

    ret = simple_topology_profile_prepare_audio(&cfg);
    if (ret < 0)
    {
        return ret;
    }
    return simple_topology_profile_find_local_endpoint(ctx->topology,
                                                       request->self_device_id,
                                                       &audio_out->tx_endpoint,
                                                       local_endpoint_out);
}
