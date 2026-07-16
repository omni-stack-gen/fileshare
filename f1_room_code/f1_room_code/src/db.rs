use std::collections::{BTreeMap, HashMap};
use std::fs::{self, File};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use base64::{engine::general_purpose::STANDARD, Engine as _};

#[derive(Clone)]
pub struct Database {
    root: PathBuf,
}

pub struct Settings {
    values: HashMap<String, String>,
}

#[derive(Clone, Debug, Default)]
pub struct WifiProfile {
    pub ssid: String,
    pub password: String,
    pub priority: i32,
    pub auto_connect: bool,
    pub last_connected_at: Option<i64>,
}

#[derive(Clone, Debug)]
struct SettingEntry {
    value: String,
    value_type: String,
}

impl Database {
    pub fn open_default() -> Result<Self, String> {
        let root = storage_root();
        fs::create_dir_all(&root).map_err(|err| {
            format!(
                "failed to create storage directory {}: {err}",
                root.display()
            )
        })?;
        let database = Self { root };
        eprintln!(
            "kv-storage: root={} settings={} wifi_profiles={} ipc_cameras={}",
            database.root.display(),
            database.settings_path().display(),
            database.wifi_profiles_path().display(),
            database.ipc_cameras_path().display()
        );
        database.ensure_default_files()?;
        Ok(database)
    }

    pub fn settings(&self) -> Result<Settings, String> {
        let values = self
            .read_settings_entries()?
            .into_iter()
            .map(|(key, entry)| (key, entry.value))
            .collect();
        Ok(Settings { values })
    }

    pub fn set_float(&self, key: &str, value: f32) -> Result<(), String> {
        self.set_setting(key, &format!("{value:.4}"), "float")
    }

    pub fn set_int(&self, key: &str, value: i32) -> Result<(), String> {
        self.set_setting(key, &value.to_string(), "int")
    }

    pub fn set_bool(&self, key: &str, value: bool) -> Result<(), String> {
        self.set_setting(key, if value { "1" } else { "0" }, "bool")
    }

    pub fn set_string(&self, key: &str, value: &str) -> Result<(), String> {
        self.set_setting(key, value, "string")
    }

    pub fn password_matches(&self, password: &str) -> Result<bool, String> {
        let settings = self.settings()?;
        let expected = settings
            .get_string("security.door_unlock_password_hash", "")
            .to_string();
        Ok(expected == sha256_hex(password))
    }

    pub fn password_plain(&self) -> Result<String, String> {
        let settings = self.settings()?;
        Ok(settings
            .get_string("security.door_unlock_password_plain", "123456")
            .to_string())
    }

    pub fn set_password(&self, password: &str) -> Result<(), String> {
        self.set_string("security.door_unlock_password_hash", &sha256_hex(password))
    }

    pub fn set_password_pair(&self, password: &str) -> Result<(), String> {
        self.set_string("security.door_unlock_password_plain", password)?;
        self.set_password(password)
    }

    pub fn wifi_profiles(&self) -> Result<Vec<WifiProfile>, String> {
        read_wifi_profiles(&self.wifi_profiles_path())
    }

    pub fn upsert_wifi_profile(
        &self,
        ssid: &str,
        password: &str,
        priority: i32,
        auto_connect: bool,
    ) -> Result<(), String> {
        let mut profiles = self.wifi_profiles()?;
        let now = unix_now();
        if let Some(profile) = profiles.iter_mut().find(|profile| profile.ssid == ssid) {
            profile.password = password.to_string();
            profile.priority = priority;
            profile.auto_connect = auto_connect;
            profile.last_connected_at = Some(now);
        } else {
            profiles.push(WifiProfile {
                ssid: ssid.to_string(),
                password: password.to_string(),
                priority,
                auto_connect,
                last_connected_at: Some(now),
            });
        }
        profiles.sort_by(|a, b| {
            b.priority
                .cmp(&a.priority)
                .then_with(|| {
                    b.last_connected_at
                        .unwrap_or(0)
                        .cmp(&a.last_connected_at.unwrap_or(0))
                })
                .then_with(|| a.ssid.cmp(&b.ssid))
        });
        write_wifi_profiles(&self.wifi_profiles_path(), &profiles)
    }

