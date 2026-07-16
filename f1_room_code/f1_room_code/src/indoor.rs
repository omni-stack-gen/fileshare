use std::ffi::{CStr, CString};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use std::thread::{self, JoinHandle};
use std::time::Duration;

use crate::call::{CallConfig, CallEvent, CallEventKind, CallSessionState};
use crate::d2d::{D2dConfig, D2dEvent, D2dEventKind};

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
use crate::indoor_sys;

#[derive(Clone, Debug)]
pub struct IndoorConfig {
    pub enabled: bool,
    pub interface: String,
    pub target_device_id: String,
    pub room_id: String,
    pub unlock_peer_device_id: String,
    pub registry_peer_device_id: String,
    pub direct_plc_cco_addr: u32,
    pub playback_volume_percent: i32,
}

impl IndoorConfig {
    pub fn from_configs(call: &CallConfig, d2d: &D2dConfig) -> Self {
        let target_device_id = if d2d.default_door_id.contains(':') {
            d2d.default_door_id.clone()
        } else {
            "2:10.2.0.1".to_string()
        };
        let interface = if !call.interface.is_empty() {
            call.interface.clone()
        } else if !d2d.interface.is_empty() {
            d2d.interface.clone()
        } else {
            "eth0".to_string()
        };

        Self {
            enabled: call.enabled || d2d.enabled,
            interface,
            target_device_id: target_device_id.clone(),
            room_id: "101".to_string(),
            unlock_peer_device_id: target_device_id,
            registry_peer_device_id: "500:10.2.0.1:aa-bb-cc-dd-ee-ff".to_string(),
            direct_plc_cco_addr: d2d.f1_plc_addr,
            playback_volume_percent: 100,
        }
    }
}

#[derive(Clone, Debug)]
pub struct SecondaryConfirmEvent {
    pub online: bool,
    pub device_id: String,
    pub dev_model: String,
    pub endpoint_addr: u32,
    pub result_code: i32,
}

#[derive(Clone, Debug)]
pub enum IndoorEvent {
    Call(CallEvent),
    D2d(D2dEvent),
    SecondaryConfirm(SecondaryConfirmEvent),
}

type EventDispatcher = Arc<dyn Fn(IndoorEvent) + Send + Sync + 'static>;

pub struct IndoorService {
    inner: Arc<IndoorInner>,
    worker: Option<JoinHandle<()>>,
}

#[derive(Clone)]
pub struct CallController {
    inner: Arc<IndoorInner>,
    default_target: String,
}

#[derive(Clone)]
pub struct D2dController {
    inner: Arc<IndoorInner>,
    default_target: String,
    room_id: String,
}

enum IndoorInner {
    #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
    Native(NativeIndoorInner),
    Stub(StubIndoorInner),
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
struct NativeIndoorInner {
    handle: Mutex<Option<std::ptr::NonNull<indoor_sys::kr_indoor_handle_t>>>,
}

struct StubIndoorInner {
    stopping: AtomicBool,
    sender: mpsc::Sender<IndoorEvent>,
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe impl Send for NativeIndoorInner {}
#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe impl Sync for NativeIndoorInner {}

impl IndoorService {
    pub fn start(config: IndoorConfig, dispatcher: EventDispatcher) -> Result<Self, String> {
        if !config.enabled {
            return Ok(Self::stub(config, dispatcher));
        }

        #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
        {
            return Self::start_native(config, dispatcher);
        }

        #[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
        {
            Ok(Self::stub(config, dispatcher))
        }
    }

