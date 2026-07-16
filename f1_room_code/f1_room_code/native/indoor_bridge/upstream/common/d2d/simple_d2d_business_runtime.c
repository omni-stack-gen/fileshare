/**
 * @file
 * @brief simple D2D business runtime 实现。
 */

#include "simple_d2d_business_runtime.h"

#include <errno.h>
#include <string.h>

#include <utils/util.h>

#include "d2d_protocol.h"
#include "simple_d2d_indoor_registry_bus_app.h"
#include "simple_log.h"

static int business_default_dev_type(simple_d2d_business_role_t role)
{
    switch (role)
    {
        case SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT:
            return D2D_DEV_INDOOR;
        case SIMPLE_D2D_BUSINESS_ROLE_UNIT_DOOR_SERVER:
            return D2D_DEV_UNIT_DOOR;
        case SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE:
            return D2D_DEV_SIGNAL_EXTENDER;
        case SIMPLE_D2D_BUSINESS_ROLE_SECONDARY_CONFIRM_SERVER:
            return D2D_DEV_VILLA_DOOR;
        default:
            return 0;
    }
}

static bool business_indoor_registry_configured(
    const simple_d2d_business_runtime_config_t *cfg)
{
    return cfg &&
           cfg->primary_unlock_enabled &&
           IS_VALID_STRING(cfg->primary.registry_peer_device_id) &&
           IS_VALID_STRING(cfg->primary.room_id) &&
           cfg->plc.local_addr != 0 &&
           cfg->plc.peer_addr != 0;
}

static bool business_indoor_registry_partial(
    const simple_d2d_business_runtime_config_t *cfg)
{
    if (!cfg || !cfg->primary_unlock_enabled)
    {
        return false;
    }
    return IS_VALID_STRING(cfg->primary.registry_peer_device_id) ||
           IS_VALID_STRING(cfg->primary.room_id);
}

static int business_validate_config(
    const simple_d2d_business_runtime_config_t *cfg)
{
    if (!cfg || cfg->role == 0)
    {
        return -EINVAL;
    }

    switch (cfg->role)
    {
        case SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT:
            if (!IS_VALID_STRING(cfg->identity.local_device_id))
            {
                return -EINVAL;
            }
            if (cfg->primary_unlock_enabled &&
                (!IS_VALID_STRING(cfg->primary.unlock_peer_device_id) ||
                 cfg->plc.local_addr == 0 ||
                 cfg->plc.peer_addr == 0))
            {
                return -EINVAL;
            }
            if (business_indoor_registry_partial(cfg) &&
                !business_indoor_registry_configured(cfg))
            {
                return -EINVAL;
            }
            if (cfg->secondary.enabled &&
                !IS_VALID_STRING(cfg->secondary.unlock_peer_device_id))
            {
                return -EINVAL;
            }
            return 0;
        case SIMPLE_D2D_BUSINESS_ROLE_UNIT_DOOR_SERVER:
            return IS_VALID_STRING(cfg->identity.local_device_id) ? 0 : -EINVAL;
        case SIMPLE_D2D_BUSINESS_ROLE_SECONDARY_CONFIRM_SERVER:
            return IS_VALID_STRING(cfg->identity.local_device_id) ? 0 : -EINVAL;
        case SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE:
            if (cfg->plc.local_addr == 0)
            {
                return -EINVAL;
            }
            return 0;
        default:
            return -EINVAL;
    }
}

void simple_d2d_business_runtime_config_init(
    simple_d2d_business_runtime_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->primary_unlock_enabled = true;
    cfg->eth.port = SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT;
    cfg->plc.port = SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT;
    cfg->secondary.endpoint.port = SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT;
}

static void business_endpoint_normalize(simple_d2d_endpoint_config_t *endpoint,
                                        uint16_t default_port)
{
    if (endpoint && endpoint->port == 0)
    {
        endpoint->port = default_port;
    }
}