    pub fn ipc_cameras(&self) -> Result<Vec<crate::ipc::IpcCameraConfig>, String> {
        read_ipc_cameras(&self.ipc_cameras_path())
    }

    pub fn replace_ipc_cameras(
        &self,
        cameras: &[crate::ipc::IpcCameraConfig],
    ) -> Result<(), String> {
        write_ipc_cameras(&self.ipc_cameras_path(), cameras)
    }

    fn set_setting(&self, key: &str, value: &str, value_type: &str) -> Result<(), String> {
        validate_setting_type(value_type)?;
        let mut entries = self.read_settings_entries()?;
        entries.insert(
            key.to_string(),
            SettingEntry {
                value: value.to_string(),
                value_type: value_type.to_string(),
            },
        );
        write_settings(&self.settings_path(), &entries)
    }

    fn ensure_default_files(&self) -> Result<(), String> {
        if !self.settings_path().exists() {
            write_settings(&self.settings_path(), &default_settings())?;
        }
        if !self.wifi_profiles_path().exists() {
            write_wifi_profiles(&self.wifi_profiles_path(), &default_wifi_profiles())?;
        }
        if !self.ipc_cameras_path().exists() {
            write_ipc_cameras(&self.ipc_cameras_path(), &[])?;
        }
        Ok(())
    }

    fn read_settings_entries(&self) -> Result<BTreeMap<String, SettingEntry>, String> {
        let mut entries = default_settings();
        let path = self.settings_path();
        if !path.exists() {
            return Ok(entries);
        }

        for (line_no, line) in read_lines(&path)?.lines().enumerate() {
            let line = line.trim_end_matches('\r');
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let parts = split_tab_fields(line, 3, &path, line_no + 1)?;
            validate_setting_type(parts[1])?;
            entries.insert(
                parts[0].to_string(),
                SettingEntry {
                    value: decode_field(parts[2], &path, line_no + 1)?,
                    value_type: parts[1].to_string(),
                },
            );
        }

        Ok(entries)
    }

    fn settings_path(&self) -> PathBuf {
        self.root.join("settings.kv")
    }

    fn wifi_profiles_path(&self) -> PathBuf {
        self.root.join("wifi_profiles.kv")
    }

    fn ipc_cameras_path(&self) -> PathBuf {
        self.root.join("ipc_cameras.kv")
    }
}

impl Settings {
    pub fn get_string<'a>(&'a self, key: &str, default: &'a str) -> &'a str {
        self.values.get(key).map(String::as_str).unwrap_or(default)
    }

    pub fn get_float(&self, key: &str, default: f32) -> f32 {
        self.values
            .get(key)
            .and_then(|value| value.parse::<f32>().ok())
            .unwrap_or(default)
    }

    pub fn get_int(&self, key: &str, default: i32) -> i32 {
        self.values
            .get(key)
            .and_then(|value| value.parse::<i32>().ok())
            .unwrap_or(default)
    }

    pub fn get_bool(&self, key: &str, default: bool) -> bool {
        self.values
            .get(key)
            .map(|value| value == "1" || value.eq_ignore_ascii_case("true"))
            .unwrap_or(default)
    }

    pub fn get_minute_label(&self, key: &str, default: i32) -> String {
        format!("{}분", self.get_int(key, default))
    }

    pub fn get_second_label(&self, key: &str, default: i32) -> String {
        format!("{}초", self.get_int(key, default))
    }
}

