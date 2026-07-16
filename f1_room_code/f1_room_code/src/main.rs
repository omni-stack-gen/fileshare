#[cfg(target_arch = "riscv64")]
mod db;

slint::include_modules!();
mod aiot;
mod aiot_sys;
mod app_event;
mod call;
mod capture_records;
mod d2d;
mod i18n;
mod indoor;
#[cfg(all(target_arch = "riscv64", not(kr_indoor_stub_backend)))]
mod indoor_sys;
mod ipc;
mod ipc_sys;
#[cfg(not(kr_wifi_stub_backend))]
mod net_sys;
mod runtime_coordinator;
mod sound;
mod sound_sys;
mod system_info;
mod webrtc;
mod webrtc_sys;
mod wifi;
#[cfg(not(kr_wifi_stub_backend))]
mod wifi_sys;

use app_event::AppEvent;
use runtime_coordinator::{
    RuntimeCoordinator, AIOT_CALLX_ACCEPTED, AIOT_CALLX_CONFIRMED, AIOT_CALLX_HUNGUP,
    AIOT_CALLX_INITIATE_ACK, AIOT_CALLX_INVITED, AIOT_CALLX_INVITE_ACK, AIOT_CALLX_REPLIED,
};
use slint::{ComponentHandle, Image, SharedString, Timer, TimerMode};
#[cfg(target_arch = "riscv64")]
use slint_g2d::{platform::G2DPlatform, SlintG2DConfig};
use std::cell::RefCell;
use std::mem::MaybeUninit;
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

static CALLX_PREEMPTING_LOCAL_MONITOR: AtomicBool = AtomicBool::new(false);
static CALL_RING_TIMEOUT_GENERATION: AtomicU64 = AtomicU64::new(0);

fn debug_call_timeout_disabled() -> bool {
    std::path::Path::new("/data/debugflag").exists()
}

#[derive(Clone, Debug)]
struct TestCloudMode {
    interface: String,
}

fn test_cloud_mode_from_env() -> Option<TestCloudMode> {
    let interface = std::env::var("KR_ROOM_TEST_CLOUD_IFACE")
        .ok()
        .map(|value| value.trim().to_string())
        .filter(|value| !value.is_empty())?;
    Some(TestCloudMode { interface })
}

fn aiot_server_addr_override_from_env() -> Option<String> {
    std::env::var("KR_ROOM_AIOT_SERVER_ADDR")
        .ok()
        .map(|value| value.trim().to_string())
        .filter(|value| !value.is_empty())
}

fn normalize_call_ring_timeout_sec(value: i32) -> i32 {
    match value {
        30 | 60 | 90 => value,
        _ => 30,
    }
}

fn main() -> Result<(), slint::PlatformError> {
    log_memory_usage("startup:begin");

    #[cfg(target_arch = "riscv64")]
    let database = match db::Database::open_default() {
        Ok(database) => Some(Rc::new(database)),
        Err(err) => {
            eprintln!("Failed to initialize KV storage: {err}");
            None
        }
    };
    log_memory_usage("startup:database-ready");
    let test_cloud_mode = test_cloud_mode_from_env();
    if let Some(mode) = test_cloud_mode.as_ref() {
        eprintln!("test-cloud: enabled webrtc_interface={}", mode.interface);
    }

    #[cfg(target_arch = "riscv64")]
    {
        let config = SlintG2DConfig {
            fb_device: "/dev/fb0",
            input_device: "/dev/input/event0",
            width: 1024,
            height: 600,
            rotation: 0,
        };

        let platform = G2DPlatform::new(config).expect("Failed to initialize G2D platform");
        slint::platform::set_platform(Box::new(platform)).expect("Failed to set Slint platform");
    }
    log_memory_usage("startup:platform-ready");

    let app = AppWindow::new()?;
    i18n::configure_i18n_bindings(&app);
    log_memory_usage("startup:app-window-created");
    refresh_system_clock(&app);
    let _system_clock_timer = setup_system_clock_timer(&app);
    let _idle_clock_timer = setup_idle_clock_timer(&app);
    log_memory_usage("startup:timers-ready");
    #[cfg(target_arch = "riscv64")]
    let call_config = database
        .as_ref()
        .and_then(|database| database.settings().ok())
        .map(|settings| call::CallConfig::from_settings(&settings))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let call_config = call::CallConfig::default();
    #[cfg(target_arch = "riscv64")]
    let d2d_config = database
        .as_ref()
        .and_then(|database| database.settings().ok())
        .map(|settings| d2d::D2dConfig::from_settings(&settings))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let d2d_config = d2d::D2dConfig::default();
    let indoor_config = indoor::IndoorConfig::from_configs(&call_config, &d2d_config);
    let aiot_controller_slot = Arc::new(Mutex::new(None::<aiot::AiotController>));
    let runtime_coordinator_slot = Arc::new(Mutex::new(None::<RuntimeCoordinator>));
    let secondary_confirm_state = Arc::new(Mutex::new(aiot::SecondaryConfirmProxyState::default()));
    #[cfg(target_arch = "riscv64")]
    let sound_initial_volume = database
        .as_ref()
        .and_then(|database| database.settings().ok())
        .map(|settings| settings.get_float("sound.ring_volume", 0.80))
        .unwrap_or(0.80);
    #[cfg(not(target_arch = "riscv64"))]
    let sound_initial_volume = 0.80;
    let sound_controller = match sound::SoundController::start(sound_initial_volume) {
        Ok(controller) => Some(controller),
        Err(err) => {
            eprintln!("sound: start failed: {err}");
            None
        }
    };
    configure_sound_bindings(&app, sound_controller.clone());
    let call_snapshot_controller = CallSnapshotController::default();
    log_memory_usage("startup:indoor-before");
    #[cfg(target_arch = "riscv64")]
    let (_indoor_service, call_controller, d2d_controller) = configure_indoor_bindings(
        &app,
        database.as_ref().map(|database| (**database).clone()),
        indoor_config,
        aiot_controller_slot.clone(),
        runtime_coordinator_slot.clone(),
        secondary_confirm_state.clone(),
        sound_controller.clone(),
    );
    #[cfg(not(target_arch = "riscv64"))]
    let (_indoor_service, call_controller, d2d_controller) = configure_indoor_bindings(
        &app,
        None,
        indoor_config,
        aiot_controller_slot.clone(),
        runtime_coordinator_slot.clone(),
        secondary_confirm_state.clone(),
        sound_controller.clone(),
    );
    log_memory_usage("startup:indoor-after");
    #[cfg(target_arch = "riscv64")]
    if let Some(database) = database.as_ref() {
        log_memory_usage("startup:database-bindings-before");
        configure_database_bindings(&app, database.clone(), aiot_controller_slot.clone());
        log_memory_usage("startup:database-bindings-after");
    }
    log_memory_usage("startup:aiot-config-before");
    #[cfg(target_arch = "riscv64")]
    let mut aiot_config = database
        .as_ref()
        .and_then(|database| database.settings().ok())
        .map(|settings| aiot::AiotConfig::from_settings(&settings))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let mut aiot_config = aiot::AiotConfig::default();
    log_memory_usage("startup:aiot-config-after");
    log_memory_usage("startup:webrtc-config-before");
    #[cfg(target_arch = "riscv64")]
    let mut webrtc_config = database
        .as_ref()
        .and_then(|database| database.settings().ok())
        .map(|settings| webrtc::WebrtcConfig::from_settings(&settings))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let mut webrtc_config = webrtc::WebrtcConfig::default();
    log_memory_usage("startup:webrtc-config-after");
    if let Some(mode) = test_cloud_mode.as_ref() {
        aiot_config.interface = mode.interface.clone();
        webrtc_config.interface = mode.interface.clone();
        eprintln!(
            "test-cloud: override aiot_interface={} webrtc_interface={}",
            aiot_config.interface, webrtc_config.interface
        );
    }
    if let Some(server_addr) = aiot_server_addr_override_from_env() {
        aiot_config.server_addr = server_addr;
        aiot_config.mqtt_server_addr.clear();
        eprintln!(
            "aiot-ui: override server_addr from KR_ROOM_AIOT_SERVER_ADDR server={}",
            aiot_config.server_addr
        );
    }
    let aiot_interface_for_test_cloud = aiot_config.interface.clone();
    log_memory_usage("startup:ipc-config-before");
    #[cfg(target_arch = "riscv64")]
    let ipc_config = database
        .as_ref()
        .map(|database| ipc::IpcConfig::from_database(database.as_ref()))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let ipc_config = ipc::IpcConfig::default();
    log_memory_usage("startup:ipc-config-after");
    log_memory_usage("startup:configs-ready");
    log_memory_usage("startup:capture-records-before");
    capture_records::configure_capture_record_bindings(&app, ipc_config.capture_dir.clone());
    log_memory_usage("startup:capture-records-after");
    #[cfg(target_arch = "riscv64")]
    let ipc_camera_source: Rc<dyn Fn() -> Vec<ipc::IpcCameraConfig>> = {
        let database = database.clone();
        Rc::new(move || {
            database
                .as_ref()
                .and_then(|database| database.ipc_cameras().ok())
                .unwrap_or_default()
                .into_iter()
                .filter(|camera| camera.is_usable())
                .collect()
        })
    };
    #[cfg(not(target_arch = "riscv64"))]
    let ipc_camera_source: Rc<dyn Fn() -> Vec<ipc::IpcCameraConfig>> = {
        let cameras = ipc_config
            .cameras
            .iter()
            .filter(|camera| camera.is_usable())
            .cloned()
            .collect::<Vec<_>>();
        Rc::new(move || cameras.clone())
    };
    let (ipc_service, ipc_controller) = configure_ipc_bindings(&app, ipc_config, ipc_camera_source);
    log_memory_usage("startup:ipc-bindings-ready");
    log_memory_usage("startup:page-resources-before");
    configure_page_resource_bindings(&app, ipc_controller.clone());
    log_memory_usage("startup:page-resources-after");
    let webrtc_runtime = configure_webrtc_bindings(
        &app,
        webrtc_config,
        d2d_controller.clone(),
        call_controller.clone(),
        aiot_controller_slot.clone(),
        runtime_coordinator_slot.clone(),
        sound_controller.clone(),
        call_snapshot_controller.clone(),
    );
    log_memory_usage("startup:webrtc-bindings-ready");
    let (aiot_runtime, runtime_coordinator) = configure_aiot_bindings(
        &app,
        aiot_config,
        d2d_controller.clone(),
        webrtc_runtime.clone(),
        ipc_controller.clone(),
        call_controller.clone(),
        sound_controller.clone(),
        call_snapshot_controller.clone(),
        aiot_controller_slot.clone(),
        secondary_confirm_state,
    );
    if let Ok(mut slot) = runtime_coordinator_slot.lock() {
        *slot = runtime_coordinator.clone();
    }
    log_memory_usage("startup:aiot-bindings-ready");
    configure_call_action_routing(
        &app,
        call_controller.clone(),
        runtime_coordinator.clone(),
        aiot_controller_slot.clone(),
        sound_controller.clone(),
        call_snapshot_controller.clone(),
    );
    configure_media_setting_routing(
        &app,
        call_controller,
        runtime_coordinator.clone(),
        ipc_controller.clone(),
        sound_controller,
    );
    log_memory_usage("startup:call-routing-ready");
    #[cfg(target_arch = "riscv64")]
    let mut wifi_seed = database
        .as_ref()
        .map(|database| wifi::WifiSeed::from_database(database.as_ref()))
        .unwrap_or_default();
    #[cfg(not(target_arch = "riscv64"))]
    let mut wifi_seed = wifi::WifiSeed::default();
    if test_cloud_mode.is_some() {
        wifi_seed.enabled = false;
    }
    #[cfg(target_arch = "riscv64")]
    let _wifi_service = configure_wifi_bindings(
        &app,
        database.as_ref().map(|database| (**database).clone()),
        wifi_seed,
        aiot_runtime.clone(),
        webrtc_runtime.clone(),
    );
    #[cfg(not(target_arch = "riscv64"))]
    let _wifi_service = configure_wifi_bindings(
        &app,
        None,
        wifi_seed,
        aiot_runtime.clone(),
        webrtc_runtime.clone(),
    );
    if let Some(mode) = test_cloud_mode.as_ref() {
        eprintln!(
            "test-cloud: skip wifi and start cloud runtimes webrtc_interface={} aiot_interface={}",
            mode.interface, aiot_interface_for_test_cloud
        );
        start_test_cloud_runtimes(
            app.as_weak(),
            aiot_runtime.as_ref(),
            webrtc_runtime.as_ref(),
            mode.interface.clone(),
            aiot_interface_for_test_cloud.clone(),
        );
    }
    log_memory_usage("startup:wifi-bindings-ready");
    system_info::configure_system_info_bindings(&app);
    log_memory_usage("startup:system-info-ready");
    Timer::single_shot(Duration::from_millis(0), || {
        log_memory_usage("startup:event-loop+0ms");
    });
    Timer::single_shot(Duration::from_millis(100), || {
        log_memory_usage("startup:event-loop+100ms");
    });
    Timer::single_shot(Duration::from_millis(500), || {
        log_memory_usage("startup:event-loop+500ms");
    });
    Timer::single_shot(Duration::from_secs(1), || {
        log_memory_usage("startup:event-loop+1s");
    });
    Timer::single_shot(Duration::from_secs(5), || {
        log_memory_usage("startup:event-loop+5s");
    });
    let _ipc_service = ipc_service;
    log_memory_usage("startup:before-app-run");
    app.run()
}

fn configure_page_resource_bindings(app: &AppWindow, ipc_controller: Option<ipc::IpcController>) {
    log_memory_usage("ui-resource:bindings-begin");
    let app_weak = app.as_weak();
    let controller_for_callback = ipc_controller.clone();
    app.global::<State>().on_page_changed(move |page| {
        if let Some(app) = app_weak.upgrade() {
            apply_page_resource_state(&app, page.as_str(), controller_for_callback.as_ref());
        }
    });

    let current_page = app.global::<State>().get_current_page();
    log_memory_usage("ui-resource:initial-apply-before");
    apply_page_resource_state(app, current_page.as_str(), ipc_controller.as_ref());
    log_memory_usage("ui-resource:initial-apply-after");
    log_memory_usage("ui-resource:bindings-done");
}

fn apply_page_resource_state(
    app: &AppWindow,
    current_page: &str,
    ipc_controller: Option<&ipc::IpcController>,
) {
    log_memory_usage(&format!("ui-resource:apply-begin:{current_page}"));
    let state = app.global::<State>();
    if current_page == "hcEj1" {
        state.set_ev_image(load_lazy_asset_image("images/Group 2.png"));
    } else {
        state.set_ev_image(Image::default());
    }

    if current_page != "NeeP0" && current_page != "hEzDU" {
        capture_records::clear_capture_record_images(app);
    }

    if let Some(controller) = ipc_controller {
        if runtime_coordinator::should_stop_ipc_for_page_change(
            current_page,
            controller.is_monitoring(),
        ) {
            spawn_ipc_stop_monitor(app.as_weak(), controller.clone(), "page-change");
        }
    }
    log_memory_usage(&format!("ui-resource:apply-done:{current_page}"));
}

fn load_lazy_asset_image(path: &str) -> Image {
    log_memory_usage(&format!("ui-resource:image-load-before:{path}"));
    let image = Image::load_from_path(std::path::Path::new(path)).unwrap_or_else(|err| {
        eprintln!("ui-resource: load image failed path={path} err={err:?}");
        Image::default()
    });
    log_memory_usage(&format!("ui-resource:image-load-after:{path}"));
    image
}

#[allow(dead_code)]
#[derive(Default)]
struct MemoryUsage {
    vm_size_kib: Option<u64>,
    vm_rss_kib: Option<u64>,
    vm_hwm_kib: Option<u64>,
    rss_anon_kib: Option<u64>,
    rss_file_kib: Option<u64>,
    rss_shmem_kib: Option<u64>,
}

pub(crate) fn log_memory_usage(label: &str) {
    let _ = label;
}

#[allow(dead_code)]
fn read_memory_usage() -> Result<MemoryUsage, String> {
    let status = std::fs::read_to_string("/proc/self/status")
        .map_err(|err| format!("read /proc/self/status failed: {err}"))?;
    let mut usage = MemoryUsage::default();

    for line in status.lines() {
        if let Some(value) = parse_status_kib(line, "VmSize") {
            usage.vm_size_kib = Some(value);
        } else if let Some(value) = parse_status_kib(line, "VmRSS") {
            usage.vm_rss_kib = Some(value);
        } else if let Some(value) = parse_status_kib(line, "VmHWM") {
            usage.vm_hwm_kib = Some(value);
        } else if let Some(value) = parse_status_kib(line, "RssAnon") {
            usage.rss_anon_kib = Some(value);
        } else if let Some(value) = parse_status_kib(line, "RssFile") {
            usage.rss_file_kib = Some(value);
        } else if let Some(value) = parse_status_kib(line, "RssShmem") {
            usage.rss_shmem_kib = Some(value);
        }
    }

    Ok(usage)
}

#[allow(dead_code)]
fn parse_status_kib(line: &str, key: &str) -> Option<u64> {
    let rest = line.strip_prefix(key)?.strip_prefix(':')?;
    rest.split_whitespace().next()?.parse::<u64>().ok()
}

#[allow(dead_code)]
fn format_kib(value: Option<u64>) -> String {
    match value {
        Some(kib) => format!("{kib}kB/{:.2}MiB", kib as f64 / 1024.0),
        None => "n/a".to_string(),
    }
}

fn setup_system_clock_timer(app: &AppWindow) -> Timer {
    let timer = Timer::default();
    let app_weak = app.as_weak();

    timer.start(TimerMode::Repeated, Duration::from_secs(1), move || {
        let Some(app) = app_weak.upgrade() else {
            return;
        };

        refresh_system_clock(&app);
    });

    timer
}

fn setup_idle_clock_timer(app: &AppWindow) -> Timer {
    let timer = Timer::default();
    let app_weak = app.as_weak();
    let main_page_since = Rc::new(RefCell::new(None::<Instant>));
    let main_page_since_ref = main_page_since.clone();

    timer.start(TimerMode::Repeated, Duration::from_secs(1), move || {
        let Some(app) = app_weak.upgrade() else {
            return;
        };

        let state = app.global::<State>();
        let on_main_page = state.get_current_page().as_str() == "LCvek";
        let idle_allowed = on_main_page
            && !state.get_absent_mode_active()
            && state.get_active_overlay().is_empty()
            && state.get_clock_screen_enabled();

        if !idle_allowed {
            *main_page_since_ref.borrow_mut() = None;
            return;
        }

        let timeout = Duration::from_secs(state.get_idle_clock_timeout_sec().max(60) as u64);
        let mut since = main_page_since_ref.borrow_mut();
        if let Some(started_at) = *since {
            if started_at.elapsed() >= timeout {
                state.set_current_page(SharedString::from("VnIoT"));
                *since = None;
            }
        } else {
            *since = Some(Instant::now());
        }
    });

    timer
}

