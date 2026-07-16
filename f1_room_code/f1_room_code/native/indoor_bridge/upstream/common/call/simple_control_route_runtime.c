#include "simple_control_route_runtime.h"

#include <errno.h>
#include <string.h>

#include <utils/util.h>

static bool route_device_id_equal(const char *a, const char *b)
{
    return IS_VALID_STRING(a) && IS_VALID_STRING(b) && strcmp(a, b) == 0;
}

static bool route_source_valid(simple_control_route_source_t source)
{
    return source >= SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP &&
           source <= SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND;
}

static unsigned route_source_priority(simple_control_route_source_t source)
{
    switch (source)
    {
    case SIMPLE_CONTROL_ROUTE_SOURCE_MANUAL:
        return 70U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER:
        return 60U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_REGISTRY:
        return 50U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_DIRECTORY:
        return 40U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND:
        return 30U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_TOPOLOGY:
        return 20U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP:
        return 10U;
    case SIMPLE_CONTROL_ROUTE_SOURCE_NONE:
    default:
        return 0U;
    }
}

static simple_control_route_runtime_entry_t *route_find_entry(
    simple_control_route_runtime_t *runtime,
    simple_control_route_source_t source,
    const char *callee_device_id)
{
    size_t i;

    if (!runtime || !route_source_valid(source) || !IS_VALID_STRING(callee_device_id))
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        simple_control_route_runtime_entry_t *entry = &runtime->entries[i];

        if (entry->online &&
            entry->source == source &&
            route_device_id_equal(entry->callee_device_id, callee_device_id))
        {
            return entry;
        }
    }
    return NULL;
}

static simple_control_route_runtime_entry_t *route_find_free(
    simple_control_route_runtime_t *runtime)
{
    size_t i;

    if (!runtime)
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        if (!runtime->entries[i].online)
        {
            return &runtime->entries[i];
        }
    }
    return NULL;
}

void simple_control_route_runtime_init(simple_control_route_runtime_t *runtime,
                                       const simple_control_route_runtime_config_t *cfg)
{
    if (!runtime)
    {
        return;
    }
    memset(runtime, 0, sizeof(*runtime));
    if (cfg)
    {
        runtime->cfg = *cfg;
    }
    runtime->next_seq = 1U;
}

void simple_control_route_runtime_clear(simple_control_route_runtime_t *runtime)
{
    if (!runtime)
    {
        return;
    }
    memset(runtime->entries, 0, sizeof(runtime->entries));
    runtime->next_seq = 1U;
}

const char *simple_control_route_source_name(simple_control_route_source_t source)
{
    switch (source)
    {
    case SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP:
        return "static_bootstrap";
    case SIMPLE_CONTROL_ROUTE_SOURCE_DIRECTORY:
        return "directory";
    case SIMPLE_CONTROL_ROUTE_SOURCE_REGISTRY:
        return "registry";
    case SIMPLE_CONTROL_ROUTE_SOURCE_TOPOLOGY:
        return "topology";
    case SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER:
        return "observed_peer";
    case SIMPLE_CONTROL_ROUTE_SOURCE_MANUAL:
        return "manual";
    case SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND:
        return "backend";
    case SIMPLE_CONTROL_ROUTE_SOURCE_NONE:
    default:
        return "none";
    }
}

int simple_control_route_runtime_load_bootstrap(
    simple_control_route_runtime_t *runtime,
    const simple_route_entry_t *routes,
    size_t route_count)
{
    size_t i;
    int ret;

    if (!runtime || (!routes && route_count > 0))
    {
        return -EINVAL;
    }
    for (i = 0; i < route_count; ++i)
    {
        simple_control_route_fact_t fact;

        memset(&fact, 0, sizeof(fact));
        fact.source = SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP;
        fact.callee_device_id = routes[i].callee_device_id;
        fact.online = true;
        fact.next_hop = routes[i].next_hop;
        ret = simple_control_route_runtime_apply_fact(runtime, &fact);
        if (ret != 0)
        {
            return ret;
        }
    }
    return 0;
}

