use std::sync::{Arc, Mutex};

use slint::SharedString;

use crate::{aiot, call, d2d, indoor, webrtc, State};

mod callx;
mod media_policy;

use callx::{CallxSession, CallxSessionKind};
pub(crate) use media_policy::{should_stop_ipc_for_callx_invited, should_stop_ipc_for_page_change};

pub const AIOT_CALLX_INVITED: i32 = 0x2005;
pub const AIOT_CALLX_INITIATE_ACK: i32 = 0x2002;
pub const AIOT_CALLX_INVITE_ACK: i32 = 0x2004;
pub const AIOT_CALLX_REPLIED: i32 = 0x2007;
pub const AIOT_CALLX_ACCEPTED: i32 = 0x200d;
pub const AIOT_CALLX_CONFIRMED: i32 = 0x200f;
pub const AIOT_CALLX_HUNGUP: i32 = 0x2013;
pub const AIOT_MANAGER_CALL_TARGET: &str = "aiot:manager";
pub(crate) const SECONDARY_CONFIRM_DEVICE_ID: &str = "3:10.2.17.1";
pub(crate) const SECONDARY_FORWARD_TO_APP_ENABLED: bool = true;

fn call_timeout_disabled_by_debug_file() -> bool {
    std::path::Path::new("/data/debugflag").exists()
}

#[derive(Clone)]
pub struct RuntimeCoordinator {
    aiot_controller: Arc<Mutex<Option<aiot::AiotController>>>,
    webrtc_runtime: Option<webrtc::WebrtcRuntime>,
    state: Arc<Mutex<CallxSession>>,
}

impl RuntimeCoordinator {
    pub fn new(
        aiot_controller: Arc<Mutex<Option<aiot::AiotController>>>,
        webrtc_runtime: Option<webrtc::WebrtcRuntime>,
    ) -> Self {
        Self {
            aiot_controller,
            webrtc_runtime,
            state: Arc::new(Mutex::new(CallxSession::default())),
        }
    }

    pub fn has_active_call(&self) -> bool {
        self.state
            .lock()
            .ok()
            .map(|state| state.is_active())
            .unwrap_or(false)
    }

    pub fn has_secondary_forward_call(&self) -> bool {
        self.state
            .lock()
            .ok()
            .map(|state| state.is_secondary_forward())
            .unwrap_or(false)
    }

    pub fn has_confirmed_secondary_forward_call(&self) -> bool {
        self.state
            .lock()
            .ok()
            .map(|state| state.is_secondary_forward_confirmed())
            .unwrap_or(false)
    }