fn refresh_system_clock(app: &AppWindow) {
    let Some((time_text, date_text)) =
        system_clock_strings(app.global::<State>().get_language().as_str())
    else {
        return;
    };

    let state = app.global::<State>();
    state.set_system_time_text(SharedString::from(time_text));
    state.set_system_date_text(SharedString::from(date_text));
}

fn system_clock_strings(language: &str) -> Option<(String, String)> {
    let mut now = MaybeUninit::<libc::time_t>::uninit();
    let mut local_tm = MaybeUninit::<libc::tm>::uninit();

    unsafe {
        libc::time(now.as_mut_ptr());
        if libc::localtime_r(now.as_ptr(), local_tm.as_mut_ptr()).is_null() {
            return None;
        }

        let local_tm = local_tm.assume_init();
        let year = local_tm.tm_year + 1900;
        let month = local_tm.tm_mon + 1;
        let day = local_tm.tm_mday;
        let weekday = localized_weekday(language, local_tm.tm_wday);

        Some((
            format!("{:02}:{:02}", local_tm.tm_hour, local_tm.tm_min),
            format!("{year:04}.{month:02}.{day:02} {weekday}"),
        ))
    }
}

fn current_general_time_parts() -> Option<(String, String, String, String, String, String)> {
    let mut now = MaybeUninit::<libc::time_t>::uninit();
    let mut local_tm = MaybeUninit::<libc::tm>::uninit();

    unsafe {
        libc::time(now.as_mut_ptr());
        if libc::localtime_r(now.as_ptr(), local_tm.as_mut_ptr()).is_null() {
            return None;
        }

        let local_tm = local_tm.assume_init();
        let hour24 = local_tm.tm_hour;
        let period = if hour24 >= 12 { "PM" } else { "AM" };
        let hour12 = match hour24 % 12 {
            0 => 12,
            value => value,
        };

        Some((
            period.to_string(),
            format!("{hour12:02}"),
            format!("{:02}", local_tm.tm_min),
            format!("{:04}", local_tm.tm_year + 1900),
            format!("{:02}", local_tm.tm_mon + 1),
            format!("{:02}", local_tm.tm_mday),
        ))
    }
}

fn refresh_general_setting_time(state: &State<'_>) {
    let Some((period, hour, minute, year, month, day)) = current_general_time_parts() else {
        return;
    };

    state.set_time_period(SharedString::from(period));
    state.set_general_hour(SharedString::from(hour));
    state.set_general_minute(SharedString::from(minute));
    state.set_general_year(SharedString::from(year));
    state.set_general_month(SharedString::from(month));
    state.set_general_day(SharedString::from(day));
}

fn general_setting_hour_24(period: &str, hour_12: i32) -> Result<i32, String> {
    if !(1..=12).contains(&hour_12) {
        return Err(format!("invalid hour: {hour_12}"));
    }

    match period {
        "AM" => Ok(if hour_12 == 12 { 0 } else { hour_12 }),
        "PM" => Ok(if hour_12 == 12 { 12 } else { hour_12 + 12 }),
        other => Err(format!("invalid time period: {other}")),
    }
}

fn set_general_system_time(
    period: &str,
    hour: &str,
    minute: &str,
    year: &str,
    month: &str,
    day: &str,
) -> Result<(), String> {
    let hour_12 = hour
        .parse::<i32>()
        .map_err(|err| format!("invalid hour {hour}: {err}"))?;
    let minute = minute
        .parse::<i32>()
        .map_err(|err| format!("invalid minute {minute}: {err}"))?;
    let year = year
        .parse::<i32>()
        .map_err(|err| format!("invalid year {year}: {err}"))?;
    let month = month
        .parse::<i32>()
        .map_err(|err| format!("invalid month {month}: {err}"))?;
    let day = day
        .parse::<i32>()
        .map_err(|err| format!("invalid day {day}: {err}"))?;
    let hour_24 = general_setting_hour_24(period, hour_12)?;

    if !(0..=59).contains(&minute) {
        return Err(format!("invalid minute: {minute}"));
    }
    if !(1..=12).contains(&month) {
        return Err(format!("invalid month: {month}"));
    }
    if !(1..=31).contains(&day) {
        return Err(format!("invalid day: {day}"));
    }

    let mut tm = libc::tm {
        tm_sec: 0,
        tm_min: minute,
        tm_hour: hour_24,
        tm_mday: day,
        tm_mon: month - 1,
        tm_year: year - 1900,
        tm_wday: 0,
        tm_yday: 0,
        tm_isdst: -1,
        #[cfg(any(
            target_env = "gnu",
            target_env = "musl",
            target_os = "android",
            target_os = "emscripten",
            target_os = "hurd"
        ))]
        tm_gmtoff: 0,
        #[cfg(any(
            target_env = "gnu",
            target_env = "musl",
            target_os = "android",
            target_os = "emscripten",
            target_os = "hurd"
        ))]
        tm_zone: std::ptr::null(),
    };

    let seconds = unsafe { libc::mktime(&mut tm as *mut libc::tm) };
    if seconds < 0 {
        return Err("mktime failed".to_string());
    }

    let spec = libc::timespec {
        tv_sec: seconds,
        tv_nsec: 0,
    };
    let ret = unsafe { libc::clock_settime(libc::CLOCK_REALTIME, &spec as *const libc::timespec) };
    if ret != 0 {
        return Err(format!(
            "clock_settime failed: {}",
            std::io::Error::last_os_error()
        ));
    }

    Ok(())
}

fn localized_weekday(language: &str, weekday: i32) -> &'static str {
    const EN: [&str; 7] = [
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
    ];
    const VI: [&str; 7] = [
        "Chu nhat", "Thu hai", "Thu ba", "Thu tu", "Thu nam", "Thu sau", "Thu bay",
    ];
    const ES: [&str; 7] = [
        "Domingo",
        "Lunes",
        "Martes",
        "Miercoles",
        "Jueves",
        "Viernes",
        "Sabado",
    ];
    const JA: [&str; 7] = [
        "日曜日",
        "月曜日",
        "火曜日",
        "水曜日",
        "木曜日",
        "金曜日",
        "土曜日",
    ];
    const AR: [&str; 7] = [
        "الأحد",
        "الاثنين",
        "الثلاثاء",
        "الأربعاء",
        "الخميس",
        "الجمعة",
        "السبت",
    ];
    const KO: [&str; 7] = [
        "일요일",
        "월요일",
        "화요일",
        "수요일",
        "목요일",
        "금요일",
        "토요일",
    ];

    let index = weekday.clamp(0, 6) as usize;
    match language {
        "en" => EN[index],
        "vi" => VI[index],
        "es" => ES[index],
        "ja" => JA[index],
        "ar" => AR[index],
        _ => KO[index],
    }
}

fn parse_cctv_ip_parts(value: &str) -> [String; 4] {
    let mut parts = [
        String::new(),
        "0".to_string(),
        "0".to_string(),
        "0".to_string(),
    ];
    let value = value.trim();
    if value.is_empty() {
        return parts;
    }

    for (index, part) in value.split('.').take(4).enumerate() {
        if !part.is_empty() && part.chars().all(|ch| ch.is_ascii_digit()) {
            parts[index] = part.to_string();
        } else {
            parts[index].clear();
        }
    }
    parts
}

fn get_cctv_ip_parts(state: &State<'_>) -> [String; 4] {
    [
        state.get_cctv_setting_ip_part1().to_string(),
        state.get_cctv_setting_ip_part2().to_string(),
        state.get_cctv_setting_ip_part3().to_string(),
        state.get_cctv_setting_ip_part4().to_string(),
    ]
}

fn set_cctv_ip_parts(state: &State<'_>, parts: &[String; 4], active_index: i32) {
    state.set_cctv_setting_ip_part1(SharedString::from(parts[0].clone()));
    state.set_cctv_setting_ip_part2(SharedString::from(parts[1].clone()));
    state.set_cctv_setting_ip_part3(SharedString::from(parts[2].clone()));
    state.set_cctv_setting_ip_part4(SharedString::from(parts[3].clone()));
    state.set_cctv_setting_ip_part_index(active_index.clamp(1, 4));
}

fn joined_cctv_ip(parts: &[String; 4]) -> String {
    if parts.iter().any(|part| part.is_empty()) {
        String::new()
    } else {
        format!("{}.{}.{}.{}", parts[0], parts[1], parts[2], parts[3])
    }
}

fn sync_cctv_ip_buffer(state: &State<'_>) {
    let value = joined_cctv_ip(&get_cctv_ip_parts(state));
    state.set_cctv_setting_input_buffer(SharedString::from(value.clone()));
    state.set_cctv_setting_input_length(value.chars().count() as i32);
}

fn cctv_password_mask(value: &str) -> String {
    "•".repeat(value.chars().count())
}

fn cctv_camera_detail(camera: &ipc::IpcCameraConfig, port: &str) -> String {
    format!("{}:{} • {}", camera.ip, port, camera.user)
}

fn cctv_default_name_number(name: &str) -> Option<usize> {
    let digits = name.trim().strip_prefix("CCTV")?;
    if digits.is_empty() || !digits.chars().all(|ch| ch.is_ascii_digit()) {
        return None;
    }
    let number = digits.parse::<usize>().ok()?;
    if number == 0 {
        None
    } else {
        Some(number)
    }
}

fn next_cctv_camera_name(cameras: &[ipc::IpcCameraConfig]) -> String {
    let used_numbers = cameras
        .iter()
        .filter_map(|camera| cctv_default_name_number(&camera.name))
        .collect::<Vec<_>>();
    let mut number = 1usize;
    while used_numbers.contains(&number) {
        number += 1;
    }
    format!("CCTV{number}")
}

fn apply_cctv_camera_list_to_state(
    state: &State<'_>,
    cameras: &[ipc::IpcCameraConfig],
    port: &str,
) {
    let names = cameras
        .iter()
        .enumerate()
        .map(|(index, camera)| {
            if camera.name.is_empty() {
                SharedString::from(format!("CCTV{}", index + 1))
            } else {
                SharedString::from(camera.name.clone())
            }
        })
        .collect::<Vec<_>>();
    let details = cameras
        .iter()
        .map(|camera| SharedString::from(cctv_camera_detail(camera, port)))
        .collect::<Vec<_>>();
    state.set_cctv_camera_options(slint::ModelRc::new(slint::VecModel::from(names)));
    state.set_cctv_camera_details(slint::ModelRc::new(slint::VecModel::from(details)));
}

fn apply_cctv_monitor_camera_options_to_state(state: &State<'_>, cameras: &[ipc::IpcCameraConfig]) {
    let names = cameras
        .iter()
        .enumerate()
        .map(|(index, camera)| {
            if camera.name.is_empty() {
                SharedString::from(format!("CCTV{}", index + 1))
            } else {
                SharedString::from(camera.name.clone())
            }
        })
        .collect::<Vec<_>>();
    state.set_cctv_camera_options(slint::ModelRc::new(slint::VecModel::from(names)));
}

fn apply_cctv_camera_to_setting_state(
    state: &State<'_>,
    camera: &ipc::IpcCameraConfig,
    port: &str,
) {
    state.set_cctv_setting_ip(SharedString::from(camera.ip.clone()));
    let cctv_ip_parts = parse_cctv_ip_parts(&camera.ip);
    set_cctv_ip_parts(state, &cctv_ip_parts, 1);
    state.set_cctv_setting_port(SharedString::from(port.to_string()));
    state.set_cctv_setting_account(SharedString::from(camera.user.clone()));
    state.set_cctv_setting_password(SharedString::from(camera.password.clone()));
    state.set_cctv_setting_password_mask(SharedString::from(cctv_password_mask(&camera.password)));
    state.set_cctv_setting_saved(camera.enabled && camera.is_usable());
    state.set_cctv_setting_tested(camera.enabled && camera.is_usable());
    state.set_cctv_setting_test_running(false);
    state.set_cctv_setting_test_step(if camera.enabled && camera.is_usable() {
        6
    } else {
        0
    });
    state.set_cctv_setting_test_error(SharedString::from(""));
}

