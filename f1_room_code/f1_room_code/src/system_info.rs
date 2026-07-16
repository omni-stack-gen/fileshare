use aes::Aes256;
use base64::engine::general_purpose::STANDARD as BASE64_STANDARD;
use base64::Engine;
use cbc::cipher::{block_padding::Pkcs7, BlockEncryptMut, KeyIvInit};
use libc::{self, freeifaddrs, getifaddrs, ifaddrs, sockaddr_in, AF_INET};
use qrcodegen::{QrCode, QrCodeEcc};
use slint::{ComponentHandle, Image, Rgba8Pixel, SharedPixelBuffer, SharedString};
use std::ffi::CStr;
use std::fs;
use std::ptr;

type Aes256CbcEnc = cbc::Encryptor<Aes256>;

const QR_KEY: &[u8; 32] = b"m~Sal1GH0YU(xs%5ah&wYl(qS$VR^uuH";
const QR_IV: &[u8; 16] = b"eOMejFQ%eNVZr&+j";
const QR_MODEL: &str = "HDC_LABS_VIDEO";
const QR_KIND: &str = "b";
const QR_KEY_ID: i32 = 1;
const QR_BORDER: i32 = 4;
const QR_TARGET_SIZE: u32 = 188;

pub fn configure_system_info_bindings(app: &crate::AppWindow) {
    crate::log_memory_usage("system-info:bindings-begin");
    crate::log_memory_usage("system-info:initial-refresh-before");
    refresh_system_info(app);
    crate::log_memory_usage("system-info:initial-refresh-after");

    let app_weak = app.as_weak();
    app.global::<crate::State>()
        .on_refresh_system_info(move || {
            if let Some(app) = app_weak.upgrade() {
                crate::log_memory_usage("system-info:callback-refresh-before");
                refresh_system_info(&app);
                crate::log_memory_usage("system-info:callback-refresh-after");
            }
        });
    crate::log_memory_usage("system-info:bindings-done");
}

pub fn refresh_system_info(app: &crate::AppWindow) {
    crate::log_memory_usage("system-info:refresh-begin");
    let state = app.global::<crate::State>();
    crate::log_memory_usage("system-info:network-read-before");
    let test_cloud_iface = std::env::var("KR_ROOM_TEST_CLOUD_IFACE")
        .ok()
        .map(|value| value.trim().to_string())
        .filter(|value| !value.is_empty());
    let preferred_interfaces = test_cloud_iface
        .as_deref()
        .map(|interface| vec![interface, "wlan0", "eth0"])
        .unwrap_or_else(|| vec!["wlan0", "eth0"]);
    let mac = select_mac_address(&preferred_interfaces);
    let ip = select_ipv4_address(&preferred_interfaces);
    crate::log_memory_usage("system-info:network-read-after");

    state.set_system_info_mac_address(SharedString::from(
        mac.clone().unwrap_or_else(|| "--".to_string()),
    ));
    state.set_system_info_ip_address(SharedString::from(ip.unwrap_or_else(|| "--".to_string())));
    crate::log_memory_usage("system-info:network-state-after");

    crate::log_memory_usage("system-info:qr-build-before");
    match build_qr_image(
        mac.as_deref(),
        state.get_site_building_number().as_str(),
        state.get_site_room_number().as_str(),
    ) {
        Ok(image) => {
            crate::log_memory_usage("system-info:qr-build-ok-before-state");
            state.set_system_info_qr_image(image);
            state.set_system_info_qr_available(true);
            crate::log_memory_usage("system-info:qr-build-ok-after-state");
        }
        Err(_) => {
            crate::log_memory_usage("system-info:qr-build-err-before-state");
            state.set_system_info_qr_image(Image::default());
            state.set_system_info_qr_available(false);
            crate::log_memory_usage("system-info:qr-build-err-after-state");
        }
    }
    crate::log_memory_usage("system-info:refresh-done");
}

