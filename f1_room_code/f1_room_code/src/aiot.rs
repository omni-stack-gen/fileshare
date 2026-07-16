use std::ffi::{CStr, CString};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};

use crate::aiot_sys;

pub const SECONDARY_CONFIRM_DEFAULT_MODEL: &str = "simple_secondary_confirm";

#[derive(Clone, Debug)]
pub struct SecondaryConfirmProxyState {
    pub online: bool,
    pub device_id: String,
    pub dev_model: String,
}

impl Default for SecondaryConfirmProxyState {
    fn default() -> Self {
        Self {
            online: false,
            device_id: String::new(),
            dev_model: SECONDARY_CONFIRM_DEFAULT_MODEL.to_string(),
        }
    }
}

impl SecondaryConfirmProxyState {
    pub fn model_or_default(&self) -> &str {
        let model = self.dev_model.trim();
        if model.is_empty() {
            SECONDARY_CONFIRM_DEFAULT_MODEL
        } else {
            model
        }
    }
}

#[derive(Clone, Debug)]
pub struct AiotConfig {
    pub enabled: bool,
    pub interface: String,
    pub model: String,
    pub server_addr: String,
    pub mqtt_server_addr: String,
    pub application_version: String,
    pub system_version: String,
    pub hardware_version: String,
    pub mac: String,
    pub keepalive: i32,
    pub is_mqtts: bool,
    pub call_ring_timeout_sec: i32,
    pub call_converse_timeout_sec: i32,
}

impl Default for AiotConfig {
    fn default() -> Self {
        Self {
            enabled: false,
            interface: "wlan0".to_string(),
            model: String::new(),
            server_addr: String::new(),
            mqtt_server_addr: String::new(),
            application_version: env!("CARGO_PKG_VERSION").to_string(),
            system_version: "unknown".to_string(),
            hardware_version: "unknown".to_string(),
            mac: String::new(),
            keepalive: 60,
            is_mqtts: false,
            call_ring_timeout_sec: 30,
            call_converse_timeout_sec: 180,
        }
    }
}

fn normalize_call_ring_timeout_sec(value: i32) -> i32 {
    match value {
        30 | 60 | 90 => value,
        _ => 30,
    }
}

#[cfg(target_arch = "riscv64")]
impl AiotConfig {
    pub fn from_settings(settings: &crate::db::Settings) -> Self {
        let communication_minutes = settings.get_int("mode.communication_timeout_min", 3).max(1);
        Self {
            enabled: settings.get_bool("aiot.enabled", false),
            interface: settings.get_string("aiot.interface", "wlan0").to_string(),
            model: settings.get_string("aiot.model", "").to_string(),
            server_addr: settings.get_string("aiot.server_addr", "").to_string(),
            mqtt_server_addr: settings.get_string("aiot.mqtt_server_addr", "").to_string(),
            application_version: settings
                .get_string("aiot.application_version", env!("CARGO_PKG_VERSION"))
                .to_string(),
            system_version: settings
                .get_string("aiot.system_version", "unknown")
                .to_string(),
            hardware_version: settings
                .get_string("aiot.hardware_version", "unknown")
                .to_string(),
            mac: settings.get_string("aiot.mac", "").to_string(),
            keepalive: settings.get_int("aiot.keepalive", 60),
            is_mqtts: settings.get_bool("aiot.is_mqtts", false),
            call_ring_timeout_sec: normalize_call_ring_timeout_sec(
                settings.get_int("mode.call_ring_timeout_sec", 30),
            ),
            call_converse_timeout_sec: communication_minutes * 60,
        }
    }
}