    #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
    fn start_native(config: IndoorConfig, dispatcher: EventDispatcher) -> Result<Self, String> {
        let interface = cstring("indoor.interface", &config.interface)?;
        let target_device_id = cstring("indoor.target_device_id", &config.target_device_id)?;
        let room_id = cstring("indoor.room_id", &config.room_id)?;
        let unlock_peer_device_id = cstring(
            "indoor.unlock_peer_device_id",
            &config.unlock_peer_device_id,
        )?;
        let registry_peer_device_id = cstring(
            "indoor.registry_peer_device_id",
            &config.registry_peer_device_id,
        )?;
        let raw_cfg = indoor_sys::kr_indoor_config_t {
            enabled: config.enabled,
            interface: interface.as_ptr(),
            target_device_id: target_device_id.as_ptr(),
            room_id: room_id.as_ptr(),
            unlock_peer_device_id: unlock_peer_device_id.as_ptr(),
            registry_peer_device_id: registry_peer_device_id.as_ptr(),
            direct_plc_cco_addr: config.direct_plc_cco_addr,
            playback_volume_percent: config.playback_volume_percent,
        };

        let mut handle = std::ptr::null_mut();
        let create_rc = unsafe { indoor_sys::kr_indoor_create(&mut handle, &raw_cfg) };
        if create_rc < 0 {
            return Err(format!("kr_indoor_create failed: {create_rc}"));
        }

        let handle = std::ptr::NonNull::new(handle)
            .ok_or_else(|| "kr_indoor_create returned null handle".to_string())?;
        let start_rc = unsafe { indoor_sys::kr_indoor_start(handle.as_ptr()) };
        if start_rc < 0 {
            unsafe { indoor_sys::kr_indoor_destroy(handle.as_ptr()) };
            return Err(format!("kr_indoor_start failed: {start_rc}"));
        }

        let inner = Arc::new(IndoorInner::Native(NativeIndoorInner {
            handle: Mutex::new(Some(handle)),
        }));
        let worker_inner = inner.clone();
        let worker = thread::spawn(move || poll_native_events(worker_inner, dispatcher));

        Ok(Self {
            inner,
            worker: Some(worker),
        })
    }

    fn stub(config: IndoorConfig, dispatcher: EventDispatcher) -> Self {
        let (tx, rx) = mpsc::channel();
        let inner = Arc::new(IndoorInner::Stub(StubIndoorInner {
            stopping: AtomicBool::new(false),
            sender: tx,
        }));
        let worker_inner = inner.clone();
        let worker = thread::spawn(move || {
            while !worker_inner.is_stopping() {
                match rx.recv_timeout(Duration::from_millis(100)) {
                    Ok(event) => dispatcher(event),
                    Err(mpsc::RecvTimeoutError::Timeout) => continue,
                    Err(mpsc::RecvTimeoutError::Disconnected) => break,
                }
            }
        });

        if config.enabled {
            eprintln!("indoor: using stub backend");
        }
        Self {
            inner,
            worker: Some(worker),
        }
    }

    pub fn call_controller(&self, config: &IndoorConfig) -> CallController {
        CallController {
            inner: self.inner.clone(),
            default_target: config.target_device_id.clone(),
        }
    }

    pub fn d2d_controller(&self, config: &IndoorConfig) -> D2dController {
        D2dController {
            inner: self.inner.clone(),
            default_target: config.unlock_peer_device_id.clone(),
            room_id: config.room_id.clone(),
        }
    }
}

impl Drop for IndoorService {
    fn drop(&mut self) {
        self.inner.stop();
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();
        }
        self.inner.destroy();
    }
}

impl CallController {
    pub fn accept(&self) -> Result<(), String> {
        self.inner.native_call("kr_indoor_accept", |handle| unsafe {
            indoor_call_accept(handle)
        })
    }

    pub fn reject(&self) -> Result<(), String> {
        self.inner.native_call("kr_indoor_reject", |handle| unsafe {
            indoor_call_reject(handle)
        })
    }

    pub fn hangup(&self) -> Result<(), String> {
        self.inner.native_call("kr_indoor_hangup", |handle| unsafe {
            indoor_call_hangup(handle)
        })
    }

    pub fn request_video_recovery(&self, use_fir: bool) -> Result<(), String> {
        self.inner
            .native_call("kr_indoor_request_video_recovery", |handle| unsafe {
                indoor_request_video_recovery(handle, use_fir)
            })
    }

    pub fn start_call(&self, target_device_id: &str) -> Result<(), String> {
        let target = self.target_or_default(target_device_id);
        self.inner
            .native_call_with_cstr("kr_indoor_call", &target, |handle, target| unsafe {
                indoor_call_start(handle, target)
            })
    }