    pub fn set_webrtc_call_busy(&self, busy: bool) -> Result<(), String> {
        if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
            webrtc_runtime.set_call_busy(busy)?;
        }
        Ok(())
    }

    pub fn set_webrtc_preview_brightness(&self, value: f32) -> Result<(), String> {
        if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
            webrtc_runtime.set_preview_brightness(value)?;
        }
        Ok(())
    }

    pub fn set_webrtc_playback_volume(&self, value: f32) -> Result<(), String> {
        if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
            webrtc_runtime.set_playback_volume(value)?;
        }
        Ok(())
    }

    pub fn clear_parent_call_after_webrtc_end(&self, webrtc_session_id: &str) -> bool {
        let snapshot = self
            .state
            .lock()
            .ok()
            .map(|session| session.snapshot())
            .unwrap_or_default();
        if !matches!(
            snapshot.kind,
            CallxSessionKind::ParentIncoming
                | CallxSessionKind::ParentOutgoing
                | CallxSessionKind::SecondaryForward
        ) {
            return false;
        }
        if !snapshot.webrtc_session_id.is_empty() && webrtc_session_id.is_empty() {
            eprintln!(
                "aiot-ui: ignore parent callx end without session id active_callid={} active_webrtc_session_id={}",
                snapshot.callid, snapshot.webrtc_session_id
            );
            return false;
        }
        if !webrtc_session_id.is_empty()
            && !snapshot.webrtc_session_id.is_empty()
            && snapshot.webrtc_session_id != webrtc_session_id
        {
            return false;
        }
        if matches!(snapshot.kind, CallxSessionKind::SecondaryForward)
            && snapshot.webrtc_session_id.is_empty()
        {
            eprintln!(
                "aiot-ui: ignore secondary forward webrtc end before confirmed callid={} event_session_id={}",
                snapshot.callid, webrtc_session_id
            );
            return false;
        }

        eprintln!(
            "aiot-ui: clear parent callx after webrtc end callid={} webrtc_session_id={}",
            snapshot.callid, webrtc_session_id
        );
        self.clear();
        true
    }

    pub fn handle_open_lock(&self, state: &State<'_>, d2d_controller: Option<&d2d::D2dController>) {
        if let Some(d2d_controller) = d2d_controller {
            if let Err(err) = d2d_controller.unlock_default_door() {
                state.set_call_last_error(SharedString::from(format!(
                    "aiot_open_lock_failed:{err}"
                )));
            }
        } else {
            state.set_call_last_error(SharedString::from(
                "aiot_open_lock_failed:no_d2d_controller",
            ));
        }
    }

    pub fn open_lock_for_active_call(&self) -> Result<bool, String> {
        let snapshot = self
            .state
            .lock()
            .map_err(|_| "cloud call state lock poisoned".to_string())?
            .snapshot();
        if !matches!(
            snapshot.kind,
            CallxSessionKind::ParentIncoming | CallxSessionKind::ParentOutgoing
        ) || snapshot.callid.is_empty()
        {
            return Ok(false);
        }

        self.aiot_controller()?
            .send_open_ask(&snapshot.callid, &snapshot.peer_sn)?;
        Ok(true)
    }

    pub fn handle_webrtc_account(&self, state: &State<'_>, event: &aiot::AiotEvent) {
        eprintln!(
            "aiot: webrtc account event serno_len={} server={} server_len={} init_len={}",
            event.text1.len(),
            event.text2,
            event.text2.len(),
            event.text3.len()
        );
        if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
            if let Err(err) = webrtc_runtime.set_account(webrtc::WebrtcAccount {
                server_addr: event.text2.clone(),
                serno: event.text1.clone(),
                initstring: event.text3.clone(),
            }) {
                state.set_call_last_error(SharedString::from(format!(
                    "webrtc_account_failed:{err}"
                )));
            }
        }
    }

    fn send_callx_invited_ack(
        &self,
        callid: &str,
        able: bool,
        unable_cause: &str,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        let aiot = self.aiot_controller()?;
        aiot.send_callx_invited_ack(callid, able, unable_cause, ring_timeout, converse_timeout)
    }

    fn send_callx_accept(
        &self,
        callid: &str,
        caller_media: &str,
        callee_media: &str,
    ) -> Result<(), String> {
        let aiot = self.aiot_controller()?;
        aiot.send_callx_accept(callid, caller_media, callee_media)
    }

    fn send_callx_hangup(&self, callid: &str, entirely: bool, cause: &str) -> Result<(), String> {
        let aiot = self.aiot_controller()?;
        aiot.send_callx_hangup(callid, entirely, cause)
    }

    fn send_callx_terminate(&self, callid: &str) -> Result<(), String> {
        let aiot = self.aiot_controller()?;
        aiot.send_callx_terminate(callid)
    }

    fn reject_invited_busy(&self, callid: &str) -> Result<(), String> {
        self.send_callx_invited_ack(callid, false, "busy", 0, 0)?;
        if let Err(err) = self.send_callx_hangup(callid, true, "busy") {
            eprintln!("aiot-ui: callx invited busy hangup failed callid={callid}: {err}");
        }
        Ok(())
    }

    fn send_callx_invite(
        &self,
        callid: &str,
        callee_sn: &str,
        callee_channel: i32,
        mode: &str,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        let aiot = self.aiot_controller()?;
        aiot.send_callx_invite(
            callid,
            callee_sn,
            callee_channel,
            mode,
            ring_timeout,
            converse_timeout,
        )
    }

    pub fn handle_invited(
        &self,
        state: &State<'_>,
        event: &aiot::AiotEvent,
        ring_timeout: i32,
        converse_timeout: i32,
    ) -> Result<(), String> {
        if !event.text1.eq_ignore_ascii_case("webrtc") {
            self.send_callx_invited_ack(&event.callid, false, "unsupported_mode", 0, 0)?;
            return Err(format!("unsupported call mode: {}", event.text1));
        }
        if self.has_active_call() || local_call_state_busy(state) {
            self.reject_invited_busy(&event.callid)?;
            return Err("busy".to_string());
        }
        let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() else {
            self.send_callx_invited_ack(&event.callid, false, "webrtc_not_ready", 0, 0)?;
            return Err("webrtc_not_ready".to_string());
        };
        if !webrtc_runtime.is_ready() {
            self.send_callx_invited_ack(&event.callid, false, "webrtc_not_ready", 0, 0)?;
            return Err("webrtc_not_ready".to_string());
        }
        self.send_callx_invited_ack(
            &event.callid,
            true,
            "",
            ring_timeout.max(0),
            converse_timeout.max(0),
        )?;

        if let Ok(mut session) = self.state.lock() {
            let peer_label = if event.text2.is_empty() {
                event.callid.clone()
            } else {
                event.text2.clone()
            };
            session.begin_parent_incoming(
                event.callid.clone(),
                peer_label,
                event.text2.clone(),
                event.text1.clone(),
            );
        }

        let incoming_page = callx_incoming_page(event.caller_device_kind.as_str());
        call::apply_event_to_state_with_incoming_page(
            state,
            &call::CallEvent {
                kind: call::CallEventKind::Incoming,
                peer_id: 0,
                peer_device_id: String::new(),
                session_id: 0,
                session_state: call::CallSessionState::Ringing,
                result_code: 0,
                video_active: false,
                audio_active: false,
            },
            Some(incoming_page),
        );
        let peer_label = self
            .state
            .lock()
            .ok()
            .map(|session| session.peer_label())
            .unwrap_or_default();
        if !peer_label.is_empty() {
            call::apply_call_peer_id(state, peer_label);
        }
        Ok(())
    }

    pub fn handle_initiate_ack(&self, event: &aiot::AiotEvent) -> Result<(), String> {
        let invite = {
            let mut session = self
                .state
                .lock()
                .map_err(|_| "cloud call state lock poisoned".to_string())?;
            let Some(invite) = session.apply_initiate_ack(
                &event.callid,
                &event.text1,
                &event.text2,
                &event.text3,
                event.callee_channel,
                event.callee_count,
            ) else {
                return Ok(());
            };
            invite
        };
        if invite.callid.is_empty() {
            return Err("missing callid after initiate ack".to_string());
        }
        if invite.callee_sn.is_empty()
            && event.callee_user_id.is_empty()
            && event.callee_web_username.is_empty()
        {
            return Err("missing callee after initiate ack".to_string());
        }
        self.send_callx_invite(
            &invite.callid,
            &invite.callee_sn,
            invite.callee_channel,
            "webrtc",
            if call_timeout_disabled_by_debug_file() {
                0
            } else {
                event.value0.max(10)
            },
            if call_timeout_disabled_by_debug_file() {
                0
            } else {
                event.value1.max(20)
            },
        )
    }

    pub fn answer_secondary_forward_local_call(
        &self,
        local_call_controller: Option<&call::CallController>,
    ) -> Result<bool, String> {
        let (is_secondary_forward, already_answered) = self
            .state
            .lock()
            .ok()
            .map(|session| {
                (
                    session.is_secondary_forward(),
                    session.secondary_forward_local_answered(),
                )
            })
            .unwrap_or((false, false));
        if !is_secondary_forward {
            return Ok(false);
        }
        self.set_webrtc_call_busy(false)?;
        if already_answered {
            return Ok(false);
        }

        let local_call_controller =
            local_call_controller.ok_or_else(|| "missing local call controller".to_string())?;
        let webrtc_handle = self
            .webrtc_runtime
            .as_ref()
            .and_then(|runtime| runtime.raw_handle())
            .ok_or_else(|| "webrtc_not_ready".to_string())?;
        local_call_controller.set_webrtc_bridge(Some(webrtc_handle))?;
        if let Err(err) = local_call_controller.set_media_bridge(true) {
            let _ = local_call_controller.set_webrtc_bridge(None);
            return Err(err);
        }
        if let Err(err) = local_call_controller.accept() {
            let _ = local_call_controller.set_media_bridge(false);
            let _ = local_call_controller.set_webrtc_bridge(None);
            return Err(err);
        }

        if let Ok(mut session) = self.state.lock() {
            session.mark_secondary_forward_local_answered();
        }
        eprintln!("aiot-ui: secondary forward local d2d answered");
        Ok(true)
    }

    pub fn handle_confirmed(
        &self,
        state: &State<'_>,
        event: &aiot::AiotEvent,
        local_call_controller: Option<&call::CallController>,
    ) -> Result<(), String> {
        if event.value0 == 0 {
            return Err(if event.text2.is_empty() {
                "confirmed_unable".to_string()
            } else {
                event.text2.clone()
            });
        }
        if !event.text1.eq_ignore_ascii_case("webrtc") {
            return Err(format!("unsupported confirmed mode: {}", event.text1));
        }
        let should_call = self
            .state
            .lock()
            .ok()
            .map(|session| session.should_start_webrtc_on_confirm())
            .unwrap_or(false);
        let is_secondary_forward = self
            .state
            .lock()
            .ok()
            .map(|session| session.is_secondary_forward())
            .unwrap_or(false);
        let accepted_media = self
            .state
            .lock()
            .ok()
            .map(|session| session.accepted_media())
            .unwrap_or_else(|| callx::CallxAcceptedMedia {
                caller_media: "video".to_string(),
                callee_media: "video".to_string(),
            });
        if is_secondary_forward {
            if event.text3.is_empty() {
                return Err("missing webrtc session id".to_string());
            }
            self.answer_secondary_forward_local_call(local_call_controller)?;
        } else if should_call {
            if event.text2.is_empty() || event.text3.is_empty() {
                return Err("missing webrtc confirm payload".to_string());
            }
            eprintln!(
                "aiot-ui: start webrtc call callid={} caller_media={} callee_media={}",
                event.callid, accepted_media.caller_media, accepted_media.callee_media
            );
            self.webrtc_runtime
                .as_ref()
                .ok_or_else(|| "webrtc_not_ready".to_string())?
                .call(
                    &event.text3,
                    &event.text2,
                    accepted_media.caller_media.as_str(),
                    accepted_media.callee_media.as_str(),
                    0,
                )?;
        }

        if let Ok(mut session) = self.state.lock() {
            session.apply_confirmed(&event.callid, &event.text1, &event.text3);
        }

        call::apply_event_to_state(
            state,
            &call::CallEvent {
                kind: call::CallEventKind::StateChanged,
                peer_id: 0,
                peer_device_id: String::new(),
                session_id: 0,
                session_state: call::CallSessionState::Active,
                result_code: 0,
                video_active: false,
                audio_active: false,
            },
        );
        call::apply_event_to_state(
            state,
            &call::CallEvent {
                kind: call::CallEventKind::MediaChanged,
                peer_id: 0,
                peer_device_id: String::new(),
                session_id: 0,
                session_state: call::CallSessionState::Active,
                result_code: 0,
                video_active: media_has_video(accepted_media.callee_media.as_str()),
                audio_active: media_has_audio(accepted_media.caller_media.as_str())
                    || media_has_audio(accepted_media.callee_media.as_str()),
            },
        );
        if is_secondary_forward {
            call::apply_call_peer_id(state, "APP 接听中");
            state.set_call_bridge_prompt_visible(true);
            state.set_call_bridge_prompt_text(SharedString::from("APP 接听中"));
        }
        Ok(())
    }

    pub(crate) fn note_secondary_forward_reply_failed(
        &self,
        event: &aiot::AiotEvent,
    ) -> Option<callx::SecondaryForwardReplyFailure> {
        let callee_key = if !event.callee_user_id.trim().is_empty() {
            format!("app:{}", event.callee_user_id.trim())
        } else if !event.callee_web_username.trim().is_empty() {
            format!("web:{}", event.callee_web_username.trim())
        } else if !event.text2.trim().is_empty() {
            format!("device:{}", event.text2.trim())
        } else {
            String::new()
        };
        self.state
            .lock()
            .ok()
            .and_then(|mut session| session.note_secondary_forward_reply_failed(&callee_key))
    }

    pub fn handle_accepted(
        &self,
        event: &aiot::AiotEvent,
        local_call_controller: Option<&call::CallController>,
    ) -> Result<(), String> {
        let is_secondary_forward = self
            .state
            .lock()
            .ok()
            .map(|session| session.is_secondary_forward())
            .unwrap_or(false);
        if let Ok(mut session) = self.state.lock() {
            session.apply_accepted(&event.text1, &event.text2);
        }
        eprintln!(
            "aiot-ui: callx accepted callid={} caller_media={} callee_media={}",
            event.callid, event.text1, event.text2
        );
        if is_secondary_forward {
            self.answer_secondary_forward_local_call(local_call_controller)?;
        }
        Ok(())
    }

    pub fn handle_hungup(&self, state: &State<'_>) {
        let snapshot = self
            .state
            .lock()
            .ok()
            .map(|session| session.snapshot())
            .unwrap_or_default();
        if !snapshot.webrtc_session_id.is_empty() {
            if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
                let _ = webrtc_runtime.close_session(&snapshot.webrtc_session_id);
            }
        }
        if matches!(
            snapshot.kind,
            CallxSessionKind::ParentOutgoing | CallxSessionKind::SecondaryForward
        ) && !snapshot.callid.is_empty()
        {
            let _ = self.send_callx_hangup(&snapshot.callid, true, "remote_hungup");
            let _ = self.send_callx_terminate(&snapshot.callid);
        }
        self.clear();
        call::apply_event_to_state(
            state,
            &call::CallEvent {
                kind: call::CallEventKind::RemoteHangup,
                peer_id: 0,
                peer_device_id: String::new(),
                session_id: 0,
                session_state: call::CallSessionState::Idle,
                result_code: 0,
                video_active: false,
                audio_active: false,
            },
        );
    }

    pub fn accept(&self) -> Result<(), String> {
        let callid = self.current_call_context()?;
        self.send_callx_accept(&callid, "video", "video")?;
        if let Ok(mut session) = self.state.lock() {
            session.apply_accepted("video", "video");
        }
        Ok(())
    }

    pub fn reject(&self) -> Result<(), String> {
        self.hangup("reject")
    }

    pub fn hangup(&self, cause: &str) -> Result<(), String> {
        let snapshot = {
            let session = self
                .state
                .lock()
                .map_err(|_| "cloud call state lock poisoned".to_string())?;
            session.snapshot()
        };

        if snapshot.callid.is_empty() {
            self.clear();
            return Ok(());
        }
        self.mark_terminating();
        self.send_callx_hangup(&snapshot.callid, true, cause)?;
        if matches!(
            snapshot.kind,
            CallxSessionKind::ParentOutgoing | CallxSessionKind::SecondaryForward
        ) {
            self.send_callx_terminate(&snapshot.callid)?;
        }
        if !snapshot.webrtc_session_id.is_empty() {
            if let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() {
                let _ = webrtc_runtime.close_session(&snapshot.webrtc_session_id);
            }
        }
        self.clear();
        Ok(())
    }

    pub fn start_outgoing_call(&self, target: &str) -> Result<String, String> {
        let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() else {
            return Err("webrtc_not_ready".to_string());
        };
        if !webrtc_runtime.is_ready() {
            return Err("webrtc_not_ready".to_string());
        }
        if self.has_active_call() {
            return Err("busy".to_string());
        }
        if is_manager_call_target(target) {
            self.aiot_controller()?.send_callx_initiate_manager()?;
            if let Ok(mut session) = self.state.lock() {
                session.begin_parent_outgoing("manager".to_string(), 1);
            }
            return Ok("manager".to_string());
        }
        let (peer_sn, peer_channel) = parse_call_target(target)?;
        self.aiot_controller()?
            .send_callx_initiate(&peer_sn, peer_channel)?;
        if let Ok(mut session) = self.state.lock() {
            session.begin_parent_outgoing(peer_sn.clone(), peer_channel);
        }
        Ok(peer_sn)
    }

    pub fn start_secondary_forward_call(&self, room_id: &str) -> Result<String, String> {
        let room_id = room_id.trim();
        if room_id.is_empty() {
            return Err("missing_room_number".to_string());
        }
        let Some(webrtc_runtime) = self.webrtc_runtime.as_ref() else {
            return Err("webrtc_not_ready".to_string());
        };
        if !webrtc_runtime.is_ready() {
            return Err("webrtc_not_ready".to_string());
        }
        if self.has_active_call() {
            return Err("busy".to_string());
        }
        self.aiot_controller()?
            .send_callx_initiate_resident(room_id)?;
        if let Ok(mut session) = self.state.lock() {
            session.begin_secondary_forward(room_id.to_string());
        }
        Ok(room_id.to_string())
    }

    fn current_call_context(&self) -> Result<String, String> {
        let session = self
            .state
            .lock()
            .map_err(|_| "cloud call state lock poisoned".to_string())?;
        session
            .current_context()
            .ok_or_else(|| "no_active_cloud_call".to_string())
    }

    fn aiot_controller(&self) -> Result<aiot::AiotController, String> {
        self.aiot_controller
            .lock()
            .map_err(|_| "cloud call controller lock poisoned".to_string())?
            .clone()
            .ok_or_else(|| "aiot controller unavailable".to_string())
    }

    fn clear(&self) {
        if let Ok(mut session) = self.state.lock() {
            session.clear();
        }
    }

    fn mark_terminating(&self) {
        if let Ok(mut session) = self.state.lock() {
            session.mark_terminating();
        }
    }
}

