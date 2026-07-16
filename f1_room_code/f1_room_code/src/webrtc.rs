use std::ffi::{CStr, CString};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};

use crate::webrtc_sys;

// const FIXED_WEBRTC_SERVER_ADDR: &str = "47.97.44.160";
const FIXED_WEBRTC_SERVER_ADDR: &str = "43.108.58.129";

#[derive(Clone, Debug)]
pub struct WebrtcConfig {
    pub enabled: bool,
    pub interface: String,
    pub serno: String,
    pub initstring: String,
    pub server_addr: String,
    pub customer_serno: String,
    pub playback_device: String,
    pub record_device: String,
    pub signal_reconnect_timeout_ms: i32,
    pub max_sdk_use_mem: i32,
    pub max_session_use_mem: i32,
    pub max_session_buffer_size: i32,
    pub enable_audio_input: bool,
    pub enable_audio_output: bool,
}

impl Default for WebrtcConfig {
    fn default() -> Self {
        Self {
            enabled: false,
            interface: "wlan0".to_string(),
            serno: String::new(),
            initstring: String::new(),
            server_addr: FIXED_WEBRTC_SERVER_ADDR.to_string(),
            customer_serno: String::new(),
            playback_device: "default".to_string(),
            record_device: "default".to_string(),
            signal_reconnect_timeout_ms: 500,
            max_sdk_use_mem: 5 * 1024 * 1024,
            max_session_use_mem: 1024 * 1024,
            max_session_buffer_size: 128,
            enable_audio_input: true,
            enable_audio_output: true,
        }
    }
}

#[cfg(target_arch = "riscv64")]
impl WebrtcConfig {
    pub fn from_settings(settings: &crate::db::Settings) -> Self {
        Self {
            enabled: settings.get_bool("webrtc.enabled", false),
            interface: settings.get_string("webrtc.interface", "wlan0").to_string(),
            serno: settings.get_string("webrtc.serno", "").to_string(),
            initstring: settings.get_string("webrtc.initstring", "").to_string(),
            server_addr: FIXED_WEBRTC_SERVER_ADDR.to_string(),
            customer_serno: settings.get_string("webrtc.customer_serno", "").to_string(),
            playback_device: settings
                .get_string("webrtc.playback_device", "default")
                .to_string(),
            record_device: settings
                .get_string("webrtc.record_device", "default")
                .to_string(),
            signal_reconnect_timeout_ms: settings
                .get_int("webrtc.signal_reconnect_timeout_ms", 500),
            max_sdk_use_mem: settings.get_int("webrtc.max_sdk_use_mem", 5 * 1024 * 1024),
            max_session_use_mem: settings.get_int("webrtc.max_session_use_mem", 1024 * 1024),
            max_session_buffer_size: settings.get_int("webrtc.max_session_buffer_size", 128),
            enable_audio_input: settings.get_bool("webrtc.enable_audio_input", true),
            enable_audio_output: settings.get_bool("webrtc.enable_audio_output", true),
        }
    }
}

