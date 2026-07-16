use std::ffi::{CStr, CString};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};

use crate::ipc_sys;

#[derive(Clone, Debug)]
pub struct IpcCameraConfig {
    pub name: String,
    pub ip: String,
    pub user: String,
    pub password: String,
    pub rtsp_url: String,
    pub sub_rtsp_url: String,
    pub enabled: bool,
    pub prefer_substream: bool,
}

impl IpcCameraConfig {
    pub fn is_usable(&self) -> bool {
        self.enabled
            && (!self.ip.trim().is_empty()
                || !self.rtsp_url.trim().is_empty()
                || !self.sub_rtsp_url.trim().is_empty())
    }
}

#[derive(Clone, Debug)]
pub struct IpcConfig {
    pub enabled: bool,
    pub interface: String,
    pub preview_x: i32,
    pub preview_y: i32,
    pub preview_width: i32,
    pub preview_height: i32,
    pub capture_dir: String,
    pub cameras: Vec<IpcCameraConfig>,
}

impl Default for IpcConfig {
    fn default() -> Self {
        Self {
            enabled: false,
            interface: "wlan0".to_string(),
            preview_x: 24,
            preview_y: 46,
            preview_width: 800,
            preview_height: 480,
            capture_dir: default_capture_dir(),
            cameras: Vec::new(),
        }
    }
}