impl AiotConfig {
    fn validate(&self) -> Result<(), String> {
        let mut missing = Vec::new();
        if self.model.trim().is_empty() {
            missing.push("aiot.model");
        }
        if self.server_addr.trim().is_empty() {
            missing.push("aiot.server_addr");
        }
        if missing.is_empty() {
            Ok(())
        } else {
            Err(format!("missing aiot settings: {}", missing.join(", ")))
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum AiotEventKind {
    None,
    ConnStatus,
    Message,
    Callx,
    OpenLock,
    SyncTime,
    Reboot,
    Reset,
    WebrtcAccount,
    Upgrade,
    Error,
}

#[derive(Clone, Debug)]
#[allow(dead_code)]
pub struct AiotEvent {
    pub kind: AiotEventKind,
    pub conn_status: i32,
    pub msg_type: i32,
    pub callx_type: i32,
    pub ack_code: i32,
    pub is_cloud_request: bool,
    pub seq: u64,
    pub callid: String,
    pub callee_kind: String,
    pub callee_user_id: String,
    pub callee_web_username: String,
    pub callee_web_session_id: String,
    pub callee_channel: i32,
    pub callee_count: i32,
    pub caller_device_kind: String,
    pub snapshot_download_url: String,
    pub text1: String,
    pub text2: String,
    pub text3: String,
    pub value0: i32,
    pub value1: i32,
}

#[derive(Clone)]
pub struct AiotController {
    inner: Arc<AiotControllerInner>,
}

pub struct AiotService {
    controller: AiotController,
    worker: Option<JoinHandle<()>>,
}

#[derive(Clone)]
pub struct AiotRuntime {
    inner: Arc<AiotRuntimeInner>,
}

type EventDispatcher = Arc<dyn Fn(AiotEvent) + Send + Sync + 'static>;

struct AiotControllerInner {
    handle: Mutex<Option<std::ptr::NonNull<aiot_sys::kr_aiot_handle_t>>>,
    started: AtomicBool,
}

struct AiotRuntimeInner {
    config: AiotConfig,
    dispatcher: EventDispatcher,
    controller_slot: Arc<Mutex<Option<AiotController>>>,
    secondary_confirm_state: Arc<Mutex<SecondaryConfirmProxyState>>,
    state: Mutex<AiotRuntimeState>,
}

struct AiotRuntimeState {
    desired_started: bool,
    service: Option<AiotService>,
}

unsafe impl Send for AiotControllerInner {}
unsafe impl Sync for AiotControllerInner {}

impl AiotService {
    pub fn start(config: AiotConfig, dispatcher: EventDispatcher) -> Result<Self, String> {
        config.validate()?;

        let interface = CString::new(config.interface.as_str())
            .map_err(|_| "aiot.interface contains interior NUL".to_string())?;
        let model = CString::new(config.model.as_str())
            .map_err(|_| "aiot.model contains interior NUL".to_string())?;
        let server_addr = CString::new(config.server_addr.as_str())
            .map_err(|_| "aiot.server_addr contains interior NUL".to_string())?;
        let mqtt_server_addr_value = if config.mqtt_server_addr.trim().is_empty() {
            config.server_addr.clone()
        } else {
            config.mqtt_server_addr.clone()
        };
        let mqtt_server_addr = CString::new(mqtt_server_addr_value.as_str())
            .map_err(|_| "aiot.mqtt_server_addr contains interior NUL".to_string())?;
        let application_version = CString::new(config.application_version.as_str())
            .map_err(|_| "aiot.application_version contains interior NUL".to_string())?;
        let system_version = CString::new(config.system_version.as_str())
            .map_err(|_| "aiot.system_version contains interior NUL".to_string())?;
        let hardware_version = CString::new(config.hardware_version.as_str())
            .map_err(|_| "aiot.hardware_version contains interior NUL".to_string())?;
        let device_mac = read_mac_address(config.interface.as_str())
            .ok_or_else(|| format!("failed to read MAC address from {}", config.interface))?;
        let sn = CString::new(device_mac.as_str())
            .map_err(|_| "derived aiot.sn contains interior NUL".to_string())?;
        let mac_value = if config.mac.trim().is_empty() {
            device_mac
        } else {
            config.mac
        };
        let mac = CString::new(mac_value.as_str())
            .map_err(|_| "aiot.mac contains interior NUL".to_string())?;

        let raw_cfg = aiot_sys::kr_aiot_config_t {
            interface: interface.as_ptr(),
            sn: sn.as_ptr(),
            model: model.as_ptr(),
            server_addr: server_addr.as_ptr(),
            mqtt_server_addr: mqtt_server_addr.as_ptr(),
            application_version: application_version.as_ptr(),
            system_version: system_version.as_ptr(),
            hardware_version: hardware_version.as_ptr(),
            mac: mac.as_ptr(),
            keepalive: config.keepalive,
            is_mqtts: config.is_mqtts,
        };

        let mut handle = std::ptr::null_mut();
        let create_rc = unsafe { aiot_sys::kr_aiot_create(&mut handle, &raw_cfg) };
        if create_rc < 0 {
            return Err(format!("kr_aiot_create failed: {create_rc}"));
        }

        let handle = std::ptr::NonNull::new(handle)
            .ok_or_else(|| "kr_aiot_create returned null handle".to_string())?;
        let inner = Arc::new(AiotControllerInner {
            handle: Mutex::new(Some(handle)),
            started: AtomicBool::new(false),
        });
        let controller = AiotController {
            inner: inner.clone(),
        };
        controller.set_call_timeouts(
            config.call_ring_timeout_sec,
            config.call_converse_timeout_sec,
        )?;
        let worker_inner = inner.clone();
        let worker_dispatcher = dispatcher.clone();
        let worker = thread::spawn(move || loop {
            let maybe_handle = worker_inner.handle.lock().ok().and_then(|guard| *guard);
            let Some(handle) = maybe_handle else {
                break;
            };

            let mut raw_event = aiot_sys::kr_aiot_event_t::default();
            let rc = unsafe { aiot_sys::kr_aiot_wait_event(handle.as_ptr(), &mut raw_event, 250) };
            if rc == 0 {
                continue;
            }
            if rc < 0 {
                if rc == -libc::ECANCELED {
                    break;
                }
                worker_dispatcher(AiotEvent {
                    kind: AiotEventKind::Error,
                    conn_status: 0,
                    msg_type: 0,
                    callx_type: 0,
                    ack_code: rc,
                    is_cloud_request: false,
                    seq: 0,
                    callid: String::new(),
                    text1: "wait_event_failed".to_string(),
                    text2: String::new(),
                    text3: String::new(),
                    value0: rc,
                    value1: 0,
                    callee_kind: String::new(),
                    callee_user_id: String::new(),
                    callee_web_username: String::new(),
                    callee_web_session_id: String::new(),
                    callee_channel: 0,
                    callee_count: 0,
                    caller_device_kind: String::new(),
                    snapshot_download_url: String::new(),
                });
                break;
            }

            let event = map_native_event(raw_event);
            if event.kind == AiotEventKind::Callx {
                eprintln!(
                    "aiot worker event kind={:?} callx_type={} callid={} seq={}",
                    event.kind, event.callx_type, event.callid, event.seq
                );
            }
            worker_dispatcher(event);
        });

        Ok(Self {
            controller,
            worker: Some(worker),
        })
    }

    pub fn controller(&self) -> AiotController {
        self.controller.clone()
    }
}

impl Drop for AiotService {
    fn drop(&mut self) {
        self.controller.stop();
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();
        }
        self.controller.destroy();
    }
}

impl AiotController {
    pub fn start(&self) -> Result<(), String> {
        if self.inner.started.load(Ordering::SeqCst) {
            return Ok(());
        }

        self.with_handle(|handle| unsafe { aiot_sys::kr_aiot_start(handle) })?;
        self.inner.started.store(true, Ordering::SeqCst);
        Ok(())
    }

    pub fn set_secondary_confirm_state(
        &self,
        state: &SecondaryConfirmProxyState,
    ) -> Result<(), String> {
        let proxy_sn = CString::new(state.device_id.as_str())
            .map_err(|_| "secondary proxy sn contains interior NUL".to_string())?;
        let proxy_model = CString::new(state.model_or_default())
            .map_err(|_| "secondary proxy model contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_set_secondary_confirm_online(
                handle,
                state.online,
                proxy_sn.as_ptr(),
                proxy_model.as_ptr(),
            )
        })
    }

