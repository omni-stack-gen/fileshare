#![allow(non_camel_case_types)]

use libc::{c_char, c_int};

#[repr(C)]
pub struct kr_indoor_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct kr_indoor_config_t {
    pub enabled: bool,
    pub interface: *const c_char,
    pub target_device_id: *const c_char,
    pub room_id: *const c_char,
    pub unlock_peer_device_id: *const c_char,
    pub registry_peer_device_id: *const c_char,
    pub direct_plc_cco_addr: u32,
    pub playback_volume_percent: c_int,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct kr_indoor_event_t {
    pub kind: c_int,
    pub peer_id: u32,
    pub session_id: u64,
    pub session_state: c_int,
    pub result_code: c_int,
    pub video_active: bool,
    pub audio_active: bool,
    pub text: [c_char; 64],
    pub text2: [c_char; 64],
}

impl Default for kr_indoor_event_t {
    fn default() -> Self {
        Self {
            kind: 0,
            peer_id: 0,
            session_id: 0,
            session_state: 0,
            result_code: 0,
            video_active: false,
            audio_active: false,
            text: [0; 64],
            text2: [0; 64],
        }
    }
}

pub const KR_INDOOR_EVENT_NONE: c_int = 0;
pub const KR_INDOOR_EVENT_CALL_INCOMING: c_int = 1;
pub const KR_INDOOR_EVENT_CALL_STATE_CHANGED: c_int = 2;
pub const KR_INDOOR_EVENT_CALL_MEDIA_CHANGED: c_int = 3;
pub const KR_INDOOR_EVENT_CALL_REMOTE_HANGUP: c_int = 4;
pub const KR_INDOOR_EVENT_CALL_ERROR: c_int = 5;
pub const KR_INDOOR_EVENT_UNLOCK_DONE: c_int = 101;
pub const KR_INDOOR_EVENT_UNLOCK_FAIL: c_int = 102;
pub const KR_INDOOR_EVENT_UNLOCK_TIMEOUT: c_int = 103;
pub const KR_INDOOR_EVENT_SECONDARY_CONFIRM_ONLINE: c_int = 150;
pub const KR_INDOOR_EVENT_SECONDARY_CONFIRM_OFFLINE: c_int = 151;
pub const KR_INDOOR_EVENT_ERROR: c_int = 200;

unsafe extern "C" {
    pub fn kr_indoor_create(
        out: *mut *mut kr_indoor_handle_t,
        cfg: *const kr_indoor_config_t,
    ) -> c_int;
    pub fn kr_indoor_start(handle: *mut kr_indoor_handle_t) -> c_int;
    pub fn kr_indoor_stop(handle: *mut kr_indoor_handle_t);
    pub fn kr_indoor_destroy(handle: *mut kr_indoor_handle_t);
    pub fn kr_indoor_accept(handle: *mut kr_indoor_handle_t) -> c_int;
    pub fn kr_indoor_reject(handle: *mut kr_indoor_handle_t) -> c_int;
    pub fn kr_indoor_hangup(handle: *mut kr_indoor_handle_t) -> c_int;
    pub fn kr_indoor_call(
        handle: *mut kr_indoor_handle_t,
        target_device_id: *const c_char,
    ) -> c_int;
    #[allow(dead_code)]
    pub fn kr_indoor_monitor(
        handle: *mut kr_indoor_handle_t,
        target_device_id: *const c_char,
    ) -> c_int;
    pub fn kr_indoor_monitor_stop(handle: *mut kr_indoor_handle_t) -> c_int;
    pub fn kr_indoor_unlock(
        handle: *mut kr_indoor_handle_t,
        target_device_id: *const c_char,
        room_id: *const c_char,
        place: c_int,
    ) -> c_int;
    pub fn kr_indoor_set_playback_volume(
        handle: *mut kr_indoor_handle_t,
        volume_percent: c_int,
    ) -> c_int;
    pub fn kr_indoor_set_preview_brightness(handle: *mut kr_indoor_handle_t, level: c_int)
        -> c_int;
    pub fn kr_indoor_set_external_call_busy(handle: *mut kr_indoor_handle_t, busy: bool) -> c_int;
    pub fn kr_indoor_set_webrtc_bridge(
        handle: *mut kr_indoor_handle_t,
        webrtc_handle: *mut libc::c_void,
    ) -> c_int;
    pub fn kr_indoor_set_media_bridge(handle: *mut kr_indoor_handle_t, enabled: bool) -> c_int;
    pub fn kr_indoor_request_video_recovery(
        handle: *mut kr_indoor_handle_t,
        use_fir: bool,
    ) -> c_int;
    pub fn kr_indoor_wait_event(
        handle: *mut kr_indoor_handle_t,
        out: *mut kr_indoor_event_t,
        timeout_ms: u32,
    ) -> c_int;
}
