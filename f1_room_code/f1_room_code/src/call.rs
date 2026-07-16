use slint::SharedString;

pub use crate::indoor::CallController;

#[allow(dead_code)]
#[derive(Clone, Debug)]
pub struct CallConfig {
    pub enabled: bool,
    pub role: String,
    pub self_id: u32,
    pub upstream_id: u32,
    pub ctrl_port: u32,
    pub interface: String,
}

impl Default for CallConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            role: "indoor".to_string(),
            self_id: 0x10000,
            upstream_id: 0x20000,
            ctrl_port: 0x02,
            interface: "eth0".to_string(),
        }
    }
}

#[cfg(target_arch = "riscv64")]
impl CallConfig {
    pub fn from_settings(settings: &crate::db::Settings) -> Self {
        Self {
            enabled: settings.get_bool("call.enabled", true),
            role: settings.get_string("call.role", "indoor").to_string(),
            self_id: settings.get_int("call.self_id", 0x10000) as u32,
            upstream_id: settings.get_int("call.upstream_id", 0x20000) as u32,
            ctrl_port: settings.get_int("call.ctrl_port", 0x02) as u32,
            interface: settings.get_string("call.interface", "eth0").to_string(),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum CallEventKind {
    Incoming,
    StateChanged,
    MediaChanged,
    RemoteHangup,
    Error,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum CallSessionState {
    Idle,
    Ringing,
    VideoOnly,
    Active,
    Monitor,
}

impl CallSessionState {
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            1 => Self::Ringing,
            2 => Self::VideoOnly,
            3 => Self::Active,
            4 => Self::Monitor,
            _ => Self::Idle,
        }
    }

    fn as_slint(self) -> &'static str {
        match self {
            Self::Idle => "idle",
            Self::Ringing => "ringing",
            Self::VideoOnly => "video_only",
            Self::Active => "active",
            Self::Monitor => "monitor",
        }
    }
}

#[derive(Clone, Debug)]
pub struct CallEvent {
    pub kind: CallEventKind,
    pub peer_id: u32,
    pub peer_device_id: String,
    #[allow(dead_code)]
    pub session_id: u64,
    pub session_state: CallSessionState,
    pub result_code: i32,
    pub video_active: bool,
    pub audio_active: bool,
}

pub fn apply_event_to_state(state: &crate::State<'_>, event: &CallEvent) {
    apply_event_to_state_with_incoming_page(state, event, None);
}

pub fn apply_event_to_state_with_incoming_page(
    state: &crate::State<'_>,
    event: &CallEvent,
    incoming_page: Option<&str>,
) {
    if event.peer_id != 0 {
        state.set_call_peer_id(SharedString::from(format!("0x{:x}", event.peer_id)));
    } else if !event.peer_device_id.is_empty() {
        state.set_call_peer_id(SharedString::from(event.peer_device_id.clone()));
    }

    match event.kind {
        CallEventKind::Incoming => {
            interrupt_monitor_for_incoming(state);
            state.set_call_last_error(SharedString::from(""));
            state.set_call_session_state(SharedString::from("ringing"));
            state.set_call_video_active(false);
            state.set_call_audio_active(false);
            if let Some(page) = incoming_page {
                state.set_current_page(SharedString::from(page));
            } else {
                state.set_current_page(state.get_incoming_call_page());
            }
        }
        CallEventKind::StateChanged => {
            state.set_call_session_state(SharedString::from(event.session_state.as_slint()));
            state.set_call_last_error(SharedString::from(""));
            if event.session_state == CallSessionState::Idle {
                cleanup_call_ui(state);
            }
        }
        CallEventKind::MediaChanged => {
            state.set_call_video_active(event.video_active);
            state.set_call_audio_active(event.audio_active);
            state.set_call_last_error(SharedString::from(""));
        }
        CallEventKind::RemoteHangup => {
            cleanup_call_ui(state);
        }
        CallEventKind::Error => {
            state.set_call_last_error(SharedString::from(result_code_text(event.result_code)));
            cleanup_call_ui(state);
        }
    }
}

pub fn apply_outgoing_ringing(state: &crate::State<'_>, peer_label: &str) {
    state.set_call_last_error(SharedString::from(""));
    state.set_call_peer_id(SharedString::from(peer_label));
    state.set_call_session_state(SharedString::from("ringing"));
    state.set_call_video_active(false);
    state.set_call_audio_active(false);
}

pub fn apply_call_error(state: &crate::State<'_>, error_text: impl Into<SharedString>) {
    state.set_call_last_error(error_text.into());
}

pub fn apply_call_peer_id(state: &crate::State<'_>, peer_label: impl Into<SharedString>) {
    state.set_call_peer_id(peer_label.into());
}

pub fn apply_call_idle(state: &crate::State<'_>) {
    cleanup_call_ui(state);
}

pub fn apply_monitor_active(state: &crate::State<'_>) {
    state.set_call_last_error(SharedString::from(""));
    state.set_call_session_state(SharedString::from("monitor"));
    state.set_call_video_active(true);
    state.set_call_audio_active(false);
}

pub fn interrupt_monitor_for_incoming(state: &crate::State<'_>) {
    if state.get_call_session_state().as_str() == "monitor" {
        state.set_call_video_active(false);
        state.set_call_audio_active(false);
        state.set_call_session_state(SharedString::from("idle"));
    }
    reset_call_effect_ui(state);
    state.set_active_overlay(SharedString::from(""));
    state.set_pressed_effect(SharedString::from(""));
    state.set_call_effect_locked(false);
}

pub fn apply_cloud_ringing(state: &crate::State<'_>) {
    state.set_call_session_state(SharedString::from("ringing"));
}

fn cleanup_call_ui(state: &crate::State<'_>) {
    reset_call_effect_ui(state);
    state.set_active_overlay(SharedString::from(""));
    state.set_pressed_effect(SharedString::from(""));
    state.set_call_effect_locked(false);
    state.set_call_session_state(SharedString::from("idle"));
    state.set_call_peer_id(SharedString::from(""));
    state.set_call_video_active(false);
    state.set_call_audio_active(false);
    state.set_call_bridge_prompt_visible(false);
    state.set_call_bridge_prompt_text(SharedString::from(""));
    state.set_current_page(SharedString::from("LCvek"));
}

fn reset_call_effect_ui(state: &crate::State<'_>) {
    state.set_door_call_effect_active(false);
    state.set_door_open_effect_active(false);
    state.set_lobby_call_effect_active(false);
    state.set_lobby_open_effect_active(false);
    state.set_lobby_end_effect_active(false);
    state.set_guard_call_effect_active(false);
    state.set_ev_call_effect_active(false);
    state.set_ev_cancel_effect_active(false);
}

fn result_code_text(result_code: i32) -> String {
    match result_code {
        1 => "busy".to_string(),
        2 => "timeout".to_string(),
        3 => "bad_request".to_string(),
        4 => "unsupported".to_string(),
        5 => "conflict".to_string(),
        6 => "internal_error".to_string(),
        other => format!("error_{other}"),
    }
}

#[cfg(test)]
mod tests {
    use super::{result_code_text, CallEvent, CallEventKind, CallSessionState};

    #[test]
    fn maps_result_codes() {
        assert_eq!(result_code_text(1), "busy");
        assert_eq!(result_code_text(6), "internal_error");
    }

    #[test]
    fn keeps_incoming_event_semantics() {
        let event = CallEvent {
            kind: CallEventKind::Incoming,
            peer_id: 0x20000,
            peer_device_id: String::new(),
            session_id: 7,
            session_state: CallSessionState::Ringing,
            result_code: 0,
            video_active: false,
            audio_active: false,
        };

        assert_eq!(event.kind, CallEventKind::Incoming);
        assert_eq!(event.session_state, CallSessionState::Ringing);
        assert_eq!(event.peer_id, 0x20000);
    }
}