    pub fn set_call_busy(&self, busy: bool) -> Result<(), String> {
        self.with_handle(|handle| unsafe { aiot_sys::kr_aiot_set_call_busy(handle, busy) })
    }

    pub fn set_call_timeouts(
        &self,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_set_call_timeouts(handle, ring_timeout, converse_timeout)
        })
    }

    pub fn send_open_ask(&self, callid: &str, sn: &str) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        let sn = CString::new(sn).map_err(|_| "sn contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_open_ask(handle, callid.as_ptr(), sn.as_ptr())
        })
    }

    pub fn send_callx_invited_ack(
        &self,
        callid: &str,
        able: bool,
        unable_cause: &str,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        let unable_cause = CString::new(unable_cause)
            .map_err(|_| "unable_cause contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_invited_ack(
                handle,
                callid.as_ptr(),
                able,
                unable_cause.as_ptr(),
                ring_timeout,
                converse_timeout,
            )
        })
    }

    pub fn send_callx_accept(
        &self,
        callid: &str,
        caller_media: &str,
        callee_media: &str,
    ) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        let caller_media = CString::new(caller_media)
            .map_err(|_| "caller_media contains interior NUL".to_string())?;
        let callee_media = CString::new(callee_media)
            .map_err(|_| "callee_media contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_accept(
                handle,
                callid.as_ptr(),
                caller_media.as_ptr(),
                callee_media.as_ptr(),
            )
        })
    }

    pub fn send_callx_hangup(
        &self,
        callid: &str,
        entirely: bool,
        cause: &str,
    ) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        let cause = CString::new(cause).map_err(|_| "cause contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_hangup(handle, callid.as_ptr(), entirely, cause.as_ptr())
        })
    }

    pub fn send_callx_initiate(&self, callee_sn: &str, callee_channel: i32) -> Result<(), String> {
        let callee_sn =
            CString::new(callee_sn).map_err(|_| "callee_sn contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_initiate(handle, callee_sn.as_ptr(), callee_channel)
        })
    }

    pub fn send_callx_initiate_manager(&self) -> Result<(), String> {
        self.with_handle(|handle| unsafe { aiot_sys::kr_aiot_send_callx_initiate_manager(handle) })
    }

    pub fn send_callx_initiate_resident(&self, callee_apartment: &str) -> Result<(), String> {
        let callee_apartment = CString::new(callee_apartment)
            .map_err(|_| "callee_apartment contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_initiate_resident(handle, callee_apartment.as_ptr())
        })
    }

    pub fn send_callx_invite(
        &self,
        callid: &str,
        callee_sn: &str,
        callee_channel: i32,
        mode: &str,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        let callee_sn =
            CString::new(callee_sn).map_err(|_| "callee_sn contains interior NUL".to_string())?;
        let mode = CString::new(mode).map_err(|_| "mode contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_invite(
                handle,
                callid.as_ptr(),
                callee_sn.as_ptr(),
                callee_channel,
                mode.as_ptr(),
                ring_timeout,
                converse_timeout,
            )
        })
    }

    pub fn send_callx_terminate(&self, callid: &str) -> Result<(), String> {
        let callid =
            CString::new(callid).map_err(|_| "callid contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_send_callx_terminate(handle, callid.as_ptr())
        })
    }

    pub fn download_file(
        &self,
        url: &str,
        output_path: &str,
        timeout_ms: i32,
    ) -> Result<(), String> {
        let url = CString::new(url).map_err(|_| "url contains interior NUL".to_string())?;
        let output_path = CString::new(output_path)
            .map_err(|_| "output_path contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            aiot_sys::kr_aiot_download_file(handle, url.as_ptr(), output_path.as_ptr(), timeout_ms)
        })
    }

    fn stop(&self) {
        if let Ok(guard) = self.inner.handle.lock() {
            if let Some(handle) = *guard {
                let _ = unsafe { aiot_sys::kr_aiot_stop(handle.as_ptr()) };
            }
        }
        self.inner.started.store(false, Ordering::SeqCst);
    }

    fn destroy(&self) {
        if let Ok(mut guard) = self.inner.handle.lock() {
            if let Some(handle) = guard.take() {
                let _ = unsafe { aiot_sys::kr_aiot_destroy(handle.as_ptr()) };
            }
        }
    }

    fn with_handle(
        &self,
        func: impl FnOnce(*mut aiot_sys::kr_aiot_handle_t) -> i32,
    ) -> Result<(), String> {
        let guard = self
            .inner
            .handle
            .lock()
            .map_err(|_| "aiot handle lock poisoned".to_string())?;
        let handle = guard.ok_or_else(|| "aiot handle already destroyed".to_string())?;
        let rc = func(handle.as_ptr());
        if rc < 0 {
            Err(format!("aiot command failed: {rc}"))
        } else {
            Ok(())
        }
    }
}

