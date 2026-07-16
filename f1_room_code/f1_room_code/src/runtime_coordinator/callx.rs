#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub(super) enum CallxPhase {
    #[default]
    Idle,
    LocalRinging,
    CloudInitiating,
    CloudRinging,
    CloudConfirmed,
    Terminating,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub(super) enum CallxSessionKind {
    #[default]
    None,
    ParentIncoming,
    ParentOutgoing,
    SecondaryForward,
}

#[derive(Clone, Debug, Default)]
pub(super) struct CallxInviteContext {
    pub callid: String,
    pub callee_sn: String,
    pub callee_channel: i32,
}

#[derive(Clone, Copy, Debug, Default)]
pub(crate) struct SecondaryForwardReplyFailure {
    pub failed_count: usize,
    pub invited_count: usize,
    pub all_failed: bool,
}

#[derive(Clone, Debug, Default)]
pub(super) struct CallxSnapshot {
    pub kind: CallxSessionKind,
    pub callid: String,
    pub peer_sn: String,
    pub webrtc_session_id: String,
}

#[derive(Clone, Debug)]
pub(super) struct CallxAcceptedMedia {
    pub caller_media: String,
    pub callee_media: String,
}

#[derive(Debug, Default)]
pub(super) struct CallxSession {
    phase: CallxPhase,
    kind: CallxSessionKind,
    callid: String,
    peer_label: String,
    peer_sn: String,
    peer_channel: i32,
    negotiated_mode: String,
    webrtc_session_id: String,
    caller_media: String,
    callee_media: String,
    secondary_forward_local_answered: bool,
    secondary_forward_invited_count: usize,
    secondary_forward_failed_callees: Vec<String>,
}

impl CallxSession {
    pub fn is_active(&self) -> bool {
        self.phase != CallxPhase::Idle
            || self.kind != CallxSessionKind::None
            || !self.callid.is_empty()
    }

    pub fn begin_parent_incoming(
        &mut self,
        callid: String,
        peer_label: String,
        peer_sn: String,
        negotiated_mode: String,
    ) {
        self.phase = CallxPhase::LocalRinging;
        self.kind = CallxSessionKind::ParentIncoming;
        self.callid = callid;
        self.peer_label = peer_label;
        self.peer_sn = peer_sn;
        self.peer_channel = 0;
        self.negotiated_mode = negotiated_mode;
        self.webrtc_session_id.clear();
        self.caller_media = "video".to_string();
        self.callee_media = "video".to_string();
        self.secondary_forward_local_answered = false;
        self.secondary_forward_invited_count = 0;
        self.secondary_forward_failed_callees.clear();
    }

    pub fn begin_parent_outgoing(&mut self, peer_sn: String, peer_channel: i32) {
        self.phase = CallxPhase::CloudInitiating;
        self.kind = CallxSessionKind::ParentOutgoing;
        self.callid.clear();
        self.peer_label = peer_sn.clone();
        self.peer_sn = peer_sn;
        self.peer_channel = peer_channel;
        self.negotiated_mode = "webrtc".to_string();
        self.webrtc_session_id.clear();
        self.caller_media = "video".to_string();
        self.callee_media = "video".to_string();
        self.secondary_forward_local_answered = false;
        self.secondary_forward_invited_count = 0;
        self.secondary_forward_failed_callees.clear();
    }

    pub fn begin_secondary_forward(&mut self, room_id: String) {
        self.phase = CallxPhase::CloudInitiating;
        self.kind = CallxSessionKind::SecondaryForward;
        self.callid.clear();
        self.peer_label = room_id.clone();
        self.peer_sn = room_id;
        self.peer_channel = 1;
        self.negotiated_mode = "webrtc".to_string();
        self.webrtc_session_id.clear();
        self.caller_media = "video".to_string();
        self.callee_media = "video".to_string();
        self.secondary_forward_local_answered = false;
        self.secondary_forward_invited_count = 0;
        self.secondary_forward_failed_callees.clear();
    }

