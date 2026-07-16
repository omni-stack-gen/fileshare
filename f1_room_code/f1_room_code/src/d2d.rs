pub use crate::indoor::D2dController;

#[allow(dead_code)]
#[derive(Clone, Debug)]
pub struct D2dConfig {
    pub enabled: bool,
    pub interface: String,
    pub plc_port: u32,
    pub f1_plc_addr: u32,
    pub indoor_plc_addr: u32,
    pub eth_port: u32,
    pub indoor_id: String,
    pub default_door_id: String,
}

impl Default for D2dConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            interface: "eth0".to_string(),
            plc_port: 0x03,
            f1_plc_addr: 0x20000,
            indoor_plc_addr: 0x10000,
            eth_port: 0x9982,
            indoor_id: "indoor-0101".to_string(),
            default_door_id: "door-0101".to_string(),
        }
    }
}

#[cfg(target_arch = "riscv64")]
impl D2dConfig {
    pub fn from_settings(settings: &crate::db::Settings) -> Self {
        Self {
            enabled: settings.get_bool("d2d.enabled", true),
            interface: settings.get_string("d2d.interface", "eth0").to_string(),
            plc_port: settings.get_int("d2d.plc_port", 0x03) as u32,
            f1_plc_addr: settings.get_int("d2d.f1_plc_addr", 0x20000) as u32,
            indoor_plc_addr: settings.get_int("d2d.indoor_plc_addr", 0x10000) as u32,
            eth_port: settings.get_int("d2d.eth_port", 0x9982) as u32,
            indoor_id: settings
                .get_string("d2d.indoor_id", "indoor-0101")
                .to_string(),
            default_door_id: settings
                .get_string("d2d.default_door_id", "door-0101")
                .to_string(),
        }
    }
}

#[allow(dead_code)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum D2dEventKind {
    UnlockDone,
    UnlockFail,
    UnlockTimeout,
    SetPasswordDone,
    SetPasswordFail,
    SetPasswordTimeout,
    Error,
}

impl D2dEventKind {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::UnlockDone => "unlock_done",
            Self::UnlockFail => "unlock_fail",
            Self::UnlockTimeout => "unlock_timeout",
            Self::SetPasswordDone => "set_password_done",
            Self::SetPasswordFail => "set_password_fail",
            Self::SetPasswordTimeout => "set_password_timeout",
            Self::Error => "error",
        }
    }
}

#[allow(dead_code)]
#[derive(Clone, Debug)]
pub struct D2dEvent {
    pub kind: D2dEventKind,
    pub result_code: i32,
    pub door_id: String,
    pub password: String,
}
