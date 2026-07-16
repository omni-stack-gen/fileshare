#include "simple_endpoint_provider.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

#include "simple_log.h"

static bool sep_media_active(const simple_media_endpoint_request_t *request, uint8_t mask)
{
    return request && request->profile && ((request->profile->media_mask & mask) != 0);
}

static bool sep_device_id_equal(const char *a, const char *b)
{
    return IS_VALID_STRING(a) && IS_VALID_STRING(b) && strcmp(a, b) == 0;
}

static bool sep_is_manage_call(const simple_endpoint_provider_config_t *cfg,
                               const simple_media_endpoint_request_t *request)
{
    return cfg && request && IS_VALID_STRING(cfg->manage_device_id) &&
           (sep_device_id_equal(request->caller_device_id, cfg->manage_device_id) ||
            sep_device_id_equal(request->callee_device_id, cfg->manage_device_id));
}

static const char *sep_role_log_name(simple_endpoint_provider_role_t role)
{
    switch (role)
    {
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE:
        return "manage";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR:
        return "indoor";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR:
        return "door";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_SECONDARY_CONFIRM:
        return "secondary_confirm";
    default:
        return "host";
    }
}

static const char *sep_role_debug_name(simple_endpoint_provider_role_t role)
{
    switch (role)
    {
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE:
        return "MANAGE_ENDPOINT_PROVIDER";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR:
        return "INDOOR_ENDPOINT_PROVIDER";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR:
        return "DOOR_ENDPOINT_PROVIDER";
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_SECONDARY_CONFIRM:
        return "SECONDARY_CONFIRM_ENDPOINT_PROVIDER";
    default:
        return "HOST_ENDPOINT_PROVIDER";
    }
}