fn callx_incoming_page(caller_device_kind: &str) -> &'static str {
    match caller_device_kind {
        "unitDoor" => "MwoLv",
        "security" => "EJDbm",
        other => {
            eprintln!("aiot-ui: callx_invited unknown caller_device_kind={other}");
            "Bu0fW"
        }
    }
}

fn local_call_state_busy(state: &State<'_>) -> bool {
    matches!(
        state.get_call_session_state().as_str(),
        "ringing" | "video_only" | "active"
    )
}

pub fn is_manager_call_target(target: &str) -> bool {
    target.trim() == AIOT_MANAGER_CALL_TARGET
}

fn media_has_video(media: &str) -> bool {
    media.eq_ignore_ascii_case("video")
}

fn media_has_audio(media: &str) -> bool {
    matches!(media.trim(), "video" | "audio")
}

fn parse_call_target(target: &str) -> Result<(String, i32), String> {
    let target = target.trim();
    if target.is_empty() {
        return Err("empty_call_target".to_string());
    }
    let mut parts = target.splitn(2, ':');
    let sn = parts.next().unwrap_or_default().trim();
    if sn.is_empty() {
        return Err("empty_call_target".to_string());
    }
    let channel = match parts.next() {
        Some(raw) if !raw.trim().is_empty() => raw
            .trim()
            .parse::<i32>()
            .map_err(|err| format!("invalid call target channel {raw}: {err}"))?,
        _ => 1,
    };
    Ok((sn.to_string(), channel))
}