#[cfg(target_arch = "riscv64")]
fn configure_database_bindings(
    app: &AppWindow,
    database: Rc<db::Database>,
    aiot_controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
) {
    let state = app.global::<State>();

    match database.settings() {
        Ok(settings) => {
            apply_settings_to_state(&state, &settings);
            apply_cctv_setting_to_state(&state, &database, &settings);
        }
        Err(err) => eprintln!("Failed to load settings: {err}"),
    }

    state.on_save_float_setting({
        let database = database.clone();
        move |key, value| {
            if let Err(err) = database.set_float(key.as_str(), value) {
                eprintln!("Failed to save float setting {key}: {err}");
            }
        }
    });

    state.on_save_int_setting({
        let database = database.clone();
        let app_weak = app.as_weak();
        let aiot_controller_slot = aiot_controller_slot.clone();
        move |key, value| {
            if let Err(err) = database.set_int(key.as_str(), value) {
                eprintln!("Failed to save int setting {key}: {err}");
            }
            if matches!(
                key.as_str(),
                "mode.call_ring_timeout_sec" | "mode.communication_timeout_min"
            ) {
                if let Some(app) = app_weak.upgrade() {
                    let state = app.global::<State>();
                    sync_aiot_call_timeouts_for_state(Some(&aiot_controller_slot), &state);
                }
            }
        }
    });

    state.on_save_string_setting({
        let database = database.clone();
        move |key, value| {
            if let Err(err) = database.set_string(key.as_str(), value.as_str()) {
                eprintln!("Failed to save string setting {key}: {err}");
            }
        }
    });

    state.on_save_bool_setting({
        let database = database.clone();
        move |key, value| {
            if let Err(err) = database.set_bool(key.as_str(), value) {
                eprintln!("Failed to save bool setting {key}: {err}");
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_set_system_time(move |period, hour, minute, year, month, day| {
        if let Err(err) = set_general_system_time(
            period.as_str(),
            hour.as_str(),
            minute.as_str(),
            year.as_str(),
            month.as_str(),
            day.as_str(),
        ) {
            eprintln!("Failed to update system time: {err}");
            return;
        }

        if let Some(app) = app_weak.upgrade() {
            refresh_system_clock(&app);
        }
    });

    let app_weak = app.as_weak();
    state.on_refresh_general_setting_time(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            refresh_general_setting_time(&state);
        }
    });

    let app_weak = app.as_weak();
    state.on_submit_setting_info({
        let database = database.clone();
        move |building, room| {
            let building = building.to_string();
            let room = room.to_string();
            let success = database
                .set_string("site.building_number", &building)
                .and_then(|_| database.set_string("site.room_number", &room))
                .is_ok();

            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                if success {
                    state.set_site_building_number(SharedString::from(building.clone()));
                    state.set_site_room_number(SharedString::from(room.clone()));
                    state.set_site_building_draft(SharedString::from(building));
                    state.set_site_room_draft(SharedString::from(room));
                }
                state.set_active_overlay(SharedString::from(if success {
                    "cqURy"
                } else {
                    "vVqV2"
                }));
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_open_setting_info_input(move |target, initial| {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let initial = initial.to_string();
            state.set_site_info_input_target(target);
            state.set_site_info_input_buffer(SharedString::from(initial.clone()));
            state.set_site_info_input_length(initial.chars().count() as i32);
            state.set_active_overlay(SharedString::from("FMNpj"));
        }
    });

    let app_weak = app.as_weak();
    state.on_append_setting_info_input(move |digit| {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let mut current = state.get_site_info_input_buffer().to_string();
            if current.chars().count() < 8 {
                current.push_str(digit.as_str());
                state.set_site_info_input_buffer(SharedString::from(current.clone()));
                state.set_site_info_input_length(current.chars().count() as i32);
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_backspace_setting_info_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let mut current = state.get_site_info_input_buffer().to_string();
            current.pop();
            state.set_site_info_input_buffer(SharedString::from(current.clone()));
            state.set_site_info_input_length(current.chars().count() as i32);
        }
    });

    let app_weak = app.as_weak();
    state.on_clear_setting_info_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            state.set_site_info_input_buffer(SharedString::from(""));
            state.set_site_info_input_length(0);
        }
    });

    let app_weak = app.as_weak();
    state.on_cctv_select_setting_camera({
        let database = database.clone();
        move |index| {
            if index <= 0 {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let port = state.get_cctv_setting_port().to_string();
                if let Ok(cameras) = database.ipc_cameras() {
                    if let Some(camera) = cameras.get(index as usize - 1) {
                        apply_cctv_camera_to_setting_state(&state, camera, &port);
                        state.set_cctv_selected_camera(index);
                        state.set_cctv_current_camera_label(SharedString::from(
                            if camera.name.is_empty() {
                                format!("CCTV{index}")
                            } else {
                                camera.name.clone()
                            },
                        ));
                    }
                }
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_cctv_delete_setting_camera({
        let database = database.clone();
        move |index| {
            if index <= 0 {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let port = state.get_cctv_setting_port().to_string();
                if let Ok(mut cameras) = database.ipc_cameras() {
                    let remove_index = index as usize - 1;
                    if remove_index < cameras.len() {
                        cameras.remove(remove_index);
                        if database.replace_ipc_cameras(&cameras).is_ok() {
                            apply_cctv_camera_list_to_state(&state, &cameras, &port);
                            state.set_cctv_setting_delete_index(0);
                            state.set_active_overlay(SharedString::from(""));
                            state.set_current_page(SharedString::from("GUr03"));
                            if let Some(camera) = cameras.first() {
                                apply_cctv_camera_to_setting_state(&state, camera, &port);
                                state.set_cctv_selected_camera(1);
                                state.set_cctv_current_camera_label(SharedString::from(
                                    if camera.name.is_empty() {
                                        "CCTV1".to_string()
                                    } else {
                                        camera.name.clone()
                                    },
                                ));
                            } else {
                                state.set_cctv_selected_camera(0);
                                state.set_cctv_current_camera_label(SharedString::from(""));
                                state.set_cctv_setting_ip(SharedString::from(""));
                                state.set_cctv_setting_account(SharedString::from(""));
                                state.set_cctv_setting_password(SharedString::from(""));
                                state.set_cctv_setting_password_mask(SharedString::from(""));
                                state.set_cctv_setting_saved(false);
                                state.set_cctv_setting_tested(false);
                                state.set_cctv_setting_test_running(false);
                                state.set_cctv_setting_test_step(0);
                                state.set_cctv_setting_test_error(SharedString::from(""));
                            }
                        }
                    }
                }
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_open_cctv_setting_input(move |target, initial| {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target_string = target.to_string();
            let initial = initial.to_string();
            state.set_cctv_setting_input_target(SharedString::from(target_string.as_str()));
            state.set_cctv_setting_input_buffer(SharedString::from(initial.clone()));
            state.set_cctv_setting_input_mask(SharedString::from(if target_string == "password" {
                cctv_password_mask(&initial)
            } else {
                String::new()
            }));
            state.set_cctv_setting_input_length(initial.chars().count() as i32);
            if target_string == "ip" {
                let parts = parse_cctv_ip_parts(&initial);
                set_cctv_ip_parts(&state, &parts, 1);
                sync_cctv_ip_buffer(&state);
                state.set_active_overlay(SharedString::from("pJGE2"));
            } else if target_string == "port" {
                state.set_active_overlay(SharedString::from("YBsES"));
            } else if target_string == "account" {
                state.set_keyboard_mode(SharedString::from("lower"));
                state.set_active_overlay(SharedString::from("KCRfp"));
            } else if target_string == "password" {
                state.set_keyboard_mode(SharedString::from("lower"));
                state.set_password_visible(false);
                state.set_active_overlay(SharedString::from("vfuIh"));
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_append_cctv_setting_input(move |value| {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target = state.get_cctv_setting_input_target();
            if target.as_str() == "ip" {
                let mut parts = get_cctv_ip_parts(&state);
                let mut index = state.get_cctv_setting_ip_part_index().clamp(1, 4) as usize;
                if value.as_str() == "." {
                    if index < 4 {
                        index += 1;
                        set_cctv_ip_parts(&state, &parts, index as i32);
                        sync_cctv_ip_buffer(&state);
                    }
                    return;
                }
                if value.chars().count() != 1
                    || !value.chars().next().is_some_and(|ch| ch.is_ascii_digit())
                {
                    return;
                }

                let slot = &mut parts[index - 1];
                let next = if slot == "0" {
                    value.to_string()
                } else {
                    if slot.chars().count() >= 3 {
                        return;
                    }
                    format!("{slot}{value}")
                };
                if next
                    .parse::<u16>()
                    .map(|value| value <= 255)
                    .unwrap_or(false)
                {
                    *slot = next;
                    set_cctv_ip_parts(&state, &parts, index as i32);
                    sync_cctv_ip_buffer(&state);
                }
                return;
            }

            let mut current = state.get_cctv_setting_input_buffer().to_string();
            let max_len = if target.as_str() == "port" { 5 } else { 32 };
            if current.chars().count() >= max_len {
                return;
            }
            current.push_str(value.as_str());
            state.set_cctv_setting_input_buffer(SharedString::from(current.clone()));
            if target.as_str() == "password" {
                state.set_cctv_setting_input_mask(SharedString::from(cctv_password_mask(&current)));
            }
            state.set_cctv_setting_input_length(current.chars().count() as i32);
        }
    });

    let app_weak = app.as_weak();
    state.on_backspace_cctv_setting_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            if state.get_cctv_setting_input_target().as_str() == "ip" {
                let mut parts = get_cctv_ip_parts(&state);
                let mut index = state.get_cctv_setting_ip_part_index().clamp(1, 4) as usize;
                if !parts[index - 1].is_empty() {
                    parts[index - 1].pop();
                } else if index > 1 {
                    index -= 1;
                    parts[index - 1].pop();
                }
                set_cctv_ip_parts(&state, &parts, index as i32);
                sync_cctv_ip_buffer(&state);
                return;
            }

            let mut current = state.get_cctv_setting_input_buffer().to_string();
            current.pop();
            state.set_cctv_setting_input_buffer(SharedString::from(current.clone()));
            if state.get_cctv_setting_input_target().as_str() == "password" {
                state.set_cctv_setting_input_mask(SharedString::from(cctv_password_mask(&current)));
            }
            state.set_cctv_setting_input_length(current.chars().count() as i32);
        }
    });

    let app_weak = app.as_weak();
    state.on_clear_cctv_setting_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            if state.get_cctv_setting_input_target().as_str() == "ip" {
                let parts = parse_cctv_ip_parts("");
                set_cctv_ip_parts(&state, &parts, 1);
            }
            state.set_cctv_setting_input_buffer(SharedString::from(""));
            state.set_cctv_setting_input_mask(SharedString::from(""));
            state.set_cctv_setting_input_length(0);
        }
    });

    let app_weak = app.as_weak();
    state.on_submit_cctv_setting_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target = state.get_cctv_setting_input_target();
            let value = if target.as_str() == "ip" {
                SharedString::from(joined_cctv_ip(&get_cctv_ip_parts(&state)))
            } else {
                state.get_cctv_setting_input_buffer()
            };
            if target.as_str() == "ip" {
                state.set_cctv_setting_ip(value);
            } else if target.as_str() == "port" {
                state.set_cctv_setting_port(value);
            } else if target.as_str() == "account" {
                state.set_cctv_setting_account(value);
            } else if target.as_str() == "password" {
                let password = value.to_string();
                state.set_cctv_setting_password(value);
                state.set_cctv_setting_password_mask(SharedString::from(cctv_password_mask(
                    &password,
                )));
            }
            state.set_cctv_setting_input_target(SharedString::from(""));
            state.set_cctv_setting_input_buffer(SharedString::from(""));
            state.set_cctv_setting_input_mask(SharedString::from(""));
            state.set_cctv_setting_input_length(0);
            state.set_cctv_setting_tested(false);
            state.set_cctv_setting_saved(false);
            state.set_cctv_setting_test_running(false);
            state.set_cctv_setting_test_step(0);
            state.set_cctv_setting_test_error(SharedString::from(""));
            state.set_active_overlay(SharedString::from(""));
        }
    });

    let app_weak = app.as_weak();
    state.on_save_cctv_setting({
        let database = database.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let ip = state.get_cctv_setting_ip().to_string();
                let port = state.get_cctv_setting_port().to_string();
                let account = state.get_cctv_setting_account().to_string();
                let password = state.get_cctv_setting_password().to_string();
                let selected = state.get_cctv_selected_camera();
                let current_page = state.get_current_page().to_string();
                let mut cameras = database.ipc_cameras().unwrap_or_default();
                let edit_index = if current_page == "foYV2"
                    && selected > 0
                    && (selected as usize) <= cameras.len()
                {
                    Some(selected as usize - 1)
                } else {
                    None
                };
                let save_index = edit_index.unwrap_or(cameras.len());
                let camera_name = edit_index
                    .and_then(|index| cameras.get(index))
                    .map(|camera| camera.name.clone())
                    .filter(|name| !name.trim().is_empty())
                    .unwrap_or_else(|| next_cctv_camera_name(&cameras));
                let camera = ipc::IpcCameraConfig {
                    name: camera_name.clone(),
                    ip: ip.clone(),
                    user: account.clone(),
                    password,
                    rtsp_url: String::new(),
                    sub_rtsp_url: String::new(),
                    enabled: true,
                    prefer_substream: false,
                };
                if let Some(index) = edit_index {
                    cameras[index] = camera;
                } else {
                    cameras.push(camera);
                }
                let success = database
                    .set_bool("ipc.enabled", true)
                    .and_then(|_| database.set_string("ipc.onvif_port", &port))
                    .and_then(|_| database.replace_ipc_cameras(&cameras))
                    .is_ok();
                if success {
                    state.set_cctv_setting_tested(true);
                    state.set_cctv_setting_saved(true);
                    state.set_cctv_setting_test_running(false);
                    state.set_cctv_setting_test_step(6);
                    state.set_cctv_setting_test_error(SharedString::from(""));
                    apply_cctv_camera_list_to_state(&state, &cameras, &port);
                    state.set_cctv_selected_camera((save_index + 1) as i32);
                    state.set_cctv_current_camera_label(SharedString::from(camera_name));
                    state.set_active_overlay(SharedString::from(""));
                } else {
                    state.set_active_overlay(SharedString::from("E0zNY"));
                }
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_append_password_digit(move |digit| {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target = state.get_keyboard_target();
            let current = if target.as_str() == "old-password" {
                state.get_old_password().to_string()
            } else if target.as_str() == "new-password" {
                state.get_new_password().to_string()
            } else {
                return;
            };

            if current.chars().count() >= 6 {
                return;
            }

            let mut next = current;
            next.push_str(digit.as_str());
            if target.as_str() == "old-password" {
                state.set_old_password(SharedString::from(next));
                state.set_old_password_length(state.get_old_password().chars().count() as i32);
                state.set_old_password_mask(SharedString::from(
                    "*".repeat(state.get_old_password_length() as usize),
                ));
            } else {
                state.set_new_password(SharedString::from(next));
                state.set_new_password_length(state.get_new_password().chars().count() as i32);
                state.set_new_password_mask(SharedString::from(
                    "*".repeat(state.get_new_password_length() as usize),
                ));
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_backspace_password_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target = state.get_keyboard_target();
            if target.as_str() == "old-password" {
                let mut value = state.get_old_password().to_string();
                value.pop();
                state.set_old_password(SharedString::from(value));
                state.set_old_password_length(state.get_old_password().chars().count() as i32);
                state.set_old_password_mask(SharedString::from(
                    "*".repeat(state.get_old_password_length() as usize),
                ));
            } else if target.as_str() == "new-password" {
                let mut value = state.get_new_password().to_string();
                value.pop();
                state.set_new_password(SharedString::from(value));
                state.set_new_password_length(state.get_new_password().chars().count() as i32);
                state.set_new_password_mask(SharedString::from(
                    "*".repeat(state.get_new_password_length() as usize),
                ));
            }
        }
    });

    let app_weak = app.as_weak();
    state.on_clear_password_input(move || {
        if let Some(app) = app_weak.upgrade() {
            let state = app.global::<State>();
            let target = state.get_keyboard_target();
            if target.as_str() == "old-password" {
                state.set_old_password(SharedString::from(""));
                state.set_old_password_length(0);
                state.set_old_password_mask(SharedString::from(""));
            } else if target.as_str() == "new-password" {
                state.set_new_password(SharedString::from(""));
                state.set_new_password_length(0);
                state.set_new_password_mask(SharedString::from(""));
            }
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn spawn_wifi_connected_tasks(
    app_weak: slint::Weak<AppWindow>,
    database: Option<db::Database>,
    aiot_runtime: Option<aiot::AiotRuntime>,
    webrtc_runtime: Option<webrtc::WebrtcRuntime>,
    ssid: String,
    password: Option<String>,
) {
    std::thread::spawn(move || {
        let mut error = None;
        if let Some(aiot_runtime) = aiot_runtime.as_ref() {
            eprintln!("aiot-ui: start requested ssid={ssid}");
            match aiot_runtime.start() {
                Ok(()) => eprintln!("aiot-ui: start ok ssid={ssid}"),
                Err(err) => {
                    eprintln!("aiot-ui: start failed ssid={ssid} err={err}");
                    error = Some(format!("aiot_start_failed:{err}"));
                }
            }
        }
        if let Some(webrtc_runtime) = webrtc_runtime.as_ref() {
            if let Err(err) = webrtc_runtime.start() {
                error.get_or_insert_with(|| format!("webrtc_start_failed:{err}"));
            }
        }
        if let Some(database) = database.as_ref() {
            if let Err(err) = database.set_string("network.connected_ssid", &ssid) {
                eprintln!("Failed to save connected SSID: {err}");
            }
            if let Some(password) = password.as_ref() {
                if let Err(err) = database.upsert_wifi_profile(&ssid, password, 100, true) {
                    eprintln!("Failed to upsert WiFi profile: {err}");
                }
            }
        }
        if let Some(error) = error {
            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = app_weak.upgrade() {
                    app.global::<State>()
                        .set_call_last_error(SharedString::from(error));
                }
            });
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn spawn_wifi_clear_connected_ssid(database: db::Database) {
    std::thread::spawn(move || {
        let _ = database.set_string("network.connected_ssid", "");
    });
}

fn start_test_cloud_runtimes(
    app_weak: slint::Weak<AppWindow>,
    aiot_runtime: Option<&aiot::AiotRuntime>,
    webrtc_runtime: Option<&webrtc::WebrtcRuntime>,
    webrtc_interface: String,
    aiot_interface: String,
) {
    let aiot_runtime = aiot_runtime.cloned();
    let webrtc_runtime = webrtc_runtime.cloned();
    std::thread::spawn(move || {
        let mut error = None;
        if let Some(aiot_runtime) = aiot_runtime.as_ref() {
            eprintln!("test-cloud: aiot start requested interface={aiot_interface}");
            match aiot_runtime.start() {
                Ok(()) => eprintln!("test-cloud: aiot start ok interface={aiot_interface}"),
                Err(err) => {
                    eprintln!("test-cloud: aiot start failed interface={aiot_interface} err={err}");
                    error = Some(format!("aiot_start_failed:{err}"));
                }
            }
        }
        if let Some(webrtc_runtime) = webrtc_runtime.as_ref() {
            eprintln!("test-cloud: webrtc start requested interface={webrtc_interface}");
            if let Err(err) = webrtc_runtime.start() {
                eprintln!("test-cloud: webrtc start failed interface={webrtc_interface} err={err}");
                error.get_or_insert_with(|| format!("webrtc_start_failed:{err}"));
            } else {
                eprintln!("test-cloud: webrtc start ok interface={webrtc_interface}");
            }
        }
        if let Some(error) = error {
            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = app_weak.upgrade() {
                    app.global::<State>()
                        .set_call_last_error(SharedString::from(error));
                }
            });
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn configure_wifi_bindings(
    app: &AppWindow,
    database: Option<db::Database>,
    wifi_seed: wifi::WifiSeed,
    aiot_runtime: Option<aiot::AiotRuntime>,
    webrtc_runtime: Option<webrtc::WebrtcRuntime>,
) -> Option<wifi::WifiService> {
    if !wifi_seed.enabled {
        eprintln!("wifi: disabled by config, skip native initialization");
        bind_wifi_fallback_callbacks(app, database);
        return None;
    }

    let app_weak = app.as_weak();
    let database_for_events = database.clone();
    let aiot_runtime_for_events = aiot_runtime.clone();
    let webrtc_runtime_for_events = webrtc_runtime.clone();
    let cloud_started_ssid = Arc::new(Mutex::new(String::new()));
    let dispatcher: Arc<dyn Fn(wifi::WifiEvent) + Send + Sync + 'static> = Arc::new(move |event| {
        let app_weak = app_weak.clone();
        let database = database_for_events.clone();
        let aiot_runtime = aiot_runtime_for_events.clone();
        let webrtc_runtime = webrtc_runtime_for_events.clone();
        let cloud_started_ssid = cloud_started_ssid.clone();
        let _ = slint::invoke_from_event_loop(move || {
            let Some(app) = app_weak.upgrade() else {
                return;
            };

            let state = app.global::<State>();
            wifi::apply_event_to_state(&state, &event);

            match event {
                wifi::WifiEvent::ConnectSucceeded {
                    ref ssid,
                    ref password,
                    foreground: _,
                    ..
                } => {
                    if let Ok(mut started_ssid) = cloud_started_ssid.lock() {
                        *started_ssid = ssid.clone();
                    }
                    spawn_wifi_connected_tasks(
                        app.as_weak(),
                        database,
                        aiot_runtime,
                        webrtc_runtime,
                        ssid.clone(),
                        Some(password.clone()),
                    );
                }
                wifi::WifiEvent::Snapshot {
                    connected_ssid,
                    status,
                    ..
                } => {
                    if matches!(status, wifi::WifiRuntimeState::Connected)
                        && !connected_ssid.is_empty()
                    {
                        let should_start = cloud_started_ssid
                            .lock()
                            .map(|mut started_ssid| {
                                if *started_ssid == connected_ssid {
                                    false
                                } else {
                                    *started_ssid = connected_ssid.clone();
                                    true
                                }
                            })
                            .unwrap_or(true);
                        if should_start {
                            spawn_wifi_connected_tasks(
                                app.as_weak(),
                                database.clone(),
                                aiot_runtime,
                                webrtc_runtime,
                                connected_ssid.clone(),
                                None,
                            );
                        }
                    }
                    if connected_ssid.is_empty()
                        && matches!(status, wifi::WifiRuntimeState::Idle)
                        && !state.get_wifi_enabled()
                    {
                        if let Ok(mut started_ssid) = cloud_started_ssid.lock() {
                            started_ssid.clear();
                        }
                        if let Some(database) = database {
                            spawn_wifi_clear_connected_ssid(database);
                        }
                    }
                }
                _ => {}
            }
        });
    });

    let service = match wifi::WifiService::start(wifi_seed, dispatcher) {
        Ok(service) => service,
        Err(err) => {
            let state = app.global::<State>();
            state.set_wifi_status(SharedString::from("failed"));
            state.set_call_last_error(SharedString::from(format!("wifi_start_failed:{err}")));
            bind_wifi_fallback_callbacks(app, database);
            return None;
        }
    };

    bind_wifi_callbacks(app, database, service.controller());
    Some(service)
}

#[cfg(not(target_arch = "riscv64"))]
fn configure_wifi_bindings(
    app: &AppWindow,
    _database: Option<()>,
    wifi_seed: wifi::WifiSeed,
    aiot_runtime: Option<aiot::AiotRuntime>,
    webrtc_runtime: Option<webrtc::WebrtcRuntime>,
) -> Option<wifi::WifiService> {
    if !wifi_seed.enabled {
        return None;
    }

    let service = wifi::WifiService::start(wifi_seed, Arc::new(|_| {})).ok()?;
    if let Some(aiot_runtime) = aiot_runtime.as_ref() {
        let _ = aiot_runtime.start();
    }
    if let Some(webrtc_runtime) = webrtc_runtime.as_ref() {
        let _ = webrtc_runtime.start();
    }
    bind_wifi_callbacks(app, None, service.controller());
    Some(service)
}

#[derive(Clone)]
#[allow(dead_code)]
struct AppEventDispatchContext {
    app_weak: slint::Weak<AppWindow>,
    d2d_controller: Option<d2d::D2dController>,
    runtime_coordinator: Option<RuntimeCoordinator>,
    ipc_controller: Option<ipc::IpcController>,
    call_controller: Option<call::CallController>,
    sound_controller: Option<sound::SoundController>,
    call_snapshot_controller: Option<CallSnapshotController>,
    aiot_controller_slot: Option<Arc<Mutex<Option<aiot::AiotController>>>>,
    runtime_coordinator_slot: Option<Arc<Mutex<Option<RuntimeCoordinator>>>>,
    secondary_confirm_state: Option<Arc<Mutex<aiot::SecondaryConfirmProxyState>>>,
}

#[derive(Clone, Default)]
struct CallSnapshotController {
    generation: Arc<AtomicU64>,
    path: Arc<Mutex<Option<std::path::PathBuf>>>,
}

impl CallSnapshotController {
    fn start_download(
        &self,
        app_weak: slint::Weak<AppWindow>,
        state: &State<'_>,
        aiot_controller_slot: Option<&Arc<Mutex<Option<aiot::AiotController>>>>,
        callid: &str,
        url: &str,
    ) {
        self.clear(Some(state));
        let url = url.trim();
        if url.is_empty() {
            return;
        }
        let Some(controller) =
            aiot_controller_slot.and_then(|slot| slot.lock().ok().and_then(|guard| guard.clone()))
        else {
            eprintln!("call-snapshot: skip download without aiot controller");
            return;
        };

        let generation = self.generation.fetch_add(1, Ordering::AcqRel) + 1;
        let callid = callid.to_string();
        let path = std::env::temp_dir().join(format!(
            "kr_room_call_snapshot_{}.jpg",
            safe_snapshot_name(callid.as_str())
        ));
        if let Ok(mut guard) = self.path.lock() {
            *guard = Some(path.clone());
        }
        let controller_self = self.clone();
        let url = url.to_string();
        std::thread::spawn(move || {
            let output_path = path.to_string_lossy().into_owned();
            eprintln!("call-snapshot: download begin callid={callid} path={output_path}");
            let download_result =
                controller.download_file(url.as_str(), output_path.as_str(), 5000);
            let _ = slint::invoke_from_event_loop(move || {
                if controller_self.generation.load(Ordering::Acquire) != generation {
                    let _ = std::fs::remove_file(&path);
                    return;
                }
                let Some(app) = app_weak.upgrade() else {
                    let _ = std::fs::remove_file(&path);
                    return;
                };
                let state = app.global::<State>();
                if state.get_call_session_state().as_str() != "ringing" {
                    controller_self.clear(Some(&state));
                    return;
                }
                match download_result {
                    Ok(()) => match Image::load_from_path(path.as_path()) {
                        Ok(image) => {
                            state.set_call_snapshot_image(image);
                            state.set_call_snapshot_visible(true);
                            eprintln!(
                                "call-snapshot: show callid={} path={}",
                                callid,
                                path.display()
                            );
                        }
                        Err(err) => {
                            eprintln!(
                                "call-snapshot: load failed callid={} path={} err={err:?}",
                                callid,
                                path.display()
                            );
                            controller_self.clear(Some(&state));
                        }
                    },
                    Err(err) => {
                        eprintln!("call-snapshot: download failed callid={callid} err={err}");
                        controller_self.clear(Some(&state));
                    }
                }
            });
        });
    }

    fn clear(&self, state: Option<&State<'_>>) {
        self.generation.fetch_add(1, Ordering::AcqRel);
        if let Some(state) = state {
            state.set_call_snapshot_visible(false);
            state.set_call_snapshot_image(Image::default());
        }
        if let Ok(mut guard) = self.path.lock() {
            if let Some(path) = guard.take() {
                let _ = std::fs::remove_file(path);
            }
        }
    }
}

fn safe_snapshot_name(callid: &str) -> String {
    let mut name = String::new();
    for ch in callid.chars() {
        if ch.is_ascii_alphanumeric() {
            name.push(ch);
        } else if !name.ends_with('_') {
            name.push('_');
        }
    }
    let name = name.trim_matches('_');
    if name.is_empty() {
        "call".to_string()
    } else {
        name.to_string()
    }
}

fn clear_call_snapshot(controller: Option<&CallSnapshotController>, state: &State<'_>) {
    if let Some(controller) = controller {
        controller.clear(Some(state));
    } else {
        state.set_call_snapshot_visible(false);
        state.set_call_snapshot_image(Image::default());
    }
}

fn spawn_aiot_upgrade_download(
    event: &aiot::AiotEvent,
    aiot_controller_slot: Option<&Arc<Mutex<Option<aiot::AiotController>>>>,
) {
    let url = event.snapshot_download_url.trim();
    if url.is_empty() {
        eprintln!("aiot-upgrade: skip empty upgrade url");
        return;
    }
    let Some(controller) =
        aiot_controller_slot.and_then(|slot| slot.lock().ok().and_then(|guard| guard.clone()))
    else {
        eprintln!("aiot-upgrade: skip without aiot controller");
        return;
    };

    let output_path = std::path::PathBuf::from("/tmp/ota.img");
    let url = url.to_string();
    let md5 = event.text1.clone();
    let version = event.text2.clone();

    std::thread::spawn(move || {
        let output = output_path.to_string_lossy().into_owned();
        eprintln!(
            "aiot-upgrade: download begin version={} md5={} path={}",
            version,
            if md5.is_empty() { "-" } else { md5.as_str() },
            output
        );
        match controller.download_file(url.as_str(), output.as_str(), 120_000) {
            Ok(()) => {
                eprintln!(
                    "aiot-upgrade: download ok version={} path={}",
                    version, output
                );
            }
            Err(err) => {
                let _ = std::fs::remove_file(output_path.as_path());
                eprintln!(
                    "aiot-upgrade: download failed version={} path={} err={err}",
                    version, output
                );
            }
        }
    });
}

fn dispatch_app_event(context: AppEventDispatchContext, event: AppEvent) {
    let indoor_proxy_error = match &event {
        AppEvent::Indoor(indoor::IndoorEvent::SecondaryConfirm(secondary)) => {
            if let (Some(aiot_controller_slot), Some(secondary_confirm_state)) = (
                context.aiot_controller_slot.as_ref(),
                context.secondary_confirm_state.as_ref(),
            ) {
                runtime_coordinator::handle_secondary_confirm_event(
                    secondary,
                    aiot_controller_slot,
                    secondary_confirm_state,
                );
            } else {
                eprintln!(
                    "indoor-aiot-link secondary_confirm no proxy context online={} proxy_sn={} dev_model={} endpoint=0x{:08x} result={}",
                    secondary.online as i32,
                    secondary.device_id,
                    secondary.dev_model,
                    secondary.endpoint_addr,
                    secondary.result_code
                );
            }
            None
        }
        _ => None,
    };

    let app_weak = context.app_weak.clone();
    let _ = slint::invoke_from_event_loop(move || {
        let Some(app) = app_weak.upgrade() else {
            return;
        };

        let state = app.global::<State>();
        match event {
            AppEvent::Aiot(event) => handle_aiot_app_event(&app, &state, event, &context),
            AppEvent::Indoor(event) => {
                handle_indoor_app_event(&state, event, indoor_proxy_error, &context)
            }
            AppEvent::Webrtc(event) => handle_webrtc_app_event(&state, event, &context),
        }
    });
}

fn handle_aiot_app_event(
    app: &AppWindow,
    state: &State<'_>,
    event: aiot::AiotEvent,
    context: &AppEventDispatchContext,
) {
    match event.kind {
        aiot::AiotEventKind::ConnStatus => {
            eprintln!("aiot-ui: conn_status={}", event.conn_status);
            state.set_server_online(event.conn_status == 0);
            if event.conn_status != 0 {
                state.set_call_last_error(SharedString::from(format!(
                    "aiot_conn_status:{}",
                    event.conn_status
                )));
            }
        }
        aiot::AiotEventKind::OpenLock => {
            if let Some(controller) = context.runtime_coordinator.as_ref() {
                controller.handle_open_lock(state, context.d2d_controller.as_ref());
            }
        }
        aiot::AiotEventKind::SyncTime => {
            if let Err(err) = apply_aiot_sync_time(event.text2.as_str()) {
                state.set_call_last_error(SharedString::from(format!(
                    "aiot_sync_time_failed:{err}"
                )));
            } else {
                refresh_system_clock(app);
            }
        }
        aiot::AiotEventKind::WebrtcAccount => {
            if let Some(controller) = context.runtime_coordinator.as_ref() {
                controller.handle_webrtc_account(state, &event);
            }
        }
        aiot::AiotEventKind::Upgrade => {
            spawn_aiot_upgrade_download(&event, context.aiot_controller_slot.as_ref());
        }
        aiot::AiotEventKind::Callx => match event.callx_type {
            AIOT_CALLX_INITIATE_ACK => {
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    if let Err(err) = controller.handle_initiate_ack(&event) {
                        call::apply_call_error(state, format!("callx_initiate_ack_failed:{err}"));
                    }
                }
            }
            AIOT_CALLX_INVITE_ACK => {}
            AIOT_CALLX_INVITED => {
                let preempted_local_monitor = stop_local_monitor_for_callx_invited(state, context);
                if let Some(ipc_controller) = context.ipc_controller.as_ref() {
                    if runtime_coordinator::should_stop_ipc_for_callx_invited(
                        ipc_controller.is_monitoring(),
                    ) {
                        spawn_ipc_stop_monitor(
                            app.as_weak(),
                            ipc_controller.clone(),
                            "callx-invited",
                        );
                    }
                }
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    let ring_timeout = current_call_ring_timeout_sec(state);
                    let converse_timeout = current_call_converse_timeout_sec(state);
                    if let Err(err) =
                        controller.handle_invited(state, &event, ring_timeout, converse_timeout)
                    {
                        if preempted_local_monitor {
                            CALLX_PREEMPTING_LOCAL_MONITOR.store(false, Ordering::Release);
                        }
                        clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                        if err != "busy" {
                            call::apply_call_error(state, format!("callx_invited_failed:{err}"));
                        }
                    } else if preempted_local_monitor {
                        if let Some(snapshot_controller) = context.call_snapshot_controller.as_ref()
                        {
                            snapshot_controller.start_download(
                                app.as_weak(),
                                state,
                                context.aiot_controller_slot.as_ref(),
                                event.callid.as_str(),
                                event.snapshot_download_url.as_str(),
                            );
                        }
                        CALLX_PREEMPTING_LOCAL_MONITOR.store(false, Ordering::Release);
                        set_external_call_busy_for_cloud(context, true);
                        set_webrtc_preview_brightness_for_cloud(
                            context.runtime_coordinator.as_ref(),
                            state.get_door_brightness_value(),
                        );
                        set_webrtc_playback_volume_for_cloud(
                            context.runtime_coordinator.as_ref(),
                            state.get_door_volume_value(),
                        );
                        start_call_ringtone(state, context.sound_controller.as_ref(), None);
                        start_incoming_call_wait_timeout(app.as_weak(), state, "aiot-callx");
                    } else {
                        if let Some(snapshot_controller) = context.call_snapshot_controller.as_ref()
                        {
                            snapshot_controller.start_download(
                                app.as_weak(),
                                state,
                                context.aiot_controller_slot.as_ref(),
                                event.callid.as_str(),
                                event.snapshot_download_url.as_str(),
                            );
                        }
                        set_external_call_busy_for_cloud(context, true);
                        set_webrtc_preview_brightness_for_cloud(
                            context.runtime_coordinator.as_ref(),
                            state.get_door_brightness_value(),
                        );
                        set_webrtc_playback_volume_for_cloud(
                            context.runtime_coordinator.as_ref(),
                            state.get_door_volume_value(),
                        );
                        start_call_ringtone(state, context.sound_controller.as_ref(), None);
                        start_incoming_call_wait_timeout(app.as_weak(), state, "aiot-callx");
                    }
                } else if preempted_local_monitor {
                    CALLX_PREEMPTING_LOCAL_MONITOR.store(false, Ordering::Release);
                }
            }
            AIOT_CALLX_REPLIED => {
                let is_secondary_forward = context
                    .runtime_coordinator
                    .as_ref()
                    .map(|controller| controller.has_secondary_forward_call())
                    .unwrap_or(false);
                if event.value0 == 0 {
                    if is_secondary_forward {
                        if let Some(controller) = context.runtime_coordinator.as_ref() {
                            let failure = controller.note_secondary_forward_reply_failed(&event);
                            let should_hangup = failure
                                .map(|failure| {
                                    eprintln!(
                                        "aiot-ui: secondary forward callee unavailable callid={} callee_kind={} user_id={} web_user={} cause={} failed={}/{} all_failed={}",
                                        event.callid,
                                        event.callee_kind,
                                        event.callee_user_id,
                                        event.callee_web_username,
                                        event.text1,
                                        failure.failed_count,
                                        failure.invited_count,
                                        failure.all_failed
                                    );
                                    failure.all_failed
                                })
                                .unwrap_or(true);
                            if should_hangup {
                                let _ = controller.hangup("secondary_forward_unavailable");
                            }
                        }
                        state.set_call_last_error(SharedString::from(if event.text1.is_empty() {
                            "secondary_forward_callee_unavailable".to_string()
                        } else {
                            format!("secondary_forward_callee_unavailable:{}", event.text1)
                        }));
                    } else {
                        set_external_call_busy_for_cloud(context, false);
                        clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                        clear_call_bridge_prompt(state);
                        call::apply_call_error(
                            state,
                            if event.text1.is_empty() {
                                "callee_unavailable".to_string()
                            } else {
                                event.text1.clone()
                            },
                        );
                    }
                } else {
                    set_external_call_busy_for_cloud(context, true);
                    set_webrtc_preview_brightness_for_cloud(
                        context.runtime_coordinator.as_ref(),
                        state.get_door_brightness_value(),
                    );
                    set_webrtc_playback_volume_for_cloud(
                        context.runtime_coordinator.as_ref(),
                        state.get_door_volume_value(),
                    );
                    if !is_secondary_forward {
                        call::apply_cloud_ringing(state);
                    }
                }
            }
            AIOT_CALLX_ACCEPTED => {
                set_external_call_busy_for_cloud(context, true);
                cancel_incoming_call_wait_timeout();
                stop_call_ringtone(context.sound_controller.as_ref());
                clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    let is_secondary_forward = controller.has_secondary_forward_call();
                    if !is_secondary_forward {
                        call::apply_cloud_ringing(state);
                    }
                    if let Err(err) =
                        controller.handle_accepted(&event, context.call_controller.as_ref())
                    {
                        set_external_call_busy_for_cloud(context, false);
                        clear_call_bridge_prompt(state);
                        call::apply_call_error(state, format!("callx_accepted_failed:{err}"));
                    } else if is_secondary_forward {
                        call::apply_call_peer_id(state, "APP 接听中");
                        state.set_call_bridge_prompt_visible(true);
                        state.set_call_bridge_prompt_text(SharedString::from("APP 接听中"));
                    }
                } else {
                    call::apply_cloud_ringing(state);
                }
            }
            AIOT_CALLX_CONFIRMED => {
                set_external_call_busy_for_cloud(context, true);
                set_webrtc_preview_brightness_for_cloud(
                    context.runtime_coordinator.as_ref(),
                    state.get_door_brightness_value(),
                );
                set_webrtc_playback_volume_for_cloud(
                    context.runtime_coordinator.as_ref(),
                    state.get_door_volume_value(),
                );
                cancel_incoming_call_wait_timeout();
                stop_call_ringtone(context.sound_controller.as_ref());
                clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    if let Err(err) =
                        controller.handle_confirmed(state, &event, context.call_controller.as_ref())
                    {
                        set_external_call_busy_for_cloud(context, false);
                        clear_call_bridge_prompt(state);
                        call::apply_call_error(state, format!("callx_confirmed_failed:{err}"));
                    } else {
                        if !controller.has_secondary_forward_call() {
                            switch_guard_call_to_communication_if_needed(state);
                        }
                    }
                }
            }
            AIOT_CALLX_HUNGUP => {
                eprintln!("aiot-ui: callx_hungup callid={}", event.callid);
                let was_secondary_forward = context
                    .runtime_coordinator
                    .as_ref()
                    .map(|controller| controller.has_secondary_forward_call())
                    .unwrap_or(false);
                cancel_incoming_call_wait_timeout();
                stop_call_ringtone(context.sound_controller.as_ref());
                clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                clear_call_bridge_prompt(state);
                return_call_page_to_idle_if_needed(state, "aiot-callx-hungup");
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    controller.handle_hungup(state);
                }
                if was_secondary_forward {
                    cleanup_secondary_forward_local_call(context);
                } else if let Some(local_controller) = context.call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                }
                set_external_call_busy_for_cloud(context, false);
            }
            _ => {}
        },
        aiot::AiotEventKind::Error => {
            state.set_server_online(false);
            let was_secondary_forward = context
                .runtime_coordinator
                .as_ref()
                .map(|controller| controller.has_secondary_forward_call())
                .unwrap_or(false);
            set_external_call_busy_for_cloud(context, false);
            cancel_incoming_call_wait_timeout();
            stop_call_ringtone(context.sound_controller.as_ref());
            clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
            clear_call_bridge_prompt(state);
            if was_secondary_forward {
                if let Some(controller) = context.runtime_coordinator.as_ref() {
                    let _ = controller.hangup("aiot_error");
                }
                cleanup_secondary_forward_local_call(context);
                return_call_page_to_idle_if_needed(state, "aiot-error-secondary-forward");
            }
            call::apply_call_error(
                state,
                format!("aiot_error:{}:{}", event.text1, event.value0),
            );
        }
        _ => {}
    }
}

fn handle_webrtc_app_event(
    state: &State<'_>,
    event: webrtc::WebrtcEvent,
    context: &AppEventDispatchContext,
) {
    match event.kind {
        webrtc::WebrtcEventKind::Online => {
            state.set_call_last_error(SharedString::from(""));
        }
        webrtc::WebrtcEventKind::Offline => {
            state.set_call_last_error(SharedString::from("webrtc_offline"));
        }
        webrtc::WebrtcEventKind::AskIframe => {
            let is_secondary_forward = context
                .runtime_coordinator
                .as_ref()
                .map(|controller| controller.has_secondary_forward_call())
                .unwrap_or(false);
            if is_secondary_forward {
                if let Some(local_controller) = context.call_controller.as_ref() {
                    if let Err(err) = local_controller.request_video_recovery(true) {
                        eprintln!("webrtc-ui: request d2d video recovery failed: {err}");
                    }
                }
            }
        }
        webrtc::WebrtcEventKind::ConnectFailed
        | webrtc::WebrtcEventKind::CallFailed
        | webrtc::WebrtcEventKind::Error => {
            if context_has_active_cloud_call(context) {
                let was_secondary_forward = context
                    .runtime_coordinator
                    .as_ref()
                    .map(|controller| controller.has_secondary_forward_call())
                    .unwrap_or(false);
                set_external_call_busy_for_cloud(context, false);
                stop_call_ringtone(context.sound_controller.as_ref());
                clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                clear_call_bridge_prompt(state);
                if was_secondary_forward {
                    if let Some(controller) = context.runtime_coordinator.as_ref() {
                        let _ = controller.hangup("webrtc_error");
                    }
                    cleanup_secondary_forward_local_call(context);
                    return_call_page_to_idle_if_needed(state, "webrtc-error-secondary-forward");
                    return;
                }
                state.set_call_last_error(SharedString::from(format!(
                    "webrtc_event:{:?}:{}:{}",
                    event.kind, event.text1, event.value0
                )));
            } else {
                eprintln!(
                    "webrtc-ui: ignore event without active cloud call kind={:?} session={}",
                    event.kind, event.text1
                );
            }
        }
        webrtc::WebrtcEventKind::CallDisconnect | webrtc::WebrtcEventKind::CallDestroy => {
            let (cleared_cloud_call, was_secondary_forward) =
                runtime_coordinator_from_context(context)
                    .as_ref()
                    .map(|controller| {
                        let was_secondary_forward = controller.has_secondary_forward_call();
                        let cleared =
                            controller.clear_parent_call_after_webrtc_end(event.text1.as_str());
                        (cleared, was_secondary_forward)
                    })
                    .unwrap_or((false, false));
            if cleared_cloud_call {
                stop_call_ringtone(context.sound_controller.as_ref());
                clear_call_snapshot(context.call_snapshot_controller.as_ref(), state);
                clear_call_bridge_prompt(state);
                if was_secondary_forward {
                    cleanup_secondary_forward_local_call(context);
                } else if let Some(local_controller) = context.call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                }
                set_external_call_busy_for_cloud(context, false);
                return_call_page_to_idle_if_needed(state, "webrtc-call-ended");
            } else {
                eprintln!(
                    "webrtc-ui: ignore call end without matched cloud call kind={:?} session={}",
                    event.kind, event.text1
                );
            }
        }
        _ => {}
    }
}

fn clear_call_bridge_prompt(state: &State<'_>) {
    state.set_call_bridge_prompt_visible(false);
    state.set_call_bridge_prompt_text(SharedString::from(""));
}

fn cleanup_secondary_forward_local_call(context: &AppEventDispatchContext) {
    if let Some(local_controller) = context.call_controller.as_ref() {
        let _ = local_controller.set_media_bridge(false);
        let _ = local_controller.set_webrtc_bridge(None);
        let _ = local_controller.hangup();
    }
}

fn return_call_page_to_idle_if_needed(state: &State<'_>, source: &str) {
    let current_page = state.get_current_page();
    if matches!(current_page.as_str(), "MwoLv" | "Bu0fW" | "EJDbm" | "YanGa") {
        eprintln!(
            "call-ui: return_idle source={} page={}",
            source,
            current_page.as_str()
        );
        call::apply_call_idle(state);
    }
}

fn switch_guard_call_to_communication_if_needed(state: &State<'_>) {
    if state.get_current_page().as_str() == "EJDbm" {
        state.set_guard_call_effect_active(false);
        state.set_current_page(SharedString::from("YanGa"));
    }
}

fn call_ringtone_kind_for_page(page: &str) -> &'static str {
    if page == "EJDbm" {
        "guard"
    } else {
        "door"
    }
}

fn start_call_ringtone(
    state: &State<'_>,
    sound_controller: Option<&sound::SoundController>,
    page: Option<&str>,
) {
    let Some(controller) = sound_controller else {
        return;
    };
    let page = page
        .map(ToOwned::to_owned)
        .unwrap_or_else(|| state.get_current_page().to_string());
    let kind = call_ringtone_kind_for_page(page.as_str());
    let melody = if kind == "guard" {
        state.get_guard_call_melody()
    } else {
        state.get_door_call_melody()
    };
    eprintln!("sound-ui: start call ringtone kind={kind} melody={melody} page={page}");
    if let Err(err) = controller.start_ring(kind, melody, state.get_sound_call_volume_value()) {
        eprintln!("sound: start ring failed kind={kind} melody={melody}: {err}");
    }
}

fn start_outgoing_call_ringtone(
    state: &State<'_>,
    sound_controller: Option<&sound::SoundController>,
    kind: &str,
) {
    let Some(controller) = sound_controller else {
        return;
    };
    let melody = if kind == "guard" {
        state.get_guard_call_melody()
    } else {
        state.get_door_call_melody()
    };
    eprintln!("sound-ui: start outgoing ringtone kind={kind} melody={melody}");
    if let Err(err) = controller.start_ring(kind, melody, state.get_sound_call_volume_value()) {
        eprintln!("sound: start outgoing ring failed kind={kind} melody={melody}: {err}");
    }
}

fn current_call_ring_timeout_sec(state: &State<'_>) -> i32 {
    if debug_call_timeout_disabled() {
        return 0;
    }
    normalize_call_ring_timeout_sec(state.get_call_timeout_seconds())
}

fn current_call_converse_timeout_sec(state: &State<'_>) -> i32 {
    if debug_call_timeout_disabled() {
        return 0;
    }
    state.get_communication_minutes().max(1) * 60
}

fn cancel_incoming_call_wait_timeout() {
    CALL_RING_TIMEOUT_GENERATION.fetch_add(1, Ordering::AcqRel);
}

fn start_incoming_call_wait_timeout(
    app_weak: slint::Weak<AppWindow>,
    state: &State<'_>,
    source: &'static str,
) {
    let timeout_sec = current_call_ring_timeout_sec(state);
    let generation = CALL_RING_TIMEOUT_GENERATION.fetch_add(1, Ordering::AcqRel) + 1;
    if timeout_sec <= 0 || debug_call_timeout_disabled() {
        eprintln!(
            "call-ui: incoming no-answer timeout disabled by /data/debugflag source={source}"
        );
        return;
    }
    std::thread::spawn(move || {
        std::thread::sleep(Duration::from_secs(timeout_sec as u64));
        let _ = slint::invoke_from_event_loop(move || {
            if CALL_RING_TIMEOUT_GENERATION.load(Ordering::Acquire) != generation {
                return;
            }
            let Some(app) = app_weak.upgrade() else {
                return;
            };
            let state = app.global::<State>();
            if state.get_call_session_state().as_str() != "ringing" {
                return;
            }
            eprintln!("call-ui: incoming no-answer timeout source={source} sec={timeout_sec}");
            state.invoke_call_reject();
        });
    });
}

fn stop_call_ringtone(sound_controller: Option<&sound::SoundController>) {
    if let Some(controller) = sound_controller {
        eprintln!("sound-ui: stop call ringtone");
        if let Err(err) = controller.stop_ring() {
            eprintln!("sound: stop ring failed: {err}");
        }
    }
}

fn stop_local_monitor_for_callx_invited(
    state: &State<'_>,
    context: &AppEventDispatchContext,
) -> bool {
    if state.get_call_session_state().as_str() != "monitor" {
        return false;
    }

    CALLX_PREEMPTING_LOCAL_MONITOR.store(true, Ordering::Release);
    call::interrupt_monitor_for_incoming(state);
    if let Some(controller) = context.call_controller.as_ref() {
        if let Err(err) = controller.stop_monitor() {
            state.set_call_last_error(SharedString::from(format!(
                "callx_stop_monitor_failed:{err}"
            )));
        }
    }
    true
}

fn should_ignore_indoor_call_event_for_cloud_call(
    event: &call::CallEvent,
    context: &AppEventDispatchContext,
) -> bool {
    let is_local_idle_event = matches!(
        event.kind,
        call::CallEventKind::StateChanged
            | call::CallEventKind::MediaChanged
            | call::CallEventKind::RemoteHangup
    ) && event.session_state == call::CallSessionState::Idle;
    if !is_local_idle_event {
        return false;
    }

    if CALLX_PREEMPTING_LOCAL_MONITOR.load(Ordering::Acquire) {
        eprintln!(
            "call-ui: ignore local idle while callx preempts monitor kind={:?}",
            event.kind
        );
        return true;
    }

    if !context_has_active_cloud_call(context) {
        return false;
    }

    true
}

fn runtime_coordinator_from_context(
    context: &AppEventDispatchContext,
) -> Option<RuntimeCoordinator> {
    if let Some(controller) = context.runtime_coordinator.as_ref() {
        return Some(controller.clone());
    }

    context
        .runtime_coordinator_slot
        .as_ref()
        .and_then(|slot| slot.lock().ok().and_then(|coordinator| coordinator.clone()))
}

fn context_has_active_cloud_call(context: &AppEventDispatchContext) -> bool {
    runtime_coordinator_from_context(context)
        .map(|controller| controller.has_active_call())
        .unwrap_or(false)
}

fn set_local_call_busy_for_platforms(context: &AppEventDispatchContext, busy: bool) {
    let runtime_coordinator = runtime_coordinator_from_context(context);
    let has_active_cloud_call = runtime_coordinator
        .as_ref()
        .map(|controller| controller.has_active_call())
        .unwrap_or(false);
    let has_secondary_forward_call = runtime_coordinator
        .as_ref()
        .map(|controller| controller.has_secondary_forward_call())
        .unwrap_or(false);

    if has_secondary_forward_call {
        eprintln!(
            "call-ui: keep aiot busy and allow webrtc incoming for secondary forward busy={busy}"
        );
        set_aiot_call_busy_for_local(context.aiot_controller_slot.as_ref(), true);
        set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
        return;
    }

    if !busy && has_active_cloud_call {
        eprintln!("call-ui: keep platform busy because cloud call is active");
        set_aiot_call_busy_for_local(context.aiot_controller_slot.as_ref(), true);
        set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), true);
        return;
    }
    set_aiot_call_busy_for_local(context.aiot_controller_slot.as_ref(), busy);
    set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), busy);
}

