const IPC_MONITOR_PAGE: &str = "JVRSN";

pub(crate) fn should_stop_ipc_for_callx_invited(is_monitoring: bool) -> bool {
    is_monitoring
}

pub(crate) fn should_stop_ipc_for_page_change(current_page: &str, is_monitoring: bool) -> bool {
    is_monitoring && current_page != IPC_MONITOR_PAGE
}