static uint8_t sep_call_link_from_topology(topology_link_kind_t kind)
{
    return kind == TOPOLOGY_LINK_PLC ?
           (uint8_t)CALL_MEDIA_ENDPOINT_LINK_PLC :
           (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP;
}

static void sep_fill_endpoint(call_media_endpoint_t *endpoint,
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

static const call_media_endpoint_t *sep_peer_video_endpoint(
    const simple_media_endpoint_request_t *request)
{
    if (!request || !request->peer_profile ||
        !request->peer_profile->has_video_endpoint)
    {
        return NULL;
    }
    if (request->peer_profile->video_endpoint.link_kind !=
            (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP ||
        request->peer_profile->video_endpoint.cast_mode !=
            (uint8_t)CALL_MEDIA_CAST_MULTICAST ||
        request->peer_profile->video_endpoint.port == 0 ||
        request->peer_profile->video_endpoint.addr == ADDR_ANY)
    {
        return NULL;
    }
    return &request->peer_profile->video_endpoint;
}

static const call_media_endpoint_t *sep_peer_udp_video_endpoint(
    const simple_media_endpoint_request_t *request)
{
    if (!request || !request->peer_profile ||
        !request->peer_profile->has_video_endpoint)
    {
        return NULL;
    }
    if (request->peer_profile->video_endpoint.link_kind !=
            (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP ||
        (request->peer_profile->video_endpoint.cast_mode !=
             (uint8_t)CALL_MEDIA_CAST_MULTICAST &&
         request->peer_profile->video_endpoint.cast_mode !=
             (uint8_t)CALL_MEDIA_CAST_UNICAST) ||
        request->peer_profile->video_endpoint.port == 0 ||
        request->peer_profile->video_endpoint.addr == ADDR_ANY)
    {
        return NULL;
    }
    return &request->peer_profile->video_endpoint;
}

static const call_media_endpoint_t *sep_peer_audio_endpoint(
    const simple_media_endpoint_request_t *request,
    bool require_udp)
{
    if (!request || !request->peer_profile ||
        !request->peer_profile->has_audio_endpoint ||
        request->peer_profile->audio_endpoint.port == 0 ||
        request->peer_profile->audio_endpoint.addr == ADDR_ANY)
    {
        return NULL;
    }
    if (require_udp &&
        request->peer_profile->audio_endpoint.link_kind !=
            (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP)
    {
        return NULL;
    }
    return &request->peer_profile->audio_endpoint;
}

static int sep_bind(simple_endpoint_provider_t *provider,
                    uint64_t session_id,
                    uint8_t media_kind,
                    const char *logical_source_id,
                    const char *domain_id,
                    const char *gateway_id,
                    simple_media_session_endpoint_binding_t *binding_out)
{
    simple_media_session_endpoint_bind_cfg_t bind_cfg;

    memset(&bind_cfg, 0, sizeof(bind_cfg));
    bind_cfg.session_id = session_id;
    bind_cfg.media_kind = media_kind;
    snprintf(bind_cfg.logical_source_id, sizeof(bind_cfg.logical_source_id),
             "%s", logical_source_id ? logical_source_id : "");
    snprintf(bind_cfg.domain_id, sizeof(bind_cfg.domain_id),
             "%s", domain_id ? domain_id : "");
    snprintf(bind_cfg.gateway_id, sizeof(bind_cfg.gateway_id),
             "%s", gateway_id ? gateway_id : "");
    return simple_media_session_endpoint_bind(&provider->allocator,
                                              &bind_cfg,
                                              binding_out);
}

static void sep_fill_host_static_binding(simple_media_endpoint_binding_t *binding,
                                         bool video)
{
    uint16_t port = video ? SIMPLE_UDP_MEDIA_PORT : (uint16_t)(SIMPLE_UDP_MEDIA_PORT + 2U);

    if (!binding)
    {
        return;
    }

    memset(binding, 0, sizeof(*binding));
    binding->has_declared_endpoint = true;
    sep_fill_endpoint(&binding->declared_endpoint,
                      (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP,
                      (uint8_t)CALL_MEDIA_CAST_MULTICAST,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_STATIC,
                      SIMPLE_UDP_MEDIA_GROUP_ADDR,
                      port);
}

static bool sep_peer_udp_video_to_plc(const simple_media_endpoint_request_t *request,
                                      uint32_t *plc_group,
                                      uint16_t *plc_port,
                                      uint8_t *scope_slot_out,
                                      uint8_t *session_slot_out)
{
    const call_media_profile_t *profile;
    uint32_t udp_addr;
    uint16_t udp_port;
    uint16_t port_delta;
    uint8_t scope_slot;
    uint8_t session_slot;

    if (!request || !plc_group || !plc_port)
    {
        return false;
    }

    profile = request->peer_profile ? request->peer_profile : request->profile;
    if (!profile ||
        (profile->media_mask & CALL_MEDIA_MASK_VIDEO) == 0 ||
        !profile->has_video_endpoint ||
        profile->video_endpoint.link_kind != (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP ||
        profile->video_endpoint.cast_mode != (uint8_t)CALL_MEDIA_CAST_MULTICAST ||
        profile->video_endpoint.addr == 0 ||
        profile->video_endpoint.port < SIMPLE_UDP_MEDIA_PORT)
    {
        return false;
    }

    udp_addr = profile->video_endpoint.addr;
    udp_port = profile->video_endpoint.port;
    port_delta = (uint16_t)(udp_port - SIMPLE_UDP_MEDIA_PORT);
    if ((port_delta % 4U) != 0)
    {
        return false;
    }
    session_slot = (uint8_t)(port_delta / 4U);
    if (session_slot >= SIMPLE_MEDIA_SESSION_SLOT_MAX)
    {
        return false;
    }
    scope_slot = (uint8_t)((udp_addr >> 8) & 0xffU);
    if (scope_slot < SIMPLE_MEDIA_SCOPE_SLOT_MIN ||
        scope_slot > SIMPLE_MEDIA_SCOPE_SLOT_MAX)
    {
        return false;
    }

    *plc_group = simple_media_session_endpoint_plc_group(scope_slot, session_slot);
    *plc_port = simple_media_session_endpoint_plc_media_port();
    if (scope_slot_out)
    {
        *scope_slot_out = scope_slot;
    }
    if (session_slot_out)
    {
        *session_slot_out = session_slot;
    }
    return true;
}

static const call_media_endpoint_t *sep_peer_plc_video_endpoint(
    const simple_media_endpoint_request_t *request)
{
    const call_media_profile_t *profile;

    if (!request)
    {
        return NULL;
    }
    profile = request->peer_profile ? request->peer_profile : request->profile;
    if (!profile ||
        (profile->media_mask & CALL_MEDIA_MASK_VIDEO) == 0 ||
        !profile->has_video_endpoint ||
        profile->video_endpoint.link_kind != (uint8_t)CALL_MEDIA_ENDPOINT_LINK_PLC ||
        profile->video_endpoint.cast_mode != (uint8_t)CALL_MEDIA_CAST_MULTICAST ||
        profile->video_endpoint.addr == 0 ||
        profile->video_endpoint.port == 0)
    {
        return NULL;
    }
    return &profile->video_endpoint;
}

static int sep_resolve_video(simple_endpoint_provider_t *provider,
                             const simple_endpoint_provider_config_t *cfg,
                             const simple_media_endpoint_request_t *request,
                             simple_media_endpoint_result_t *result)
{
    simple_media_session_endpoint_binding_t binding;
    const call_media_endpoint_t *peer_video = NULL;
    const char *video_source = "local_alloc";
    const char *logical_source_id = request->self_device_id;
    const char *domain_id = "udp-lan";
    uint32_t video_group = 0;
    uint16_t video_port = 0;
    uint32_t tx_addr = 0;
    uint16_t tx_port = 0;
    uint8_t cast_mode = (uint8_t)CALL_MEDIA_CAST_MULTICAST;
    uint8_t scope_slot = 0;
    uint8_t session_slot = 0;
    uint8_t link_kind = (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP;
    int ret;

    if (!cfg->video_enabled || !sep_media_active(request, CALL_MEDIA_MASK_VIDEO))
    {
        return 0;
    }
    if (cfg->role == SIMPLE_ENDPOINT_PROVIDER_ROLE_HOST)
    {
        sep_fill_host_static_binding(&result->video, true);
        return 0;
    }

    memset(&binding, 0, sizeof(binding));
    switch (cfg->role)
    {
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE:
        peer_video = sep_peer_video_endpoint(request);
        domain_id = "udp-lan";
        logical_source_id = request->callee_device_id[0] ?
                            request->callee_device_id : request->self_device_id;
        break;
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR:
        if (cfg->static_udp_enabled)
        {
            domain_id = IS_VALID_STRING(cfg->static_udp_domain_id) ?
                        cfg->static_udp_domain_id : "spe-udp";
            logical_source_id = request->callee_device_id[0] ?
                                request->callee_device_id : request->self_device_id;
            peer_video = sep_peer_udp_video_endpoint(request);
            if (cfg->local_media_addr != 0 && peer_video &&
                peer_video->cast_mode == (uint8_t)CALL_MEDIA_CAST_UNICAST)
            {
                video_group = cfg->local_media_addr;
                video_port = SIMPLE_UDP_MEDIA_PORT;
                tx_addr = peer_video->addr;
                tx_port = peer_video->port;
                cast_mode = (uint8_t)CALL_MEDIA_CAST_UNICAST;
                video_source = "peer_profile_unicast";
                peer_video = NULL;
            }
            else if (cfg->local_media_addr != 0 && request->peer_id != 0)
            {
                video_group = cfg->local_media_addr;
                video_port = SIMPLE_UDP_MEDIA_PORT;
                tx_addr = request->peer_id;
                tx_port = SIMPLE_UDP_MEDIA_PORT;
                cast_mode = (uint8_t)CALL_MEDIA_CAST_UNICAST;
                video_source = "static_unicast";
                peer_video = NULL;
            }
            break;
        }
        link_kind = (uint8_t)CALL_MEDIA_ENDPOINT_LINK_PLC;
        domain_id = "plc";
        logical_source_id = request->callee_device_id[0] ?
                            request->callee_device_id : request->self_device_id;
        if (sep_peer_udp_video_to_plc(request, &video_group, &video_port,
                                      &scope_slot, &session_slot))
        {
            video_source = "peer_profile";
            peer_video = NULL;
            break;
        }
        peer_video = sep_peer_plc_video_endpoint(request);
        if (peer_video)
        {
            video_group = peer_video->addr;
            video_port = peer_video->port;
            video_source = "peer_profile";
            peer_video = NULL;
            break;
        }
        if (cfg->existing_plc_video_endpoint_enabled &&
            cfg->existing_plc_video_addr != 0 &&
            cfg->existing_plc_video_port != 0 &&
            cfg->video_desired_active)
        {
            uint32_t slot_id;

            video_group = cfg->existing_plc_video_addr;
            video_port = cfg->existing_plc_video_port;
            if ((video_group & 0xff0000U) == 0x120000U)
            {
                slot_id = video_group & 0xffU;
                scope_slot = (uint8_t)((video_group >> 8) & 0xffU);
                session_slot = slot_id > 0 ? (uint8_t)(slot_id - 1U) : 0;
            }
            video_source = "existing";
            break;
        }
        break;
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR:
        peer_video = sep_is_manage_call(cfg, request) ?
                     sep_peer_video_endpoint(request) : NULL;
        if (cfg->video_link_override_enabled)
        {
            link_kind = sep_call_link_from_topology(cfg->video_link_kind);
            domain_id = IS_VALID_STRING(cfg->video_domain_id) ?
                        cfg->video_domain_id :
                        (cfg->video_link_kind == TOPOLOGY_LINK_PLC ? "plc" : "udp-downlink");
        }
        else
        {
            domain_id = "udp-downlink";
        }
        logical_source_id = request->self_device_id;
        break;
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_SECONDARY_CONFIRM:
        domain_id = "spe-udp";
        logical_source_id = request->self_device_id;
        peer_video = sep_peer_udp_video_endpoint(request);
        if (cfg->local_media_addr != 0 && peer_video &&
            peer_video->cast_mode == (uint8_t)CALL_MEDIA_CAST_UNICAST)
        {
            video_group = cfg->local_media_addr;
            video_port = SIMPLE_UDP_MEDIA_PORT;
            tx_addr = peer_video->addr;
            tx_port = peer_video->port;
            cast_mode = (uint8_t)CALL_MEDIA_CAST_UNICAST;
            video_source = "peer_profile_unicast";
            peer_video = NULL;
        }
        else if (cfg->static_udp_enabled && cfg->local_media_addr != 0 &&
                 request->peer_id != 0)
        {
            video_group = cfg->local_media_addr;
            video_port = SIMPLE_UDP_MEDIA_PORT;
            tx_addr = request->peer_id;
            tx_port = SIMPLE_UDP_MEDIA_PORT;
            cast_mode = (uint8_t)CALL_MEDIA_CAST_UNICAST;
            video_source = "static_unicast";
            peer_video = NULL;
        }
        break;
    default:
        return -EINVAL;
    }

    if (peer_video)
    {
        video_group = peer_video->addr;
        video_port = peer_video->port;
        cast_mode = peer_video->cast_mode;
        video_source = "peer_profile";
    }
    else if (video_group == 0 || video_port == 0)
    {
        ret = sep_bind(provider, request->session_id, SIMPLE_MEDIA_KIND_VIDEO,
                       logical_source_id, domain_id, NULL, &binding);
        if (ret < 0)
        {
            return ret;
        }
        scope_slot = binding.scope_slot;
        session_slot = binding.session_slot;
        if (link_kind == (uint8_t)CALL_MEDIA_ENDPOINT_LINK_PLC)
        {
            video_group = simple_media_session_endpoint_plc_group(binding.scope_slot,
                                                                  binding.session_slot);
            video_port = simple_media_session_endpoint_plc_media_port();
        }
        else
        {
            video_group = simple_media_session_endpoint_udp_group(binding.scope_slot,
                                                                  binding.session_slot);
            video_port = simple_media_session_endpoint_udp_video_port(binding.session_slot);
        }
    }

    result->video.has_declared_endpoint = true;
    sep_fill_endpoint(&result->video.declared_endpoint,
                      link_kind,
                      cast_mode,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_SESSION,
                      video_group,
                      video_port);
    result->video.has_tx_remote_endpoint = true;
    result->video.tx_remote_endpoint = result->video.declared_endpoint;
    if (tx_addr != 0 && tx_port != 0)
    {
        result->video.tx_remote_endpoint.addr = tx_addr;
        result->video.tx_remote_endpoint.port = tx_port;
    }
    result->video.has_rx_match_endpoint = true;
    result->video.rx_match_addr =
        (cast_mode == (uint8_t)CALL_MEDIA_CAST_UNICAST && tx_addr != 0) ?
            tx_addr : ADDR_ANY;
    result->video.rx_match_port = video_port;

    if (cfg->role == SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR && !cfg->static_udp_enabled)
    {
        SIMPLE_LOGI("indoor video endpoint: session=0x%" PRIx64
                    " plc=0x%x:%u rx_match=0x%x:%u scope=%u slot=%u source=%s\n",
                    request->session_id, video_group, video_port,
                    result->video.rx_match_addr, result->video.rx_match_port,
                    scope_slot, session_slot, video_source);
    }
    else
    {
        SIMPLE_LOGI("%s video endpoint: session=0x%" PRIx64
                    " udp=0x%x:%u rx_match=0x%x:%u scope=%u slot=%u source=%s\n",
                    sep_role_log_name(cfg->role),
                    request->session_id, video_group, video_port,
                    result->video.rx_match_addr, result->video.rx_match_port,
                    scope_slot, session_slot, video_source);
    }
    return 0;
}

static int sep_resolve_static_udp_audio(simple_endpoint_provider_t *provider,
                                        const simple_endpoint_provider_config_t *cfg,
                                        const simple_media_endpoint_request_t *request,
                                        simple_media_endpoint_result_t *result,
                                        bool require_udp_peer)
{
    simple_media_session_endpoint_binding_t binding;
    const call_media_endpoint_t *peer_audio;
    uint32_t tx_addr;
    uint16_t tx_port;
    int ret;

    tx_addr = request->peer_id;
    tx_port = SIMPLE_UDP_MEDIA_PORT;
    peer_audio = sep_peer_audio_endpoint(request, require_udp_peer);
    if (peer_audio)
    {
        tx_addr = peer_audio->addr;
        tx_port = peer_audio->port;
    }
    if (cfg->local_media_addr == 0 || tx_addr == 0 || tx_port == 0)
    {
        return -EINVAL;
    }

    memset(&binding, 0, sizeof(binding));
    ret = sep_bind(provider, request->session_id, SIMPLE_MEDIA_KIND_AUDIO,
                   request->caller_device_id[0] ?
                       request->caller_device_id : request->self_device_id,
                   "udp-lan", NULL, &binding);
    if (ret < 0)
    {
        return ret;
    }

    result->audio.has_declared_endpoint = true;
    sep_fill_endpoint(&result->audio.declared_endpoint,
                      (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP,
                      (uint8_t)CALL_MEDIA_CAST_UNICAST,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_STATIC,
                      cfg->local_media_addr,
                      SIMPLE_UDP_MEDIA_PORT);
    result->audio.has_tx_remote_endpoint = true;
    sep_fill_endpoint(&result->audio.tx_remote_endpoint,
                      (uint8_t)CALL_MEDIA_ENDPOINT_LINK_UDP,
                      (uint8_t)CALL_MEDIA_CAST_UNICAST,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_STATIC,
                      tx_addr,
                      tx_port);
    result->audio.has_rx_match_endpoint = true;
    result->audio.rx_match_addr = tx_addr;
    result->audio.rx_match_port = tx_port;

    if (cfg->role != SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR)
    {
        SIMPLE_LOGD("%s media=audio session=0x%" PRIx64
                    " slot=%u scope=%u source=static declared=0x%x:%u tx=0x%x:%u\n",
                    sep_role_debug_name(cfg->role),
                    request->session_id, binding.session_slot, binding.scope_slot,
                    cfg->local_media_addr, SIMPLE_UDP_MEDIA_PORT,
                    tx_addr, tx_port);
    }
    return 0;
}

static int sep_resolve_topology_audio(simple_endpoint_provider_t *provider,
                                      const simple_endpoint_provider_config_t *cfg,
                                      const simple_media_endpoint_request_t *request,
                                      simple_media_endpoint_result_t *result,
                                      const char *default_domain_id)
{
    simple_media_session_endpoint_binding_t binding;
    topology_audio_endpoint_result_t audio;
    topology_endpoint_t local_endpoint;
    const char *domain_id;
    int ret;

    if (!cfg->audio_resolver)
    {
        return -ENOENT;
    }
    memset(&audio, 0, sizeof(audio));
    memset(&local_endpoint, 0, sizeof(local_endpoint));
    ret = cfg->audio_resolver(cfg->audio_resolver_ctx, request, &audio, &local_endpoint);
    if (ret < 0)
    {
        return ret;
    }
    if (local_endpoint.addr == ADDR_ANY || local_endpoint.port == 0 ||
        audio.tx_endpoint.port == 0 || audio.rx_match_endpoint.port == 0)
    {
        return -EINVAL;
    }

    domain_id = audio.tx_endpoint.domain_id[0] ?
                audio.tx_endpoint.domain_id : default_domain_id;
    ret = sep_bind(provider, request->session_id, SIMPLE_MEDIA_KIND_AUDIO,
                   request->caller_device_id[0] ?
                       request->caller_device_id : request->self_device_id,
                   domain_id, audio.gateway_device_id, &binding);
    if (ret < 0)
    {
        return ret;
    }

    result->audio.has_declared_endpoint = true;
    sep_fill_endpoint(&result->audio.declared_endpoint,
                      sep_call_link_from_topology(local_endpoint.link_kind),
                      (uint8_t)CALL_MEDIA_CAST_UNICAST,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_TOPOLOGY,
                      local_endpoint.addr,
                      local_endpoint.port);
    result->audio.has_tx_remote_endpoint = true;
    sep_fill_endpoint(&result->audio.tx_remote_endpoint,
                      sep_call_link_from_topology(audio.tx_endpoint.link_kind),
                      (uint8_t)CALL_MEDIA_CAST_UNICAST,
                      (uint8_t)CALL_MEDIA_ENDPOINT_SOURCE_TOPOLOGY,
                      audio.tx_endpoint.addr,
                      audio.tx_endpoint.port);
    result->audio.has_rx_match_endpoint = true;
    result->audio.rx_match_addr = audio.rx_match_endpoint.addr;
    result->audio.rx_match_port = audio.rx_match_endpoint.port;

    if (cfg->role == SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR ||
        cfg->role == SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR)
    {
        SIMPLE_LOGD("%s media=audio session=0x%" PRIx64
                    " slot=%u scope=%u source=topology gateway=%s declared=0x%x:%u "
                    "tx=0x%x:%u explain=\"%s\"\n",
                    sep_role_debug_name(cfg->role),
                    request->session_id, binding.session_slot, binding.scope_slot,
                    audio.gateway_device_id[0] ? audio.gateway_device_id : "-",
                    local_endpoint.addr, local_endpoint.port,
                    audio.tx_endpoint.addr, audio.tx_endpoint.port,
                    audio.explain);
    }
    return 0;
}

static int sep_resolve_audio(simple_endpoint_provider_t *provider,
                             const simple_endpoint_provider_config_t *cfg,
                             const simple_media_endpoint_request_t *request,
                             simple_media_endpoint_result_t *result)
{
    if (!cfg->audio_enabled || !sep_media_active(request, CALL_MEDIA_MASK_AUDIO))
    {
        return 0;
    }

    switch (cfg->role)
    {
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE:
        return sep_resolve_static_udp_audio(provider, cfg, request, result, true);
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_SECONDARY_CONFIRM:
        return sep_resolve_static_udp_audio(provider, cfg, request, result, false);
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR:
        if (cfg->static_udp_enabled)
        {
            return sep_resolve_static_udp_audio(provider, cfg, request, result, false);
        }
        return sep_resolve_topology_audio(provider, cfg, request, result, "plc");
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR:
        if (sep_is_manage_call(cfg, request))
        {
            return sep_resolve_static_udp_audio(provider, cfg, request, result, false);
        }
        return sep_resolve_topology_audio(provider, cfg, request, result, "udp-downlink");
    case SIMPLE_ENDPOINT_PROVIDER_ROLE_HOST:
        if (cfg->audio_resolver)
        {
            return sep_resolve_topology_audio(provider, cfg, request, result, "udp-lan");
        }
        sep_fill_host_static_binding(&result->audio, false);
        return 0;
    default:
        return -EINVAL;
    }
}

int simple_endpoint_provider_init(simple_endpoint_provider_t *provider)
{
    if (!provider)
    {
        return -EINVAL;
    }
    simple_media_session_endpoint_allocator_init(&provider->allocator);
    return 0;
}

int simple_endpoint_provider_resolve(
    simple_endpoint_provider_t *provider,
    const simple_endpoint_provider_config_t *cfg,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result)
{
    int ret;

    if (!provider || !cfg || !request || !request->profile || !result)
    {
        return -EINVAL;
    }
    if (cfg->role == SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE &&
        cfg->local_media_addr == 0)
    {
        return -EINVAL;
    }

    memset(result, 0, sizeof(*result));
    ret = sep_resolve_video(provider, cfg, request, result);
    if (ret < 0)
    {
        return ret;
    }
    return sep_resolve_audio(provider, cfg, request, result);
}

void simple_endpoint_provider_release_session(simple_endpoint_provider_t *provider,
                                              uint64_t session_id)
{
    if (!provider)
    {
        return;
    }
    simple_media_session_endpoint_release_session(&provider->allocator, session_id);
}