impl WebrtcConfig {
    fn validate(&self) -> Result<(), String> {
        let mut missing = Vec::new();
        if self.initstring.trim().is_empty() {
            missing.push("webrtc.initstring");
        }
        if self.server_addr.trim().is_empty() {
            missing.push("webrtc.server_addr");
        }
        if missing.is_empty() {
            Ok(())
        } else {
            Err(format!("missing webrtc settings: {}", missing.join(", ")))
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum WebrtcEventKind {
    None,
    Online,
    Offline,
    AskIframe,
    CallStart,
    CallLink,
    CallDisconnect,
    CallDestroy,
    ConnectFailed,
    ConnectReconnect,
    CallFailed,
    DataChannelOpen,
    Error,
}

#[derive(Clone, Debug)]
#[allow(dead_code)]
pub struct WebrtcEvent {
    pub kind: WebrtcEventKind,
    pub value0: i32,
    pub value1: i32,
    pub text1: String,
    pub text2: String,
}

#[derive(Clone)]
pub struct WebrtcController {
    inner: Arc<WebrtcControllerInner>,
}

pub struct WebrtcService {
    controller: WebrtcController,
    worker: Option<JoinHandle<()>>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct WebrtcAccount {
    pub server_addr: String,
    pub serno: String,
    pub initstring: String,
}

#[derive(Clone)]
pub struct WebrtcRuntime {
    inner: Arc<WebrtcRuntimeInner>,
}

type EventDispatcher = Arc<dyn Fn(WebrtcEvent) + Send + Sync + 'static>;

struct WebrtcControllerInner {
    handle: Mutex<Option<std::ptr::NonNull<webrtc_sys::kr_webrtc_handle_t>>>,
    started: AtomicBool,
}

struct WebrtcRuntimeInner {
    base_config: WebrtcConfig,
    dispatcher: EventDispatcher,
    state: Mutex<WebrtcRuntimeState>,
}

struct WebrtcRuntimeState {
    desired_started: bool,
    call_busy: bool,
    preview_brightness: f32,
    playback_volume: f32,
    account: Option<WebrtcAccount>,
    service: Option<WebrtcService>,
}

unsafe impl Send for WebrtcControllerInner {}
unsafe impl Sync for WebrtcControllerInner {}

impl WebrtcService {
    pub fn start(config: WebrtcConfig, dispatcher: EventDispatcher) -> Result<Self, String> {
        eprintln!(
            "webrtc: service start requested init_len={} server_len={} serno_len={} enabled={} audio_in={} audio_out={}",
            config.initstring.len(),
            config.server_addr.len(),
            config.serno.len(),
            config.enabled,
            config.enable_audio_input,
            config.enable_audio_output
        );
        config.validate()?;

        let serno_value = if config.serno.trim().is_empty() {
            read_mac_address(config.interface.as_str())
                .ok_or_else(|| format!("failed to read MAC address from {}", config.interface))?
        } else {
            config.serno.clone()
        };
        eprintln!(
            "webrtc: service config ready serno={} server={} init_len={}",
            serno_value,
            config.server_addr,
            config.initstring.len()
        );
        let initstring = CString::new(config.initstring.as_str())
            .map_err(|_| "webrtc.initstring contains interior NUL".to_string())?;
        let serno = CString::new(serno_value.as_str())
            .map_err(|_| "derived webrtc serno contains interior NUL".to_string())?;
        let server_addr = CString::new(config.server_addr.as_str())
            .map_err(|_| "webrtc.server_addr contains interior NUL".to_string())?;
        let customer_serno = CString::new(config.customer_serno.as_str())
            .map_err(|_| "webrtc.customer_serno contains interior NUL".to_string())?;
        let playback_device = CString::new(config.playback_device.as_str())
            .map_err(|_| "webrtc.playback_device contains interior NUL".to_string())?;
        let record_device = CString::new(config.record_device.as_str())
            .map_err(|_| "webrtc.record_device contains interior NUL".to_string())?;

        let raw_cfg = webrtc_sys::kr_webrtc_config_t {
            initstring: initstring.as_ptr(),
            serno: serno.as_ptr(),
            server_addr: server_addr.as_ptr(),
            customer_serno: customer_serno.as_ptr(),
            playback_device: playback_device.as_ptr(),
            record_device: record_device.as_ptr(),
            signal_reconnect_timeout_ms: config.signal_reconnect_timeout_ms,
            max_sdk_use_mem: config.max_sdk_use_mem,
            max_session_use_mem: config.max_session_use_mem,
            max_session_buffer_size: config.max_session_buffer_size,
            enable_audio_input: config.enable_audio_input,
            enable_audio_output: config.enable_audio_output,
        };

        let mut handle = std::ptr::null_mut();
        let create_rc = unsafe { webrtc_sys::kr_webrtc_create(&mut handle, &raw_cfg) };
        eprintln!("webrtc: kr_webrtc_create rc={create_rc}");
        if create_rc < 0 {
            return Err(format!("kr_webrtc_create failed: {create_rc}"));
        }

        let handle = std::ptr::NonNull::new(handle)
            .ok_or_else(|| "kr_webrtc_create returned null handle".to_string())?;
        let inner = Arc::new(WebrtcControllerInner {
            handle: Mutex::new(Some(handle)),
            started: AtomicBool::new(false),
        });
        let worker_inner = inner.clone();
        let worker_dispatcher = dispatcher.clone();
        let worker = thread::spawn(move || loop {
            let maybe_handle = worker_inner.handle.lock().ok().and_then(|guard| *guard);
            let Some(handle) = maybe_handle else {
                break;
            };

            let mut raw_event = webrtc_sys::kr_webrtc_event_t::default();
            let rc =
                unsafe { webrtc_sys::kr_webrtc_wait_event(handle.as_ptr(), &mut raw_event, 250) };
            if rc == 0 {
                continue;
            }
            if rc < 0 {
                if rc == -libc::ECANCELED {
                    break;
                }
                worker_dispatcher(WebrtcEvent {
                    kind: WebrtcEventKind::Error,
                    value0: rc,
                    value1: 0,
                    text1: "wait_event_failed".to_string(),
                    text2: String::new(),
                });
                break;
            }

            let event = map_native_event(raw_event);
            eprintln!(
                "webrtc: native event kind={:?} value0={} value1={} text1_len={} text2_len={}",
                event.kind,
                event.value0,
                event.value1,
                event.text1.len(),
                event.text2.len()
            );
            worker_dispatcher(event);
        });

        Ok(Self {
            controller: WebrtcController { inner },
            worker: Some(worker),
        })
    }

    pub fn controller(&self) -> WebrtcController {
        self.controller.clone()
    }
}

impl Drop for WebrtcService {
    fn drop(&mut self) {
        self.controller.stop();
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();
        }
        self.controller.destroy();
    }
}

impl WebrtcController {
    pub fn start(&self) -> Result<(), String> {
        if self.inner.started.load(Ordering::SeqCst) {
            eprintln!("webrtc: controller start skipped already_started=1");
            return Ok(());
        }

        eprintln!("webrtc: controller start begin");
        let rc = self.with_handle_rc(|handle| unsafe { webrtc_sys::kr_webrtc_start(handle) })?;
        eprintln!("webrtc: kr_webrtc_start rc={rc}");
        if rc < 0 {
            return Err(format!("webrtc command failed: {rc}"));
        }
        self.inner.started.store(true, Ordering::SeqCst);
        eprintln!("webrtc: controller start done");
        Ok(())
    }

    #[allow(dead_code)]
    pub fn is_logged_in(&self) -> Result<bool, String> {
        let rc =
            self.with_handle_rc(|handle| unsafe { webrtc_sys::kr_webrtc_is_logged_in(handle) })?;
        Ok(rc > 0)
    }

    #[allow(dead_code)]
    pub fn send_video_frame(
        &self,
        stream_type: i32,
        video_codec: i32,
        data: &[u8],
    ) -> Result<(), String> {
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_send_video_frame(
                handle,
                stream_type,
                video_codec,
                data.as_ptr(),
                data.len(),
            )
        })
    }

    #[allow(dead_code)]
    pub fn send_audio_frame(&self, data: &[u8]) -> Result<(), String> {
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_send_audio_frame(handle, data.as_ptr(), data.len())
        })
    }

    pub fn call(
        &self,
        session_id: &str,
        to: &str,
        audio: &str,
        video: &str,
        datachannel: i32,
    ) -> Result<(), String> {
        let session_id =
            CString::new(session_id).map_err(|_| "session_id contains interior NUL".to_string())?;
        let to = CString::new(to).map_err(|_| "to contains interior NUL".to_string())?;
        let audio = CString::new(audio).map_err(|_| "audio contains interior NUL".to_string())?;
        let video = CString::new(video).map_err(|_| "video contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_call(
                handle,
                session_id.as_ptr(),
                to.as_ptr(),
                audio.as_ptr(),
                video.as_ptr(),
                datachannel,
            )
        })
    }

    pub fn close_session(&self, session_id: &str) -> Result<(), String> {
        let session_id =
            CString::new(session_id).map_err(|_| "session_id contains interior NUL".to_string())?;
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_close_session(handle, session_id.as_ptr())
        })
    }

    pub fn set_call_busy(&self, busy: bool) -> Result<(), String> {
        self.with_handle(|handle| unsafe { webrtc_sys::kr_webrtc_set_call_busy(handle, busy) })
    }

    pub fn set_preview_brightness(&self, value: f32) -> Result<(), String> {
        let scaled = (value.clamp(0.0, 1.0) * 100.0).round() as i32;
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_set_preview_brightness(handle, scaled)
        })
    }

    pub fn set_playback_volume(&self, value: f32) -> Result<(), String> {
        let scaled = (value.clamp(0.0, 1.0) * 100.0).round() as i32;
        self.with_handle(|handle| unsafe {
            webrtc_sys::kr_webrtc_set_playback_volume(handle, scaled)
        })
    }

    pub fn raw_handle(&self) -> Option<*mut webrtc_sys::kr_webrtc_handle_t> {
        self.inner
            .handle
            .lock()
            .ok()
            .and_then(|guard| guard.map(|handle| handle.as_ptr()))
    }

    fn stop(&self) {
        if let Ok(guard) = self.inner.handle.lock() {
            if let Some(handle) = *guard {
                let _ = unsafe { webrtc_sys::kr_webrtc_stop(handle.as_ptr()) };
            }
        }
        self.inner.started.store(false, Ordering::SeqCst);
    }

    fn destroy(&self) {
        if let Ok(mut guard) = self.inner.handle.lock() {
            if let Some(handle) = guard.take() {
                let _ = unsafe { webrtc_sys::kr_webrtc_destroy(handle.as_ptr()) };
            }
        }
    }

    fn with_handle(
        &self,
        func: impl FnOnce(*mut webrtc_sys::kr_webrtc_handle_t) -> i32,
    ) -> Result<(), String> {
        let rc = self.with_handle_rc(func)?;
        if rc < 0 {
            Err(format!("webrtc command failed: {rc}"))
        } else {
            Ok(())
        }
    }

    fn with_handle_rc(
        &self,
        func: impl FnOnce(*mut webrtc_sys::kr_webrtc_handle_t) -> i32,
    ) -> Result<i32, String> {
        let guard = self
            .inner
            .handle
            .lock()
            .map_err(|_| "webrtc handle lock poisoned".to_string())?;
        let handle = guard.ok_or_else(|| "webrtc handle already destroyed".to_string())?;
        Ok(func(handle.as_ptr()))
    }
}