fn storage_root() -> PathBuf {
    if let Some(path) = std::env::var_os("KR_ROOM_DB_PATH").filter(|path| !path.is_empty()) {
        let path = PathBuf::from(path);
        return if path.extension().is_some() {
            path.parent()
                .filter(|parent| !parent.as_os_str().is_empty())
                .map(Path::to_path_buf)
                .unwrap_or_else(|| PathBuf::from("."))
        } else {
            path
        };
    }

    if Path::new("/data").exists() {
        PathBuf::from("/data/kr_room")
    } else {
        PathBuf::from(".")
    }
}

fn read_lines(path: &Path) -> Result<String, String> {
    fs::read_to_string(path).map_err(|err| format!("failed to read {}: {err}", path.display()))
}

fn split_tab_fields<'a>(
    line: &'a str,
    expected: usize,
    path: &Path,
    line_no: usize,
) -> Result<Vec<&'a str>, String> {
    let parts = line.split('\t').collect::<Vec<_>>();
    if parts.len() != expected {
        return Err(format!(
            "{}:{} expected {expected} tab-separated fields, got {}",
            path.display(),
            line_no,
            parts.len()
        ));
    }
    Ok(parts)
}

fn validate_setting_type(value_type: &str) -> Result<(), String> {
    match value_type {
        "bool" | "int" | "float" | "string" => Ok(()),
        _ => Err(format!("unsupported setting type: {value_type}")),
    }
}

fn encode_field(value: &str) -> String {
    STANDARD.encode(value.as_bytes())
}

fn decode_field(value: &str, path: &Path, line_no: usize) -> Result<String, String> {
    let bytes = STANDARD
        .decode(value.as_bytes())
        .map_err(|err| format!("{}:{} invalid base64 field: {err}", path.display(), line_no))?;
    String::from_utf8(bytes)
        .map_err(|err| format!("{}:{} invalid utf8 field: {err}", path.display(), line_no))
}

fn write_atomic(path: &Path, content: &str) -> Result<(), String> {
    if let Some(parent) = path.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent).map_err(|err| {
                format!(
                    "failed to create storage directory {}: {err}",
                    parent.display()
                )
            })?;
        }
    }

    let tmp_path = path.with_extension("tmp");
    {
        let mut file = File::create(&tmp_path)
            .map_err(|err| format!("failed to create {}: {err}", tmp_path.display()))?;
        file.write_all(content.as_bytes())
            .map_err(|err| format!("failed to write {}: {err}", tmp_path.display()))?;
        file.sync_all()
            .map_err(|err| format!("failed to sync {}: {err}", tmp_path.display()))?;
    }
    fs::rename(&tmp_path, path).map_err(|err| {
        format!(
            "failed to replace {} with {}: {err}",
            path.display(),
            tmp_path.display()
        )
    })
}

fn write_settings(path: &Path, entries: &BTreeMap<String, SettingEntry>) -> Result<(), String> {
    let mut content = String::new();
    for (key, entry) in entries {
        validate_setting_type(&entry.value_type)?;
        content.push_str(key);
        content.push('\t');
        content.push_str(&entry.value_type);
        content.push('\t');
        content.push_str(&encode_field(&entry.value));
        content.push('\n');
    }
    write_atomic(path, &content)
}

fn read_wifi_profiles(path: &Path) -> Result<Vec<WifiProfile>, String> {
    if !path.exists() {
        return Ok(default_wifi_profiles());
    }

    let mut profiles = Vec::new();
    for (line_no, line) in read_lines(path)?.lines().enumerate() {
        let line = line.trim_end_matches('\r');
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let parts = split_tab_fields(line, 5, path, line_no + 1)?;
        let last_connected_at = parts[4].parse::<i64>().ok().filter(|value| *value > 0);
        profiles.push(WifiProfile {
            ssid: decode_field(parts[0], path, line_no + 1)?,
            password: decode_field(parts[1], path, line_no + 1)?,
            priority: parts[2].parse::<i32>().unwrap_or(0),
            auto_connect: parts[3] == "1" || parts[3].eq_ignore_ascii_case("true"),
            last_connected_at,
        });
    }

    Ok(profiles)
}