    #[allow(dead_code)]
    pub fn monitor(&self, target_device_id: &str) -> Result<(), String> {
        let target = self.target_or_default(target_device_id);
        self.inner
            .native_call_with_cstr("kr_indoor_monitor", &target, |handle, target| unsafe {
                indoor_call_monitor(handle, target)
            })
    }

    pub fn stop_monitor(&self) -> Result<(), String> {
        self.inner
            .native_call("kr_indoor_monitor_stop", |handle| unsafe {
                indoor_call_monitor_stop(handle)
            })
    }

    pub fn set_playback_volume(&self, volume: f32) -> Result<(), String> {
        let scaled = (volume.clamp(0.0, 1.0) * 100.0).round() as i32;
        self.inner
            .native_call("kr_indoor_set_playback_volume", |handle| unsafe {
                indoor_set_playback_volume(handle, scaled)
            })
    }

    pub fn set_preview_brightness(&self, value: f32) -> Result<(), String> {
        let scaled = (value.clamp(0.0, 1.0) * 100.0).round() as i32;
        self.inner
            .native_call("kr_indoor_set_preview_brightness", |handle| unsafe {
                indoor_set_preview_brightness(handle, scaled)
            })
    }

    pub fn set_external_call_busy(&self, busy: bool) -> Result<(), String> {
        self.inner
            .native_call("kr_indoor_set_external_call_busy", |handle| unsafe {
                indoor_set_external_call_busy(handle, busy)
            })
    }

    pub fn set_webrtc_bridge(
        &self,
        webrtc_handle: Option<*mut crate::webrtc_sys::kr_webrtc_handle_t>,
    ) -> Result<(), String> {
        self.inner
            .native_call("kr_indoor_set_webrtc_bridge", |handle| unsafe {
                indoor_set_webrtc_bridge(handle, webrtc_handle)
            })
    }

    pub fn set_media_bridge(&self, enabled: bool) -> Result<(), String> {
        self.inner
            .native_call("kr_indoor_set_media_bridge", |handle| unsafe {
                indoor_set_media_bridge(handle, enabled)
            })
    }

    fn target_or_default(&self, target_device_id: &str) -> String {
        if target_device_id.is_empty() {
            self.default_target.clone()
        } else {
            target_device_id.to_string()
        }
    }
}

impl D2dController {
    pub fn unlock_default_door(&self) -> Result<(), String> {
        self.unlock(&self.default_target)
    }

    pub fn unlock(&self, door_id: &str) -> Result<(), String> {
        let target = if door_id.is_empty() {
            self.default_target.as_str()
        } else {
            door_id
        };
        let target =
            CString::new(target).map_err(|_| "door id contains interior NUL".to_string())?;
        let room_id = CString::new(self.room_id.as_str())
            .map_err(|_| "room id contains interior NUL".to_string())?;
        self.inner.native_call("kr_indoor_unlock", |handle| unsafe {
            indoor_unlock(handle, target.as_ptr(), room_id.as_ptr(), 1)
        })
    }

    pub fn set_unlock_password(&self, password: &str) -> Result<(), String> {
        if password.as_bytes().contains(&0) {
            return Err("password contains interior NUL".to_string());
        }
        eprintln!(
            "indoor: remote_password_sync_pending password_len={}",
            password.len()
        );
        Ok(())
    }

    pub fn set_local_password(&self, password: &str) -> Result<(), String> {
        if password.as_bytes().contains(&0) {
            return Err("password contains interior NUL".to_string());
        }
        Ok(())
    }
}

impl IndoorInner {
    fn is_stopping(&self) -> bool {
        match self {
            #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
            Self::Native(native) => native
                .handle
                .lock()
                .ok()
                .map(|guard| guard.is_none())
                .unwrap_or(true),
            Self::Stub(stub) => stub.stopping.load(Ordering::SeqCst),
        }
    }

    fn stop(&self) {
        match self {
            #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
            Self::Native(native) => {
                if let Ok(guard) = native.handle.lock() {
                    if let Some(handle) = *guard {
                        unsafe { indoor_sys::kr_indoor_stop(handle.as_ptr()) };
                    }
                }
            }
            Self::Stub(stub) => {
                stub.stopping.store(true, Ordering::SeqCst);
            }
        }
    }