impl WebrtcRuntime {
    pub fn new(config: WebrtcConfig, dispatcher: EventDispatcher) -> Self {
        Self {
            inner: Arc::new(WebrtcRuntimeInner {
                base_config: config,
                dispatcher,
                state: Mutex::new(WebrtcRuntimeState {
                    desired_started: false,
                    call_busy: false,
                    preview_brightness: 0.77,
                    playback_volume: 1.0,
                    account: None,
                    service: None,
                }),
            }),
        }
    }

    pub fn start(&self) -> Result<(), String> {
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        state.desired_started = true;
        eprintln!(
            "webrtc: runtime start desired=1 service_present={} account_present={}",
            state.service.is_some(),
            state.account.is_some()
        );
        if let Some(service) = state.service.as_ref() {
            service.controller().start()?;
        }
        Ok(())
    }

    pub fn set_account(&self, account: WebrtcAccount) -> Result<(), String> {
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        let fixed_account = WebrtcAccount {
            server_addr: FIXED_WEBRTC_SERVER_ADDR.to_string(),
            ..account
        };
        eprintln!(
            "webrtc: set_account server={} server_len={} serno={} init_len={} desired_started={}",
            fixed_account.server_addr,
            fixed_account.server_addr.len(),
            fixed_account.serno,
            fixed_account.initstring.len(),
            state.desired_started
        );
        if state.account.as_ref() == Some(&fixed_account) && state.service.is_some() {
            eprintln!("webrtc: set_account skipped unchanged account");
            if state.desired_started {
                if let Some(service) = state.service.as_ref() {
                    service.controller().start()?;
                }
            }
            return Ok(());
        }

        state.service = None;
        state.account = Some(fixed_account.clone());

        let mut config = self.inner.base_config.clone();
        config.enabled = true;
        config.server_addr = fixed_account.server_addr;
        config.serno = fixed_account.serno;
        config.initstring = fixed_account.initstring;

        let service = WebrtcService::start(config, self.inner.dispatcher.clone())?;
        service.controller().set_call_busy(state.call_busy)?;
        service
            .controller()
            .set_preview_brightness(state.preview_brightness)?;
        service
            .controller()
            .set_playback_volume(state.playback_volume)?;
        if state.desired_started {
            eprintln!("webrtc: set_account starting service because desired_started=1");
            service.controller().start()?;
        } else {
            eprintln!("webrtc: set_account stored service, waiting for runtime start");
        }
        state.service = Some(service);
        Ok(())
    }

