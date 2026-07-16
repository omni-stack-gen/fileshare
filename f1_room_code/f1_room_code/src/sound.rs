use std::ffi::CString;
use std::ptr::NonNull;
use std::sync::{Arc, Mutex};

#[derive(Clone)]
pub struct SoundController {
    inner: Arc<SoundInner>,
}

struct SoundInner {
    handle: Mutex<Option<NonNull<crate::sound_sys::kr_sound_handle_t>>>,
}

unsafe impl Send for SoundInner {}
unsafe impl Sync for SoundInner {}

impl SoundController {
    pub fn start(initial_volume: f32) -> Result<Self, String> {
        let asset_dir = std::env::var("KR_SOUND_ASSET_DIR")
            .unwrap_or_else(|_| "/system/resources/sound/".to_string());
        let asset_dir = cstring("sound.asset_dir", &asset_dir)?;
        let cfg = crate::sound_sys::kr_sound_config_t {
            asset_dir: asset_dir.as_ptr(),
            volume_percent: volume_percent(initial_volume),
        };

        let mut handle = std::ptr::null_mut();
        let rc = unsafe { crate::sound_sys::kr_sound_create(&mut handle, &cfg) };
        if rc < 0 {
            return Err(format!("kr_sound_create failed: {rc}"));
        }
        let handle = NonNull::new(handle)
            .ok_or_else(|| "kr_sound_create returned null handle".to_string())?;
        Ok(Self {
            inner: Arc::new(SoundInner {
                handle: Mutex::new(Some(handle)),
            }),
        })
    }

    pub fn play_touch(&self) -> Result<(), String> {
        self.native_call("kr_sound_play_touch", |handle| unsafe {
            crate::sound_sys::kr_sound_play_touch(handle)
        })
    }

    pub fn start_ring(&self, kind: &str, melody: i32, volume: f32) -> Result<(), String> {
        let kind = cstring("sound.ring_kind", kind)?;
        self.native_call("kr_sound_start_ring", |handle| unsafe {
            crate::sound_sys::kr_sound_start_ring(
                handle,
                kind.as_ptr(),
                melody,
                volume_percent(volume),
            )
        })
    }

    pub fn stop_ring(&self) -> Result<(), String> {
        self.native_call("kr_sound_stop_ring", |handle| unsafe {
            crate::sound_sys::kr_sound_stop_ring(handle)
        })
    }

    pub fn set_volume(&self, volume: f32) -> Result<(), String> {
        self.native_call("kr_sound_set_volume", |handle| unsafe {
            crate::sound_sys::kr_sound_set_volume(handle, volume_percent(volume))
        })
    }

    fn native_call<F>(&self, label: &str, f: F) -> Result<(), String>
    where
        F: FnOnce(*mut crate::sound_sys::kr_sound_handle_t) -> i32,
    {
        let guard = self
            .inner
            .handle
            .lock()
            .map_err(|_| "sound handle lock poisoned".to_string())?;
        let Some(handle) = *guard else {
            return Err("sound handle closed".to_string());
        };
        let rc = f(handle.as_ptr());
        if rc < 0 {
            Err(format!("{label} failed: {rc}"))
        } else {
            Ok(())
        }
    }
}

impl Drop for SoundInner {
    fn drop(&mut self) {
        if let Ok(mut guard) = self.handle.lock() {
            if let Some(handle) = guard.take() {
                unsafe {
                    crate::sound_sys::kr_sound_destroy(handle.as_ptr());
                }
            }
        }
    }
}

fn volume_percent(value: f32) -> i32 {
    (value.clamp(0.0, 1.0) * 100.0).round() as i32
}

fn cstring(label: &str, value: &str) -> Result<CString, String> {
    CString::new(value).map_err(|_| format!("{label} contains interior NUL"))
}