fn write_wifi_profiles(path: &Path, profiles: &[WifiProfile]) -> Result<(), String> {
    let mut content = String::new();
    for profile in profiles {
        content.push_str(&encode_field(&profile.ssid));
        content.push('\t');
        content.push_str(&encode_field(&profile.password));
        content.push('\t');
        content.push_str(&profile.priority.to_string());
        content.push('\t');
        content.push_str(if profile.auto_connect { "1" } else { "0" });
        content.push('\t');
        content.push_str(&profile.last_connected_at.unwrap_or(0).to_string());
        content.push('\n');
    }
    write_atomic(path, &content)
}

fn read_ipc_cameras(path: &Path) -> Result<Vec<crate::ipc::IpcCameraConfig>, String> {
    if !path.exists() {
        return Ok(Vec::new());
    }

    let mut cameras = Vec::new();
    for (line_no, line) in read_lines(path)?.lines().enumerate() {
        let line = line.trim_end_matches('\r');
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let parts = split_tab_fields(line, 9, path, line_no + 1)?;
        let name = decode_field(parts[3], path, line_no + 1)?;
        cameras.push(crate::ipc::IpcCameraConfig {
            name: if name.trim().is_empty() {
                format!("CCTV{}", line_no + 1)
            } else {
                name
            },
            ip: decode_field(parts[4], path, line_no + 1)?,
            user: decode_field(parts[5], path, line_no + 1)?,
            password: decode_field(parts[6], path, line_no + 1)?,
            rtsp_url: decode_field(parts[7], path, line_no + 1)?,
            sub_rtsp_url: decode_field(parts[8], path, line_no + 1)?,
            enabled: parts[1] == "1" || parts[1].eq_ignore_ascii_case("true"),
            prefer_substream: parts[2] == "1" || parts[2].eq_ignore_ascii_case("true"),
        });
    }
    Ok(cameras)
}

fn write_ipc_cameras(path: &Path, cameras: &[crate::ipc::IpcCameraConfig]) -> Result<(), String> {
    let mut content = String::new();
    for (index, camera) in cameras.iter().enumerate() {
        content.push_str(&(index + 1).to_string());
        content.push('\t');
        content.push_str(if camera.enabled { "1" } else { "0" });
        content.push('\t');
        content.push_str(if camera.prefer_substream { "1" } else { "0" });
        content.push('\t');
        content.push_str(&encode_field(&camera.name));
        content.push('\t');
        content.push_str(&encode_field(&camera.ip));
        content.push('\t');
        content.push_str(&encode_field(&camera.user));
        content.push('\t');
        content.push_str(&encode_field(&camera.password));
        content.push('\t');
        content.push_str(&encode_field(&camera.rtsp_url));
        content.push('\t');
        content.push_str(&encode_field(&camera.sub_rtsp_url));
        content.push('\n');
    }
    write_atomic(path, &content)
}

fn unix_now() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|duration| duration.as_secs() as i64)
        .unwrap_or(0)
}

fn default_wifi_profiles() -> Vec<WifiProfile> {
    vec![WifiProfile {
        ssid: "HomeNetwork_5G".to_string(),
        password: String::new(),
        priority: 100,
        auto_connect: true,
        last_connected_at: Some(unix_now()),
    }]
}

fn default_settings() -> BTreeMap<String, SettingEntry> {
    DEFAULT_SETTINGS
        .iter()
        .map(|(key, value, value_type)| {
            (
                (*key).to_string(),
                SettingEntry {
                    value: (*value).to_string(),
                    value_type: (*value_type).to_string(),
                },
            )
        })
        .collect()
}