    pub fn set_preview_brightness(&self, value: f32) -> Result<(), String> {
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        state.preview_brightness = value.clamp(0.0, 1.0);
        if let Some(service) = state.service.as_ref() {
            service
                .controller()
                .set_preview_brightness(state.preview_brightness)?;
        }
        Ok(())
    }

    pub fn set_playback_volume(&self, value: f32) -> Result<(), String> {
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        state.playback_volume = value.clamp(0.0, 1.0);
        if let Some(service) = state.service.as_ref() {
            service
                .controller()
                .set_playback_volume(state.playback_volume)?;
        }
        Ok(())
    }

    pub fn set_call_busy(&self, busy: bool) -> Result<(), String> {
        let mut state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        state.call_busy = busy;
        if let Some(service) = state.service.as_ref() {
            service.controller().set_call_busy(busy)?;
        }
        Ok(())
    }

    pub fn is_ready(&self) -> bool {
        self.inner
            .state
            .lock()
            .ok()
            .and_then(|state| state.service.as_ref().map(|_| true))
            .unwrap_or(false)
    }

    pub fn call(
        &self,
        session_id: &str,
        to: &str,
        audio: &str,
        video: &str,
        datachannel: i32,
    ) -> Result<(), String> {
        let state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        let service = state
            .service
            .as_ref()
            .ok_or_else(|| "webrtc runtime not initialized".to_string())?;
        service
            .controller()
            .call(session_id, to, audio, video, datachannel)
    }