static void business_fill_unlock_config(
    const simple_d2d_business_runtime_config_t *cfg,
    simple_d2d_unlock_config_t *unlock_cfg)
{
    simple_d2d_unlock_config_init(unlock_cfg);
    unlock_cfg->identity.local_dev_type = cfg->identity.local_dev_type ?
                                 cfg->identity.local_dev_type :
                                 business_default_dev_type(cfg->role);
    unlock_cfg->eth.if_name = cfg->eth.if_name;
    unlock_cfg->plc.if_name = cfg->plc.if_name;
    unlock_cfg->eth.local_addr = cfg->eth.local_addr;
    unlock_cfg->eth.peer_addr = cfg->eth.peer_addr;
    unlock_cfg->eth.port = cfg->eth.port;
    unlock_cfg->plc.local_addr = cfg->plc.local_addr;
    unlock_cfg->plc.peer_addr = cfg->plc.peer_addr;
    unlock_cfg->plc.port = cfg->plc.port;
    unlock_cfg->identity.local_device_id = cfg->identity.local_device_id;
    unlock_cfg->identity.peer_device_id = cfg->primary.unlock_peer_device_id;
    unlock_cfg->callbacks.observer.observer = cfg->callbacks.observer;
    unlock_cfg->callbacks.observer.observer_user_ctx = cfg->callbacks.observer_user_ctx;
    unlock_cfg->callbacks.server_unlock_cb = cfg->callbacks.unlock_server_cb;
    unlock_cfg->callbacks.server_unlock_user_ctx = cfg->callbacks.unlock_server_user_ctx;

    switch (cfg->role)
    {
        case SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT:
            unlock_cfg->role = SIMPLE_D2D_UNLOCK_ROLE_CLIENT;
            break;
        case SIMPLE_D2D_BUSINESS_ROLE_UNIT_DOOR_SERVER:
        case SIMPLE_D2D_BUSINESS_ROLE_SECONDARY_CONFIRM_SERVER:
            unlock_cfg->role = SIMPLE_D2D_UNLOCK_ROLE_SERVER;
            break;
        case SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE:
            unlock_cfg->role = SIMPLE_D2D_UNLOCK_ROLE_BRIDGE;
            break;
        default:
            break;
    }
}