fn sha256_hex(input: &str) -> String {
    const K: [u32; 64] = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2,
    ];

    let mut h = [
        0x6a09e667u32,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19,
    ];

    let mut message = input.as_bytes().to_vec();
    let bit_len = (message.len() as u64) * 8;
    message.push(0x80);
    while message.len() % 64 != 56 {
        message.push(0);
    }
    message.extend_from_slice(&bit_len.to_be_bytes());

    for chunk in message.chunks_exact(64) {
        let mut w = [0u32; 64];
        for (index, word) in w.iter_mut().take(16).enumerate() {
            let offset = index * 4;
            *word = u32::from_be_bytes([
                chunk[offset],
                chunk[offset + 1],
                chunk[offset + 2],
                chunk[offset + 3],
            ]);
        }
        for index in 16..64 {
            let s0 = w[index - 15].rotate_right(7)
                ^ w[index - 15].rotate_right(18)
                ^ (w[index - 15] >> 3);
            let s1 = w[index - 2].rotate_right(17)
                ^ w[index - 2].rotate_right(19)
                ^ (w[index - 2] >> 10);
            w[index] = w[index - 16]
                .wrapping_add(s0)
                .wrapping_add(w[index - 7])
                .wrapping_add(s1);
        }

        let mut a = h[0];
        let mut b = h[1];
        let mut c = h[2];
        let mut d = h[3];
        let mut e = h[4];
        let mut f = h[5];
        let mut g = h[6];
        let mut hh = h[7];

        for index in 0..64 {
            let s1 = e.rotate_right(6) ^ e.rotate_right(11) ^ e.rotate_right(25);
            let ch = (e & f) ^ ((!e) & g);
            let temp1 = hh
                .wrapping_add(s1)
                .wrapping_add(ch)
                .wrapping_add(K[index])
                .wrapping_add(w[index]);
            let s0 = a.rotate_right(2) ^ a.rotate_right(13) ^ a.rotate_right(22);
            let maj = (a & b) ^ (a & c) ^ (b & c);
            let temp2 = s0.wrapping_add(maj);

            hh = g;
            g = f;
            f = e;
            e = d.wrapping_add(temp1);
            d = c;
            c = b;
            b = a;
            a = temp1.wrapping_add(temp2);
        }

        h[0] = h[0].wrapping_add(a);
        h[1] = h[1].wrapping_add(b);
        h[2] = h[2].wrapping_add(c);
        h[3] = h[3].wrapping_add(d);
        h[4] = h[4].wrapping_add(e);
        h[5] = h[5].wrapping_add(f);
        h[6] = h[6].wrapping_add(g);
        h[7] = h[7].wrapping_add(hh);
    }

    let mut out = String::with_capacity(64);
    for word in h {
        use std::fmt::Write;
        write!(&mut out, "{word:08x}").expect("writing to String cannot fail");
    }
    out
}