    fn destroy(&self) {
        match self {
            #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
            Self::Native(native) => {
                if let Ok(mut guard) = native.handle.lock() {
                    if let Some(handle) = guard.take() {
                        unsafe { indoor_sys::kr_indoor_destroy(handle.as_ptr()) };
                    }
                }
            }
            Self::Stub(_) => {}
        }
    }

    fn native_call(
        &self,
        name: &str,
        func: impl FnOnce(*mut IndoorNativeHandle) -> i32,
    ) -> Result<(), String> {
        match self {
            #[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
            Self::Native(native) => {
                let guard = native
                    .handle
                    .lock()
                    .map_err(|_| "indoor handle lock poisoned".to_string())?;
                let handle = guard.ok_or_else(|| "indoor handle already destroyed".to_string())?;
                let rc = func(handle.as_ptr());
                if rc < 0 {
                    Err(format!("{name} failed: {rc}"))
                } else {
                    Ok(())
                }
            }
            Self::Stub(stub) => stub_call(name, stub, func),
        }
    }

    fn native_call_with_cstr(
        &self,
        name: &str,
        value: &str,
        func: impl FnOnce(*mut IndoorNativeHandle, *const libc::c_char) -> i32,
    ) -> Result<(), String> {
        let value =
            CString::new(value).map_err(|_| format!("{name} argument contains interior NUL"))?;
        self.native_call(name, |handle| func(handle, value.as_ptr()))
    }
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
type IndoorNativeHandle = indoor_sys::kr_indoor_handle_t;
#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
type IndoorNativeHandle = std::ffi::c_void;

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
fn poll_native_events(inner: Arc<IndoorInner>, dispatcher: EventDispatcher) {
    loop {
        let maybe_handle = match &*inner {
            IndoorInner::Native(native) => native.handle.lock().ok().and_then(|guard| *guard),
            IndoorInner::Stub(_) => None,
        };
        let Some(handle) = maybe_handle else {
            break;
        };

        let mut raw_event = indoor_sys::kr_indoor_event_t::default();
        let rc = unsafe { indoor_sys::kr_indoor_wait_event(handle.as_ptr(), &mut raw_event, 250) };
        if rc == 0 {
            continue;
        }
        if rc < 0 {
            if rc == -libc::ECANCELED {
                break;
            }
            dispatcher(IndoorEvent::Call(CallEvent {
                kind: CallEventKind::Error,
                peer_id: 0,
                peer_device_id: String::new(),
                session_id: 0,
                session_state: CallSessionState::Idle,
                result_code: rc,
                video_active: false,
                audio_active: false,
            }));
            break;
        }

        if let Some(event) = map_native_event(raw_event) {
            dispatcher(event);
        }
    }
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
fn map_native_event(raw: indoor_sys::kr_indoor_event_t) -> Option<IndoorEvent> {
    match raw.kind {
        indoor_sys::KR_INDOOR_EVENT_CALL_INCOMING => Some(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::Incoming,
            peer_id: raw.peer_id,
            peer_device_id: c_string_field(&raw.text),
            session_id: raw.session_id,
            session_state: CallSessionState::from_raw(raw.session_state),
            result_code: raw.result_code,
            video_active: raw.video_active,
            audio_active: raw.audio_active,
        })),
        indoor_sys::KR_INDOOR_EVENT_CALL_STATE_CHANGED => Some(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::StateChanged,
            peer_id: raw.peer_id,
            peer_device_id: c_string_field(&raw.text),
            session_id: raw.session_id,
            session_state: CallSessionState::from_raw(raw.session_state),
            result_code: raw.result_code,
            video_active: raw.video_active,
            audio_active: raw.audio_active,
        })),
        indoor_sys::KR_INDOOR_EVENT_CALL_MEDIA_CHANGED => Some(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::MediaChanged,
            peer_id: raw.peer_id,
            peer_device_id: c_string_field(&raw.text),
            session_id: raw.session_id,
            session_state: CallSessionState::from_raw(raw.session_state),
            result_code: raw.result_code,
            video_active: raw.video_active,
            audio_active: raw.audio_active,
        })),
        indoor_sys::KR_INDOOR_EVENT_CALL_REMOTE_HANGUP => Some(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::RemoteHangup,
            peer_id: raw.peer_id,
            peer_device_id: c_string_field(&raw.text),
            session_id: raw.session_id,
            session_state: CallSessionState::from_raw(raw.session_state),
            result_code: raw.result_code,
            video_active: raw.video_active,
            audio_active: raw.audio_active,
        })),
        indoor_sys::KR_INDOOR_EVENT_CALL_ERROR => Some(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::Error,
            peer_id: raw.peer_id,
            peer_device_id: c_string_field(&raw.text),
            session_id: raw.session_id,
            session_state: CallSessionState::from_raw(raw.session_state),
            result_code: raw.result_code,
            video_active: raw.video_active,
            audio_active: raw.audio_active,
        })),
        indoor_sys::KR_INDOOR_EVENT_UNLOCK_DONE => Some(IndoorEvent::D2d(D2dEvent {
            kind: D2dEventKind::UnlockDone,
            result_code: raw.result_code,
            door_id: c_string_field(&raw.text),
            password: String::new(),
        })),
        indoor_sys::KR_INDOOR_EVENT_UNLOCK_FAIL => Some(IndoorEvent::D2d(D2dEvent {
            kind: D2dEventKind::UnlockFail,
            result_code: raw.result_code,
            door_id: c_string_field(&raw.text),
            password: String::new(),
        })),
        indoor_sys::KR_INDOOR_EVENT_UNLOCK_TIMEOUT => Some(IndoorEvent::D2d(D2dEvent {
            kind: D2dEventKind::UnlockTimeout,
            result_code: raw.result_code,
            door_id: c_string_field(&raw.text),
            password: String::new(),
        })),
        indoor_sys::KR_INDOOR_EVENT_SECONDARY_CONFIRM_ONLINE => {
            Some(IndoorEvent::SecondaryConfirm(SecondaryConfirmEvent {
                online: true,
                device_id: c_string_field(&raw.text),
                dev_model: c_string_field(&raw.text2),
                endpoint_addr: raw.peer_id,
                result_code: raw.result_code,
            }))
        }
        indoor_sys::KR_INDOOR_EVENT_SECONDARY_CONFIRM_OFFLINE => {
            Some(IndoorEvent::SecondaryConfirm(SecondaryConfirmEvent {
                online: false,
                device_id: c_string_field(&raw.text),
                dev_model: c_string_field(&raw.text2),
                endpoint_addr: raw.peer_id,
                result_code: raw.result_code,
            }))
        }
        indoor_sys::KR_INDOOR_EVENT_ERROR => Some(IndoorEvent::D2d(D2dEvent {
            kind: D2dEventKind::Error,
            result_code: raw.result_code,
            door_id: c_string_field(&raw.text),
            password: String::new(),
        })),
        indoor_sys::KR_INDOOR_EVENT_NONE => None,
        _ => None,
    }
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
fn cstring(name: &str, value: &str) -> Result<CString, String> {
    CString::new(value).map_err(|_| format!("{name} contains interior NUL"))
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
fn c_string_field<const N: usize>(field: &[libc::c_char; N]) -> String {
    if field[0] == 0 {
        return String::new();
    }
    unsafe { CStr::from_ptr(field.as_ptr()) }
        .to_string_lossy()
        .into_owned()
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_call_accept(handle: *mut IndoorNativeHandle) -> i32 {
    indoor_sys::kr_indoor_accept(handle)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_accept(_handle: *mut IndoorNativeHandle) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_call_reject(handle: *mut IndoorNativeHandle) -> i32 {
    indoor_sys::kr_indoor_reject(handle)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_reject(_handle: *mut IndoorNativeHandle) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_call_hangup(handle: *mut IndoorNativeHandle) -> i32 {
    indoor_sys::kr_indoor_hangup(handle)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_hangup(_handle: *mut IndoorNativeHandle) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_request_video_recovery(handle: *mut IndoorNativeHandle, use_fir: bool) -> i32 {
    indoor_sys::kr_indoor_request_video_recovery(handle, use_fir)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_request_video_recovery(_handle: *mut IndoorNativeHandle, _use_fir: bool) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_call_start(handle: *mut IndoorNativeHandle, target: *const libc::c_char) -> i32 {
    indoor_sys::kr_indoor_call(handle, target)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_start(_handle: *mut IndoorNativeHandle, _target: *const libc::c_char) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
#[allow(dead_code)]
unsafe fn indoor_call_monitor(handle: *mut IndoorNativeHandle, target: *const libc::c_char) -> i32 {
    indoor_sys::kr_indoor_monitor(handle, target)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_monitor(
    _handle: *mut IndoorNativeHandle,
    _target: *const libc::c_char,
) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_call_monitor_stop(handle: *mut IndoorNativeHandle) -> i32 {
    indoor_sys::kr_indoor_monitor_stop(handle)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_call_monitor_stop(_handle: *mut IndoorNativeHandle) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_unlock(
    handle: *mut IndoorNativeHandle,
    target: *const libc::c_char,
    room_id: *const libc::c_char,
    place: i32,
) -> i32 {
    indoor_sys::kr_indoor_unlock(handle, target, room_id, place)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_unlock(
    _handle: *mut IndoorNativeHandle,
    _target: *const libc::c_char,
    _room_id: *const libc::c_char,
    _place: i32,
) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_set_playback_volume(handle: *mut IndoorNativeHandle, volume: i32) -> i32 {
    indoor_sys::kr_indoor_set_playback_volume(handle, volume)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_set_playback_volume(_handle: *mut IndoorNativeHandle, _volume: i32) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_set_preview_brightness(handle: *mut IndoorNativeHandle, level: i32) -> i32 {
    indoor_sys::kr_indoor_set_preview_brightness(handle, level)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_set_preview_brightness(_handle: *mut IndoorNativeHandle, _level: i32) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_set_external_call_busy(handle: *mut IndoorNativeHandle, busy: bool) -> i32 {
    indoor_sys::kr_indoor_set_external_call_busy(handle, busy)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_set_external_call_busy(_handle: *mut IndoorNativeHandle, _busy: bool) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_set_webrtc_bridge(
    handle: *mut IndoorNativeHandle,
    webrtc_handle: Option<*mut crate::webrtc_sys::kr_webrtc_handle_t>,
) -> i32 {
    indoor_sys::kr_indoor_set_webrtc_bridge(
        handle,
        webrtc_handle.unwrap_or(std::ptr::null_mut()).cast(),
    )
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_set_webrtc_bridge(
    _handle: *mut IndoorNativeHandle,
    _webrtc_handle: Option<*mut crate::webrtc_sys::kr_webrtc_handle_t>,
) -> i32 {
    0
}

#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
unsafe fn indoor_set_media_bridge(handle: *mut IndoorNativeHandle, enabled: bool) -> i32 {
    indoor_sys::kr_indoor_set_media_bridge(handle, enabled)
}

#[cfg(any(not(target_arch = "riscv64"), kr_indoor_stub_backend))]
unsafe fn indoor_set_media_bridge(_handle: *mut IndoorNativeHandle, _enabled: bool) -> i32 {
    0
}

fn stub_call(
    name: &str,
    stub: &StubIndoorInner,
    func: impl FnOnce(*mut IndoorNativeHandle) -> i32,
) -> Result<(), String> {
    let rc = func(std::ptr::null_mut());
    if rc < 0 {
        return Err(format!("{name} failed: {rc}"));
    }
    if name == "kr_indoor_call" || name == "kr_indoor_monitor" || name == "kr_indoor_monitor_stop" {
        let _ = stub.sender.send(IndoorEvent::Call(CallEvent {
            kind: CallEventKind::StateChanged,
            peer_id: 0,
            peer_device_id: String::new(),
            session_id: 0,
            session_state: if name == "kr_indoor_monitor" {
                CallSessionState::Monitor
            } else if name == "kr_indoor_monitor_stop" {
                CallSessionState::Idle
            } else {
                CallSessionState::Ringing
            },
            result_code: 0,
            video_active: name == "kr_indoor_monitor",
            audio_active: false,
        }));
    }
    Ok(())
}
