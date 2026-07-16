/**
 * @file
 * @brief simple 层多会话管理器实现。
 */

#include "simple_session_manager.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

static int simple_session_manager_find_slot(const simple_session_manager_t *manager,
                                            uint64_t session_id)
{
    size_t i;

    if (!manager || session_id == 0)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (manager->slots[i].in_use &&
            manager->slots[i].record.session_id == session_id)
        {
            return (int)i;
        }
    }

    return -ENOENT;
}

static int simple_session_manager_find_free_slot(const simple_session_manager_t *manager)
{
    size_t i;

    if (!manager)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (!manager->slots[i].in_use)
        {
            return (int)i;
        }
    }

    return -ENOSPC;
}

static void simple_session_manager_remember_release(simple_session_manager_t *manager,
                                                    const simple_session_record_t *record)
{
    size_t index;

    if (!manager || !record)
    {
        return;
    }

    index = manager->release_history_next % SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX;
    manager->release_history[index] = *record;
    manager->release_history_next =
        (index + 1) % SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX;
    if (manager->release_history_count < SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX)
    {
        manager->release_history_count++;
    }
}

static void simple_session_manager_decision_init(
    simple_session_manager_admission_decision_t *decision)
{
    if (!decision)
    {
        return;
    }

    memset(decision, 0, sizeof(*decision));
    decision->admitted = false;
    decision->preempt = false;
    decision->ret = -EINVAL;
    decision->result_code = (uint8_t)CALL_RESULT_BAD_REQ;
    safe_strncpy(decision->reason, "invalid", sizeof(decision->reason));
}

static void simple_session_manager_decision_set(
    simple_session_manager_admission_decision_t *decision,
    bool admitted,
    int ret,
    uint8_t result_code,
    const char *reason,
    const simple_session_record_t *conflict)
{
    if (!decision)
    {
        return;
    }

    decision->admitted = admitted;
    decision->ret = ret;
    decision->result_code = result_code;
    safe_strncpy(decision->reason, reason ? reason : "", sizeof(decision->reason));
    if (conflict)
    {
        decision->conflict_session_id = conflict->session_id;
        decision->conflict_kind = conflict->kind;
        decision->conflict_state = conflict->state;
    }
}

void simple_session_manager_init(simple_session_manager_t *manager)
{
    simple_session_manager_clear(manager);
}

void simple_session_manager_clear(simple_session_manager_t *manager)
{
    if (!manager)
    {
        return;
    }

    memset(manager, 0, sizeof(*manager));
}

int simple_session_manager_upsert(simple_session_manager_t *manager,
                                  simple_session_record_kind_t kind,
                                  uint64_t session_id,
                                  uint32_t txn_id,
                                  uint32_t peer_id,
                                  const char *caller_device_id,
                                  const char *callee_device_id,
                                  const char *logical_source_id,
                                  const char *domain_id,
                                  const char *gateway_id,
                                  simple_session_record_state_t state,
                                  bool *created_out)
{
    simple_session_record_t *record;
    int slot;
    bool created = false;

    if (!manager || session_id == 0 || kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return -EINVAL;
    }

    slot = simple_session_manager_find_slot(manager, session_id);
    if (slot < 0)
    {
        slot = simple_session_manager_find_free_slot(manager);
        if (slot < 0)
        {
            return slot;
        }
        memset(&manager->slots[slot], 0, sizeof(manager->slots[slot]));
        manager->slots[slot].in_use = true;
        manager->slots[slot].record.session_id = session_id;
        manager->slots[slot].record.created_seq = ++manager->next_created_seq;
        created = true;
    }

    record = &manager->slots[slot].record;
    record->kind = kind;
    record->state = state;
    record->txn_id = txn_id;
    record->peer_id = peer_id;
    safe_strncpy(record->caller_device_id,
                 caller_device_id ? caller_device_id : "",
                 sizeof(record->caller_device_id));
    safe_strncpy(record->callee_device_id,
                 callee_device_id ? callee_device_id : "",
                 sizeof(record->callee_device_id));
    safe_strncpy(record->logical_source_id,
                 logical_source_id ? logical_source_id : "",
                 sizeof(record->logical_source_id));
    safe_strncpy(record->domain_id, domain_id ? domain_id : "", sizeof(record->domain_id));
    safe_strncpy(record->gateway_id, gateway_id ? gateway_id : "", sizeof(record->gateway_id));
    record->release_reason[0] = '\0';

    if (created_out)
    {
        *created_out = created;
    }
    return 0;
}