fn set_external_call_busy_for_cloud(context: &AppEventDispatchContext, busy: bool) {
    if let Some(controller) = context.call_controller.as_ref() {
        if let Err(err) = controller.set_external_call_busy(busy) {
            eprintln!("call-ui: set external call busy failed busy={busy}: {err}");
        }
    }
    set_aiot_call_busy_for_local(context.aiot_controller_slot.as_ref(), busy);
}

fn set_webrtc_call_busy_for_local(controller: Option<&RuntimeCoordinator>, busy: bool) {
    if let Some(controller) = controller {
        if let Err(err) = controller.set_webrtc_call_busy(busy) {
            eprintln!("webrtc-ui: set local call busy failed busy={busy}: {err}");
        }
    }
}

fn set_webrtc_preview_brightness_for_cloud(controller: Option<&RuntimeCoordinator>, value: f32) {
    if let Some(controller) = controller {
        if let Err(err) = controller.set_webrtc_preview_brightness(value) {
            eprintln!("webrtc-ui: set preview brightness failed value={value}: {err}");
        }
    }
}

fn set_webrtc_playback_volume_for_cloud(controller: Option<&RuntimeCoordinator>, value: f32) {
    if let Some(controller) = controller {
        if let Err(err) = controller.set_webrtc_playback_volume(value) {
            eprintln!("webrtc-ui: set playback volume failed value={value}: {err}");
        }
    }
}

