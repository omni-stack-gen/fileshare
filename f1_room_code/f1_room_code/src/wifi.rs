use std::collections::HashMap;
use std::ffi::CStr;
use std::sync::mpsc::{self, RecvTimeoutError, Sender};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

use libc::{freeifaddrs, getifaddrs, ifaddrs, sockaddr_in, AF_INET};
use slint::{ModelRc, SharedString, VecModel};

#[cfg(not(kr_wifi_stub_backend))]
use crate::net_sys;
#[cfg(not(kr_wifi_stub_backend))]
use crate::wifi_sys;

#[derive(Clone, Debug)]
pub struct WifiSeed {
    pub enabled: bool,
    pub preferred_ssid: String,
    pub profiles: Vec<WifiProfile>,
}

impl Default for WifiSeed {
    fn default() -> Self {
        Self {
            enabled: true,
            preferred_ssid: String::new(),
            profiles: Vec::new(),
        }
    }
}

#[cfg(target_arch = "riscv64")]
impl WifiSeed {
    pub fn from_database(database: &crate::db::Database) -> Self {
        let settings = database.settings().ok();
        let profiles = database
            .wifi_profiles()
            .unwrap_or_default()
            .into_iter()
            .map(|profile| WifiProfile {
                ssid: profile.ssid,
                password: profile.password,
                priority: profile.priority,
                auto_connect: profile.auto_connect,
                last_connected_at: profile.last_connected_at,
            })
            .collect();
        Self {
            enabled: settings
                .as_ref()
                .map(|settings| settings.get_bool("network.wifi_enabled", true))
                .unwrap_or(true),
            preferred_ssid: settings
                .as_ref()
                .map(|settings| {
                    settings
                        .get_string("network.connected_ssid", "")
                        .to_string()
                })
                .unwrap_or_default(),
            profiles,
        }
    }
}

#[derive(Clone, Debug)]
pub struct WifiProfile {
    pub ssid: String,
    pub password: String,
    pub priority: i32,
    pub auto_connect: bool,
    pub last_connected_at: Option<i64>,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum WifiRuntimeState {
    Idle,
    Scanning,
    Connecting,
    Connected,
    Failed,
}

impl WifiRuntimeState {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Idle => "idle",
            Self::Scanning => "scanning",
            Self::Connecting => "connecting",
            Self::Connected => "connected",
            Self::Failed => "failed",
        }
    }
}

#[derive(Clone, Debug)]
pub enum WifiEvent {
    ScanResults {
        ssids: Vec<String>,
        open_flags: Vec<bool>,
    },
    Snapshot {
        status: WifiRuntimeState,
        connecting_ssid: String,
        connected_ssid: String,
        ipv4: String,
        dialog_locked: bool,
    },
    ConnectSucceeded {
        ssid: String,
        password: String,
        ipv4: String,
        foreground: bool,
    },
    ConnectFailed {
        #[allow(dead_code)]
        ssid: String,
        foreground: bool,
    },
}

#[derive(Clone)]
pub struct WifiController {
    sender: Sender<WorkerMessage>,
}

pub struct WifiService {
    controller: WifiController,
    worker: Option<JoinHandle<()>>,
}

type EventDispatcher = Arc<dyn Fn(WifiEvent) + Send + Sync + 'static>;

#[derive(Clone, Debug)]
enum WorkerCommand {
    RefreshScan,
    SetEnabled(bool),
    Connect { ssid: String, password: String },
    Stop,
}

#[derive(Clone, Debug)]
enum WorkerMessage {
    Command(WorkerCommand),
    NativeStatus(i32),
    DhcpStatus(i32),
}

#[derive(Clone, Debug)]
struct ScanRecord {
    ssid: String,
    security: i32,
    rssi: i32,
}

#[derive(Clone, Debug)]
struct PendingAttempt {
    ssid: String,
    password: String,
    security: i32,
    foreground: bool,
}

struct WorkerState {
    enabled: bool,
    preferred_ssid: String,
    profiles: Vec<WifiProfile>,
    status: WifiRuntimeState,
    scan_results: Vec<ScanRecord>,
    current_connected_ssid: String,
    current_ipv4: String,
    connecting_ssid: String,
    pending_attempt: Option<PendingAttempt>,
    pending_ip_deadline: Option<Instant>,
    last_scan_at: Option<Instant>,
    last_reconnect_at: Option<Instant>,
    dhcp_in_progress: bool,
    active_ssid_buf: Option<Vec<u8>>,
    active_password_buf: Option<Vec<u8>>,
}