fn build_qr_image(mac: Option<&str>, building: &str, room: &str) -> Result<Image, String> {
    crate::log_memory_usage("system-info:qr-input-parse-before");
    let mac = mac.ok_or_else(|| "missing mac address".to_string())?;
    let building = building
        .trim()
        .parse::<i32>()
        .map_err(|_| "invalid building number".to_string())?;
    let room = room
        .trim()
        .parse::<i32>()
        .map_err(|_| "invalid room number".to_string())?;
    crate::log_memory_usage("system-info:qr-input-parse-after");

    let plaintext =
        format!("t={QR_KIND}&sn={mac}&m={QR_MODEL}&k={QR_KEY_ID}&b={building}&n={room}");
    crate::log_memory_usage("system-info:qr-encrypt-before");
    let ciphertext = Aes256CbcEnc::new(QR_KEY.into(), QR_IV.into())
        .encrypt_padded_vec_mut::<Pkcs7>(plaintext.as_bytes());
    crate::log_memory_usage("system-info:qr-encrypt-after");
    crate::log_memory_usage("system-info:qr-base64-before");
    let encoded = BASE64_STANDARD.encode(ciphertext);
    crate::log_memory_usage("system-info:qr-base64-after");
    crate::log_memory_usage("system-info:qr-encode-before");
    let code = QrCode::encode_text(&encoded, QrCodeEcc::Medium)
        .map_err(|err| format!("failed to encode qr: {err:?}"))?;
    crate::log_memory_usage("system-info:qr-encode-after");

    Ok(render_qr_image(&code))
}

fn render_qr_image(code: &QrCode) -> Image {
    crate::log_memory_usage("system-info:qr-render-before");
    let modules = (code.size() + (QR_BORDER * 2)) as u32;
    let scale = (QR_TARGET_SIZE / modules).max(1);
    let size = modules * scale;
    crate::log_memory_usage("system-info:qr-buffer-alloc-before");
    let mut pixels = SharedPixelBuffer::<Rgba8Pixel>::new(size, size);
    crate::log_memory_usage("system-info:qr-buffer-alloc-after");
    pixels.make_mut_slice().fill(Rgba8Pixel {
        r: 255,
        g: 255,
        b: 255,
        a: 255,
    });

    {
        let buffer = pixels.make_mut_slice();
        for y in 0..code.size() {
            for x in 0..code.size() {
                if !code.get_module(x, y) {
                    continue;
                }

                let start_x = ((x + QR_BORDER) as u32) * scale;
                let start_y = ((y + QR_BORDER) as u32) * scale;
                for yy in start_y..(start_y + scale) {
                    for xx in start_x..(start_x + scale) {
                        buffer[(yy * size + xx) as usize] = Rgba8Pixel {
                            r: 0,
                            g: 0,
                            b: 0,
                            a: 255,
                        };
                    }
                }
            }
        }
    }

    crate::log_memory_usage("system-info:qr-image-create-before");
    let image = Image::from_rgba8(pixels);
    crate::log_memory_usage("system-info:qr-image-create-after");
    crate::log_memory_usage("system-info:qr-render-after");
    image
}

fn select_mac_address(interfaces: &[&str]) -> Option<String> {
    interfaces
        .iter()
        .find_map(|name| read_mac_address(name).map(format_mac_address))
}

fn read_mac_address(interface: &str) -> Option<String> {
    let raw = fs::read_to_string(format!("/sys/class/net/{interface}/address")).ok()?;
    let normalized = raw.trim().to_ascii_lowercase();
    if normalized.is_empty() || normalized == "00:00:00:00:00:00" {
        return None;
    }
    Some(normalized)
}

fn format_mac_address(mac: String) -> String {
    mac.split(':')
        .map(|part| part.to_ascii_uppercase())
        .collect::<Vec<_>>()
        .join("-")
}

fn select_ipv4_address(interfaces: &[&str]) -> Option<String> {
    interfaces.iter().find_map(|name| read_ipv4_address(name))
}

fn read_ipv4_address(interface: &str) -> Option<String> {
    let mut addrs: *mut ifaddrs = ptr::null_mut();
    if unsafe { getifaddrs(&mut addrs) } != 0 || addrs.is_null() {
        return None;
    }

    let mut current = addrs;
    let mut result = None;
    while !current.is_null() {
        let entry = unsafe { &*current };
        if !entry.ifa_addr.is_null() && unsafe { (*entry.ifa_addr).sa_family as i32 } == AF_INET {
            let name = unsafe { CStr::from_ptr(entry.ifa_name) }
                .to_string_lossy()
                .into_owned();
            if name == interface {
                let sockaddr = unsafe { &*(entry.ifa_addr as *const sockaddr_in) };
                let bytes = sockaddr.sin_addr.s_addr.to_ne_bytes();
                result = Some(format!(
                    "{}.{}.{}.{}",
                    bytes[0], bytes[1], bytes[2], bytes[3]
                ));
                break;
            }
        }
        current = unsafe { (*current).ifa_next };
    }

    unsafe {
        freeifaddrs(addrs);
    }
    result
}