fn set_aiot_call_busy_for_local(
    controller_slot: Option<&Arc<Mutex<Option<aiot::AiotController>>>>,
    busy: bool,
) {
    let Some(controller_slot) = controller_slot else {
        return;
    };
    let controller = controller_slot
        .lock()
        .ok()
        .and_then(|controller| controller.clone());
    if let Some(controller) = controller {
        if let Err(err) = controller.set_call_busy(busy) {
            eprintln!("aiot-ui: set local call busy failed busy={busy}: {err}");
        }
    }
}

fn sync_aiot_call_timeouts_for_state(
    controller_slot: Option<&Arc<Mutex<Option<aiot::AiotController>>>>,
    state: &State<'_>,
) {
    let Some(controller_slot) = controller_slot else {
        return;
    };
    let controller = controller_slot
        .lock()
        .ok()
        .and_then(|controller| controller.clone());
    if let Some(controller) = controller {
        let ring_timeout = current_call_ring_timeout_sec(state);
        let converse_timeout = current_call_converse_timeout_sec(state);
        if let Err(err) = controller.set_call_timeouts(ring_timeout, converse_timeout) {
            eprintln!(
                "aiot-ui: set call timeouts failed ring={ring_timeout} converse={converse_timeout}: {err}"
            );
        }
    }
}

fn local_call_event_busy(event: &call::CallEvent) -> bool {
    match event.kind {
        call::CallEventKind::Incoming
        | call::CallEventKind::StateChanged
        | call::CallEventKind::MediaChanged => matches!(
            event.session_state,
            call::CallSessionState::Ringing
                | call::CallSessionState::VideoOnly
                | call::CallSessionState::Active
        ),
        call::CallEventKind::RemoteHangup | call::CallEventKind::Error => false,
    }
}

fn handle_indoor_app_event(
    state: &State<'_>,
    event: indoor::IndoorEvent,
    proxy_error: Option<String>,
    context: &AppEventDispatchContext,
) {
    if let Some(err) = proxy_error {
        state.set_call_last_error(SharedString::from(err));
    }
    match event {
        indoor::IndoorEvent::Call(event) => {
            if should_ignore_indoor_call_event_for_cloud_call(&event, context) {
                return;
            }
            set_local_call_busy_for_platforms(context, local_call_event_busy(&event));
            let incoming_page = if event.kind == call::CallEventKind::Incoming
                && event.peer_device_id == runtime_coordinator::SECONDARY_CONFIRM_DEVICE_ID
            {
                Some("MwoLv")
            } else {
                None
            };
            if runtime_coordinator::SECONDARY_FORWARD_TO_APP_ENABLED
                && incoming_page == Some("MwoLv")
            {
                if let Some(controller) = runtime_coordinator_from_context(context) {
                    if !controller.has_secondary_forward_call() {
                        let room_id = state.get_site_room_number().to_string();
                        match controller.start_secondary_forward_call(room_id.as_str()) {
                            Ok(room) => {
                                eprintln!("aiot-ui: secondary forward resident call room={room}");
                                set_external_call_busy_for_cloud(context, true);
                            }
                            Err(err) => {
                                eprintln!(
                                    "aiot-ui: secondary forward resident call skipped room={} err={err}",
                                    room_id
                                );
                                if err != "busy" {
                                    state.set_call_last_error(SharedString::from(format!(
                                        "secondary_forward_failed:{err}"
                                    )));
                                }
                            }
                        }
                    }
                }
            }
            let should_start_ring = event.kind == call::CallEventKind::Incoming;
            let should_stop_ring = matches!(
                event.kind,
                call::CallEventKind::StateChanged
                    | call::CallEventKind::MediaChanged
                    | call::CallEventKind::RemoteHangup
                    | call::CallEventKind::Error
            ) && event.session_state != call::CallSessionState::Ringing;
            if should_stop_ring {
                stop_call_ringtone(context.sound_controller.as_ref());
                cancel_incoming_call_wait_timeout();
            }
            if matches!(
                event.kind,
                call::CallEventKind::RemoteHangup | call::CallEventKind::Error
            ) {
                if let Some(controller) = runtime_coordinator_from_context(context) {
                    if controller.has_secondary_forward_call() {
                        let _ = controller.hangup("local_ended");
                    }
                }
                if let Some(local_controller) = context.call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                }
                set_external_call_busy_for_cloud(context, false);
            }
            call::apply_event_to_state_with_incoming_page(state, &event, incoming_page);
            if runtime_coordinator_from_context(context)
                .as_ref()
                .map(|controller| controller.has_confirmed_secondary_forward_call())
                .unwrap_or(false)
                && matches!(
                    event.kind,
                    call::CallEventKind::StateChanged | call::CallEventKind::MediaChanged
                )
                && event.session_state != call::CallSessionState::Ringing
            {
                call::apply_call_peer_id(state, "APP 接听中");
                state.set_call_bridge_prompt_visible(true);
                state.set_call_bridge_prompt_text(SharedString::from("APP 接听中"));
            }
            if should_start_ring {
                start_call_ringtone(state, context.sound_controller.as_ref(), incoming_page);
                start_incoming_call_wait_timeout(context.app_weak.clone(), state, "indoor-call");
            }
        }
        indoor::IndoorEvent::D2d(event) => apply_d2d_event_to_state(state, &event),
        indoor::IndoorEvent::SecondaryConfirm(_) => {}
    }
}

fn configure_aiot_bindings(
    app: &AppWindow,
    aiot_config: aiot::AiotConfig,
    d2d_controller: Option<d2d::D2dController>,
    webrtc_runtime: Option<webrtc::WebrtcRuntime>,
    ipc_controller: Option<ipc::IpcController>,
    call_controller: Option<call::CallController>,
    sound_controller: Option<sound::SoundController>,
    call_snapshot_controller: CallSnapshotController,
    controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
    secondary_confirm_state: Arc<Mutex<aiot::SecondaryConfirmProxyState>>,
) -> (Option<aiot::AiotRuntime>, Option<RuntimeCoordinator>) {
    eprintln!(
        "aiot-ui: configure enabled={} interface={} model_present={} server={} mqtt_present={} keepalive={} mqtts={}",
        aiot_config.enabled,
        aiot_config.interface,
        !aiot_config.model.trim().is_empty(),
        aiot_config.server_addr,
        !aiot_config.mqtt_server_addr.trim().is_empty(),
        aiot_config.keepalive,
        aiot_config.is_mqtts
    );
    if !aiot_config.enabled {
        eprintln!("aiot-ui: disabled by config");
        return (None, None);
    }

    let runtime_coordinator = Some(RuntimeCoordinator::new(
        controller_slot.clone(),
        webrtc_runtime.clone(),
    ));
    let dispatch_context = AppEventDispatchContext {
        app_weak: app.as_weak(),
        d2d_controller,
        runtime_coordinator: runtime_coordinator.clone(),
        ipc_controller,
        call_controller,
        sound_controller,
        call_snapshot_controller: Some(call_snapshot_controller),
        aiot_controller_slot: Some(controller_slot.clone()),
        runtime_coordinator_slot: None,
        secondary_confirm_state: None,
    };
    let dispatcher: Arc<dyn Fn(aiot::AiotEvent) + Send + Sync + 'static> = Arc::new(move |event| {
        dispatch_app_event(dispatch_context.clone(), AppEvent::Aiot(event));
    });

    let runtime = aiot::AiotRuntime::new(
        aiot_config,
        dispatcher,
        controller_slot,
        secondary_confirm_state,
    );
    (Some(runtime), runtime_coordinator)
}

fn configure_webrtc_bindings(
    app: &AppWindow,
    webrtc_config: webrtc::WebrtcConfig,
    d2d_controller: Option<d2d::D2dController>,
    call_controller: Option<call::CallController>,
    aiot_controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
    runtime_coordinator_slot: Arc<Mutex<Option<RuntimeCoordinator>>>,
    sound_controller: Option<sound::SoundController>,
    call_snapshot_controller: CallSnapshotController,
) -> Option<webrtc::WebrtcRuntime> {
    if !webrtc_config.enabled {
        eprintln!("webrtc-ui: disabled by config");
        return None;
    }
    eprintln!(
        "webrtc-ui: configure enabled=1 server_len={} init_len={} serno_len={} interface={}",
        webrtc_config.server_addr.len(),
        webrtc_config.initstring.len(),
        webrtc_config.serno.len(),
        webrtc_config.interface
    );

    let dispatch_context = AppEventDispatchContext {
        app_weak: app.as_weak(),
        d2d_controller,
        runtime_coordinator: None,
        ipc_controller: None,
        call_controller,
        sound_controller,
        call_snapshot_controller: Some(call_snapshot_controller),
        aiot_controller_slot: Some(aiot_controller_slot),
        runtime_coordinator_slot: Some(runtime_coordinator_slot),
        secondary_confirm_state: None,
    };
    let dispatcher: Arc<dyn Fn(webrtc::WebrtcEvent) + Send + Sync + 'static> =
        Arc::new(move |event| {
            dispatch_app_event(dispatch_context.clone(), AppEvent::Webrtc(event));
        });

    Some(webrtc::WebrtcRuntime::new(webrtc_config, dispatcher))
}

