#include "simple_media_session_endpoint.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

static bool session_endpoint_str_eq(const char *a, const char *b)
{
    return strcmp(a ? a : "", b ? b : "") == 0;
}

static uint32_t session_endpoint_hash_update(uint32_t hash, const char *s)
{
    const unsigned char *p = (const unsigned char *)(s ? s : "");

    while (*p)
    {
        hash ^= (uint32_t)(*p++);
        hash *= 16777619U;
    }
    hash ^= (uint32_t)'|';
    hash *= 16777619U;
    return hash;
}

uint8_t simple_media_session_endpoint_hash_scope(const char *domain_id,
                                                 const char *gateway_id,
                                                 const char *logical_source_id)
{
    uint32_t hash = 2166136261U;

    hash = session_endpoint_hash_update(hash, domain_id);
    hash = session_endpoint_hash_update(hash, gateway_id);
    hash = session_endpoint_hash_update(hash, logical_source_id);
    return (uint8_t)((hash % SIMPLE_MEDIA_SCOPE_SLOT_MAX) + SIMPLE_MEDIA_SCOPE_SLOT_MIN);
}

void simple_media_session_endpoint_allocator_init(
    simple_media_session_endpoint_allocator_t *allocator)
{
    if (allocator)
    {
        memset(allocator, 0, sizeof(*allocator));
    }
}

static bool session_endpoint_same_base(
    const simple_media_session_endpoint_binding_t *binding,
    const simple_media_session_endpoint_bind_cfg_t *cfg)
{
    return binding && cfg &&
           binding->session_id == cfg->session_id &&
           session_endpoint_str_eq(binding->logical_source_id, cfg->logical_source_id) &&
           session_endpoint_str_eq(binding->domain_id, cfg->domain_id) &&
           session_endpoint_str_eq(binding->gateway_id, cfg->gateway_id);
}

static bool session_endpoint_same_media_key(
    const simple_media_session_endpoint_binding_t *binding,
    const simple_media_session_endpoint_bind_cfg_t *cfg)
{
    return session_endpoint_same_base(binding, cfg) &&
           binding->media_kind == cfg->media_kind;
}

static bool session_endpoint_conflicts(
    const simple_media_session_endpoint_binding_t *binding,
    const simple_media_session_endpoint_bind_cfg_t *cfg)
{
    return binding && cfg &&
           binding->session_id == cfg->session_id &&
           binding->media_kind == cfg->media_kind &&
           session_endpoint_str_eq(binding->logical_source_id, cfg->logical_source_id) &&
           (!session_endpoint_str_eq(binding->domain_id, cfg->domain_id) ||
            !session_endpoint_str_eq(binding->gateway_id, cfg->gateway_id));
}

static bool session_endpoint_slot_used(
    const simple_media_session_endpoint_allocator_t *allocator,
    uint8_t session_slot)
{
    size_t i;

    for (i = 0; i < sizeof(allocator->entries) / sizeof(allocator->entries[0]); i++)
    {
        if (allocator->entries[i].in_use &&
            allocator->entries[i].binding.session_slot == session_slot)
        {
            return true;
        }
    }
    return false;
}

static int session_endpoint_pick_slot(
    const simple_media_session_endpoint_allocator_t *allocator,
    const simple_media_session_endpoint_bind_cfg_t *cfg,
    uint8_t *slot_out)
{
    uint8_t slot;
    size_t i;

    for (i = 0; i < sizeof(allocator->entries) / sizeof(allocator->entries[0]); i++)
    {
        if (allocator->entries[i].in_use &&
            session_endpoint_same_base(&allocator->entries[i].binding, cfg))
        {
            *slot_out = allocator->entries[i].binding.session_slot;
            return 0;
        }
    }

    for (slot = 0; slot < SIMPLE_MEDIA_SESSION_SLOT_MAX; slot++)
    {
        if (!session_endpoint_slot_used(allocator, slot))
        {
            *slot_out = slot;
            return 0;
        }
    }
    return -ENOSPC;
}