impl AiotRuntime {
    pub fn new(
        config: AiotConfig,
        dispatcher: EventDispatcher,
        controller_slot: Arc<Mutex<Option<AiotController>>>,
        secondary_confirm_state: Arc<Mutex<SecondaryConfirmProxyState>>,
    ) -> Self {
        Self {
            inner: Arc::new(AiotRuntimeInner {
                config,
                dispatcher,
                controller_slot,
                secondary_confirm_state,
                state: Mutex::new(AiotRuntimeState {
                    desired_started: false,
                    service: None,
                }),
            }),
        }
    }

    pub fn start(&self) -> Result<(), String> {
        eprintln!(
            "aiot-runtime: start requested enabled={} interface={} model_present={} server_present={}",
            self.inner.config.enabled,
            self.inner.config.interface,
            !self.inner.config.model.trim().is_empty(),
            !self.inner.config.server_addr.trim().is_empty()
        );
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "aiot runtime lock poisoned".to_string())?;
        state.desired_started = true;
        if state.service.is_none() {
            eprintln!("aiot-runtime: create service begin");
            let service =
                AiotService::start(self.inner.config.clone(), self.inner.dispatcher.clone())?;
            eprintln!("aiot-runtime: create service ok");
            let controller = service.controller();
            if let Ok(mut slot) = self.inner.controller_slot.lock() {
                *slot = Some(controller);
            }
            state.service = Some(service);
        } else {
            eprintln!("aiot-runtime: reuse existing service");
        }
        if let Some(service) = state.service.as_ref() {
            let secondary_state = self
                .inner
                .secondary_confirm_state
                .lock()
                .map(|state| state.clone())
                .unwrap_or_default();
            service
                .controller()
                .set_secondary_confirm_state(&secondary_state)?;
            eprintln!(
                "aiot-runtime: secondary_confirm sync online={} proxy_sn={} dev_model={}",
                secondary_state.online as i32,
                secondary_state.device_id,
                secondary_state.model_or_default()
            );
            eprintln!("aiot-runtime: controller start begin");
            service.controller().start()?;
            eprintln!("aiot-runtime: controller start ok");
        }
        Ok(())
    }
}