static int business_start_indoor(simple_d2d_business_runtime_t *runtime)
{
    simple_d2d_unlock_config_t unlock_cfg;
    int ret;

    if (runtime->cfg.primary_unlock_enabled)
    {
        business_fill_unlock_config(&runtime->cfg, &unlock_cfg);
        ret = simple_d2d_unlock_start(&runtime->unlock, &unlock_cfg);
        if (ret != 0)
        {
            SIMPLE_LOGE("simple_d2d_business indoor unlock start failed: %d\n", ret);
            return ret;
        }
    }

    if (business_indoor_registry_configured(&runtime->cfg))
    {
        simple_d2d_indoor_heartbeat_config_t heartbeat_cfg;
        simple_d2d_indoor_registry_config_t registry_cfg;

        simple_d2d_indoor_heartbeat_config_init(&heartbeat_cfg);
        heartbeat_cfg.role = SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_CLIENT;
        heartbeat_cfg.identity.local_device_id = runtime->cfg.identity.local_device_id;
        heartbeat_cfg.identity.peer_device_id = runtime->cfg.primary.registry_peer_device_id;
        heartbeat_cfg.room_id = runtime->cfg.primary.room_id;
        heartbeat_cfg.plc.local_addr = runtime->cfg.plc.local_addr;
        heartbeat_cfg.plc.peer_addr = runtime->cfg.plc.peer_addr;
        heartbeat_cfg.plc.port = runtime->cfg.plc.port;
        heartbeat_cfg.shared_service =
            simple_d2d_bus_service(&runtime->unlock.bus);
        ret = simple_d2d_indoor_heartbeat_start(&runtime->heartbeat,
                                                &heartbeat_cfg);
        if (ret != 0)
        {
            SIMPLE_LOGE("simple_d2d_business indoor heartbeat start failed: %d\n",
                        ret);
            simple_d2d_business_runtime_stop(runtime);
            return ret;
        }

        simple_d2d_indoor_registry_config_init(&registry_cfg);
        registry_cfg.role = SIMPLE_D2D_INDOOR_REGISTRY_ROLE_CLIENT;
        registry_cfg.identity.local_device_id = runtime->cfg.identity.local_device_id;
        registry_cfg.identity.peer_device_id = runtime->cfg.primary.registry_peer_device_id;
        registry_cfg.room_id = runtime->cfg.primary.room_id;
        registry_cfg.plc.local_addr = runtime->cfg.plc.local_addr;
        registry_cfg.plc.peer_addr = runtime->cfg.plc.peer_addr;
        registry_cfg.plc.port = runtime->cfg.plc.port;
        registry_cfg.shared_service =
            simple_d2d_bus_service(&runtime->unlock.bus);
        ret = simple_d2d_indoor_registry_start(&runtime->registry,
                                               &registry_cfg);
        if (ret != 0)
        {
            SIMPLE_LOGE("simple_d2d_business indoor registry start failed: %d\n",
                        ret);
            simple_d2d_business_runtime_stop(runtime);
            return ret;
        }
    }

    if (runtime->cfg.secondary.enabled)
    {
        business_fill_unlock_config(&runtime->cfg, &unlock_cfg);
        unlock_cfg.role = SIMPLE_D2D_UNLOCK_ROLE_CLIENT;
        unlock_cfg.client_link = SIMPLE_D2D_UNLOCK_CLIENT_LINK_ETH;
        unlock_cfg.identity.peer_device_id = runtime->cfg.secondary.unlock_peer_device_id;
        if (runtime->cfg.secondary.endpoint.if_name)
        {
            unlock_cfg.eth.if_name = runtime->cfg.secondary.endpoint.if_name;
        }
        if (runtime->cfg.secondary.endpoint.local_addr != 0)
        {
            unlock_cfg.eth.local_addr = runtime->cfg.secondary.endpoint.local_addr;
        }
        unlock_cfg.eth.peer_addr = runtime->cfg.secondary.endpoint.peer_addr;
        unlock_cfg.eth.port = runtime->cfg.secondary.endpoint.port;
        unlock_cfg.shared_service = runtime->cfg.secondary.shared_service;
        ret = simple_d2d_unlock_start(&runtime->secondary_unlock, &unlock_cfg);
        if (ret != 0)
        {
            SIMPLE_LOGE("simple_d2d_business secondary unlock client start failed: %d\n",
                        ret);
            simple_d2d_business_runtime_stop(runtime);
            return ret;
        }
        SIMPLE_LOGI("secondary unlock client enabled: peer=%s addr=0x%08x:%u\n",
                    runtime->cfg.secondary.unlock_peer_device_id,
                    runtime->cfg.secondary.endpoint.peer_addr,
                    runtime->cfg.secondary.endpoint.port);
    }

    SIMPLE_LOGI("indoor registry mode: enabled=%d\n",
                business_indoor_registry_configured(&runtime->cfg) ? 1 : 0);
    return 0;
}

static int business_start_unit_door(simple_d2d_business_runtime_t *runtime)
{
    simple_d2d_unlock_config_t unlock_cfg;
    int ret;

    business_fill_unlock_config(&runtime->cfg, &unlock_cfg);
    ret = simple_d2d_unlock_start(&runtime->unlock, &unlock_cfg);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business door unlock start failed: %d\n", ret);
    }
    return ret;
}