int simple_control_route_runtime_apply_fact(
    simple_control_route_runtime_t *runtime,
    const simple_control_route_fact_t *fact)
{
    simple_control_route_runtime_entry_t *entry;

    if (!runtime || !fact || !route_source_valid(fact->source) ||
        !IS_VALID_STRING(fact->callee_device_id))
    {
        return -EINVAL;
    }

    entry = route_find_entry(runtime, fact->source, fact->callee_device_id);
    if (!fact->online)
    {
        if (entry)
        {
            memset(entry, 0, sizeof(*entry));
        }
        return 0;
    }

    if (!entry)
    {
        entry = route_find_free(runtime);
        if (!entry)
        {
            return -ENOSPC;
        }
        memset(entry, 0, sizeof(*entry));
        entry->source = fact->source;
        entry->seq = runtime->next_seq++;
        if (runtime->next_seq == 0)
        {
            runtime->next_seq = 1U;
        }
        if (safe_strncpy(entry->callee_device_id,
                         fact->callee_device_id,
                         sizeof(entry->callee_device_id)) >=
            sizeof(entry->callee_device_id))
        {
            memset(entry, 0, sizeof(*entry));
            return -ENOSPC;
        }
    }

    entry->next_hop = fact->next_hop;
    entry->observed_ms = fact->observed_ms;
    entry->ttl_ms = fact->ttl_ms;
    entry->online = true;
    return 0;
}

int simple_control_route_runtime_apply_online(
    simple_control_route_runtime_t *runtime,
    simple_control_route_source_t source,
    const char *callee_device_id,
    const call_endpoint_t *next_hop,
    uint64_t observed_ms,
    uint32_t ttl_ms)
{
    simple_control_route_fact_t fact;

    if (!next_hop)
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
    return simple_control_route_runtime_apply_fact(runtime, &fact);
}

int simple_control_route_runtime_apply_offline(
    simple_control_route_runtime_t *runtime,
    simple_control_route_source_t source,
    const char *callee_device_id)
{
    simple_control_route_fact_t fact;

    memset(&fact, 0, sizeof(fact));
    fact.source = source;
    fact.callee_device_id = callee_device_id;
    fact.online = false;
    return simple_control_route_runtime_apply_fact(runtime, &fact);
}

size_t simple_control_route_runtime_expire(simple_control_route_runtime_t *runtime,
                                           uint64_t now_ms)
{
    size_t expired = 0;
    size_t i;

    if (!runtime)
    {
        return 0;
    }
    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        simple_control_route_runtime_entry_t *entry = &runtime->entries[i];

        if (!entry->online || entry->ttl_ms == 0 || entry->observed_ms == 0)
        {
            continue;
        }
        if (now_ms >= entry->observed_ms &&
            now_ms - entry->observed_ms >= entry->ttl_ms)
        {
            memset(entry, 0, sizeof(*entry));
            expired++;
        }
    }
    return expired;
}

size_t simple_control_route_runtime_count(const simple_control_route_runtime_t *runtime)
{
    size_t count = 0;
    size_t i;

    if (!runtime)
    {
        return 0;
    }
    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        if (runtime->entries[i].online)
        {
            count++;
        }
    }
    return count;
}

size_t simple_control_route_runtime_snapshot(
    const simple_control_route_runtime_t *runtime,
    simple_control_route_runtime_entry_t *entries,
    size_t entry_capacity)
{
    size_t count = 0;
    size_t i;

    if (!runtime)
    {
        return 0;
    }
    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        const simple_control_route_runtime_entry_t *entry = &runtime->entries[i];

        if (!entry->online)
        {
            continue;
        }
        if (entries && count < entry_capacity)
        {
            entries[count] = *entry;
        }
        count++;
    }
    return entries ? (count < entry_capacity ? count : entry_capacity) : count;
}

bool simple_control_route_runtime_lookup(simple_control_route_runtime_t *runtime,
                                         const char *callee_device_id,
                                         call_endpoint_t *next_hop)
{
    const simple_control_route_runtime_entry_t *best = NULL;
    const simple_control_route_runtime_entry_t *fallback = NULL;
    unsigned best_priority = 0;
    size_t i;

    if (!runtime || !next_hop || !IS_VALID_STRING(callee_device_id))
    {
        return false;
    }

    for (i = 0; i < SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS; ++i)
    {
        const simple_control_route_runtime_entry_t *entry = &runtime->entries[i];
        unsigned priority;

        if (!entry->online)
        {
            continue;
        }
        if (!fallback || entry->seq < fallback->seq)
        {
            fallback = entry;
        }
        if (!route_device_id_equal(entry->callee_device_id, callee_device_id))
        {
            continue;
        }

        priority = route_source_priority(entry->source);
        if (!best || priority > best_priority ||
            (priority == best_priority && entry->seq > best->seq))
        {
            best = entry;
            best_priority = priority;
        }
    }

    if (best)
    {
        *next_hop = best->next_hop;
        return true;
    }
    if (runtime->cfg.legacy_fallback_enabled && fallback)
    {
        *next_hop = fallback->next_hop;
        return true;
    }
    return false;
}