#[cfg(target_arch = "riscv64")]
impl IpcConfig {
    pub fn from_database(database: &crate::db::Database) -> Self {
        let settings = database.settings().ok();
        let cameras = database.ipc_cameras().unwrap_or_default();
        let Some(settings) = settings else {
            return Self {
                cameras,
                ..Self::default()
            };
        };

        let capture_dir = settings.get_string("ipc.capture_dir", "").to_string();
        Self {
            enabled: settings.get_bool("ipc.enabled", false),
            interface: settings.get_string("ipc.interface", "wlan0").to_string(),
            preview_x: settings.get_int("ipc.preview_x", 24),
            preview_y: settings.get_int("ipc.preview_y", 46),
            preview_width: settings.get_int("ipc.preview_width", 800),
            preview_height: settings.get_int("ipc.preview_height", 480),
            capture_dir: if capture_dir.trim().is_empty() {
                default_capture_dir()
            } else {
                capture_dir
            },
            cameras,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum IpcEventKind {
    None,
    MonitorStarted,
    MonitorStopped,
    StreamReady,
    Offline,
    CaptureSaved,
    Info,
    Error,
}

#[allow(dead_code)]
#[derive(Clone, Debug)]
pub struct IpcEvent {
    pub kind: IpcEventKind,
    pub value0: i32,
    pub value1: i32,
    pub text1: String,
    pub text2: String,
    pub text3: String,
}

#[derive(Clone)]
pub struct IpcController {
    inner: Arc<IpcControllerInner>,
}

pub struct IpcService {
    controller: IpcController,
    worker: Option<JoinHandle<()>>,
}

type EventDispatcher = Arc<dyn Fn(IpcEvent) + Send + Sync + 'static>;

struct IpcControllerInner {
    handle: Mutex<Option<std::ptr::NonNull<ipc_sys::kr_ipc_handle_t>>>,
    operation_lock: Mutex<()>,
}

unsafe impl Send for IpcControllerInner {}
unsafe impl Sync for IpcControllerInner {}

fn ipc_log(message: impl AsRef<str>) {
    eprintln!("ipc: {}", message.as_ref());
}

impl IpcService {
    pub fn start(config: &IpcConfig, dispatcher: EventDispatcher) -> Result<Self, String> {
        ipc_log(format!(
            "service start interface={} preview=({},{} {}x{}) enabled_cameras={}",
            config.interface,
            config.preview_x,
            config.preview_y,
            config.preview_width,
            config.preview_height,
            config
                .cameras
                .iter()
                .filter(|camera| camera.is_usable())
                .count()
        ));
        let interface = CString::new(config.interface.as_str())
            .map_err(|_| "ipc.interface contains interior NUL".to_string())?;
        let raw_cfg = ipc_sys::kr_ipc_config_t {
            interface: interface.as_ptr(),
            preview_x: config.preview_x,
            preview_y: config.preview_y,
            preview_width: config.preview_width,
            preview_height: config.preview_height,
        };

        let mut handle = std::ptr::null_mut();
        let create_rc = unsafe { ipc_sys::kr_ipc_create(&mut handle, &raw_cfg) };
        ipc_log(format!("kr_ipc_create rc={create_rc}"));
        if create_rc < 0 {
            return Err(format!("kr_ipc_create failed: {create_rc}"));
        }

        let handle = std::ptr::NonNull::new(handle)
            .ok_or_else(|| "kr_ipc_create returned null handle".to_string())?;
        let inner = Arc::new(IpcControllerInner {
            handle: Mutex::new(Some(handle)),
            operation_lock: Mutex::new(()),
        });
        let worker_inner = inner.clone();
        let worker_dispatcher = dispatcher.clone();
        let worker = thread::spawn(move || loop {
            let maybe_handle = worker_inner.handle.lock().ok().and_then(|guard| *guard);
            let Some(handle) = maybe_handle else {
                break;
            };

            let mut raw_event = ipc_sys::kr_ipc_event_t::default();
            let rc = unsafe { ipc_sys::kr_ipc_wait_event(handle.as_ptr(), &mut raw_event, 250) };
            if rc == 0 {
                continue;
            }
            if rc < 0 {
                if rc == -libc::ECANCELED {
                    break;
                }
                worker_dispatcher(IpcEvent {
                    kind: IpcEventKind::Error,
                    value0: rc,
                    value1: 0,
                    text1: "wait_event_failed".to_string(),
                    text2: String::new(),
                    text3: String::new(),
                });
                break;
            }

            let event = map_native_event(raw_event);
            ipc_log(format!(
                "event kind={:?} value0={} value1={} text1={} text2={} text3={}",
                event.kind, event.value0, event.value1, event.text1, event.text2, event.text3
            ));
            worker_dispatcher(event);
        });

        Ok(Self {
            controller: IpcController { inner },
            worker: Some(worker),
        })
    }

    pub fn controller(&self) -> IpcController {
        self.controller.clone()
    }
}

impl Drop for IpcService {
    fn drop(&mut self) {
        ipc_log("service drop");
        let _ = self.controller.stop_monitor();
        self.controller.destroy();
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();
        }
    }
}

impl IpcController {
    pub fn start_monitor(&self, camera: &IpcCameraConfig) -> Result<(), String> {
        if !camera.is_usable() {
            return Err("ipc_camera_not_configured".to_string());
        }
        ipc_log(format!(
            "start_monitor name={} ip={} user={} prefer_substream={} rtsp_url_present={} sub_rtsp_url_present={}",
            camera.name,
            camera.ip,
            camera.user,
            camera.prefer_substream,
            !camera.rtsp_url.trim().is_empty(),
            !camera.sub_rtsp_url.trim().is_empty()
        ));

        let name = CString::new(camera.name.as_str())
            .map_err(|_| "ipc camera name contains interior NUL".to_string())?;
        let ip = CString::new(camera.ip.as_str())
            .map_err(|_| "ipc camera ip contains interior NUL".to_string())?;
        let user = CString::new(camera.user.as_str())
            .map_err(|_| "ipc camera user contains interior NUL".to_string())?;
        let password = CString::new(camera.password.as_str())
            .map_err(|_| "ipc camera password contains interior NUL".to_string())?;
        let rtsp_url = CString::new(camera.rtsp_url.as_str())
            .map_err(|_| "ipc camera rtsp_url contains interior NUL".to_string())?;
        let sub_rtsp_url = CString::new(camera.sub_rtsp_url.as_str())
            .map_err(|_| "ipc camera sub_rtsp_url contains interior NUL".to_string())?;

        let raw_cfg = ipc_sys::kr_ipc_monitor_config_t {
            name: name.as_ptr(),
            ip: ip.as_ptr(),
            user: user.as_ptr(),
            password: password.as_ptr(),
            rtsp_url: rtsp_url.as_ptr(),
            sub_rtsp_url: sub_rtsp_url.as_ptr(),
            prefer_substream: camera.prefer_substream,
        };

        let rc =
            self.with_handle(|handle| unsafe { ipc_sys::kr_ipc_start_monitor(handle, &raw_cfg) })?;
        ipc_log(format!("kr_ipc_start_monitor rc={rc} name={}", camera.name));
        Ok(())
    }

    pub fn stop_monitor(&self) -> Result<(), String> {
        let rc = self.with_handle(|handle| unsafe { ipc_sys::kr_ipc_stop_monitor(handle) })?;
        ipc_log(format!("kr_ipc_stop_monitor rc={rc}"));
        Ok(())
    }

    pub fn capture_jpeg(&self, output_path: &str) -> Result<(), String> {
        ipc_log(format!("capture_jpeg path={output_path}"));
        let output_path = CString::new(output_path)
            .map_err(|_| "ipc capture path contains interior NUL".to_string())?;
        let rc = self.with_handle(|handle| unsafe {
            ipc_sys::kr_ipc_capture_jpeg(handle, output_path.as_ptr())
        })?;
        ipc_log(format!("kr_ipc_capture_jpeg rc={rc}"));
        Ok(())
    }

    pub fn is_monitoring(&self) -> bool {
        self.with_handle(|handle| unsafe { ipc_sys::kr_ipc_is_monitoring(handle) })
            .map(|value| value > 0)
            .unwrap_or(false)
    }

    pub fn set_preview_brightness(&self, value: f32) -> Result<(), String> {
        let level = (value.clamp(0.0, 1.0) * 100.0).round() as i32;
        self.with_handle(|handle| unsafe {
            ipc_sys::kr_ipc_set_preview_brightness(handle, level)
        })?;
        Ok(())
    }

    fn with_handle<F>(&self, f: F) -> Result<i32, String>
    where
        F: FnOnce(*mut ipc_sys::kr_ipc_handle_t) -> i32,
    {
        let _operation_guard = self
            .inner
            .operation_lock
            .lock()
            .map_err(|_| "ipc operation lock poisoned".to_string())?;
        let handle = self
            .inner
            .handle
            .lock()
            .map_err(|_| "ipc controller lock poisoned".to_string())?
            .ok_or_else(|| "ipc handle unavailable".to_string())?;
        let rc = f(handle.as_ptr());
        if rc < 0 {
            Err(format!("ipc call failed: {rc}"))
        } else {
            Ok(rc)
        }
    }

    fn destroy(&self) {
        if let Ok(mut guard) = self.inner.handle.lock() {
            if let Some(handle) = guard.take() {
                ipc_log("destroy handle");
                let _ = unsafe { ipc_sys::kr_ipc_destroy(handle.as_ptr()) };
            }
        }
    }
}

fn default_capture_dir() -> String {
    if std::path::Path::new("/data").exists() {
        "/data/kr_room/captures".to_string()
    } else {
        "captures".to_string()
    }
}

fn map_native_event(raw: ipc_sys::kr_ipc_event_t) -> IpcEvent {
    IpcEvent {
        kind: match raw.kind {
            1 => IpcEventKind::MonitorStarted,
            2 => IpcEventKind::MonitorStopped,
            3 => IpcEventKind::StreamReady,
            4 => IpcEventKind::Offline,
            5 => IpcEventKind::CaptureSaved,
            6 => IpcEventKind::Info,
            7 => IpcEventKind::Error,
            _ => IpcEventKind::None,
        },
        value0: raw.value0,
        value1: raw.value1,
        text1: c_buf_to_string(&raw.text1),
        text2: c_buf_to_string(&raw.text2),
        text3: c_buf_to_string(&raw.text3),
    }
}

fn c_buf_to_string(buf: &[std::os::raw::c_char]) -> String {
    let ptr = buf.as_ptr();
    if ptr.is_null() {
        return String::new();
    }
    unsafe { CStr::from_ptr(ptr) }
        .to_string_lossy()
        .into_owned()
}