static int business_start_converter(simple_d2d_business_runtime_t *runtime)
{
    simple_d2d_indoor_heartbeat_config_t heartbeat_cfg;
    simple_d2d_indoor_registry_config_t registry_cfg;
    simple_d2d_unlock_config_t unlock_cfg;
    int ret;

    simple_d2d_indoor_heartbeat_config_init(&heartbeat_cfg);
    heartbeat_cfg.role = SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_SERVER;
    heartbeat_cfg.identity.local_device_id = runtime->cfg.identity.local_device_id;
    heartbeat_cfg.plc.local_addr = runtime->cfg.plc.local_addr;
    heartbeat_cfg.plc.port = runtime->cfg.plc.port;
    heartbeat_cfg.callbacks.update_cb = runtime->cfg.callbacks.heartbeat_update_cb;
    heartbeat_cfg.callbacks.user_ctx = runtime->cfg.callbacks.heartbeat_user_ctx;
    ret = simple_d2d_indoor_heartbeat_init(&runtime->heartbeat, &heartbeat_cfg);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business converter heartbeat init failed: %d\n",
                    ret);
        return ret;
    }
    simple_d2d_indoor_heartbeat_bus_app_init(
        &runtime->extra_apps[runtime->extra_app_count++],
        &runtime->heartbeat,
        true);

    simple_d2d_indoor_registry_config_init(&registry_cfg);
    registry_cfg.role = SIMPLE_D2D_INDOOR_REGISTRY_ROLE_SERVER;
    registry_cfg.identity.local_device_id = runtime->cfg.identity.local_device_id;
    registry_cfg.plc.local_addr = runtime->cfg.plc.local_addr;
    registry_cfg.plc.port = runtime->cfg.plc.port;
    registry_cfg.callbacks.update_cb = runtime->cfg.callbacks.registry_update_cb;
    registry_cfg.callbacks.user_ctx = runtime->cfg.callbacks.registry_user_ctx;
    ret = simple_d2d_indoor_registry_init(&runtime->registry, &registry_cfg);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business converter registry init failed: %d\n",
                    ret);
        simple_d2d_business_runtime_stop(runtime);
        return ret;
    }
    simple_d2d_indoor_registry_bus_app_init(
        &runtime->extra_apps[runtime->extra_app_count++],
        &runtime->registry,
        true);

    business_fill_unlock_config(&runtime->cfg, &unlock_cfg);
    unlock_cfg.extra_apps.apps = runtime->extra_apps;
    unlock_cfg.extra_apps.count = runtime->extra_app_count;
    ret = simple_d2d_unlock_start(&runtime->unlock, &unlock_cfg);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business converter unlock bridge start failed: %d\n",
                    ret);
        simple_d2d_business_runtime_stop(runtime);
        return ret;
    }

    ret = simple_d2d_indoor_heartbeat_start_expire_thread(&runtime->heartbeat);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business converter heartbeat expire start failed: %d\n",
                    ret);
        simple_d2d_business_runtime_stop(runtime);
        return ret;
    }
    return 0;
}

