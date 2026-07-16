use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct kr_aiot_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct kr_aiot_config_t {
    pub interface: *const c_char,
    pub sn: *const c_char,
    pub model: *const c_char,
    pub server_addr: *const c_char,
    pub mqtt_server_addr: *const c_char,
    pub application_version: *const c_char,
    pub system_version: *const c_char,
    pub hardware_version: *const c_char,
    pub mac: *const c_char,
    pub keepalive: c_int,
    pub is_mqtts: bool,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct kr_aiot_event_t {
    pub kind: c_int,
    pub conn_status: c_int,
    pub msg_type: c_int,
    pub callx_type: c_int,
    pub ack_code: c_int,
    pub is_cloud_request: c_int,
    pub seq: u64,
    pub callid: [c_char; 64],
    pub text1: [c_char; 128],
    pub text2: [c_char; 256],
    pub text3: [c_char; 1024],
    pub value0: c_int,
    pub value1: c_int,
    pub callee_kind: [c_char; 32],
    pub callee_user_id: [c_char; 64],
    pub callee_web_username: [c_char; 64],
    pub callee_web_session_id: [c_char; 64],
    pub callee_channel: c_int,
    pub callee_count: c_int,
    pub caller_device_kind: [c_char; 64],
    pub snapshot_download_url: [c_char; 1024],
}

impl Default for kr_aiot_event_t {
    fn default() -> Self {
        Self {
            kind: 0,
            conn_status: 0,
            msg_type: 0,
            callx_type: 0,
            ack_code: 0,
            is_cloud_request: 0,
            seq: 0,
            callid: [0; 64],
            text1: [0; 128],
            text2: [0; 256],
            text3: [0; 1024],
            value0: 0,
            value1: 0,
            callee_kind: [0; 32],
            callee_user_id: [0; 64],
            callee_web_username: [0; 64],
            callee_web_session_id: [0; 64],
            callee_channel: 0,
            callee_count: 0,
            caller_device_kind: [0; 64],
            snapshot_download_url: [0; 1024],
        }
    }
}

unsafe extern "C" {
    pub fn kr_aiot_create(out: *mut *mut kr_aiot_handle_t, cfg: *const kr_aiot_config_t) -> c_int;
    pub fn kr_aiot_start(handle: *mut kr_aiot_handle_t) -> c_int;
    pub fn kr_aiot_stop(handle: *mut kr_aiot_handle_t) -> c_int;
    pub fn kr_aiot_destroy(handle: *mut kr_aiot_handle_t) -> c_int;
    pub fn kr_aiot_wait_event(
        handle: *mut kr_aiot_handle_t,
        out: *mut kr_aiot_event_t,
        timeout_ms: c_int,
    ) -> c_int;
    pub fn kr_aiot_set_secondary_confirm_online(
        handle: *mut kr_aiot_handle_t,
        online: bool,
        proxy_sn: *const c_char,
        proxy_model: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_set_call_busy(handle: *mut kr_aiot_handle_t, busy: bool) -> c_int;
    pub fn kr_aiot_set_call_timeouts(
        handle: *mut kr_aiot_handle_t,
        ring_timeout: c_int,
        converse_timeout: c_int,
    ) -> c_int;
    pub fn kr_aiot_send_open_ask(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
        sn: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_send_callx_invited_ack(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
        able: bool,
        unable_cause: *const c_char,
        ring_timeout: c_int,
        converse_timeout: c_int,
    ) -> c_int;
    pub fn kr_aiot_send_callx_accept(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
        caller_media: *const c_char,
        callee_media: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_send_callx_hangup(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
        entirely: bool,
        cause: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_send_callx_initiate(
        handle: *mut kr_aiot_handle_t,
        callee_sn: *const c_char,
        callee_channel: c_int,
    ) -> c_int;
    pub fn kr_aiot_send_callx_initiate_manager(handle: *mut kr_aiot_handle_t) -> c_int;
    pub fn kr_aiot_send_callx_initiate_resident(
        handle: *mut kr_aiot_handle_t,
        callee_apartment: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_send_callx_invite(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
        callee_sn: *const c_char,
        callee_channel: c_int,
        mode: *const c_char,
        ring_timeout: c_int,
        converse_timeout: c_int,
    ) -> c_int;
    pub fn kr_aiot_send_callx_terminate(
        handle: *mut kr_aiot_handle_t,
        callid: *const c_char,
    ) -> c_int;
    pub fn kr_aiot_download_file(
        handle: *mut kr_aiot_handle_t,
        url: *const c_char,
        output_path: *const c_char,
        timeout_ms: c_int,
    ) -> c_int;
}