fn configure_ipc_bindings(
    app: &AppWindow,
    ipc_config: ipc::IpcConfig,
    camera_source: Rc<dyn Fn() -> Vec<ipc::IpcCameraConfig>>,
) -> (Option<ipc::IpcService>, Option<ipc::IpcController>) {
    let state = app.global::<State>();
    let cameras = camera_source();
    let enabled_camera_count = ipc_config
        .cameras
        .iter()
        .filter(|camera| camera.enabled)
        .count();
    let addressed_camera_count = ipc_config
        .cameras
        .iter()
        .filter(|camera| {
            !camera.ip.trim().is_empty()
                || !camera.rtsp_url.trim().is_empty()
                || !camera.sub_rtsp_url.trim().is_empty()
        })
        .count();
    eprintln!(
        "ipc-ui: configure enabled={} interface={} preview=({},{} {}x{}) total_cameras={} usable_cameras={}",
        ipc_config.enabled,
        ipc_config.interface,
        ipc_config.preview_x,
        ipc_config.preview_y,
        ipc_config.preview_width,
        ipc_config.preview_height,
        ipc_config.cameras.len(),
        cameras.len()
    );
    for (index, camera) in ipc_config.cameras.iter().enumerate() {
        eprintln!(
            "ipc-ui: camera{} enabled={} usable={} name={} ip={} user={} rtsp_url_present={} sub_rtsp_url_present={}",
            index + 1,
            camera.enabled,
            camera.is_usable(),
            camera.name,
            camera.ip,
            camera.user,
            !camera.rtsp_url.trim().is_empty(),
            !camera.sub_rtsp_url.trim().is_empty()
        );
    }

    apply_cctv_monitor_camera_options_to_state(&state, &cameras);
    state.set_cctv_selected_camera(if cameras.is_empty() { 0 } else { 1 });
    state.set_cctv_current_camera_label(SharedString::from(
        cameras
            .first()
            .map(|camera| camera.name.clone())
            .unwrap_or_default(),
    ));
    state.set_cctv_monitor_online(false);
    state.set_cctv_last_error(SharedString::from(""));
    state.set_cctv_last_capture_path(SharedString::from(""));

    if !ipc_config.enabled || cameras.is_empty() {
        let reason = if !ipc_config.enabled {
            "ipc_disabled".to_string()
        } else {
            format!(
                "ipc_no_usable_cameras(total={},enabled={},addressed={})",
                ipc_config.cameras.len(),
                enabled_camera_count,
                addressed_camera_count
            )
        };
        eprintln!("ipc-ui: bindings inactive for monitor page reason={reason}; keeping ipc service available for cctv setting test");
    }

    let app_weak = app.as_weak();
    let cctv_test_generation = Arc::new(Mutex::new(0_u64));
    let cctv_test_controller = Arc::new(Mutex::new(None::<ipc::IpcController>));
    let cctv_test_controller_for_dispatch = cctv_test_controller.clone();
    let dispatcher: Arc<dyn Fn(ipc::IpcEvent) + Send + Sync + 'static> = Arc::new(move |event| {
        eprintln!(
            "ipc-ui: dispatch event kind={:?} value0={} value1={} text1={} text2={} text3={}",
            event.kind, event.value0, event.value1, event.text1, event.text2, event.text3
        );
        let app_weak = app_weak.clone();
        let cctv_test_controller = cctv_test_controller_for_dispatch.clone();
        let _ = slint::invoke_from_event_loop(move || {
            let Some(app) = app_weak.upgrade() else {
                return;
            };

            let state = app.global::<State>();
            let test_event_kind = event.kind;
            let test_event_value0 = event.value0;
            let test_event_text1 = event.text1.clone();
            let test_event_text2 = event.text2.clone();
            match event.kind {
                ipc::IpcEventKind::MonitorStarted => {
                    if !event.text1.is_empty() {
                        state.set_cctv_current_camera_label(SharedString::from(event.text1));
                    }
                    state.set_cctv_monitor_online(false);
                    state.set_cctv_last_error(SharedString::from(""));
                }
                ipc::IpcEventKind::Info => {
                    if !event.text1.is_empty() {
                        state.set_cctv_current_camera_label(SharedString::from(event.text1));
                    }
                }
                ipc::IpcEventKind::StreamReady => {
                    state.set_cctv_monitor_online(true);
                    state.set_cctv_last_error(SharedString::from(""));
                }
                ipc::IpcEventKind::MonitorStopped => {
                    state.set_cctv_monitor_online(false);
                }
                ipc::IpcEventKind::Offline => {
                    state.set_cctv_monitor_online(false);
                    state.set_cctv_last_error(SharedString::from(if event.text1.is_empty() {
                        "ipc_offline".to_string()
                    } else {
                        event.text1
                    }));
                }
                ipc::IpcEventKind::CaptureSaved => {
                    state.set_cctv_last_capture_path(SharedString::from(event.text3));
                    state.set_cctv_last_error(SharedString::from(""));
                }
                ipc::IpcEventKind::Error => {
                    state.set_cctv_monitor_online(false);
                    state.set_cctv_last_error(SharedString::from(if event.text1.is_empty() {
                        format!("ipc_error:{}", event.value0)
                    } else {
                        event.text1
                    }));
                }
                ipc::IpcEventKind::None => {}
            }

            if state.get_cctv_setting_test_running() {
                match test_event_kind {
                    ipc::IpcEventKind::MonitorStarted => {
                        if state.get_cctv_setting_test_step() < 3 {
                            state.set_cctv_setting_test_step(3);
                        }
                    }
                    ipc::IpcEventKind::Info => {
                        if state.get_cctv_setting_test_step() < 4 {
                            state.set_cctv_setting_test_step(4);
                        }
                        if !test_event_text2.is_empty() {
                            state.set_cctv_setting_test_model(SharedString::from(test_event_text2));
                        }
                    }
                    ipc::IpcEventKind::StreamReady => {
                        state.set_cctv_setting_test_running(false);
                        state.set_cctv_setting_test_step(6);
                        state.set_cctv_setting_test_error(SharedString::from(""));
                        state.set_cctv_setting_tested(true);
                        state.set_active_overlay(SharedString::from("hMGBM"));
                        let controller = cctv_test_controller
                            .lock()
                            .ok()
                            .and_then(|guard| guard.clone());
                        if let Some(controller) = controller {
                            spawn_ipc_stop_monitor(
                                app_weak.clone(),
                                controller,
                                "cctv-setting-test-success",
                            );
                        }
                    }
                    ipc::IpcEventKind::Offline | ipc::IpcEventKind::Error => {
                        let error = if test_event_text1.is_empty() {
                            format!("ipc_error:{}", test_event_value0)
                        } else {
                            test_event_text1
                        };
                        state.set_cctv_setting_test_running(false);
                        state.set_cctv_setting_tested(false);
                        state.set_cctv_setting_test_error(SharedString::from(error));
                        state.set_active_overlay(SharedString::from("E0zNY"));
                    }
                    _ => {}
                }
            }
        });
    });

    let service = match ipc::IpcService::start(&ipc_config, dispatcher) {
        Ok(service) => service,
        Err(err) => {
            eprintln!("ipc-ui: service start failed err={err}");
            state.set_cctv_last_error(SharedString::from(format!("ipc_init_failed:{err}")));
            bind_ipc_fallback_callbacks(app, format!("ipc_init_failed:{err}").as_str());
            return (None, None);
        }
    };
    let controller = service.controller();
    if let Ok(mut guard) = cctv_test_controller.lock() {
        *guard = Some(controller.clone());
    }

    state.on_test_cctv_setting({
        let app_weak = app.as_weak();
        let controller = controller.clone();
        let cctv_test_generation = cctv_test_generation.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let ip = state.get_cctv_setting_ip().to_string();
                let account = state.get_cctv_setting_account().to_string();
                let password = state.get_cctv_setting_password().to_string();
                if ip.trim().is_empty() || account.trim().is_empty() || password.trim().is_empty() {
                    state.set_cctv_setting_test_running(false);
                    state.set_cctv_setting_test_step(1);
                    state.set_cctv_setting_tested(false);
                    state.set_cctv_setting_test_error(SharedString::from(
                        "IPC 설정 값을 확인하세요",
                    ));
                    state.set_active_overlay(SharedString::from("E0zNY"));
                    return;
                }

                let generation = {
                    let mut guard = cctv_test_generation
                        .lock()
                        .unwrap_or_else(|err| err.into_inner());
                    *guard = guard.wrapping_add(1);
                    *guard
                };
                let camera = ipc::IpcCameraConfig {
                    name: "CCTV Test".to_string(),
                    ip,
                    user: account,
                    password,
                    rtsp_url: String::new(),
                    sub_rtsp_url: String::new(),
                    enabled: true,
                    prefer_substream: false,
                };
                state.set_cctv_setting_test_running(true);
                state.set_cctv_setting_test_step(1);
                state.set_cctv_setting_test_error(SharedString::from(""));
                state.set_cctv_setting_tested(false);
                state.set_cctv_setting_saved(false);
                state.set_active_overlay(SharedString::from("E0zNY"));
                spawn_cctv_setting_test(
                    app_weak.clone(),
                    controller.clone(),
                    camera,
                    state.get_cctv_brightness_value(),
                    cctv_test_generation.clone(),
                    generation,
                );
            }
        }
    });

    state.on_cctv_open({
        let app_weak = app.as_weak();
        let controller = controller.clone();
        let camera_source = camera_source.clone();
        move || {
            eprintln!("ipc-ui: cctv-open callback triggered");
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let cameras = camera_source();
                apply_cctv_monitor_camera_options_to_state(&state, &cameras);
                let selected = state.get_cctv_selected_camera();
                eprintln!(
                    "ipc-ui: cctv-open current_page={} selected_camera={} option_count={}",
                    state.get_current_page(),
                    selected,
                    cameras.len()
                );
                if let Some(camera) = camera_for_display_index(&cameras, selected) {
                    eprintln!(
                        "ipc-ui: cctv-open starting name={} ip={} user={}",
                        camera.name, camera.ip, camera.user
                    );
                    state.set_cctv_last_error(SharedString::from(""));
                    state.set_cctv_current_camera_label(SharedString::from(camera.name.clone()));
                    spawn_ipc_start_monitor(
                        app_weak.clone(),
                        controller.clone(),
                        camera,
                        state.get_cctv_brightness_value(),
                        "cctv-open",
                    );
                } else {
                    eprintln!("ipc-ui: cctv-open no camera for selected index={selected}");
                }
            }
        }
    });

    state.on_cctv_select_camera({
        let app_weak = app.as_weak();
        let controller = controller.clone();
        let camera_source = camera_source.clone();
        move |selected| {
            eprintln!("ipc-ui: cctv-select-camera selected={selected}");
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let cameras = camera_source();
                apply_cctv_monitor_camera_options_to_state(&state, &cameras);
                state.set_cctv_selected_camera(selected);
                if let Some(camera) = camera_for_display_index(&cameras, selected) {
                    eprintln!(
                        "ipc-ui: cctv-select-camera resolved name={} ip={} current_page={}",
                        camera.name,
                        camera.ip,
                        state.get_current_page()
                    );
                    state.set_cctv_current_camera_label(SharedString::from(camera.name.clone()));
                    if state.get_current_page().as_str() == "JVRSN" {
                        state.set_cctv_last_error(SharedString::from(""));
                        spawn_ipc_start_monitor(
                            app_weak.clone(),
                            controller.clone(),
                            camera,
                            state.get_cctv_brightness_value(),
                            "cctv-select-camera",
                        );
                    }
                } else {
                    eprintln!("ipc-ui: cctv-select-camera no camera for selected index={selected}");
                }
            }
        }
    });

    state.on_cctv_stop({
        let app_weak = app.as_weak();
        let controller = controller.clone();
        move || {
            eprintln!("ipc-ui: cctv-stop callback triggered");
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>().set_cctv_monitor_online(false);
            }
            spawn_ipc_stop_monitor(app_weak.clone(), controller.clone(), "cctv-stop");
        }
    });

    state.on_cctv_capture({
        let app_weak = app.as_weak();
        let controller = controller.clone();
        let camera_source = camera_source.clone();
        let capture_dir = ipc_config.capture_dir.clone();
        move || {
            eprintln!("ipc-ui: cctv-capture callback triggered");
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                let cameras = camera_source();
                let selected = state.get_cctv_selected_camera();
                if let Some(camera) = camera_for_display_index(&cameras, selected) {
                    let path = ipc_capture_output_path(capture_dir.as_str(), camera.name.as_str());
                    eprintln!("ipc-ui: cctv-capture path={path}");
                    if let Err(err) = controller.capture_jpeg(path.as_str()) {
                        eprintln!("ipc-ui: cctv-capture failed err={err}");
                        state.set_cctv_last_error(SharedString::from(err));
                    }
                } else {
                    eprintln!("ipc-ui: cctv-capture no camera for selected index={selected}");
                }
            }
        }
    });

    (Some(service), Some(controller))
}

fn bind_ipc_fallback_callbacks(app: &AppWindow, reason: &str) {
    let state = app.global::<State>();
    let reason = reason.to_string();
    state.on_cctv_open({
        let app_weak = app.as_weak();
        let reason = reason.clone();
        move || {
            eprintln!("ipc-ui: cctv-open fallback reason={reason}");
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>()
                    .set_cctv_last_error(SharedString::from(reason.clone()));
            }
        }
    });
    state.on_cctv_select_camera({
        let app_weak = app.as_weak();
        let reason = reason.clone();
        move |selected| {
            eprintln!(
                "ipc-ui: cctv-select-camera fallback selected={} reason={reason}",
                selected
            );
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                state.set_cctv_selected_camera(selected);
                state.set_cctv_last_error(SharedString::from(reason.clone()));
            }
        }
    });
    state.on_cctv_stop({
        let reason = reason.clone();
        move || {
            eprintln!("ipc-ui: cctv-stop fallback reason={reason}");
        }
    });
    state.on_cctv_capture({
        let app_weak = app.as_weak();
        let reason = reason.clone();
        move || {
            eprintln!("ipc-ui: cctv-capture fallback reason={reason}");
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>()
                    .set_cctv_last_error(SharedString::from(reason.clone()));
            }
        }
    });
    state.on_test_cctv_setting({
        let app_weak = app.as_weak();
        let reason = reason.clone();
        move || {
            eprintln!("ipc-ui: cctv-setting-test fallback reason={reason}");
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                state.set_cctv_setting_test_running(false);
                state.set_cctv_setting_test_step(1);
                state.set_cctv_setting_tested(false);
                state.set_cctv_setting_test_error(SharedString::from(reason.clone()));
                state.set_active_overlay(SharedString::from("E0zNY"));
            }
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn bind_wifi_callbacks(
    app: &AppWindow,
    database: Option<db::Database>,
    controller: wifi::WifiController,
) {
    let state = app.global::<State>();
    let app_weak = app.as_weak();
    state.on_refresh_wifi({
        let controller = controller.clone();
        move || {
            let _ = controller.refresh_scan();
        }
    });
    state.on_wifi_set_enabled({
        let controller = controller.clone();
        let database = database.clone();
        let app_weak = app_weak.clone();
        move |enabled| {
            let _ = controller.set_enabled(enabled);
            if let Some(database) = database.as_ref() {
                let _ = database.set_bool("network.wifi_enabled", enabled);
                if !enabled {
                    let _ = database.set_string("network.connected_ssid", "");
                }
            }
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                state.set_wifi_enabled(enabled);
                if !enabled {
                    state.set_connected_ssid(SharedString::from(""));
                    state.set_wifi_ipv4(SharedString::from(""));
                    state.set_wifi_scan_ssids(slint::ModelRc::new(slint::VecModel::from(Vec::<
                        SharedString,
                    >::new(
                    ))));
                    state.set_wifi_scan_open_flags(slint::ModelRc::new(slint::VecModel::from(
                        Vec::<bool>::new(),
                    )));
                }
            }
        }
    });
    state.on_connect_wifi({
        let controller = controller.clone();
        move |ssid, password| {
            let _ = controller.connect(ssid.as_str(), password.as_str());
        }
    });
}

#[cfg(not(target_arch = "riscv64"))]
fn bind_wifi_callbacks(app: &AppWindow, _database: Option<()>, controller: wifi::WifiController) {
    let state = app.global::<State>();
    let app_weak = app.as_weak();
    state.on_refresh_wifi({
        let controller = controller.clone();
        move || {
            let _ = controller.refresh_scan();
        }
    });
    state.on_wifi_set_enabled({
        let controller = controller.clone();
        let app_weak = app_weak.clone();
        move |enabled| {
            let _ = controller.set_enabled(enabled);
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>().set_wifi_enabled(enabled);
            }
        }
    });
    state.on_connect_wifi({
        let controller = controller.clone();
        move |ssid, password| {
            let _ = controller.connect(ssid.as_str(), password.as_str());
        }
    });
}

