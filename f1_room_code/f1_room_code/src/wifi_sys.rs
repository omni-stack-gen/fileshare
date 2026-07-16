#![allow(non_camel_case_types)]
#![allow(dead_code)]

use libc::{c_char, c_int, c_void, size_t};

pub const DPLIB_WIFI_VER_STRING: &[u8] = b"0.1.6\0";

pub type wifi_mode_t = c_int;
pub const WIFI_MODE_STA: wifi_mode_t = 0;

pub type wifi_security_t = c_int;
pub const WIFI_SECURITY_OPEN: wifi_security_t = 0;
pub const WIFI_SECURITY_WPA_PSK: wifi_security_t = 1;
pub const WIFI_SECURITY_WPA2_PSK: wifi_security_t = 2;

pub type wifi_status_t = c_int;
pub const WIFI_STA_DISCONNECTED: wifi_status_t = 0;
pub const WIFI_STA_SCAN_RESULTS: wifi_status_t = 1;
pub const WIFI_STA_SCAN_FAIL: wifi_status_t = 2;
pub const WIFI_STA_CONNECTED: wifi_status_t = 3;
pub const WIFI_STA_NETWORK_NOT_EXIST: wifi_status_t = 4;
pub const WIFI_STA_PASSWD_ERROR: wifi_status_t = 5;
pub const WIFI_STA_CONNECT_REJECT: wifi_status_t = 6;
pub const WIFI_STA_CONNECT_ABORT: wifi_status_t = 7;
pub const WIFI_STA_UNKNOWN_EVENT: wifi_status_t = 8;
pub const WIFI_STA_CONNEECTING_EVENT: wifi_status_t = 9;

pub type dplib_wifi_event_cb =
    Option<unsafe extern "C" fn(status: wifi_status_t, arg: *mut c_void)>;
pub type dplib_wifi_malloc_ptr = Option<unsafe extern "C" fn(size: size_t) -> *mut c_void>;
pub type dplib_wifi_free_ptr = Option<unsafe extern "C" fn(ptr: *mut c_void)>;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct hotspot_info_t {
    pub channel: c_int,
    pub rssi: c_int,
    pub security: wifi_security_t,
    pub ssid: [c_char; 33],
    pub bssid: [c_char; 18],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct scan_info_t {
    pub num: c_int,
    pub info: *mut hotspot_info_t,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct connect_info_t {
    pub ssid: [c_char; 33],
    pub bssid: [c_char; 18],
}

unsafe extern "C" {
    pub fn dplib_wifi_init(
        user_wifi_ver: *mut c_char,
        mode: wifi_mode_t,
        cb: dplib_wifi_event_cb,
        arg: *mut c_void,
    ) -> c_int;
    pub fn dplib_wifi_deinit() -> c_int;
    pub fn dplib_wifi_connect(
        ssid: *mut c_char,
        password: *mut c_char,
        security: wifi_security_t,
    ) -> c_int;
    pub fn dplib_wifi_disconnect() -> c_int;
    pub fn dplib_wifi_scan() -> c_int;
    pub fn dplib_wifi_get_scan_results(scan_info: *mut scan_info_t) -> c_int;
    pub fn dplib_wifi_free_scan_results(scan_info: *mut scan_info_t) -> c_int;
    pub fn dplib_wifi_is_sta_connected() -> c_int;
    pub fn dplib_wifi_sta_get_connect_info(info: *mut connect_info_t) -> c_int;
    pub fn dplib_wifi_sta_delete_by_ssid(ssid: *const c_char, security: c_int) -> c_int;
    pub fn dplib_wifi_set_supp_config_path(path: *mut c_char) -> c_int;
    pub fn dplib_wifi_set_supp_config_template(path: *mut c_char) -> c_int;
    pub fn dplib_wifi_set_wifi_iface_dir(path: *mut c_char) -> c_int;
    pub fn dplib_wifi_set_wifi_sta_path(path: *mut c_char) -> c_int;
    pub fn dplib_wifi_set_interface(interface: *mut c_char) -> c_int;
    pub fn dplib_wifi_init_mem_hook(
        malloc_fn: dplib_wifi_malloc_ptr,
        free_fn: dplib_wifi_free_ptr,
    ) -> c_int;
}