    pub fn close_session(&self, session_id: &str) -> Result<(), String> {
        let state = self
            .inner
            .state
            .lock()
            .map_err(|_| "webrtc runtime lock poisoned".to_string())?;
        let service = state
            .service
            .as_ref()
            .ok_or_else(|| "webrtc runtime not initialized".to_string())?;
        service.controller().close_session(session_id)
    }

    pub fn controller(&self) -> Option<WebrtcController> {
        self.inner
            .state
            .lock()
            .ok()
            .and_then(|state| state.service.as_ref().map(|service| service.controller()))
    }

    pub fn raw_handle(&self) -> Option<*mut webrtc_sys::kr_webrtc_handle_t> {
        self.controller()
            .and_then(|controller| controller.raw_handle())
    }
}

fn map_native_event(raw: webrtc_sys::kr_webrtc_event_t) -> WebrtcEvent {
    WebrtcEvent {
        kind: match raw.kind {
            1 => WebrtcEventKind::Online,
            2 => WebrtcEventKind::Offline,
            3 => WebrtcEventKind::AskIframe,
            4 => WebrtcEventKind::CallStart,
            5 => WebrtcEventKind::CallLink,
            6 => WebrtcEventKind::CallDisconnect,
            7 => WebrtcEventKind::CallDestroy,
            8 => WebrtcEventKind::ConnectFailed,
            9 => WebrtcEventKind::ConnectReconnect,
            10 => WebrtcEventKind::CallFailed,
            14 => WebrtcEventKind::DataChannelOpen,
            15 => WebrtcEventKind::Error,
            _ => WebrtcEventKind::None,
        },
        value0: raw.value0,
        value1: raw.value1,
        text1: c_string_from_buf(&raw.text1),
        text2: c_string_from_buf(&raw.text2),
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