int simple_media_session_endpoint_bind(
    simple_media_session_endpoint_allocator_t *allocator,
    const simple_media_session_endpoint_bind_cfg_t *cfg,
    simple_media_session_endpoint_binding_t *binding_out)
{
    simple_media_session_endpoint_entry_t *free_entry = NULL;
    uint8_t session_slot = 0;
    uint8_t scope_slot;
    bool scope_derived;
    size_t i;
    int ret;

    if (!allocator || !cfg || cfg->session_id == 0 ||
        (cfg->media_kind != SIMPLE_MEDIA_KIND_VIDEO &&
         cfg->media_kind != SIMPLE_MEDIA_KIND_AUDIO) ||
        cfg->logical_source_id[0] == '\0' || cfg->domain_id[0] == '\0')
    {
        return -EINVAL;
    }

    for (i = 0; i < sizeof(allocator->entries) / sizeof(allocator->entries[0]); i++)
    {
        simple_media_session_endpoint_entry_t *entry = &allocator->entries[i];

        if (!entry->in_use)
        {
            if (!free_entry)
            {
                free_entry = entry;
            }
            continue;
        }
        if (session_endpoint_same_media_key(&entry->binding, cfg))
        {
            if (binding_out)
            {
                *binding_out = entry->binding;
            }
            return 0;
        }
        if (session_endpoint_conflicts(&entry->binding, cfg))
        {
            return -EEXIST;
        }
    }

    if (!free_entry)
    {
        return -ENOSPC;
    }

    ret = session_endpoint_pick_slot(allocator, cfg, &session_slot);
    if (ret < 0)
    {
        return ret;
    }

    scope_derived = cfg->preferred_scope_slot == 0;
    scope_slot = cfg->preferred_scope_slot != 0 ?
                 cfg->preferred_scope_slot :
                 simple_media_session_endpoint_hash_scope(cfg->domain_id,
                                                          cfg->gateway_id,
                                                          cfg->logical_source_id);

    memset(free_entry, 0, sizeof(*free_entry));
    free_entry->in_use = true;
    free_entry->binding.session_id = cfg->session_id;
    free_entry->binding.media_kind = cfg->media_kind;
    safe_strncpy(free_entry->binding.logical_source_id,
                 cfg->logical_source_id,
                 sizeof(free_entry->binding.logical_source_id));
    safe_strncpy(free_entry->binding.domain_id,
                 cfg->domain_id,
                 sizeof(free_entry->binding.domain_id));
    safe_strncpy(free_entry->binding.gateway_id,
                 cfg->gateway_id,
                 sizeof(free_entry->binding.gateway_id));
    free_entry->binding.session_slot = session_slot;
    free_entry->binding.scope_slot = scope_slot;
    free_entry->binding.scope_derived = scope_derived;

    if (binding_out)
    {
        *binding_out = free_entry->binding;
    }
    return 0;
}

void simple_media_session_endpoint_release_session(
    simple_media_session_endpoint_allocator_t *allocator,
    uint64_t session_id)
{
    size_t i;

    if (!allocator || session_id == 0)
    {
        return;
    }

    for (i = 0; i < sizeof(allocator->entries) / sizeof(allocator->entries[0]); i++)
    {
        if (allocator->entries[i].in_use &&
            allocator->entries[i].binding.session_id == session_id)
        {
            memset(&allocator->entries[i], 0, sizeof(allocator->entries[i]));
        }
    }
}

uint32_t simple_media_session_endpoint_udp_group(uint8_t scope_slot,
                                                 uint8_t session_slot)
{
    return ((uint32_t)239U << 24) |
           ((uint32_t)10U << 16) |
           ((uint32_t)scope_slot << 8) |
           ((uint32_t)session_slot + 1U);
}

uint16_t simple_media_session_endpoint_udp_video_port(uint8_t session_slot)
{
    return (uint16_t)(SIMPLE_UDP_MEDIA_PORT + ((uint16_t)session_slot * 4U));
}

uint16_t simple_media_session_endpoint_udp_audio_port(uint8_t session_slot)
{
    return (uint16_t)(SIMPLE_UDP_MEDIA_PORT + 2U + ((uint16_t)session_slot * 4U));
}

uint32_t simple_media_session_endpoint_plc_group(uint8_t scope_slot,
                                                 uint8_t session_slot)
{
    return 0x120000U | ((uint32_t)scope_slot << 8) |
           ((uint32_t)session_slot + 1U);
}

uint16_t simple_media_session_endpoint_plc_media_port(void)
{
    return SIMPLE_PLC_PORT_MEDIA;
}