const DEFAULT_SETTINGS: &[(&str, &str, &str)] = &[
    ("general.language", "ko-KR", "string"),
    ("general.time_period", "PM", "string"),
    ("general.hour", "12", "string"),
    ("general.minute", "00", "string"),
    ("general.year", "2026", "string"),
    ("general.month", "04", "string"),
    ("general.day", "23", "string"),
    ("site.building_number", "01", "string"),
    ("site.room_number", "101", "string"),
    ("display.brightness", "0.66", "float"),
    ("display.hue", "0.0", "float"),
    ("display.contrast", "0.67", "float"),
    ("display.saturation", "0.45", "float"),
    ("sound.door_call_melody", "1", "int"),
    ("sound.guard_call_melody", "3", "int"),
    ("sound.ring_volume", "0.80", "float"),
    ("sound.talk_volume", "0.70", "float"),
    ("sound.mic_sensitivity", "0.65", "float"),
    ("network.wifi_enabled", "1", "bool"),
    ("network.connected_ssid", "HomeNetwork_5G", "string"),
    ("aiot.enabled", "1", "bool"),
    ("aiot.interface", "wlan0", "string"),
    ("aiot.model", "EVP-70DP", "string"),
    // ("aiot.server_addr", "192.168.253.40:30007", "string"),
    ("aiot.server_addr", "43.108.59.118:30007", "string"),
    ("aiot.mqtt_server_addr", "", "string"),
    ("aiot.application_version", "1.0.1", "string"),
    ("aiot.system_version", "1.0.0", "string"),
    ("aiot.hardware_version", "1.0.0", "string"),
    ("aiot.mac", "", "string"),
    ("aiot.keepalive", "60", "int"),
    ("aiot.is_mqtts", "1", "bool"),
    ("webrtc.enabled", "1", "bool"),
    ("webrtc.interface", "wlan0", "string"),
    ("webrtc.serno", "", "string"),
    ("webrtc.initstring", "", "string"),
    ("webrtc.server_addr", "", "string"),
    ("webrtc.customer_serno", "", "string"),
    ("webrtc.playback_device", "default", "string"),
    ("webrtc.record_device", "default", "string"),
    ("webrtc.signal_reconnect_timeout_ms", "500", "int"),
    ("webrtc.max_sdk_use_mem", "5242880", "int"),
    ("webrtc.max_session_use_mem", "1048576", "int"),
    ("webrtc.max_session_buffer_size", "128", "int"),
    ("webrtc.enable_audio_input", "1", "bool"),
    ("webrtc.enable_audio_output", "1", "bool"),
    ("call.enabled", "1", "bool"),
    ("call.role", "indoor", "string"),
    ("call.self_id", "65536", "int"),
    ("call.upstream_id", "131072", "int"),
    ("call.ctrl_port", "2", "int"),
    ("call.interface", "eth0", "string"),
    ("mode.clock_screen_enabled", "1", "bool"),
    ("mode.auto_capture_enabled", "0", "bool"),
    ("mode.absent_active", "0", "bool"),
    ("mode.absent_timeout_min", "5", "int"),
    ("mode.communication_timeout_min", "3", "int"),
    ("mode.call_ring_timeout_sec", "30", "int"),
    ("mode.door_open_duration_sec", "3", "int"),
    ("call.door.brightness", "0.77", "float"),
    ("call.door.volume", "0.70", "float"),
    ("call.lobby.brightness", "0.77", "float"),
    ("call.lobby.volume", "0.70", "float"),
    ("call.guard.brightness", "0.0", "float"),
    ("call.guard.volume", "0.70", "float"),
    ("call.ev.brightness", "0.0", "float"),
    ("call.ev.volume", "0.0", "float"),
    ("cctv.brightness", "0.77", "float"),
    ("cctv.volume", "0.70", "float"),
    ("ipc.enabled", "0", "bool"),
    ("ipc.interface", "wlan0", "string"),
    ("ipc.preview_x", "24", "int"),
    ("ipc.preview_y", "46", "int"),
    ("ipc.preview_width", "800", "int"),
    ("ipc.preview_height", "480", "int"),
    ("ipc.capture_dir", "", "string"),
    ("ipc.onvif_port", "80", "string"),
    ("idle.clock_timeout_sec", "60", "int"),
    ("d2d.enabled", "1", "bool"),
    ("d2d.interface", "eth0", "string"),
    ("d2d.plc_port", "3", "int"),
    ("d2d.f1_plc_addr", "131072", "int"),
    ("d2d.indoor_plc_addr", "65536", "int"),
    ("d2d.eth_port", "9982", "int"),
    ("d2d.indoor_id", "indoor-0101", "string"),
    ("d2d.default_door_id", "door-0101", "string"),
    ("security.password_hash_algorithm", "sha256", "string"),
    ("security.door_unlock_password_plain", "123456", "string"),
    (
        "security.door_unlock_password_hash",
        "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92",
        "string",
    ),
];