int simple_session_manager_check_admission(
    const simple_session_manager_t *manager,
    const simple_session_manager_admission_request_t *request,
    simple_session_manager_admission_decision_t *decision)
{
    size_t i;
    bool has_free = false;

    if (decision)
    {
        simple_session_manager_decision_init(decision);
    }
    if (!manager || !request || request->session_id == 0 ||
        request->kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        const simple_session_manager_slot_t *slot = &manager->slots[i];

        if (!slot->in_use)
        {
            has_free = true;
            continue;
        }
        if (slot->record.session_id == request->session_id)
        {
            if (slot->record.kind != request->kind)
            {
                simple_session_manager_decision_set(
                    decision, false, -EINVAL, (uint8_t)CALL_RESULT_CONFLICT,
                    "session_kind_mismatch", &slot->record);
                return -EINVAL;
            }
            simple_session_manager_decision_set(
                decision, true, 0, (uint8_t)CALL_RESULT_OK,
                "same_session", NULL);
            return 0;
        }
        if (request->kind == SIMPLE_SESSION_RECORD_KIND_CALL &&
            slot->record.kind == SIMPLE_SESSION_RECORD_KIND_CALL)
        {
            simple_session_manager_decision_set(
                decision, false, -EBUSY, (uint8_t)CALL_RESULT_BUSY,
                "call_busy", &slot->record);
            return -EBUSY;
        }
    }

    if (!has_free)
    {
        simple_session_manager_decision_set(
            decision, false, -ENOSPC, (uint8_t)CALL_RESULT_CONFLICT,
            "session_capacity", NULL);
        return -ENOSPC;
    }

    simple_session_manager_decision_set(decision, true, 0,
                                        (uint8_t)CALL_RESULT_OK,
                                        "admitted", NULL);
    return 0;
}

int simple_session_manager_reserve(simple_session_manager_t *manager,
                                   simple_session_record_kind_t kind,
                                   uint64_t session_id,
                                   uint32_t txn_id,
                                   uint32_t peer_id,
                                   const char *caller_device_id,
                                   const char *callee_device_id,
                                   const char *logical_source_id,
                                   const char *domain_id,
                                   const char *gateway_id,
                                   simple_session_record_state_t state,
                                   simple_session_manager_admission_decision_t *decision,
                                   bool *created_out)
{
    simple_session_manager_admission_request_t request;
    int ret;

    if (created_out)
    {
        *created_out = false;
    }
    memset(&request, 0, sizeof(request));
    request.kind = kind;
    request.session_id = session_id;
    ret = simple_session_manager_check_admission(manager, &request, decision);
    if (ret < 0)
    {
        return ret;
    }

    return simple_session_manager_upsert(manager, kind, session_id, txn_id, peer_id,
                                         caller_device_id, callee_device_id,
                                         logical_source_id, domain_id, gateway_id,
                                         state, created_out);
}

int simple_session_manager_update_state(simple_session_manager_t *manager,
                                        uint64_t session_id,
                                        simple_session_record_state_t state)
{
    int slot;

    slot = simple_session_manager_find_slot(manager, session_id);
    if (slot < 0)
    {
        return slot;
    }

    manager->slots[slot].record.state = state;
    return 0;
}

int simple_session_manager_release(simple_session_manager_t *manager,
                                   uint64_t session_id,
                                   const char *reason)
{
    int slot;
    simple_session_record_t released;

    slot = simple_session_manager_find_slot(manager, session_id);
    if (slot < 0)
    {
        return slot;
    }

    manager->slots[slot].record.state = SIMPLE_SESSION_RECORD_STATE_RELEASED;
    safe_strncpy(manager->slots[slot].record.release_reason,
                 reason ? reason : "",
                 sizeof(manager->slots[slot].record.release_reason));
    released = manager->slots[slot].record;
    simple_session_manager_remember_release(manager, &released);
    memset(&manager->slots[slot], 0, sizeof(manager->slots[slot]));
    return 0;
}

int simple_session_manager_get(const simple_session_manager_t *manager,
                               uint64_t session_id,
                               simple_session_record_t *record_out)
{
    int slot;

    if (!record_out)
    {
        return -EINVAL;
    }

    slot = simple_session_manager_find_slot(manager, session_id);
    if (slot < 0)
    {
        return slot;
    }

    *record_out = manager->slots[slot].record;
    return 0;
}

size_t simple_session_manager_count(const simple_session_manager_t *manager)
{
    size_t i;
    size_t count = 0;

    if (!manager)
    {
        return 0;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (manager->slots[i].in_use)
        {
            count++;
        }
    }

    return count;
}

size_t simple_session_manager_count_kind(const simple_session_manager_t *manager,
                                         simple_session_record_kind_t kind)
{
    size_t i;
    size_t count = 0;

    if (!manager || kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return 0;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (manager->slots[i].in_use &&
            manager->slots[i].record.kind == kind)
        {
            count++;
        }
    }

    return count;
}