int simple_d2d_business_runtime_start(
    simple_d2d_business_runtime_t *runtime,
    const simple_d2d_business_runtime_config_t *cfg)
{
    int ret;

    if (!runtime || runtime->started || business_validate_config(cfg) != 0)
    {
        return -EINVAL;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->cfg = *cfg;
    if (runtime->cfg.identity.local_dev_type == 0)
    {
        runtime->cfg.identity.local_dev_type =
            business_default_dev_type(runtime->cfg.role);
    }
    business_endpoint_normalize(&runtime->cfg.eth,
                                SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT);
    business_endpoint_normalize(&runtime->cfg.plc,
                                SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT);
    business_endpoint_normalize(&runtime->cfg.secondary.endpoint,
                                SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT);

    switch (runtime->cfg.role)
    {
        case SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT:
            ret = business_start_indoor(runtime);
            break;
        case SIMPLE_D2D_BUSINESS_ROLE_UNIT_DOOR_SERVER:
            ret = business_start_unit_door(runtime);
            break;
        case SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE:
            ret = business_start_converter(runtime);
            break;
        case SIMPLE_D2D_BUSINESS_ROLE_SECONDARY_CONFIRM_SERVER:
            ret = business_start_unit_door(runtime);
            break;
        default:
            ret = -EINVAL;
            break;
    }

    if (ret != 0)
    {
        simple_d2d_business_runtime_stop(runtime);
        return ret;
    }
    runtime->started = true;
    SIMPLE_LOGI("simple_d2d_business started: role=%d indoor_registry=%d\n",
                runtime->cfg.role,
                business_indoor_registry_configured(&runtime->cfg) ||
                runtime->cfg.role == SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE ?
                1 : 0);
    return 0;
}

void simple_d2d_business_runtime_stop(simple_d2d_business_runtime_t *runtime)
{
    simple_d2d_business_role_t role;

    if (!runtime)
    {
        return;
    }

    role = runtime->cfg.role;
    if (role == SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT)
    {
        simple_d2d_unlock_stop(&runtime->secondary_unlock);
        simple_d2d_indoor_registry_stop(&runtime->registry);
        simple_d2d_indoor_heartbeat_stop(&runtime->heartbeat);
        simple_d2d_unlock_stop(&runtime->unlock);
    }
    else if (role == SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE)
    {
        simple_d2d_unlock_stop(&runtime->unlock);
        simple_d2d_indoor_registry_stop(&runtime->registry);
        simple_d2d_indoor_heartbeat_stop(&runtime->heartbeat);
    }
    else
    {
        simple_d2d_unlock_stop(&runtime->unlock);
    }

    if (runtime->started)
    {
        SIMPLE_LOGI("simple_d2d_business stopped\n");
    }
    memset(runtime, 0, sizeof(*runtime));
}

simple_d2d_unlock_t *simple_d2d_business_runtime_unlock(
    simple_d2d_business_runtime_t *runtime)
{
    return runtime ? &runtime->unlock : NULL;
}

simple_d2d_unlock_t *simple_d2d_business_runtime_secondary_unlock(
    simple_d2d_business_runtime_t *runtime)
{
    return runtime ? &runtime->secondary_unlock : NULL;
}

int simple_d2d_business_runtime_update_secondary_unlock_peer(
    simple_d2d_business_runtime_t *runtime,
    uint32_t peer_addr)
{
    if (!runtime || !runtime->cfg.secondary.enabled)
    {
        return -EINVAL;
    }

    runtime->cfg.secondary.endpoint.peer_addr = peer_addr;
    runtime->secondary_unlock.cfg.eth.peer_addr = peer_addr;
    return 0;
}

int simple_d2d_business_runtime_send_unlock(
    simple_d2d_business_runtime_t *runtime,
    const char *target_device_id,
    const char *id,
    int place)
{
    if (!runtime || !target_device_id)
    {
        return -EINVAL;
    }
    if (runtime->cfg.secondary.enabled &&
        IS_VALID_STRING(runtime->cfg.secondary.unlock_peer_device_id) &&
        strcmp(target_device_id,
               runtime->cfg.secondary.unlock_peer_device_id) == 0)
    {
        return simple_d2d_unlock_send_to(&runtime->secondary_unlock,
                                         target_device_id,
                                         id,
                                         place);
    }
    if (!runtime->cfg.primary_unlock_enabled)
    {
        return -ENOTCONN;
    }
    return simple_d2d_unlock_send_to(&runtime->unlock,
                                     target_device_id,
                                     id,
                                     place);
}

simple_d2d_indoor_heartbeat_t *simple_d2d_business_runtime_heartbeat(
    simple_d2d_business_runtime_t *runtime)
{
    return runtime ? &runtime->heartbeat : NULL;
}

simple_d2d_indoor_registry_t *simple_d2d_business_runtime_registry(
    simple_d2d_business_runtime_t *runtime)
{
    return runtime ? &runtime->registry : NULL;
}

simple_d2d_service_t *simple_d2d_business_runtime_bus_service(
    simple_d2d_business_runtime_t *runtime)
{
    return runtime ? simple_d2d_bus_service(&runtime->unlock.bus) : NULL;
}