fn configure_call_action_routing(
    app: &AppWindow,
    call_controller: Option<call::CallController>,
    runtime_coordinator: Option<RuntimeCoordinator>,
    aiot_controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
    sound_controller: Option<sound::SoundController>,
    call_snapshot_controller: CallSnapshotController,
) {
    let state = app.global::<State>();

    state.on_call_start({
        let app_weak = app.as_weak();
        let call_controller = call_controller.clone();
        let runtime_coordinator = runtime_coordinator.clone();
        let aiot_controller_slot = aiot_controller_slot.clone();
        let sound_controller = sound_controller.clone();
        move |target| {
            let target_text = target.to_string();
            let is_manager_call = runtime_coordinator::is_manager_call_target(target_text.as_str());
            if let Some(controller) = runtime_coordinator.as_ref() {
                match controller.start_outgoing_call(target_text.as_str()) {
                    Ok(peer_label) => {
                        if let Some(app) = app_weak.upgrade() {
                            let state = app.global::<State>();
                            if let Some(local_controller) = call_controller.as_ref() {
                                if let Err(err) = local_controller.set_external_call_busy(true) {
                                    eprintln!(
                                        "call-ui: set external call busy failed busy=true: {err}"
                                    );
                                }
                            }
                            set_aiot_call_busy_for_local(Some(&aiot_controller_slot), true);
                            call::apply_outgoing_ringing(&state, peer_label.as_str());
                            start_outgoing_call_ringtone(
                                &state,
                                sound_controller.as_ref(),
                                if is_manager_call { "guard" } else { "door" },
                            );
                        }
                        return;
                    }
                    Err(err) => {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    }
                }
            }

            if is_manager_call {
                if let Some(app) = app_weak.upgrade() {
                    call::apply_call_error(&app.global::<State>(), "aiot_not_ready");
                }
                return;
            }

            if let Some(controller) = call_controller.as_ref() {
                match controller.start_call(target_text.as_str()) {
                    Ok(()) => {
                        set_aiot_call_busy_for_local(Some(&aiot_controller_slot), true);
                        set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), true);
                        if let Some(app) = app_weak.upgrade() {
                            let state = app.global::<State>();
                            call::apply_outgoing_ringing(&state, target_text.as_str());
                            start_outgoing_call_ringtone(&state, sound_controller.as_ref(), "door");
                        }
                    }
                    Err(err) => {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    }
                }
            }
        }
    });

    state.on_call_monitor_start({
        let app_weak = app.as_weak();
        let call_controller = call_controller.clone();
        move || {
            if let Some(controller) = call_controller.as_ref() {
                if let Some(app) = app_weak.upgrade() {
                    let state = app.global::<State>();
                    if let Err(err) =
                        controller.set_playback_volume(current_call_playback_volume(&state))
                    {
                        call::apply_call_error(&state, err);
                        return;
                    }
                    if let Err(err) =
                        controller.set_preview_brightness(state.get_door_brightness_value())
                    {
                        call::apply_call_error(&state, err);
                        return;
                    }
                }

                match controller.monitor(runtime_coordinator::SECONDARY_CONFIRM_DEVICE_ID) {
                    Ok(()) => {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_monitor_active(&app.global::<State>());
                        }
                    }
                    Err(err) => {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(
                                &app.global::<State>(),
                                format!("monitor_failed:{err}"),
                            );
                        }
                    }
                }
            } else if let Some(app) = app_weak.upgrade() {
                call::apply_call_error(&app.global::<State>(), "monitor_failed:no_call_controller");
            }
        }
    });

    state.on_call_accept({
        let app_weak = app.as_weak();
        let call_controller = call_controller.clone();
        let runtime_coordinator = runtime_coordinator.clone();
        let sound_controller = sound_controller.clone();
        let call_snapshot_controller = call_snapshot_controller.clone();
        move || {
            cancel_incoming_call_wait_timeout();
            stop_call_ringtone(sound_controller.as_ref());
            if let Some(app) = app_weak.upgrade() {
                clear_call_snapshot(Some(&call_snapshot_controller), &app.global::<State>());
            }
            let prefer_local_secondary = app_weak
                .upgrade()
                .map(|app| {
                    let on_secondary_page =
                        app.global::<State>().get_current_page().as_str() == "MwoLv";
                    let cloud_is_secondary_forward = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_secondary_forward_call())
                        .unwrap_or(false);
                    let has_cloud_call = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_active_call())
                        .unwrap_or(false);
                    on_secondary_page && (cloud_is_secondary_forward || !has_cloud_call)
                })
                .unwrap_or(false);
            if prefer_local_secondary {
                if let Some(controller) = runtime_coordinator.as_ref() {
                    if controller.has_secondary_forward_call() {
                        if let Err(err) = controller.hangup("local_answer") {
                            if let Some(app) = app_weak.upgrade() {
                                call::apply_call_error(&app.global::<State>(), err);
                            }
                        }
                    }
                }
                if let Some(controller) = call_controller.as_ref() {
                    if let Some(app) = app_weak.upgrade() {
                        let state = app.global::<State>();
                        if let Err(err) =
                            controller.set_playback_volume(current_call_playback_volume(&state))
                        {
                            call::apply_call_error(&state, err);
                            return;
                        }
                    }
                    if let Err(err) = controller.accept() {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    }
                }
                return;
            }
            if let Some(controller) = runtime_coordinator.as_ref() {
                if controller.has_active_call() {
                    if let Some(app) = app_weak.upgrade() {
                        set_webrtc_playback_volume_for_cloud(
                            runtime_coordinator.as_ref(),
                            current_call_playback_volume(&app.global::<State>()),
                        );
                        if let Err(err) = controller.accept() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    }
                    return;
                }
            }

            if let Some(controller) = call_controller.as_ref() {
                if let Some(app) = app_weak.upgrade() {
                    let state = app.global::<State>();
                    if let Err(err) =
                        controller.set_playback_volume(current_call_playback_volume(&state))
                    {
                        call::apply_call_error(&state, err);
                        return;
                    }
                }
                if let Err(err) = controller.accept() {
                    if let Some(app) = app_weak.upgrade() {
                        call::apply_call_error(&app.global::<State>(), err);
                    }
                }
            }
        }
    });

    state.on_call_reject({
        let app_weak = app.as_weak();
        let call_controller = call_controller.clone();
        let runtime_coordinator = runtime_coordinator.clone();
        let aiot_controller_slot = aiot_controller_slot.clone();
        let sound_controller = sound_controller.clone();
        let call_snapshot_controller = call_snapshot_controller.clone();
        move || {
            cancel_incoming_call_wait_timeout();
            stop_call_ringtone(sound_controller.as_ref());
            if let Some(app) = app_weak.upgrade() {
                clear_call_snapshot(Some(&call_snapshot_controller), &app.global::<State>());
            }
            let prefer_local_call = app_weak
                .upgrade()
                .map(|app| {
                    let on_secondary_page =
                        app.global::<State>().get_current_page().as_str() == "MwoLv";
                    let cloud_is_secondary_forward = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_secondary_forward_call())
                        .unwrap_or(false);
                    let has_cloud_call = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_active_call())
                        .unwrap_or(false);
                    on_secondary_page && (cloud_is_secondary_forward || !has_cloud_call)
                })
                .unwrap_or(false);
            if prefer_local_call {
                if let Some(controller) = runtime_coordinator.as_ref() {
                    if controller.has_secondary_forward_call() {
                        if let Err(err) = controller.reject() {
                            if let Some(app) = app_weak.upgrade() {
                                call::apply_call_error(&app.global::<State>(), err);
                            }
                        }
                    }
                }
                if let Some(local_controller) = call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                    if let Err(err) = local_controller.reject() {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                        return;
                    }
                    if let Err(err) = local_controller.set_external_call_busy(false) {
                        eprintln!("call-ui: set external call busy failed busy=false: {err}");
                    }
                }
                set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
                if let Some(app) = app_weak.upgrade() {
                    call::apply_call_idle(&app.global::<State>());
                }
                return;
            }
            if let Some(controller) = runtime_coordinator.as_ref() {
                if controller.has_active_call() {
                    if let Err(err) = controller.reject() {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    } else if let Some(app) = app_weak.upgrade() {
                        if let Some(local_controller) = call_controller.as_ref() {
                            let _ = local_controller.set_media_bridge(false);
                            let _ = local_controller.set_webrtc_bridge(None);
                            if let Err(err) = local_controller.set_external_call_busy(false) {
                                eprintln!(
                                    "call-ui: set external call busy failed busy=false: {err}"
                                );
                            }
                        }
                        set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                        call::apply_call_idle(&app.global::<State>());
                    }
                    return;
                }
            }

            if let Some(controller) = call_controller.as_ref() {
                if let Err(err) = controller.reject() {
                    if let Some(app) = app_weak.upgrade() {
                        call::apply_call_error(&app.global::<State>(), err);
                    }
                } else {
                    set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                    set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
                    if prefer_local_call {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_idle(&app.global::<State>());
                        }
                    } else if let Some(app) = app_weak.upgrade() {
                        call::apply_call_idle(&app.global::<State>());
                    }
                }
            }
        }
    });

    state.on_call_hangup({
        let app_weak = app.as_weak();
        let call_controller = call_controller.clone();
        let runtime_coordinator = runtime_coordinator.clone();
        let aiot_controller_slot = aiot_controller_slot.clone();
        let sound_controller = sound_controller.clone();
        let call_snapshot_controller = call_snapshot_controller.clone();
        move || {
            cancel_incoming_call_wait_timeout();
            stop_call_ringtone(sound_controller.as_ref());
            if let Some(app) = app_weak.upgrade() {
                clear_call_snapshot(Some(&call_snapshot_controller), &app.global::<State>());
            }
            let (prefer_local_call, is_monitor, cloud_is_secondary_forward) = app_weak
                .upgrade()
                .map(|app| {
                    let state = app.global::<State>();
                    let on_secondary_page = state.get_current_page().as_str() == "MwoLv";
                    let cloud_is_secondary_forward = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_secondary_forward_call())
                        .unwrap_or(false);
                    let has_cloud_call = runtime_coordinator
                        .as_ref()
                        .map(|controller| controller.has_active_call())
                        .unwrap_or(false);
                    (
                        on_secondary_page && (cloud_is_secondary_forward || !has_cloud_call),
                        state.get_call_session_state().as_str() == "monitor",
                        cloud_is_secondary_forward,
                    )
                })
                .unwrap_or((false, false, false));
            if cloud_is_secondary_forward {
                if let Some(controller) = runtime_coordinator.as_ref() {
                    if let Err(err) = controller.hangup("local_hangup") {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    }
                }
                if let Some(local_controller) = call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                    if let Err(err) = local_controller.hangup() {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                        return;
                    }
                    if let Err(err) = local_controller.set_external_call_busy(false) {
                        eprintln!("call-ui: set external call busy failed busy=false: {err}");
                    }
                }
                set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
                if let Some(app) = app_weak.upgrade() {
                    call::apply_call_idle(&app.global::<State>());
                }
                return;
            }
            if prefer_local_call {
                if let Some(controller) = runtime_coordinator.as_ref() {
                    if controller.has_secondary_forward_call() {
                        if let Err(err) = controller.hangup("local_hangup") {
                            if let Some(app) = app_weak.upgrade() {
                                call::apply_call_error(&app.global::<State>(), err);
                            }
                        }
                    }
                }
                if let Some(local_controller) = call_controller.as_ref() {
                    let _ = local_controller.set_media_bridge(false);
                    let _ = local_controller.set_webrtc_bridge(None);
                    let result = if is_monitor {
                        local_controller.stop_monitor()
                    } else {
                        local_controller.hangup()
                    };
                    if let Err(err) = result {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                        return;
                    }
                    if let Err(err) = local_controller.set_external_call_busy(false) {
                        eprintln!("call-ui: set external call busy failed busy=false: {err}");
                    }
                }
                set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
                if let Some(app) = app_weak.upgrade() {
                    call::apply_call_idle(&app.global::<State>());
                }
                return;
            }
            if let Some(controller) = runtime_coordinator.as_ref() {
                if controller.has_active_call() {
                    if let Err(err) = controller.hangup("local_hangup") {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_error(&app.global::<State>(), err);
                        }
                    } else if let Some(app) = app_weak.upgrade() {
                        if let Some(local_controller) = call_controller.as_ref() {
                            let _ = local_controller.set_media_bridge(false);
                            let _ = local_controller.set_webrtc_bridge(None);
                            if let Err(err) = local_controller.set_external_call_busy(false) {
                                eprintln!(
                                    "call-ui: set external call busy failed busy=false: {err}"
                                );
                            }
                        }
                        set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                        call::apply_call_idle(&app.global::<State>());
                    }
                    return;
                }
            }

            if let Some(controller) = call_controller.as_ref() {
                let result = if is_monitor {
                    controller.stop_monitor()
                } else {
                    controller.hangup()
                };
                if let Err(err) = result {
                    if let Some(app) = app_weak.upgrade() {
                        call::apply_call_error(&app.global::<State>(), err);
                    }
                } else {
                    set_aiot_call_busy_for_local(Some(&aiot_controller_slot), false);
                    set_webrtc_call_busy_for_local(runtime_coordinator.as_ref(), false);
                    if prefer_local_call {
                        if let Some(app) = app_weak.upgrade() {
                            call::apply_call_idle(&app.global::<State>());
                        }
                    }
                }
            }
        }
    });
}

fn configure_media_setting_routing(
    app: &AppWindow,
    call_controller: Option<call::CallController>,
    runtime_coordinator: Option<RuntimeCoordinator>,
    ipc_controller: Option<ipc::IpcController>,
    sound_controller: Option<sound::SoundController>,
) {
    let state = app.global::<State>();
    state.on_float_setting_live_changed({
        let app_weak = app.as_weak();
        move |key, value| {
            if matches!(
                key.as_str(),
                "call.door.volume"
                    | "call.lobby.volume"
                    | "call.guard.volume"
                    | "call.ev.volume"
                    | "sound.talk_volume"
            ) {
                set_webrtc_playback_volume_for_cloud(runtime_coordinator.as_ref(), value);
                if let Some(controller) = call_controller.as_ref() {
                    if let Err(err) = controller.set_playback_volume(value) {
                        if let Some(app) = app_weak.upgrade() {
                            app.global::<State>()
                                .set_call_last_error(SharedString::from(err));
                        }
                    }
                }
            } else if key.as_str() == "sound.ring_volume" {
                if let Some(controller) = sound_controller.as_ref() {
                    if let Err(err) = controller.set_volume(value) {
                        eprintln!("sound: set volume failed: {err}");
                    }
                }
            } else if key.as_str() == "call.door.brightness" {
                set_webrtc_preview_brightness_for_cloud(runtime_coordinator.as_ref(), value);
                if let Some(controller) = call_controller.as_ref() {
                    if let Err(err) = controller.set_preview_brightness(value) {
                        if let Some(app) = app_weak.upgrade() {
                            app.global::<State>()
                                .set_call_last_error(SharedString::from(err));
                        }
                    }
                }
            } else if key.as_str() == "cctv.brightness" {
                if let Some(controller) = ipc_controller.as_ref() {
                    if let Err(err) = controller.set_preview_brightness(value) {
                        if let Some(app) = app_weak.upgrade() {
                            app.global::<State>()
                                .set_cctv_last_error(SharedString::from(err));
                        }
                    }
                }
            }
        }
    });
}

fn current_call_playback_volume(state: &State<'_>) -> f32 {
    match state.get_current_page().as_str() {
        "MwoLv" => state.get_door_volume_value(),
        "Bu0fW" => state.get_lobby_volume_value(),
        "EJDbm" => state.get_guard_volume_value(),
        "hcEj1" => state.get_ev_volume_value(),
        _ => state.get_sound_talk_volume_value(),
    }
}

fn apply_aiot_sync_time(time_text: &str) -> Result<(), String> {
    if time_text.len() != 14 || !time_text.bytes().all(|byte| byte.is_ascii_digit()) {
        return Err(format!("invalid sync time format: {time_text}"));
    }

    let year = &time_text[0..4];
    let month = &time_text[4..6];
    let day = &time_text[6..8];
    let hour24 = time_text[8..10]
        .parse::<i32>()
        .map_err(|err| format!("invalid sync hour {}: {err}", &time_text[8..10]))?;
    let minute = &time_text[10..12];

    let period = if hour24 >= 12 { "PM" } else { "AM" };
    let hour12 = match hour24 % 12 {
        0 => 12,
        value => value,
    };

    set_general_system_time(period, &format!("{hour12:02}"), minute, year, month, day)
}

fn camera_for_display_index(
    cameras: &[ipc::IpcCameraConfig],
    selected: i32,
) -> Option<ipc::IpcCameraConfig> {
    if selected <= 0 {
        return cameras.first().cloned();
    }
    cameras.get((selected - 1) as usize).cloned()
}

fn ipc_capture_output_path(base_dir: &str, camera_name: &str) -> String {
    capture_records::new_capture_output_path(base_dir, camera_name)
}

fn cctv_test_generation_matches(
    generation_state: &Arc<Mutex<u64>>,
    expected_generation: u64,
) -> bool {
    generation_state
        .lock()
        .map(|generation| *generation == expected_generation)
        .unwrap_or(false)
}

fn spawn_cctv_test_step_update(
    app_weak: slint::Weak<AppWindow>,
    generation_state: Arc<Mutex<u64>>,
    expected_generation: u64,
    step: i32,
    delay: Duration,
) {
    std::thread::spawn(move || {
        std::thread::sleep(delay);
        let _ = slint::invoke_from_event_loop(move || {
            if !cctv_test_generation_matches(&generation_state, expected_generation) {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                if state.get_cctv_setting_test_running()
                    && state.get_cctv_setting_test_step() < step
                {
                    state.set_cctv_setting_test_step(step);
                }
            }
        });
    });
}

fn spawn_cctv_test_failure(
    app_weak: slint::Weak<AppWindow>,
    generation_state: Arc<Mutex<u64>>,
    expected_generation: u64,
    step: i32,
    message: String,
    delay: Duration,
) {
    std::thread::spawn(move || {
        std::thread::sleep(delay);
        let _ = slint::invoke_from_event_loop(move || {
            if !cctv_test_generation_matches(&generation_state, expected_generation) {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                if state.get_cctv_setting_test_running() {
                    state.set_cctv_setting_test_running(false);
                    state.set_cctv_setting_test_step(step);
                    state.set_cctv_setting_tested(false);
                    state.set_cctv_setting_test_error(SharedString::from(message));
                    state.set_active_overlay(SharedString::from("E0zNY"));
                }
            }
        });
    });
}

fn spawn_cctv_setting_test(
    app_weak: slint::Weak<AppWindow>,
    controller: ipc::IpcController,
    camera: ipc::IpcCameraConfig,
    brightness: f32,
    generation_state: Arc<Mutex<u64>>,
    expected_generation: u64,
) {
    spawn_cctv_test_step_update(
        app_weak.clone(),
        generation_state.clone(),
        expected_generation,
        2,
        Duration::from_millis(250),
    );
    std::thread::spawn(move || {
        eprintln!(
            "ipc-ui: cctv-setting-test async start begin ip={} user={}",
            camera.ip, camera.user
        );
        std::thread::sleep(Duration::from_millis(320));
        let result = controller.start_monitor(&camera);
        match result {
            Ok(()) => {
                let _ = controller.set_preview_brightness(brightness);
                spawn_cctv_test_step_update(
                    app_weak.clone(),
                    generation_state.clone(),
                    expected_generation,
                    3,
                    Duration::from_millis(300),
                );
                spawn_cctv_test_step_update(
                    app_weak.clone(),
                    generation_state.clone(),
                    expected_generation,
                    4,
                    Duration::from_millis(900),
                );
                spawn_cctv_test_step_update(
                    app_weak.clone(),
                    generation_state.clone(),
                    expected_generation,
                    5,
                    Duration::from_millis(1500),
                );
                spawn_cctv_test_failure(
                    app_weak,
                    generation_state,
                    expected_generation,
                    5,
                    "IPC 모니터링 응답 시간이 초과되었습니다".to_string(),
                    Duration::from_secs(10),
                );
                eprintln!("ipc-ui: cctv-setting-test async start requested");
            }
            Err(err) => {
                eprintln!("ipc-ui: cctv-setting-test async start failed err={err}");
                spawn_cctv_test_failure(
                    app_weak,
                    generation_state,
                    expected_generation,
                    2,
                    err,
                    Duration::from_millis(0),
                );
            }
        }
    });
}

fn spawn_ipc_start_monitor(
    app_weak: slint::Weak<AppWindow>,
    controller: ipc::IpcController,
    camera: ipc::IpcCameraConfig,
    brightness: f32,
    source: &'static str,
) {
    std::thread::spawn(move || {
        eprintln!(
            "ipc-ui: {source} async start begin name={} ip={}",
            camera.name, camera.ip
        );
        let result = controller.start_monitor(&camera);
        match result {
            Ok(()) => {
                let _ = controller.set_preview_brightness(brightness);
                let _ = slint::invoke_from_event_loop(move || {
                    if let Some(app) = app_weak.upgrade() {
                        let state = app.global::<State>();
                        state.set_cctv_last_error(SharedString::from(""));
                        state.set_cctv_current_camera_label(SharedString::from(camera.name));
                    }
                });
                eprintln!("ipc-ui: {source} async start requested");
            }
            Err(err) => {
                let error_text = err.clone();
                let _ = slint::invoke_from_event_loop(move || {
                    if let Some(app) = app_weak.upgrade() {
                        app.global::<State>()
                            .set_cctv_last_error(SharedString::from(error_text));
                    }
                });
                eprintln!("ipc-ui: {source} async start failed err={err}");
            }
        }
    });
}

