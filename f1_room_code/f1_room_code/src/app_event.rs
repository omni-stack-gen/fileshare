use crate::{aiot, indoor, webrtc};

#[derive(Clone, Debug)]
pub enum AppEvent {
    Aiot(aiot::AiotEvent),
    Indoor(indoor::IndoorEvent),
    Webrtc(webrtc::WebrtcEvent),
}