    pub fn apply_initiate_ack(
        &mut self,
        callid: &str,
        peer_sn: &str,
        negotiated_mode: &str,
        peer_label: &str,
        callee_channel: i32,
        callee_count: i32,
    ) -> Option<CallxInviteContext> {
        if !matches!(
            self.kind,
            CallxSessionKind::ParentOutgoing | CallxSessionKind::SecondaryForward
        ) {
            return None;
        }
        if !callid.is_empty() {
            self.callid = callid.to_string();
        }
        if !peer_sn.is_empty() {
            self.peer_sn = peer_sn.to_string();
        }
        if !negotiated_mode.is_empty() {
            self.negotiated_mode = negotiated_mode.to_string();
        }
        self.peer_label = if peer_label.is_empty() {
            self.peer_sn.clone()
        } else {
            peer_label.to_string()
        };
        self.phase = CallxPhase::CloudRinging;
        if self.kind == CallxSessionKind::SecondaryForward {
            self.secondary_forward_invited_count = callee_count.max(0) as usize;
            self.secondary_forward_failed_callees.clear();
        }

        Some(CallxInviteContext {
            callid: self.callid.clone(),
            callee_sn: self.peer_sn.clone(),
            callee_channel: if callee_channel != 0 {
                callee_channel
            } else {
                self.peer_channel
            },
        })
    }

    pub fn should_start_webrtc_on_confirm(&self) -> bool {
        matches!(self.kind, CallxSessionKind::ParentOutgoing)
    }

    pub fn is_secondary_forward(&self) -> bool {
        self.kind == CallxSessionKind::SecondaryForward
    }

    pub fn is_secondary_forward_confirmed(&self) -> bool {
        self.kind == CallxSessionKind::SecondaryForward && self.phase == CallxPhase::CloudConfirmed
    }

    pub fn secondary_forward_local_answered(&self) -> bool {
        self.secondary_forward_local_answered
    }

    pub fn mark_secondary_forward_local_answered(&mut self) {
        if self.kind == CallxSessionKind::SecondaryForward {
            self.secondary_forward_local_answered = true;
        }
    }

    pub fn note_secondary_forward_reply_failed(
        &mut self,
        callee_key: &str,
    ) -> Option<SecondaryForwardReplyFailure> {
        if self.kind != CallxSessionKind::SecondaryForward {
            return None;
        }
        let callee_key = callee_key.trim();
        if callee_key.is_empty() {
            return Some(SecondaryForwardReplyFailure {
                failed_count: self.secondary_forward_failed_callees.len(),
                invited_count: self.secondary_forward_invited_count,
                all_failed: false,
            });
        }
        if !self
            .secondary_forward_failed_callees
            .iter()
            .any(|value| value == callee_key)
        {
            self.secondary_forward_failed_callees
                .push(callee_key.to_string());
        }
        let failed_count = self.secondary_forward_failed_callees.len();
        let invited_count = self.secondary_forward_invited_count;
        Some(SecondaryForwardReplyFailure {
            failed_count,
            invited_count,
            all_failed: invited_count > 0 && failed_count >= invited_count,
        })
    }

    pub fn apply_accepted(&mut self, caller_media: &str, callee_media: &str) {
        let caller_media = normalize_media(caller_media);
        let callee_media = normalize_media(callee_media);
        self.caller_media = caller_media.to_string();
        self.callee_media = callee_media.to_string();
    }

    pub fn accepted_media(&self) -> CallxAcceptedMedia {
        CallxAcceptedMedia {
            caller_media: normalize_media(&self.caller_media).to_string(),
            callee_media: normalize_media(&self.callee_media).to_string(),
        }
    }

    pub fn apply_confirmed(
        &mut self,
        callid: &str,
        negotiated_mode: &str,
        webrtc_session_id: &str,
    ) {
        self.phase = CallxPhase::CloudConfirmed;
        if self.callid.is_empty() {
            self.callid = callid.to_string();
        }
        if self.negotiated_mode.is_empty() {
            self.negotiated_mode = negotiated_mode.to_string();
        }
        self.webrtc_session_id = webrtc_session_id.to_string();
    }

    pub fn mark_terminating(&mut self) {
        if self.is_active() {
            self.phase = CallxPhase::Terminating;
        }
    }

    pub fn current_context(&self) -> Option<String> {
        if self.callid.is_empty() {
            None
        } else {
            Some(self.callid.clone())
        }
    }

    pub fn snapshot(&self) -> CallxSnapshot {
        CallxSnapshot {
            kind: self.kind,
            callid: self.callid.clone(),
            peer_sn: self.peer_sn.clone(),
            webrtc_session_id: self.webrtc_session_id.clone(),
        }
    }

    pub fn peer_label(&self) -> String {
        self.peer_label.clone()
    }

    pub fn clear(&mut self) {
        *self = Self::default();
    }
}

fn normalize_media(media: &str) -> &str {
    match media.trim() {
        "audio" => "audio",
        "none" => "none",
        _ => "video",
    }
}