size_t simple_session_manager_release_count(const simple_session_manager_t *manager)
{
    if (!manager)
    {
        return 0;
    }

    return manager->release_history_count;
}

int simple_session_manager_get_latest(const simple_session_manager_t *manager,
                                      simple_session_record_kind_t kind,
                                      simple_session_record_t *record_out)
{
    size_t i;
    bool found = false;
    uint64_t latest_seq = 0;

    if (!manager || !record_out || kind == SIMPLE_SESSION_RECORD_KIND_NONE)
    {
        return -EINVAL;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (manager->slots[i].in_use &&
            manager->slots[i].record.kind == kind)
        {
            if (!found || manager->slots[i].record.created_seq > latest_seq)
            {
                *record_out = manager->slots[i].record;
                latest_seq = manager->slots[i].record.created_seq;
                found = true;
            }
        }
    }

    return found ? 0 : -ENOENT;
}

int simple_session_manager_get_current_call(const simple_session_manager_t *manager,
                                            simple_session_record_t *record_out)
{
    return simple_session_manager_get_latest(manager,
                                             SIMPLE_SESSION_RECORD_KIND_CALL,
                                             record_out);
}

int simple_session_manager_get_current_monitor(const simple_session_manager_t *manager,
                                               simple_session_record_t *record_out)
{
    return simple_session_manager_get_latest(manager,
                                             SIMPLE_SESSION_RECORD_KIND_MONITOR,
                                             record_out);
}

simple_session_state_t simple_session_manager_public_state(
    const simple_session_manager_t *manager)
{
    simple_session_record_t record;

    if (!manager)
    {
        return SIMPLE_SESSION_IDLE;
    }

    if (simple_session_manager_get_current_call(manager, &record) == 0)
    {
        switch (record.state)
        {
        case SIMPLE_SESSION_RECORD_STATE_CREATED:
            return SIMPLE_SESSION_CALL_RINGING;
        case SIMPLE_SESSION_RECORD_STATE_RINGING:
            return SIMPLE_SESSION_CALL_VIDEO_ONLY;
        case SIMPLE_SESSION_RECORD_STATE_ACCEPTED:
        case SIMPLE_SESSION_RECORD_STATE_ACTIVE:
            return SIMPLE_SESSION_CALL_ACTIVE;
        case SIMPLE_SESSION_RECORD_STATE_EMPTY:
        case SIMPLE_SESSION_RECORD_STATE_RELEASED:
        default:
            break;
        }
    }

    if (simple_session_manager_count_kind(manager,
                                          SIMPLE_SESSION_RECORD_KIND_MONITOR) > 0)
    {
        return SIMPLE_SESSION_MONITOR_ONLY;
    }

    return SIMPLE_SESSION_IDLE;
}

void simple_session_manager_dump(const simple_session_manager_t *manager,
                                 simple_session_manager_dump_cb cb,
                                 void *user_ctx)
{
    size_t i;

    if (!manager || !cb)
    {
        return;
    }

    for (i = 0; i < SIMPLE_SESSION_MANAGER_MAX_SESSIONS; i++)
    {
        if (manager->slots[i].in_use)
        {
            cb(user_ctx, &manager->slots[i].record);
        }
    }
}

void simple_session_manager_dump_releases(const simple_session_manager_t *manager,
                                          simple_session_manager_dump_cb cb,
                                          void *user_ctx)
{
    size_t i;
    size_t start;
    size_t index;

    if (!manager || !cb)
    {
        return;
    }

    start = (manager->release_history_next + SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX -
             manager->release_history_count) % SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX;
    for (i = 0; i < manager->release_history_count; i++)
    {
        index = (start + i) % SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX;
        cb(user_ctx, &manager->release_history[index]);
    }
}

const char *simple_session_record_kind_name(simple_session_record_kind_t kind)
{
    switch (kind)
    {
    case SIMPLE_SESSION_RECORD_KIND_CALL:
        return "call";
    case SIMPLE_SESSION_RECORD_KIND_MONITOR:
        return "monitor";
    case SIMPLE_SESSION_RECORD_KIND_NONE:
    default:
        return "none";
    }
}

const char *simple_session_record_state_name(simple_session_record_state_t state)
{
    switch (state)
    {
    case SIMPLE_SESSION_RECORD_STATE_CREATED:
        return "created";
    case SIMPLE_SESSION_RECORD_STATE_RINGING:
        return "ringing";
    case SIMPLE_SESSION_RECORD_STATE_ACCEPTED:
        return "accepted";
    case SIMPLE_SESSION_RECORD_STATE_ACTIVE:
        return "active";
    case SIMPLE_SESSION_RECORD_STATE_RELEASED:
        return "released";
    case SIMPLE_SESSION_RECORD_STATE_EMPTY:
    default:
        return "empty";
    }
}