fn spawn_ipc_stop_monitor(
    app_weak: slint::Weak<AppWindow>,
    controller: ipc::IpcController,
    source: &'static str,
) {
    std::thread::spawn(move || {
        eprintln!("ipc-ui: {source} async stop begin");
        let result = controller.stop_monitor();
        let _ = slint::invoke_from_event_loop(move || {
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>().set_cctv_monitor_online(false);
            }
        });
        if let Err(err) = result {
            eprintln!("ipc-ui: {source} async stop failed err={err}");
        } else {
            eprintln!("ipc-ui: {source} async stop requested");
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn bind_wifi_fallback_callbacks(app: &AppWindow, database: Option<db::Database>) {
    let state = app.global::<State>();
    state.on_refresh_wifi(|| {});
    state.on_wifi_set_enabled({
        let app_weak = app.as_weak();
        move |enabled| {
            if let Some(database) = database.as_ref() {
                let _ = database.set_bool("network.wifi_enabled", enabled);
            }
            if let Some(app) = app_weak.upgrade() {
                app.global::<State>().set_wifi_enabled(enabled);
            }
        }
    });
    state.on_connect_wifi({
        let app_weak = app.as_weak();
        move |_, _| {
            if let Some(app) = app_weak.upgrade() {
                let state = app.global::<State>();
                state.set_wifi_dialog_locked(false);
                state.set_active_overlay(SharedString::from("HZltp"));
            }
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn configure_indoor_bindings(
    app: &AppWindow,
    database: Option<db::Database>,
    indoor_config: indoor::IndoorConfig,
    aiot_controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
    runtime_coordinator_slot: Arc<Mutex<Option<RuntimeCoordinator>>>,
    secondary_confirm_state: Arc<Mutex<aiot::SecondaryConfirmProxyState>>,
    sound_controller: Option<sound::SoundController>,
) -> (
    Option<indoor::IndoorService>,
    Option<call::CallController>,
    Option<d2d::D2dController>,
) {
    let state = app.global::<State>();
    state.set_call_enabled(indoor_config.enabled);
    state.set_call_session_state(SharedString::from("idle"));
    state.set_call_peer_id(SharedString::from(""));
    state.set_call_video_active(false);
    state.set_call_audio_active(false);
    state.set_call_last_error(SharedString::from(""));
    state.set_incoming_call_page(SharedString::from("Bu0fW"));
    #[cfg(kr_indoor_stub_backend)]
    {
        state.set_call_enabled(false);
        state.set_call_last_error(SharedString::from("stub_indoor_backend"));
    }

    let dispatch_context = AppEventDispatchContext {
        app_weak: app.as_weak(),
        d2d_controller: None,
        runtime_coordinator: None,
        ipc_controller: None,
        call_controller: None,
        sound_controller,
        call_snapshot_controller: None,
        aiot_controller_slot: Some(aiot_controller_slot),
        runtime_coordinator_slot: Some(runtime_coordinator_slot.clone()),
        secondary_confirm_state: Some(secondary_confirm_state),
    };
    let dispatcher: Arc<dyn Fn(indoor::IndoorEvent) + Send + Sync + 'static> =
        Arc::new(move |event| {
            dispatch_app_event(dispatch_context.clone(), AppEvent::Indoor(event));
        });

    let service = match indoor::IndoorService::start(indoor_config.clone(), dispatcher) {
        Ok(service) => service,
        Err(err) => {
            state.set_call_enabled(false);
            state.set_call_last_error(SharedString::from(format!("indoor_start_failed:{err}")));
            return (None, None, None);
        }
    };

    let call_controller = service.call_controller(&indoor_config);
    let d2d_controller = service.d2d_controller(&indoor_config);

    if let Some(database) = &database {
        match database.password_plain() {
            Ok(password) => {
                if let Err(err) = d2d_controller.set_local_password(&password) {
                    state.set_call_last_error(SharedString::from(format!(
                        "d2d_password_init_failed:{err}"
                    )));
                }
            }
            Err(err) => {
                state.set_call_last_error(SharedString::from(format!(
                    "d2d_password_load_failed:{err}"
                )));
            }
        }
    }

    state.on_call_open_door({
        let controller = d2d_controller.clone();
        let app_weak = app.as_weak();
        let runtime_coordinator_slot = runtime_coordinator_slot.clone();
        move || {
            let cloud_open_result = runtime_coordinator_slot
                .lock()
                .ok()
                .and_then(|slot| slot.as_ref().cloned())
                .map(|coordinator| coordinator.open_lock_for_active_call());
            if let Some(result) = cloud_open_result {
                match result {
                    Ok(true) => return,
                    Ok(false) => {}
                    Err(err) => {
                        if let Some(app) = app_weak.upgrade() {
                            app.global::<State>()
                                .set_call_last_error(SharedString::from(format!(
                                    "aiot_open_ask_failed:{err}"
                                )));
                        }
                        return;
                    }
                }
            }

            if let Err(err) = controller.unlock_default_door() {
                if let Some(app) = app_weak.upgrade() {
                    app.global::<State>()
                        .set_call_last_error(SharedString::from(err));
                }
            }
        }
    });

    state.on_change_password({
        let controller = d2d_controller.clone();
        let app_weak = app.as_weak();
        let database = database.clone();
        move |old_password, new_password| {
            let old_password = old_password.to_string();
            let new_password = new_password.to_string();
            let valid = old_password.len() == 6
                && new_password.len() == 6
                && database
                    .as_ref()
                    .and_then(|db| db.password_matches(&old_password).ok())
                    .unwrap_or(false);

            if !valid {
                if let Some(app) = app_weak.upgrade() {
                    app.global::<State>()
                        .set_active_overlay(SharedString::from("CMf10"));
                }
                return;
            }

            let saved = database
                .as_ref()
                .and_then(|db| db.set_password_pair(&new_password).ok())
                .is_some();
            if saved {
                let _ = controller.set_local_password(&new_password);
                let _ = controller.set_unlock_password(&new_password);
                if let Some(app) = app_weak.upgrade() {
                    reset_password_dialog_success(&app.global::<State>());
                }
            } else if let Some(app) = app_weak.upgrade() {
                app.global::<State>()
                    .set_active_overlay(SharedString::from("CMf10"));
            }
        }
    });

    (Some(service), Some(call_controller), Some(d2d_controller))
}

#[cfg(not(target_arch = "riscv64"))]
fn configure_indoor_bindings(
    app: &AppWindow,
    _database: Option<()>,
    indoor_config: indoor::IndoorConfig,
    aiot_controller_slot: Arc<Mutex<Option<aiot::AiotController>>>,
    runtime_coordinator_slot: Arc<Mutex<Option<RuntimeCoordinator>>>,
    secondary_confirm_state: Arc<Mutex<aiot::SecondaryConfirmProxyState>>,
    sound_controller: Option<sound::SoundController>,
) -> (
    Option<indoor::IndoorService>,
    Option<call::CallController>,
    Option<d2d::D2dController>,
) {
    let state = app.global::<State>();
    state.set_call_enabled(indoor_config.enabled);
    state.set_call_session_state(SharedString::from("idle"));
    state.set_call_peer_id(SharedString::from(""));
    state.set_call_video_active(false);
    state.set_call_audio_active(false);
    state.set_call_last_error(SharedString::from(""));
    state.set_incoming_call_page(SharedString::from("Bu0fW"));

    let dispatch_context = AppEventDispatchContext {
        app_weak: app.as_weak(),
        d2d_controller: None,
        runtime_coordinator: None,
        ipc_controller: None,
        call_controller: None,
        sound_controller,
        call_snapshot_controller: None,
        aiot_controller_slot: Some(aiot_controller_slot),
        runtime_coordinator_slot: Some(runtime_coordinator_slot.clone()),
        secondary_confirm_state: Some(secondary_confirm_state),
    };
    let dispatcher: Arc<dyn Fn(indoor::IndoorEvent) + Send + Sync + 'static> =
        Arc::new(move |event| {
            dispatch_app_event(dispatch_context.clone(), AppEvent::Indoor(event));
        });

    let service = match indoor::IndoorService::start(indoor_config.clone(), dispatcher) {
        Ok(service) => service,
        Err(err) => {
            state.set_call_enabled(false);
            state.set_call_last_error(SharedString::from(err));
            return (None, None, None);
        }
    };

    let call_controller = service.call_controller(&indoor_config);
    let d2d_controller = service.d2d_controller(&indoor_config);

    state.on_call_open_door({
        let controller = d2d_controller.clone();
        let runtime_coordinator_slot = runtime_coordinator_slot.clone();
        move || {
            let cloud_open_result = runtime_coordinator_slot
                .lock()
                .ok()
                .and_then(|slot| slot.as_ref().cloned())
                .map(|coordinator| coordinator.open_lock_for_active_call());
            if let Some(Ok(true)) = cloud_open_result {
                return;
            }
            let _ = controller.unlock_default_door();
        }
    });
    state.on_change_password({
        let controller = d2d_controller.clone();
        let app_weak = app.as_weak();
        move |old_password, new_password| {
            if old_password.len() == 6 && new_password.len() == 6 {
                let _ = controller.set_local_password(new_password.as_str());
                let _ = controller.set_unlock_password(new_password.as_str());
                if let Some(app) = app_weak.upgrade() {
                    reset_password_dialog_success(&app.global::<State>());
                }
            } else if let Some(app) = app_weak.upgrade() {
                app.global::<State>()
                    .set_active_overlay(SharedString::from("CMf10"));
            }
        }
    });

    (Some(service), Some(call_controller), Some(d2d_controller))
}

fn apply_d2d_event_to_state(state: &State<'_>, event: &d2d::D2dEvent) {
    match event.kind {
        d2d::D2dEventKind::UnlockDone => {
            eprintln!("d2d unlock succeeded: door_id={}", event.door_id);
        }
        d2d::D2dEventKind::UnlockFail | d2d::D2dEventKind::UnlockTimeout => {
            state.set_call_last_error(SharedString::from(format!(
                "unlock_failed:{}:{}",
                event.kind.as_str(),
                event.result_code
            )));
        }
        d2d::D2dEventKind::SetPasswordDone => {
            state.set_active_overlay(SharedString::from("rIvrD"));
        }
        d2d::D2dEventKind::SetPasswordFail | d2d::D2dEventKind::SetPasswordTimeout => {
            state.set_active_overlay(SharedString::from("CMf10"));
        }
        d2d::D2dEventKind::Error => {
            state.set_call_last_error(SharedString::from(format!(
                "d2d_error:{}",
                event.result_code
            )));
        }
    }
}

fn reset_password_dialog_success(state: &State<'_>) {
    state.set_active_overlay(SharedString::from("rIvrD"));
    state.set_old_password(SharedString::from(""));
    state.set_old_password_length(0);
    state.set_old_password_mask(SharedString::from(""));
    state.set_old_password_visible(false);
    state.set_new_password(SharedString::from(""));
    state.set_new_password_length(0);
    state.set_new_password_mask(SharedString::from(""));
    state.set_password_visible(false);
}

fn configure_sound_bindings(app: &AppWindow, sound_controller: Option<sound::SoundController>) {
    let state = app.global::<State>();
    state.on_play_touch_sound({
        let sound_controller = sound_controller.clone();
        move || {
            if let Some(controller) = sound_controller.as_ref() {
                if let Err(err) = controller.play_touch() {
                    eprintln!("sound: play touch failed: {err}");
                }
            }
        }
    });
}

#[cfg(target_arch = "riscv64")]
fn apply_settings_to_state(state: &State<'_>, settings: &db::Settings) {
    state.set_language(SharedString::from(
        settings.get_string("general.language", "ko-KR"),
    ));
    state.set_time_period(SharedString::from(
        settings.get_string("general.time_period", "PM"),
    ));
    state.set_general_hour(SharedString::from(
        settings.get_string("general.hour", "12"),
    ));
    state.set_general_minute(SharedString::from(
        settings.get_string("general.minute", "00"),
    ));
    state.set_general_year(SharedString::from(
        settings.get_string("general.year", "2026"),
    ));
    state.set_general_month(SharedString::from(
        settings.get_string("general.month", "04"),
    ));
    state.set_general_day(SharedString::from(settings.get_string("general.day", "23")));
    let site_building_number = settings.get_string("site.building_number", "01").trim();
    let site_building_number = if site_building_number.is_empty() {
        "01"
    } else {
        site_building_number
    };
    state.set_site_building_number(SharedString::from(site_building_number));
    let site_room_number = settings.get_string("site.room_number", "101").trim();
    let site_room_number = if site_room_number.is_empty() {
        "101"
    } else {
        site_room_number
    };
    state.set_site_room_number(SharedString::from(site_room_number));
    state.set_site_building_draft(SharedString::from(site_building_number));
    state.set_site_room_draft(SharedString::from(site_room_number));
    state.set_site_info_input_length(0);
    state.set_old_password_length(0);
    state.set_old_password_mask(SharedString::from(""));
    state.set_old_password_visible(false);
    state.set_new_password_length(0);
    state.set_new_password_mask(SharedString::from(""));
    state.set_door_call_melody(settings.get_int("sound.door_call_melody", 1));
    state.set_guard_call_melody(settings.get_int("sound.guard_call_melody", 3));
    state.set_wifi_enabled(settings.get_bool("network.wifi_enabled", true));
    state.set_connected_ssid(SharedString::from(
        settings.get_string("network.connected_ssid", "HomeNetwork_5G"),
    ));
    state.set_wifi_target_open(false);
    state.set_wifi_status(SharedString::from("idle"));
    state.set_wifi_connecting_ssid(SharedString::from(""));
    state.set_wifi_dialog_locked(false);
    state.set_wifi_ipv4(SharedString::from(""));
    state.set_wifi_scan_ssids(slint::ModelRc::new(slint::VecModel::from(Vec::<
        SharedString,
    >::new())));
    state.set_wifi_scan_open_flags(slint::ModelRc::new(slint::VecModel::from(
        Vec::<bool>::new(),
    )));
    state.set_call_enabled(settings.get_bool("call.enabled", true));
    state.set_call_session_state(SharedString::from("idle"));
    state.set_call_peer_id(SharedString::from(""));
    state.set_call_video_active(false);
    state.set_call_audio_active(false);
    state.set_call_last_error(SharedString::from(""));
    state.set_incoming_call_page(SharedString::from("Bu0fW"));
    state.set_clock_screen_enabled(settings.get_bool("mode.clock_screen_enabled", true));
    state.set_idle_clock_timeout_sec(settings.get_int("idle.clock_timeout_sec", 60).max(60));
    state.set_auto_capture_enabled(settings.get_bool("mode.auto_capture_enabled", false));
    state.set_absent_mode_active(settings.get_bool("mode.absent_active", false));
    state.set_absent_mode_minutes(settings.get_int("mode.absent_timeout_min", 5));
    state.set_communication_minutes(settings.get_int("mode.communication_timeout_min", 3));
    state.set_call_timeout_seconds(normalize_call_ring_timeout_sec(
        settings.get_int("mode.call_ring_timeout_sec", 30),
    ));
    state.set_door_open_seconds(settings.get_int("mode.door_open_duration_sec", 3));
    state.set_absent_mode_time(SharedString::from(
        settings.get_minute_label("mode.absent_timeout_min", 5),
    ));
    state.set_communication_time(SharedString::from(
        settings.get_minute_label("mode.communication_timeout_min", 3),
    ));
    state.set_call_timeout_value(SharedString::from(format!(
        "{}초",
        normalize_call_ring_timeout_sec(settings.get_int("mode.call_ring_timeout_sec", 30))
    )));
    state.set_second_level_value(SharedString::from(
        settings.get_second_label("mode.door_open_duration_sec", 3),
    ));
    state.set_display_brightness_value(settings.get_float("display.brightness", 0.66));
    state.set_display_color_value(settings.get_float("display.hue", 0.0));
    state.set_display_contrast_value(settings.get_float("display.contrast", 0.67));
    state.set_display_saturation_value(settings.get_float("display.saturation", 0.45));
    state.set_sound_call_volume_value(settings.get_float("sound.ring_volume", 0.80));
    state.set_sound_talk_volume_value(settings.get_float("sound.talk_volume", 0.70));
    state.set_sound_mic_volume_value(settings.get_float("sound.mic_sensitivity", 0.65));
    state.set_door_brightness_value(settings.get_float("call.door.brightness", 0.77));
    state.set_door_volume_value(settings.get_float("call.door.volume", 0.70));
    state.set_lobby_brightness_value(settings.get_float("call.lobby.brightness", 0.77));
    state.set_lobby_volume_value(settings.get_float("call.lobby.volume", 0.70));
    state.set_guard_brightness_value(settings.get_float("call.guard.brightness", 0.0));
    state.set_guard_volume_value(settings.get_float("call.guard.volume", 0.70));
    state.set_ev_brightness_value(settings.get_float("call.ev.brightness", 0.0));
    state.set_ev_volume_value(settings.get_float("call.ev.volume", 0.0));
    state.set_cctv_brightness_value(settings.get_float("cctv.brightness", 0.77));
    state.set_cctv_volume_value(settings.get_float("cctv.volume", 0.70));
    state.set_cctv_camera_options(slint::ModelRc::new(slint::VecModel::from(Vec::<
        SharedString,
    >::new())));
    state.set_cctv_camera_details(slint::ModelRc::new(slint::VecModel::from(Vec::<
        SharedString,
    >::new())));
    state.set_cctv_selected_camera(0);
    state.set_cctv_setting_delete_index(0);
    state.set_cctv_current_camera_label(SharedString::from(""));
    state.set_cctv_monitor_online(false);
    state.set_server_online(false);
    state.set_cctv_last_error(SharedString::from(""));
    state.set_cctv_last_capture_path(SharedString::from(""));
    state.set_cctv_setting_ip(SharedString::from(""));
    state.set_cctv_setting_port(SharedString::from(
        settings.get_string("ipc.onvif_port", "80"),
    ));
    state.set_cctv_setting_account(SharedString::from(""));
    state.set_cctv_setting_password(SharedString::from(""));
    state.set_cctv_setting_password_mask(SharedString::from(""));
    state.set_cctv_setting_input_target(SharedString::from(""));
    state.set_cctv_setting_input_buffer(SharedString::from(""));
    state.set_cctv_setting_input_mask(SharedString::from(""));
    state.set_cctv_setting_input_length(0);
    let cctv_ip_parts = parse_cctv_ip_parts("");
    set_cctv_ip_parts(state, &cctv_ip_parts, 1);
    state.set_cctv_setting_tested(false);
    state.set_cctv_setting_saved(false);
    state.set_cctv_setting_test_running(false);
    state.set_cctv_setting_test_step(0);
    state.set_cctv_setting_test_error(SharedString::from(""));
    state.set_cctv_setting_test_model(SharedString::from("ONVIF IP Camera HD"));
    state.set_cctv_setting_test_resolution(SharedString::from("1920 x 1080"));
    state.set_cctv_setting_test_frame(SharedString::from("25fps"));
}

fn apply_cctv_setting_to_state(
    state: &State<'_>,
    database: &db::Database,
    settings: &db::Settings,
) {
    let port = settings.get_string("ipc.onvif_port", "80");
    state.set_cctv_setting_port(SharedString::from(port));
    if let Ok(cameras) = database.ipc_cameras() {
        apply_cctv_camera_list_to_state(state, &cameras, &port);
        if let Some(camera) = cameras.first() {
            apply_cctv_camera_to_setting_state(state, camera, &port);
        }
    }
}
