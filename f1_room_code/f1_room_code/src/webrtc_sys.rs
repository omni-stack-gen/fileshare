use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct kr_webrtc_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct kr_webrtc_config_t {
    pub initstring: *const c_char,
    pub serno: *const c_char,
    pub server_addr: *const c_char,
    pub customer_serno: *const c_char,
    pub playback_device: *const c_char,
    pub record_device: *const c_char,
    pub signal_reconnect_timeout_ms: c_int,
    pub max_sdk_use_mem: c_int,
    pub max_session_use_mem: c_int,
    pub max_session_buffer_size: c_int,
    pub enable_audio_input: bool,
    pub enable_audio_output: bool,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct kr_webrtc_event_t {
    pub kind: c_int,
    pub value0: c_int,
    pub value1: c_int,
    pub text1: [c_char; 128],
    pub text2: [c_char; 128],
}

impl Default for kr_webrtc_event_t {
    fn default() -> Self {
        Self {
            kind: 0,
            value0: 0,
            value1: 0,
            text1: [0; 128],
            text2: [0; 128],
        }
    }
}

unsafe extern "C" {
    pub fn kr_webrtc_create(
        out: *mut *mut kr_webrtc_handle_t,
        cfg: *const kr_webrtc_config_t,
    ) -> c_int;
    pub fn kr_webrtc_start(handle: *mut kr_webrtc_handle_t) -> c_int;
    pub fn kr_webrtc_stop(handle: *mut kr_webrtc_handle_t) -> c_int;
    pub fn kr_webrtc_destroy(handle: *mut kr_webrtc_handle_t) -> c_int;
    pub fn kr_webrtc_wait_event(
        handle: *mut kr_webrtc_handle_t,
        out: *mut kr_webrtc_event_t,
        timeout_ms: c_int,
    ) -> c_int;
    pub fn kr_webrtc_is_logged_in(handle: *mut kr_webrtc_handle_t) -> c_int;
    pub fn kr_webrtc_send_video_frame(
        handle: *mut kr_webrtc_handle_t,
        stream_type: c_int,
        video_codec: c_int,
        data: *const u8,
        len: usize,
    ) -> c_int;
    pub fn kr_webrtc_send_audio_frame(
        handle: *mut kr_webrtc_handle_t,
        data: *const u8,
        len: usize,
    ) -> c_int;
    pub fn kr_webrtc_call(
        handle: *mut kr_webrtc_handle_t,
        session_id: *const c_char,
        to: *const c_char,
        audio: *const c_char,
        video: *const c_char,
        datachannel: c_int,
    ) -> c_int;
    pub fn kr_webrtc_close_session(
        handle: *mut kr_webrtc_handle_t,
        session_id: *const c_char,
    ) -> c_int;
    pub fn kr_webrtc_set_call_busy(handle: *mut kr_webrtc_handle_t, busy: bool) -> c_int;
    pub fn kr_webrtc_set_preview_brightness(handle: *mut kr_webrtc_handle_t, level: c_int)
        -> c_int;
    pub fn kr_webrtc_set_playback_volume(
        handle: *mut kr_webrtc_handle_t,
        volume_percent: c_int,
    ) -> c_int;
}