impl WorkerState {
    fn new(seed: WifiSeed) -> Self {
        Self {
            enabled: seed.enabled,
            preferred_ssid: seed.preferred_ssid,
            profiles: seed.profiles,
            status: WifiRuntimeState::Idle,
            scan_results: Vec::new(),
            current_connected_ssid: String::new(),
            current_ipv4: String::new(),
            connecting_ssid: String::new(),
            pending_attempt: None,
            pending_ip_deadline: None,
            last_scan_at: None,
            last_reconnect_at: None,
            dhcp_in_progress: false,
            active_ssid_buf: None,
            active_password_buf: None,
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
static WIFI_EVENT_SENDER: OnceLock<Mutex<Option<Sender<WorkerMessage>>>> = OnceLock::new();
#[cfg(not(kr_wifi_stub_backend))]
const WLAN0_NAME: &[u8] = b"wlan0\0";
#[cfg(not(kr_wifi_stub_backend))]
const WIFI_SSID_BUFFER_LEN: usize = 33;
#[cfg(not(kr_wifi_stub_backend))]
const WIFI_PASSWORD_BUFFER_LEN: usize = 65;

#[cfg(not(kr_wifi_stub_backend))]
fn wifi_log(message: impl AsRef<str>) {
    eprintln!("wifi: {}", message.as_ref());
}

impl WifiService {
    pub fn start(seed: WifiSeed, dispatcher: EventDispatcher) -> Result<Self, String> {
        let (tx, rx) = mpsc::channel::<WorkerMessage>();
        let controller = WifiController { sender: tx.clone() };

        #[cfg(kr_wifi_stub_backend)]
        {
            let worker_dispatcher = dispatcher.clone();
            let worker = thread::spawn(move || {
                let mut connected = String::new();
                let mut scan_ssids = vec![
                    "HomeNetwork_5G".to_string(),
                    "Apartment_Public".to_string(),
                    "GuestNetwork".to_string(),
                ];
                let mut scan_open = vec![false, true, false];
                worker_dispatcher(WifiEvent::ScanResults {
                    ssids: scan_ssids.clone(),
                    open_flags: scan_open.clone(),
                });
                worker_dispatcher(WifiEvent::Snapshot {
                    status: WifiRuntimeState::Idle,
                    connecting_ssid: String::new(),
                    connected_ssid: connected.clone(),
                    ipv4: String::new(),
                    dialog_locked: false,
                });

                loop {
                    match rx.recv_timeout(Duration::from_millis(100)) {
                        Ok(WorkerMessage::Command(WorkerCommand::RefreshScan)) => {
                            worker_dispatcher(WifiEvent::ScanResults {
                                ssids: scan_ssids.clone(),
                                open_flags: scan_open.clone(),
                            });
                        }
                        Ok(WorkerMessage::Command(WorkerCommand::SetEnabled(enabled))) => {
                            if !enabled {
                                connected.clear();
                                worker_dispatcher(WifiEvent::ScanResults {
                                    ssids: Vec::new(),
                                    open_flags: Vec::new(),
                                });
                            }
                            worker_dispatcher(WifiEvent::Snapshot {
                                status: WifiRuntimeState::Idle,
                                connecting_ssid: String::new(),
                                connected_ssid: connected.clone(),
                                ipv4: if connected.is_empty() {
                                    String::new()
                                } else {
                                    "192.168.1.20".to_string()
                                },
                                dialog_locked: false,
                            });
                        }
                        Ok(WorkerMessage::Command(WorkerCommand::Connect { ssid, password })) => {
                            worker_dispatcher(WifiEvent::Snapshot {
                                status: WifiRuntimeState::Connecting,
                                connecting_ssid: ssid.clone(),
                                connected_ssid: connected.clone(),
                                ipv4: String::new(),
                                dialog_locked: true,
                            });
                            thread::sleep(Duration::from_millis(300));
                            if password.is_empty() && ssid != "Apartment_Public" {
                                worker_dispatcher(WifiEvent::ConnectFailed {
                                    ssid,
                                    foreground: true,
                                });
                            } else {
                                connected = ssid.clone();
                                worker_dispatcher(WifiEvent::ConnectSucceeded {
                                    ssid,
                                    password,
                                    ipv4: "192.168.1.20".to_string(),
                                    foreground: true,
                                });
                            }
                        }
                        Ok(WorkerMessage::Command(WorkerCommand::Stop))
                        | Err(RecvTimeoutError::Disconnected) => break,
                        Err(RecvTimeoutError::Timeout) => continue,
                        Ok(WorkerMessage::NativeStatus(_)) | Ok(WorkerMessage::DhcpStatus(_)) => {}
                    }
                }

                drop(scan_ssids);
                drop(scan_open);
            });

            return Ok(Self {
                controller,
                worker: Some(worker),
            });
        }

        #[cfg(not(kr_wifi_stub_backend))]
        {
            let callback_tx = tx.clone();
            let worker_dispatcher = dispatcher.clone();
            let worker = thread::spawn(move || {
                if let Err(err) =
                    run_native_worker(seed, rx, callback_tx, worker_dispatcher.clone())
                {
                    eprintln!("wifi worker failed: {err}");
                    worker_dispatcher(WifiEvent::Snapshot {
                        status: WifiRuntimeState::Failed,
                        connecting_ssid: String::new(),
                        connected_ssid: String::new(),
                        ipv4: String::new(),
                        dialog_locked: false,
                    });
                }
            });

            Ok(Self {
                controller,
                worker: Some(worker),
            })
        }
    }

    pub fn controller(&self) -> WifiController {
        self.controller.clone()
    }
}

impl Drop for WifiService {
    fn drop(&mut self) {
        let _ = self
            .controller
            .sender
            .send(WorkerMessage::Command(WorkerCommand::Stop));
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();
        }
    }
}

impl WifiController {
    pub fn refresh_scan(&self) -> Result<(), String> {
        self.sender
            .send(WorkerMessage::Command(WorkerCommand::RefreshScan))
            .map_err(|err| format!("wifi refresh send failed: {err}"))
    }

    pub fn set_enabled(&self, enabled: bool) -> Result<(), String> {
        self.sender
            .send(WorkerMessage::Command(WorkerCommand::SetEnabled(enabled)))
            .map_err(|err| format!("wifi enabled send failed: {err}"))
    }

    pub fn connect(&self, ssid: &str, password: &str) -> Result<(), String> {
        self.sender
            .send(WorkerMessage::Command(WorkerCommand::Connect {
                ssid: ssid.to_string(),
                password: password.to_string(),
            }))
            .map_err(|err| format!("wifi connect send failed: {err}"))
    }
}

pub fn apply_event_to_state(state: &crate::State<'_>, event: &WifiEvent) {
    match event {
        WifiEvent::ScanResults { ssids, open_flags } => {
            let ssid_model = VecModel::from(
                ssids
                    .iter()
                    .cloned()
                    .map(SharedString::from)
                    .collect::<Vec<_>>(),
            );
            let open_model = VecModel::from(open_flags.clone());
            state.set_wifi_scan_ssids(ModelRc::new(ssid_model));
            state.set_wifi_scan_open_flags(ModelRc::new(open_model));
        }
        WifiEvent::Snapshot {
            status,
            connecting_ssid,
            connected_ssid,
            ipv4,
            dialog_locked,
        } => {
            state.set_wifi_status(SharedString::from(status.as_str()));
            state.set_wifi_connecting_ssid(SharedString::from(connecting_ssid.clone()));
            state.set_connected_ssid(SharedString::from(connected_ssid.clone()));
            state.set_wifi_ipv4(SharedString::from(ipv4.clone()));
            state.set_wifi_dialog_locked(*dialog_locked);
        }
        WifiEvent::ConnectSucceeded {
            ssid,
            ipv4,
            foreground,
            ..
        } => {
            state.set_wifi_status(SharedString::from(WifiRuntimeState::Connected.as_str()));
            state.set_wifi_connecting_ssid(SharedString::from(""));
            state.set_connected_ssid(SharedString::from(ssid.clone()));
            state.set_wifi_ipv4(SharedString::from(ipv4.clone()));
            state.set_wifi_dialog_locked(false);
            state.set_keyboard_target(SharedString::from(""));
            state.set_keyboard_mode(SharedString::from("lower"));
            if *foreground {
                state.set_active_overlay(SharedString::from("de16y"));
            }
        }
        WifiEvent::ConnectFailed { foreground, .. } => {
            state.set_wifi_status(SharedString::from(WifiRuntimeState::Failed.as_str()));
            state.set_wifi_connecting_ssid(SharedString::from(""));
            state.set_wifi_dialog_locked(false);
            state.set_keyboard_target(SharedString::from(""));
            state.set_keyboard_mode(SharedString::from("lower"));
            if *foreground {
                state.set_active_overlay(SharedString::from("HZltp"));
            }
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
unsafe extern "C" fn wifi_event_cb(status: wifi_sys::wifi_status_t, arg: *mut libc::c_void) {
    let _ = arg;
    wifi_log(format!("native event status={status}"));
    if let Some(lock) = WIFI_EVENT_SENDER.get() {
        if let Ok(guard) = lock.lock() {
            if let Some(sender) = guard.as_ref() {
                let _ = sender.send(WorkerMessage::NativeStatus(status));
            }
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
unsafe extern "C" fn dhcp_status_cb(
    _dev_name: *const libc::c_char,
    status: net_sys::net_dhcp_status_t,
) {
    wifi_log(format!("dhcp callback status={status}"));
    if let Some(lock) = WIFI_EVENT_SENDER.get() {
        if let Ok(guard) = lock.lock() {
            if let Some(sender) = guard.as_ref() {
                let _ = sender.send(WorkerMessage::DhcpStatus(status));
            }
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn run_native_worker(
    seed: WifiSeed,
    rx: mpsc::Receiver<WorkerMessage>,
    callback_tx: Sender<WorkerMessage>,
    dispatcher: EventDispatcher,
) -> Result<(), String> {
    crate::log_memory_usage("wifi:worker-start");
    let version = wifi_sys::DPLIB_WIFI_VER_STRING.to_vec();
    let mut wifi_sta_path = b"/system/misc/wifi/wifi_sta\0".to_vec();
    let mut supp_template = b"/system/misc/wifi/wpa_supplicant_src.conf\0".to_vec();
    let mut supp_config = b"/tmp/wpa_supplicant.conf\0".to_vec();
    let mut interface = b"wlan0\0".to_vec();
    let sender_slot = WIFI_EVENT_SENDER.get_or_init(|| Mutex::new(None));
    if let Ok(mut guard) = sender_slot.lock() {
        *guard = Some(callback_tx.clone());
    }
    wifi_log("worker start");

    if let Err(err) = wait_for_wlan0() {
        if let Ok(mut guard) = sender_slot.lock() {
            *guard = None;
        }
        return Err(err);
    }
    wifi_log("wlan0 ready");
    crate::log_memory_usage("wifi:wlan0-ready");

    unsafe {
        if wifi_sys::dplib_wifi_set_wifi_sta_path(wifi_sta_path.as_mut_ptr().cast()) < 0
            || wifi_sys::dplib_wifi_set_supp_config_template(supp_template.as_mut_ptr().cast()) < 0
            || wifi_sys::dplib_wifi_set_supp_config_path(supp_config.as_mut_ptr().cast()) < 0
            || wifi_sys::dplib_wifi_set_interface(interface.as_mut_ptr().cast()) < 0
        {
            if let Ok(mut guard) = sender_slot.lock() {
                *guard = None;
            }
            return Err("failed to configure dplib_wifi paths".to_string());
        }
        wifi_log("configured dplib_wifi paths");

        if wifi_sys::dplib_wifi_init(
            version.as_ptr().cast_mut().cast(),
            wifi_sys::WIFI_MODE_STA,
            Some(wifi_event_cb),
            std::ptr::null_mut(),
        ) < 0
        {
            if let Ok(mut guard) = sender_slot.lock() {
                *guard = None;
            }
            return Err("dplib_wifi_init failed".to_string());
        }
    }
    wifi_log("dplib_wifi_init ok");
    crate::log_memory_usage("wifi:dplib-init-ok");

    let mut state = WorkerState::new(seed);
    refresh_actual_connection(&mut state);
    crate::log_memory_usage("wifi:actual-connection-refreshed");
    dispatch_snapshot(&dispatcher, &state, false);
    if state.enabled {
        request_scan(&mut state);
        crate::log_memory_usage("wifi:initial-scan-requested");
        dispatch_snapshot(&dispatcher, &state, false);
    } else {
        dispatcher(WifiEvent::ScanResults {
            ssids: Vec::new(),
            open_flags: Vec::new(),
        });
    }

    let mut exiting = false;
    while !exiting {
        match rx.recv_timeout(Duration::from_secs(1)) {
            Ok(WorkerMessage::Command(command)) => match command {
                WorkerCommand::RefreshScan => {
                    wifi_log("command refresh_scan");
                    if state.enabled {
                        request_scan(&mut state);
                        dispatch_snapshot(&dispatcher, &state, false);
                    }
                }
                WorkerCommand::SetEnabled(enabled) => {
                    wifi_log(format!("command set_enabled enabled={enabled}"));
                    state.enabled = enabled;
                    if !enabled {
                        let should_disconnect = state.pending_attempt.is_some()
                            || matches!(
                                state.status,
                                WifiRuntimeState::Connecting | WifiRuntimeState::Connected
                            )
                            || unsafe { wifi_sys::dplib_wifi_is_sta_connected() } == 1;
                        state.status = WifiRuntimeState::Idle;
                        state.connecting_ssid.clear();
                        state.pending_attempt = None;
                        state.active_ssid_buf = None;
                        state.active_password_buf = None;
                        state.pending_ip_deadline = None;
                        state.dhcp_in_progress = false;
                        state.current_connected_ssid.clear();
                        state.current_ipv4.clear();
                        if should_disconnect {
                            let _ = unsafe { wifi_sys::dplib_wifi_disconnect() };
                        }
                        dispatcher(WifiEvent::ScanResults {
                            ssids: Vec::new(),
                            open_flags: Vec::new(),
                        });
                    } else {
                        request_scan(&mut state);
                    }
                    dispatch_snapshot(&dispatcher, &state, false);
                }
                WorkerCommand::Connect { ssid, password } => {
                    wifi_log(format!(
                        "command connect ssid={} password_len={}",
                        ssid,
                        password.len()
                    ));
                    if !state.enabled {
                        dispatcher(WifiEvent::ConnectFailed {
                            ssid,
                            foreground: true,
                        });
                        continue;
                    }

                    let Some(security) = lookup_security(&state.scan_results, &ssid) else {
                        wifi_log(format!(
                            "connect rejected missing scan security ssid={ssid}"
                        ));
                        dispatcher(WifiEvent::ConnectFailed {
                            ssid,
                            foreground: true,
                        });
                        continue;
                    };

                    start_connect_attempt(
                        &mut state,
                        PendingAttempt {
                            ssid,
                            password,
                            security,
                            foreground: true,
                        },
                        &dispatcher,
                    );
                }
                WorkerCommand::Stop => {
                    wifi_log("command stop");
                    exiting = true;
                }
            },
            Ok(WorkerMessage::NativeStatus(status)) => {
                handle_native_status(&mut state, status, &dispatcher);
            }
            Ok(WorkerMessage::DhcpStatus(status)) => {
                handle_dhcp_status(&mut state, status, &dispatcher);
            }
            Err(RecvTimeoutError::Timeout) => {
                tick_worker(&mut state, &dispatcher);
            }
            Err(RecvTimeoutError::Disconnected) => {
                exiting = true;
            }
        }
    }

    let _ = unsafe { wifi_sys::dplib_wifi_disconnect() };
    let _ = unsafe { wifi_sys::dplib_wifi_deinit() };
    if let Ok(mut guard) = sender_slot.lock() {
        *guard = None;
    }
    wifi_log("worker stop");
    Ok(())
}

#[cfg(not(kr_wifi_stub_backend))]
fn handle_native_status(state: &mut WorkerState, status: i32, dispatcher: &EventDispatcher) {
    wifi_log(format!(
        "handle native status={} state={} connected_ssid={} connecting_ssid={}",
        status,
        state.status.as_str(),
        state.current_connected_ssid,
        state.connecting_ssid
    ));
    match status {
        wifi_sys::WIFI_STA_SCAN_RESULTS => {
            load_scan_results(state);
            dispatch_scan_results(dispatcher, state);
            if state.enabled
                && state.pending_attempt.is_none()
                && state.current_connected_ssid.is_empty()
            {
                maybe_start_auto_reconnect(state, dispatcher);
            }
        }
        wifi_sys::WIFI_STA_SCAN_FAIL => {
            if matches!(state.status, WifiRuntimeState::Scanning) {
                state.status = WifiRuntimeState::Failed;
                dispatch_snapshot(dispatcher, state, false);
            }
            if matches!(state.pending_attempt.as_ref(), Some(attempt) if attempt.foreground) {
                finish_failure(state, dispatcher);
            }
        }
        wifi_sys::WIFI_STA_CONNEECTING_EVENT => {
            state.status = WifiRuntimeState::Connecting;
            dispatch_snapshot(
                dispatcher,
                state,
                state
                    .pending_attempt
                    .as_ref()
                    .map(|attempt| attempt.foreground)
                    .unwrap_or(false),
            );
        }
        wifi_sys::WIFI_STA_CONNECTED => {
            state.status = WifiRuntimeState::Connecting;
            if state.connecting_ssid.is_empty() {
                state.connecting_ssid = current_connect_info()
                    .map(|info| info.ssid)
                    .unwrap_or_default();
            }
            state.pending_ip_deadline = Some(Instant::now() + Duration::from_secs(20));
            if !state.dhcp_in_progress {
                state.dhcp_in_progress = true;
                start_dhcp_request();
            }
            dispatch_snapshot(
                dispatcher,
                state,
                state
                    .pending_attempt
                    .as_ref()
                    .map(|attempt| attempt.foreground)
                    .unwrap_or(false),
            );
        }
        wifi_sys::WIFI_STA_PASSWD_ERROR
        | wifi_sys::WIFI_STA_NETWORK_NOT_EXIST
        | wifi_sys::WIFI_STA_CONNECT_REJECT
        | wifi_sys::WIFI_STA_CONNECT_ABORT => {
            finish_failure(state, dispatcher);
        }
        wifi_sys::WIFI_STA_DISCONNECTED => {
            state.current_connected_ssid.clear();
            state.current_ipv4.clear();
            if state.pending_attempt.is_some() {
                finish_failure(state, dispatcher);
            } else {
                state.status = WifiRuntimeState::Idle;
                dispatch_snapshot(dispatcher, state, false);
            }
        }
        wifi_sys::WIFI_STA_UNKNOWN_EVENT => {}
        _ => {}
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn handle_dhcp_status(state: &mut WorkerState, status: i32, dispatcher: &EventDispatcher) {
    state.dhcp_in_progress = false;
    wifi_log(format!(
        "handle dhcp status={} connecting_ssid={} pending={}",
        status,
        state.connecting_ssid,
        state.pending_attempt.is_some()
    ));
    match status {
        net_sys::NET_DHCP_SUCCESS => {
            if let Some(ipv4) = read_ipv4_address("wlan0") {
                let ssid = current_connect_info()
                    .map(|info| info.ssid)
                    .or_else(|| {
                        state
                            .pending_attempt
                            .as_ref()
                            .map(|attempt| attempt.ssid.clone())
                    })
                    .unwrap_or_default();
                finish_success(state, dispatcher, ssid, ipv4);
            } else {
                wifi_log("dhcp success but wlan0 has no ipv4 yet");
            }
        }
        net_sys::NET_DHCP_TIMEOUT | net_sys::NET_DHCP_ABORT | net_sys::NET_DHCP_ERROR => {
            let _ = unsafe { wifi_sys::dplib_wifi_disconnect() };
            finish_failure(state, dispatcher);
        }
        _ => {}
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn tick_worker(state: &mut WorkerState, dispatcher: &EventDispatcher) {
    if !state.enabled {
        return;
    }

    if let Some(deadline) = state.pending_ip_deadline {
        if let Some(ipv4) = read_ipv4_address("wlan0") {
            let ssid = current_connect_info()
                .map(|info| info.ssid)
                .or_else(|| {
                    state
                        .pending_attempt
                        .as_ref()
                        .map(|attempt| attempt.ssid.clone())
                })
                .unwrap_or_default();
            finish_success(state, dispatcher, ssid, ipv4);
            return;
        }

        if Instant::now() >= deadline {
            wifi_log(format!(
                "ip wait timeout ssid={} connected_ssid={}",
                state.connecting_ssid, state.current_connected_ssid
            ));
            state.dhcp_in_progress = false;
            let _ = unsafe { wifi_sys::dplib_wifi_disconnect() };
            finish_failure(state, dispatcher);
            return;
        }
    }

    if !state.current_connected_ssid.is_empty() {
        if let Some(ipv4) = read_ipv4_address("wlan0") {
            if ipv4 != state.current_ipv4 {
                state.current_ipv4 = ipv4;
                dispatch_snapshot(dispatcher, state, false);
            }
        }
        return;
    }

    let now = Instant::now();
    let should_scan = state
        .last_scan_at
        .map(|last| now.duration_since(last) >= Duration::from_secs(10))
        .unwrap_or(true);
    if should_scan {
        request_scan(state);
        dispatch_snapshot(dispatcher, state, false);
        return;
    }

    maybe_start_auto_reconnect(state, dispatcher);
}

#[cfg(not(kr_wifi_stub_backend))]
fn request_scan(state: &mut WorkerState) {
    let rc = unsafe { wifi_sys::dplib_wifi_scan() };
    wifi_log(format!("request scan rc={rc}"));
    if rc == 0 {
        state.last_scan_at = Some(Instant::now());
        if state.pending_attempt.is_none() && state.current_connected_ssid.is_empty() {
            state.status = WifiRuntimeState::Scanning;
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn load_scan_results(state: &mut WorkerState) {
    let mut raw = wifi_sys::scan_info_t {
        num: 0,
        info: std::ptr::null_mut(),
    };
    let rc = unsafe { wifi_sys::dplib_wifi_get_scan_results(&mut raw) };
    wifi_log(format!("get scan results rc={rc} num={}", raw.num));
    if rc < 0 {
        return;
    }

    let mut dedup = HashMap::<String, ScanRecord>::new();
    if raw.num > 0 && !raw.info.is_null() {
        for index in 0..raw.num {
            let item = unsafe { *raw.info.add(index as usize) };
            let ssid = c_array_to_string(&item.ssid);
            if ssid.is_empty() {
                continue;
            }
            let record = ScanRecord {
                ssid: ssid.clone(),
                security: item.security,
                rssi: item.rssi,
            };
            match dedup.get(&ssid) {
                Some(existing) if existing.rssi >= record.rssi => {}
                _ => {
                    dedup.insert(ssid, record);
                }
            }
        }
    }

    let _ = unsafe { wifi_sys::dplib_wifi_free_scan_results(&mut raw) };

    let mut records = dedup.into_values().collect::<Vec<_>>();
    records.sort_by(|left, right| {
        right
            .rssi
            .cmp(&left.rssi)
            .then_with(|| left.ssid.cmp(&right.ssid))
    });
    state.scan_results = records;
    wifi_log(format!(
        "scan results loaded count={}",
        state.scan_results.len()
    ));
    if state.pending_attempt.is_none() && state.current_connected_ssid.is_empty() {
        state.status = WifiRuntimeState::Idle;
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn dispatch_scan_results(dispatcher: &EventDispatcher, state: &WorkerState) {
    let mut ssids = Vec::new();
    let mut open_flags = Vec::new();
    for record in &state.scan_results {
        if !state.current_connected_ssid.is_empty() && record.ssid == state.current_connected_ssid {
            continue;
        }
        ssids.push(record.ssid.clone());
        open_flags.push(record.security == wifi_sys::WIFI_SECURITY_OPEN);
    }
    dispatcher(WifiEvent::ScanResults { ssids, open_flags });
}

#[cfg(not(kr_wifi_stub_backend))]
fn dispatch_snapshot(dispatcher: &EventDispatcher, state: &WorkerState, dialog_locked: bool) {
    dispatcher(WifiEvent::Snapshot {
        status: state.status,
        connecting_ssid: state.connecting_ssid.clone(),
        connected_ssid: state.current_connected_ssid.clone(),
        ipv4: state.current_ipv4.clone(),
        dialog_locked,
    });
}

#[cfg(not(kr_wifi_stub_backend))]
fn lookup_security(records: &[ScanRecord], ssid: &str) -> Option<i32> {
    records
        .iter()
        .find(|record| record.ssid == ssid)
        .map(|record| record.security)
}

#[cfg(not(kr_wifi_stub_backend))]
fn maybe_start_auto_reconnect(state: &mut WorkerState, dispatcher: &EventDispatcher) {
    if !state.enabled || state.pending_attempt.is_some() || state.scan_results.is_empty() {
        return;
    }

    let should_retry = state
        .last_reconnect_at
        .map(|last| Instant::now().duration_since(last) >= Duration::from_secs(15))
        .unwrap_or(true);
    if !should_retry {
        return;
    }

    let mut profiles = state
        .profiles
        .iter()
        .filter(|profile| profile.auto_connect)
        .collect::<Vec<_>>();
    profiles.sort_by(|left, right| {
        if !state.preferred_ssid.is_empty() {
            let left_preferred = left.ssid == state.preferred_ssid;
            let right_preferred = right.ssid == state.preferred_ssid;
            match right_preferred.cmp(&left_preferred) {
                std::cmp::Ordering::Equal => {}
                ordering => return ordering,
            }
        }
        right
            .priority
            .cmp(&left.priority)
            .then_with(|| right.last_connected_at.cmp(&left.last_connected_at))
    });

    for profile in profiles {
        if let Some(record) = state
            .scan_results
            .iter()
            .find(|record| record.ssid == profile.ssid)
        {
            wifi_log(format!(
                "auto reconnect choose ssid={} security={} rssi={}",
                record.ssid, record.security, record.rssi
            ));
            start_connect_attempt(
                state,
                PendingAttempt {
                    ssid: record.ssid.clone(),
                    password: profile.password.clone(),
                    security: record.security,
                    foreground: false,
                },
                dispatcher,
            );
            break;
        }
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn start_connect_attempt(
    state: &mut WorkerState,
    attempt: PendingAttempt,
    dispatcher: &EventDispatcher,
) {
    state.status = WifiRuntimeState::Connecting;
    state.connecting_ssid = attempt.ssid.clone();
    state.last_reconnect_at = Some(Instant::now());
    state.pending_ip_deadline = None;
    wifi_log(format!(
        "start connect attempt ssid={} security={} foreground={} password_len={}",
        attempt.ssid,
        attempt.security,
        attempt.foreground,
        attempt.password.len()
    ));
    dispatch_snapshot(dispatcher, state, attempt.foreground);

    if attempt.ssid.as_bytes().contains(&0) || attempt.password.as_bytes().contains(&0) {
        state.pending_attempt = Some(attempt);
        finish_failure(state, dispatcher);
        return;
    }
    if attempt.ssid.len() > WIFI_SSID_BUFFER_LEN - 1
        || attempt.password.len() > WIFI_PASSWORD_BUFFER_LEN - 1
    {
        wifi_log(format!(
            "connect rejected invalid lengths ssid_len={} password_len={}",
            attempt.ssid.len(),
            attempt.password.len()
        ));
        state.pending_attempt = Some(attempt);
        finish_failure(state, dispatcher);
        return;
    }

    state.pending_attempt = Some(attempt.clone());
    state.active_ssid_buf = Some(build_fixed_c_buffer(&attempt.ssid, WIFI_SSID_BUFFER_LEN));
    state.active_password_buf = Some(build_fixed_c_buffer(
        &attempt.password,
        WIFI_PASSWORD_BUFFER_LEN,
    ));

    let ssid_ptr = state
        .active_ssid_buf
        .as_mut()
        .map(|bytes| bytes.as_mut_ptr().cast())
        .unwrap_or(std::ptr::null_mut());
    let password_ptr = state
        .active_password_buf
        .as_mut()
        .map(|bytes| bytes.as_mut_ptr().cast())
        .unwrap_or(std::ptr::null_mut());

    let rc = unsafe { wifi_sys::dplib_wifi_connect(ssid_ptr, password_ptr, attempt.security) };
    wifi_log(format!("dplib_wifi_connect rc={rc} ssid={}", attempt.ssid));
    if rc < 0 {
        finish_failure(state, dispatcher);
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn build_fixed_c_buffer(value: &str, len: usize) -> Vec<u8> {
    let mut buffer = vec![0u8; len];
    let bytes = value.as_bytes();
    let copy_len = bytes.len().min(len.saturating_sub(1));
    buffer[..copy_len].copy_from_slice(&bytes[..copy_len]);
    buffer
}

#[cfg(not(kr_wifi_stub_backend))]
fn finish_success(
    state: &mut WorkerState,
    dispatcher: &EventDispatcher,
    ssid: String,
    ipv4: String,
) {
    let attempt = state.pending_attempt.take();
    let foreground = attempt
        .as_ref()
        .map(|attempt| attempt.foreground)
        .unwrap_or(false);
    let password = attempt
        .as_ref()
        .map(|attempt| attempt.password.clone())
        .or_else(|| {
            state
                .profiles
                .iter()
                .find(|profile| profile.ssid == ssid)
                .map(|profile| profile.password.clone())
        })
        .unwrap_or_default();

    state.pending_ip_deadline = None;
    state.status = WifiRuntimeState::Connected;
    state.connecting_ssid.clear();
    state.dhcp_in_progress = false;
    state.active_ssid_buf = None;
    state.active_password_buf = None;
    state.current_connected_ssid = ssid.clone();
    state.current_ipv4 = ipv4.clone();
    state.preferred_ssid = ssid.clone();
    upsert_profile_memory(state, &ssid, &password);
    wifi_log(format!(
        "connect success ssid={} ipv4={} foreground={}",
        ssid, ipv4, foreground
    ));
    dispatch_scan_results(dispatcher, state);
    dispatcher(WifiEvent::ConnectSucceeded {
        ssid,
        password,
        ipv4,
        foreground,
    });
}

#[cfg(not(kr_wifi_stub_backend))]
fn finish_failure(state: &mut WorkerState, dispatcher: &EventDispatcher) {
    let attempt = state.pending_attempt.take();
    let ssid = attempt
        .as_ref()
        .map(|attempt| attempt.ssid.clone())
        .unwrap_or_else(|| state.connecting_ssid.clone());
    let foreground = attempt
        .as_ref()
        .map(|attempt| attempt.foreground)
        .unwrap_or(false);
    state.pending_ip_deadline = None;
    state.status = WifiRuntimeState::Failed;
    state.connecting_ssid.clear();
    state.dhcp_in_progress = false;
    state.active_ssid_buf = None;
    state.active_password_buf = None;
    state.current_ipv4.clear();
    wifi_log(format!(
        "connect failure ssid={} foreground={} connected_ssid={}",
        ssid, foreground, state.current_connected_ssid
    ));
    dispatcher(WifiEvent::ConnectFailed { ssid, foreground });
    if !foreground {
        state.status = WifiRuntimeState::Idle;
        dispatch_snapshot(dispatcher, state, false);
    }
}

#[cfg(not(kr_wifi_stub_backend))]
fn refresh_actual_connection(state: &mut WorkerState) {
    state.current_connected_ssid.clear();
    state.current_ipv4.clear();
    if unsafe { wifi_sys::dplib_wifi_is_sta_connected() } == 1 {
        if let Some(info) = current_connect_info() {
            state.current_connected_ssid = info.ssid;
        }
        if let Some(ipv4) = read_ipv4_address("wlan0") {
            state.current_ipv4 = ipv4;
            state.status = WifiRuntimeState::Connected;
        } else {
            state.status = WifiRuntimeState::Connecting;
            state.connecting_ssid = state.current_connected_ssid.clone();
            state.pending_ip_deadline = Some(Instant::now() + Duration::from_secs(20));
        }
    } else {
        state.status = WifiRuntimeState::Idle;
    }
    wifi_log(format!(
        "refresh actual connection status={} connected_ssid={} ipv4={}",
        state.status.as_str(),
        state.current_connected_ssid,
        state.current_ipv4
    ));
}

#[cfg(not(kr_wifi_stub_backend))]
fn upsert_profile_memory(state: &mut WorkerState, ssid: &str, password: &str) {
    let now = unix_timestamp();
    if let Some(profile) = state
        .profiles
        .iter_mut()
        .find(|profile| profile.ssid == ssid)
    {
        profile.password = password.to_string();
        profile.auto_connect = true;
        profile.last_connected_at = Some(now);
        if profile.priority <= 0 {
            profile.priority = 100;
        }
        return;
    }

    state.profiles.push(WifiProfile {
        ssid: ssid.to_string(),
        password: password.to_string(),
        priority: 100,
        auto_connect: true,
        last_connected_at: Some(now),
    });
}

#[cfg(not(kr_wifi_stub_backend))]
fn current_connect_info() -> Option<ConnectInfoSnapshot> {
    let mut raw = wifi_sys::connect_info_t {
        ssid: [0; 33],
        bssid: [0; 18],
    };
    if unsafe { wifi_sys::dplib_wifi_sta_get_connect_info(&mut raw) } < 0 {
        return None;
    }
    let ssid = c_array_to_string(&raw.ssid);
    if ssid.is_empty() {
        return None;
    }
    Some(ConnectInfoSnapshot { ssid })
}

#[cfg(not(kr_wifi_stub_backend))]
struct ConnectInfoSnapshot {
    ssid: String,
}

#[cfg(not(kr_wifi_stub_backend))]
fn c_array_to_string(buffer: &[libc::c_char]) -> String {
    if buffer.is_empty() || buffer[0] == 0 {
        return String::new();
    }
    unsafe { CStr::from_ptr(buffer.as_ptr()) }
        .to_string_lossy()
        .trim()
        .to_string()
}

#[cfg(not(kr_wifi_stub_backend))]
fn read_ipv4_address(interface: &str) -> Option<String> {
    let mut addrs: *mut ifaddrs = std::ptr::null_mut();
    if unsafe { getifaddrs(&mut addrs) } != 0 || addrs.is_null() {
        return None;
    }

    let mut current = addrs;
    let mut result = None;
    while !current.is_null() {
        let entry = unsafe { &*current };
        if !entry.ifa_addr.is_null() && unsafe { (*entry.ifa_addr).sa_family as i32 } == AF_INET {
            let name = unsafe { CStr::from_ptr(entry.ifa_name) }
                .to_string_lossy()
                .into_owned();
            if name == interface {
                let sockaddr = unsafe { &*(entry.ifa_addr as *const sockaddr_in) };
                let bytes = sockaddr.sin_addr.s_addr.to_ne_bytes();
                let ip = format!("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
                if ip != "0.0.0.0" {
                    result = Some(ip);
                    break;
                }
            }
        }
        current = unsafe { (*current).ifa_next };
    }

    unsafe {
        freeifaddrs(addrs);
    }
    result
}

#[cfg(not(kr_wifi_stub_backend))]
fn unix_timestamp() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|duration| duration.as_secs() as i64)
        .unwrap_or(0)
}

#[cfg(not(kr_wifi_stub_backend))]
fn wait_for_wlan0() -> Result<(), String> {
    let deadline = Instant::now() + Duration::from_secs(15);
    while Instant::now() < deadline {
        let exists = unsafe { net_sys::dplib_net_dev_is_exists(WLAN0_NAME.as_ptr().cast()) };
        wifi_log(format!("wait wlan0 exists={exists}"));
        if exists == 1 {
            return Ok(());
        }
        thread::sleep(Duration::from_millis(100));
    }
    Err("timed out waiting for wlan0".to_string())
}

#[cfg(not(kr_wifi_stub_backend))]
fn start_dhcp_request() {
    wifi_log("start dhcp request wlan0");
    thread::spawn(|| {
        let rc = unsafe {
            net_sys::dplib_net_dev_start_dhcp(
                WLAN0_NAME.as_ptr().cast(),
                Some(dhcp_status_cb),
                true,
            )
        };
        wifi_log(format!("dplib_net_dev_start_dhcp rc={rc}"));
        if rc < 0 {
            if let Some(lock) = WIFI_EVENT_SENDER.get() {
                if let Ok(guard) = lock.lock() {
                    if let Some(sender) = guard.as_ref() {
                        let _ = sender.send(WorkerMessage::DhcpStatus(net_sys::NET_DHCP_ERROR));
                    }
                }
            }
        }
    });
}
