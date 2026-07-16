use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct kr_sound_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct kr_sound_config_t {
    pub asset_dir: *const c_char,
    pub volume_percent: c_int,
}

unsafe extern "C" {
    pub fn kr_sound_create(
        out: *mut *mut kr_sound_handle_t,
        cfg: *const kr_sound_config_t,
    ) -> c_int;
    pub fn kr_sound_destroy(handle: *mut kr_sound_handle_t) -> c_int;
    pub fn kr_sound_play_touch(handle: *mut kr_sound_handle_t) -> c_int;
    pub fn kr_sound_start_ring(
        handle: *mut kr_sound_handle_t,
        kind: *const c_char,
        melody: c_int,
        volume_percent: c_int,
    ) -> c_int;
    pub fn kr_sound_stop_ring(handle: *mut kr_sound_handle_t) -> c_int;
    pub fn kr_sound_set_volume(handle: *mut kr_sound_handle_t, volume_percent: c_int) -> c_int;
}
