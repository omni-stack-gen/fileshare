#![allow(non_camel_case_types)]
#![allow(dead_code)]

use libc::{c_char, c_int};

pub type net_dhcp_status_t = c_int;
pub const NET_DHCP_ERROR: net_dhcp_status_t = -3;
pub const NET_DHCP_ABORT: net_dhcp_status_t = -2;
pub const NET_DHCP_TIMEOUT: net_dhcp_status_t = -1;
pub const NET_DHCP_SUCCESS: net_dhcp_status_t = 0;

pub type dhcp_status_handler =
    Option<unsafe extern "C" fn(dev_name: *const c_char, status: net_dhcp_status_t)>;

unsafe extern "C" {
    pub fn dplib_net_dev_is_exists(dev_name: *const c_char) -> c_int;
    pub fn dplib_net_dev_start_dhcp(
        dev_name: *const c_char,
        handler: dhcp_status_handler,
        is_sync: bool,
    ) -> c_int;
}