fn map_native_event(raw: aiot_sys::kr_aiot_event_t) -> AiotEvent {
    AiotEvent {
        kind: match raw.kind {
            1 => AiotEventKind::ConnStatus,
            2 => AiotEventKind::Message,
            3 => AiotEventKind::Callx,
            4 => AiotEventKind::OpenLock,
            5 => AiotEventKind::SyncTime,
            6 => AiotEventKind::Reboot,
            7 => AiotEventKind::Reset,
            8 => AiotEventKind::Error,
            9 => AiotEventKind::WebrtcAccount,
            10 => AiotEventKind::Upgrade,
            _ => AiotEventKind::None,
        },
        conn_status: raw.conn_status,
        msg_type: raw.msg_type,
        callx_type: raw.callx_type,
        ack_code: raw.ack_code,
        is_cloud_request: raw.is_cloud_request != 0,
        seq: raw.seq,
        callid: c_string_from_buf(&raw.callid),
        callee_kind: c_string_from_buf(&raw.callee_kind),
        callee_user_id: c_string_from_buf(&raw.callee_user_id),
        callee_web_username: c_string_from_buf(&raw.callee_web_username),
        callee_web_session_id: c_string_from_buf(&raw.callee_web_session_id),
        callee_channel: raw.callee_channel,
        callee_count: raw.callee_count,
        caller_device_kind: c_string_from_buf(&raw.caller_device_kind),
        snapshot_download_url: c_string_from_buf(&raw.snapshot_download_url),
        text1: c_string_from_buf(&raw.text1),
        text2: c_string_from_buf(&raw.text2),
        text3: c_string_from_buf(&raw.text3),
        value0: raw.value0,
        value1: raw.value1,
    }
}

fn c_string_from_buf(buf: &[libc::c_char]) -> String {
    unsafe { CStr::from_ptr(buf.as_ptr()) }
        .to_string_lossy()
        .into_owned()
}

fn read_mac_address(interface: &str) -> Option<String> {
    let raw = std::fs::read_to_string(format!("/sys/class/net/{interface}/address")).ok()?;
    let mac = raw.trim().to_ascii_uppercase();
    if mac.is_empty() || mac == "00:00:00:00:00:00" {
        return None;
    }
    Some(mac.replace(':', "-"))
}
