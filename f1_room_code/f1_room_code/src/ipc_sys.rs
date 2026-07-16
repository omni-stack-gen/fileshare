use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct kr_ipc_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct kr_ipc_config_t {
    pub interface: *const c_char,
    pub preview_x: c_int,
    pub preview_y: c_int,
    pub preview_width: c_int,
    pub preview_height: c_int,
}

#[repr(C)]
pub struct kr_ipc_monitor_config_t {
    pub name: *const c_char,
    pub ip: *const c_char,
    pub user: *const c_char,
    pub password: *const c_char,
    pub rtsp_url: *const c_char,
    pub sub_rtsp_url: *const c_char,
    pub prefer_substream: bool,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct kr_ipc_event_t {
    pub kind: c_int,
    pub value0: c_int,
    pub value1: c_int,
    pub text1: [c_char; 128],
    pub text2: [c_char; 256],
    pub text3: [c_char; 512],
}

impl Default for kr_ipc_event_t {
    fn default() -> Self {
        Self {
            kind: 0,
            value0: 0,
            value1: 0,
            text1: [0; 128],
            text2: [0; 256],
            text3: [0; 512],
        }
    }
}

unsafe extern "C" {
    pub fn kr_ipc_create(out: *mut *mut kr_ipc_handle_t, cfg: *const kr_ipc_config_t) -> c_int;
    pub fn kr_ipc_destroy(handle: *mut kr_ipc_handle_t) -> c_int;
    pub fn kr_ipc_wait_event(
        handle: *mut kr_ipc_handle_t,
        out: *mut kr_ipc_event_t,
        timeout_ms: c_int,
    ) -> c_int;
    pub fn kr_ipc_start_monitor(
        handle: *mut kr_ipc_handle_t,
        cfg: *const kr_ipc_monitor_config_t,
    ) -> c_int;
    pub fn kr_ipc_stop_monitor(handle: *mut kr_ipc_handle_t) -> c_int;
    pub fn kr_ipc_capture_jpeg(handle: *mut kr_ipc_handle_t, output_path: *const c_char) -> c_int;
    pub fn kr_ipc_is_monitoring(handle: *mut kr_ipc_handle_t) -> c_int;
    pub fn kr_ipc_set_preview_brightness(handle: *mut kr_ipc_handle_t, level: c_int) -> c_int;
}