#[allow(dead_code)]
pub fn handle_secondary_confirm_event(
    event: &indoor::SecondaryConfirmEvent,
    aiot_controller_slot: &Arc<Mutex<Option<aiot::AiotController>>>,
    secondary_confirm_state: &Arc<Mutex<aiot::SecondaryConfirmProxyState>>,
) {
    let snapshot = {
        let mut state = secondary_confirm_state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.online = event.online;
        if !event.device_id.trim().is_empty() {
            state.device_id = event.device_id.clone();
        }
        state.dev_model = if event.dev_model.trim().is_empty() {
            aiot::SECONDARY_CONFIRM_DEFAULT_MODEL.to_string()
        } else {
            event.dev_model.clone()
        };
        state.clone()
    };
    eprintln!(
        "indoor-aiot-link secondary_confirm online={} proxy_sn={} dev_model={} endpoint=0x{:08x} result={}",
        event.online as i32,
        snapshot.device_id.as_str(),
        snapshot.model_or_default(),
        event.endpoint_addr,
        event.result_code
    );

    let controller = aiot_controller_slot
        .lock()
        .ok()
        .and_then(|guard| guard.as_ref().cloned());
    if let Some(controller) = controller {
        if let Err(err) = controller.set_secondary_confirm_state(&snapshot) {
            eprintln!("indoor-aiot-link set_secondary_confirm_online failed: {err}");
        }
    }
}
