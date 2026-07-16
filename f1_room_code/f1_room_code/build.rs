use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let lib_dir = manifest_dir.join("lib");
    let target = env::var("TARGET").unwrap_or_default();
    println!("cargo:rustc-check-cfg=cfg(kr_indoor_stub_backend)");
    println!("cargo:rustc-check-cfg=cfg(kr_aiot_stub_backend)");
    println!("cargo:rustc-check-cfg=cfg(kr_webrtc_stub_backend)");
    println!("cargo:rustc-check-cfg=cfg(kr_ipc_stub_backend)");
    println!("cargo:rustc-check-cfg=cfg(kr_wifi_stub_backend)");
    println!("cargo:rustc-check-cfg=cfg(kr_sound_stub_backend)");

    compile_native_utils(&manifest_dir);

    if target.contains("riscv64") {
        println!("cargo:rustc-link-search=native={}", lib_dir.display());

        if indoor_runtime_available(&manifest_dir) {
            compile_indoor_bridge_real(&manifest_dir);
            link_indoor_runtime();
        } else {
            println!("cargo:warning=No riscv64-compatible indoor runtime found; building indoor stub backend");
            println!("cargo:rustc-cfg=kr_indoor_stub_backend");
            println!("cargo:rustc-link-lib=static=kr_native_utils");
        }

        if wifi_runtime_available(&manifest_dir) {
            println!("cargo:rustc-link-lib=static=dplib_wifi");
            println!("cargo:rustc-link-lib=static=dplib_net");
            println!("cargo:rustc-link-lib=dylib=wpa_client");
        } else {
            println!("cargo:warning=No riscv64-compatible WiFi runtime found; building wifi stub backend");
            println!("cargo:rustc-cfg=kr_wifi_stub_backend");
        }
        if aiot_runtime_available(&manifest_dir) {
            compile_aiot_bridge_real(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_aiot_bridge");
            println!("cargo:rustc-link-lib=dylib=goblin_aiot");
            println!("cargo:rustc-link-lib=dylib=paho-mqtt3as");
            println!("cargo:rustc-link-lib=dylib=paho-mqtt3cs");
            println!("cargo:rustc-link-lib=dylib=curl");
            println!("cargo:rustc-link-lib=dylib=ssl");
            println!("cargo:rustc-link-lib=dylib=crypto");
        } else {
            println!("cargo:warning=No riscv64-compatible AIOT runtime found; building aiot stub backend");
            println!("cargo:rustc-cfg=kr_aiot_stub_backend");
            compile_aiot_bridge_stub(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_aiot_bridge");
        }
        if webrtc_runtime_available(&manifest_dir) {
            compile_webrtc_bridge_real(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_webrtc_bridge");
            println!("cargo:rustc-link-lib=static=webrtc-stream");
            println!("cargo:rustc-link-lib=static=mbedtls");
            println!("cargo:rustc-link-lib=static=mbedx509");
            println!("cargo:rustc-link-lib=static=mbedcrypto");
            println!("cargo:rustc-link-lib=dylib=AudioCore");
            println!("cargo:rustc-link-lib=dylib=VideoDec");
            println!("cargo:rustc-link-lib=dylib=ssl");
            println!("cargo:rustc-link-lib=dylib=crypto");
            println!("cargo:rustc-link-lib=dylib=m");
        } else {
            println!("cargo:warning=No riscv64-compatible WebRTC runtime found; building webrtc stub backend");
            println!("cargo:rustc-cfg=kr_webrtc_stub_backend");
            compile_webrtc_bridge_stub(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_webrtc_bridge");
        }
        if sound_runtime_available(&manifest_dir) {
            compile_sound_bridge_real(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_sound_bridge");
            println!("cargo:rustc-link-lib=dylib=AudioCore");
        } else {
            println!("cargo:warning=No riscv64-compatible sound runtime found; building sound stub backend");
            println!("cargo:rustc-cfg=kr_sound_stub_backend");
            compile_sound_bridge_stub(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_sound_bridge");
        }
        if ipc_runtime_available(&manifest_dir) {
            compile_ipc_bridge_real(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_ipc_bridge");
            println!("cargo:rustc-link-lib=dylib=ipcammer");
            println!("cargo:rustc-link-lib=dylib=xml2");
            println!("cargo:rustc-link-lib=dylib=VideoDec");
        } else {
            println!(
                "cargo:warning=No riscv64-compatible IPC runtime found; building ipc stub backend"
            );
            println!("cargo:rustc-cfg=kr_ipc_stub_backend");
            compile_ipc_bridge_stub(&manifest_dir);
            println!("cargo:rustc-link-lib=static=kr_ipc_bridge");
        }
        println!("cargo:rustc-link-lib=static=kr_native_utils");
        println!("cargo:rustc-link-lib=dylib=dl");
        println!("cargo:rustc-link-lib=dylib=pthread");
        println!("cargo:rustc-link-lib=dylib=rt");
    } else {
        println!("cargo:rustc-cfg=kr_indoor_stub_backend");
        println!("cargo:rustc-cfg=kr_aiot_stub_backend");
        println!("cargo:rustc-cfg=kr_webrtc_stub_backend");
        println!("cargo:rustc-cfg=kr_ipc_stub_backend");
        println!("cargo:rustc-cfg=kr_wifi_stub_backend");
        println!("cargo:rustc-cfg=kr_sound_stub_backend");
        compile_aiot_bridge_stub(&manifest_dir);
        println!("cargo:rustc-link-lib=static=kr_aiot_bridge");
        compile_webrtc_bridge_stub(&manifest_dir);
        println!("cargo:rustc-link-lib=static=kr_webrtc_bridge");
        compile_ipc_bridge_stub(&manifest_dir);
        println!("cargo:rustc-link-lib=static=kr_ipc_bridge");
        compile_sound_bridge_stub(&manifest_dir);
        println!("cargo:rustc-link-lib=static=kr_sound_bridge");
        println!("cargo:rustc-link-lib=static=kr_native_utils");
    }

    slint_build::compile("ui/app.slint").expect("failed to compile Slint UI");
}

fn elf_machine_matches_riscv(path: &PathBuf) -> bool {
    const ELF_MACHINE_OFFSET: usize = 18;
    const ELF_MACHINE_RISCV: u16 = 243;

    let path = match fs::canonicalize(path) {
        Ok(path) => path,
        Err(_) => path.clone(),
    };
    let bytes = match fs::read(path) {
        Ok(bytes) => bytes,
        Err(_) => return false,
    };
    if bytes.len() <= ELF_MACHINE_OFFSET + 1 || &bytes[0..4] != b"\x7fELF" {
        return false;
    }

    u16::from_le_bytes([bytes[ELF_MACHINE_OFFSET], bytes[ELF_MACHINE_OFFSET + 1]])
        == ELF_MACHINE_RISCV
}

fn indoor_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    [
        "libmtrans.so",
        "libd2d_protocol.so",
        "libtransport_adapter.so",
        "libtopology.so",
        "libgoblin_plc_trancore.so",
        "libopus.so",
        "libAudioCore.so",
        "libVideoDec.so",
        "libmpp_base.so",
        "libmpp_decoder.so",
        "libmpp_encoder.so",
        "libmpp_ge.so",
        "libmpp_ve.so",
    ]
    .into_iter()
    .all(|name| {
        let path = lib_dir.join(name);
        path.exists() && elf_machine_matches_riscv(&path)
    }) && lib_dir.join("mtrans/mtrans.h").exists()
        && lib_dir.join("mtrans/call/call_flow_runtime.h").exists()
        && lib_dir.join("transport/transport.h").exists()
        && lib_dir.join("protocol/d2d_protocol.h").exists()
        && lib_dir.join("topology/topology.h").exists()
}

fn wifi_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    lib_dir.join("libdplib_wifi.a").exists()
        && lib_dir.join("libdplib_net.a").exists()
        && lib_dir.join("libwpa_client.so").exists()
        && elf_machine_matches_riscv(&lib_dir.join("libwpa_client.so"))
}

fn aiot_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    [
        "libgoblin_aiot.so",
        "libpaho-mqtt3as.so",
        "libpaho-mqtt3cs.so",
        "libcurl.so",
        "libssl.so",
        "libcrypto.so",
    ]
    .into_iter()
    .all(|name| {
        let path = lib_dir.join(name);
        path.exists() && elf_machine_matches_riscv(&path)
    })
}

fn webrtc_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    lib_dir.join("webrtc/webrtc_streamer.h").exists()
        && lib_dir.join("codec/AudioCore.h").exists()
        && lib_dir.join("codec/video/vd/VDecApi.h").exists()
        && lib_dir.join("codec/video/vd/VoApi.h").exists()
        && lib_dir.join("libwebrtc-stream.a").exists()
        && lib_dir.join("libmbedtls.a").exists()
        && lib_dir.join("libmbedx509.a").exists()
        && lib_dir.join("libmbedcrypto.a").exists()
        && lib_dir.join("libVideoDec.so").exists()
        && elf_machine_matches_riscv(&lib_dir.join("libVideoDec.so"))
}

fn sound_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    lib_dir.join("codec/AudioCore.h").exists()
        && lib_dir.join("libAudioCore.so").exists()
        && elf_machine_matches_riscv(&lib_dir.join("libAudioCore.so"))
}

fn ipc_runtime_available(manifest_dir: &PathBuf) -> bool {
    let lib_dir = manifest_dir.join("lib");
    lib_dir.join("IpCammer.h").exists()
        && lib_dir.join("codec/video/vd/VDecApi.h").exists()
        && lib_dir.join("codec/video/vd/VoApi.h").exists()
        && lib_dir.join("libipcammer.so").exists()
        && elf_machine_matches_riscv(&lib_dir.join("libipcammer.so"))
        && lib_dir.join("libVideoDec.so").exists()
        && elf_machine_matches_riscv(&lib_dir.join("libVideoDec.so"))
}

fn link_indoor_runtime() {
    println!("cargo:rustc-link-lib=static=kr_indoor_bridge");
    println!("cargo:rustc-link-lib=static=kr_native_utils");
    for lib in [
        "mtrans",
        "d2d_protocol",
        "transport_adapter",
        "topology",
        "goblin_plc_trancore",
        "opus",
        "AudioCore",
        "VideoDec",
        "mpp_base",
        "mpp_decoder",
        "mpp_encoder",
        "mpp_ge",
        "mpp_ve",
    ] {
        println!("cargo:rustc-link-lib=dylib={lib}");
    }
}

fn add_sources(build: &mut cc::Build, sources: &[PathBuf]) {
    for source in sources {
        println!("cargo:rerun-if-changed={}", source.display());
        build.file(source);
    }
}

fn add_includes(build: &mut cc::Build, includes: &[PathBuf]) {
    for include in includes {
        build.include(include);
    }
}

fn compile_native_utils(manifest_dir: &PathBuf) {
    let shared_dir = manifest_dir.join("native/shared");
    let utils_dir = shared_dir.join("utils");
    let source_files = [
        utils_dir.join("event_queue.c"),
        utils_dir.join("bmem.c"),
        utils_dir.join("file_helper.c"),
        utils_dir.join("rb_tree.c"),
        utils_dir.join("log.c"),
        utils_dir.join("time_helper.c"),
        utils_dir.join("util.c"),
        utils_dir.join("wave_file_fmt.c"),
    ];

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .flag("-DOS_LINUX")
        .include(&shared_dir);

    add_sources(&mut build, &source_files);

    build.compile("kr_native_utils");
}

fn compile_indoor_bridge_real(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/indoor_bridge");
    let upstream_dir = bridge_dir.join("upstream");
    let common_dir = upstream_dir.join("common");
    let project_lib_dir = manifest_dir.join("lib");
    let shared_dir = manifest_dir.join("native/shared");

    let bridge_sources = [bridge_dir.join("kr_indoor_bridge.c")];
    let f1_common_sources = [
        common_dir.join("call/simple_control_route_runtime.c"),
        common_dir.join("call/simple_control_route_provider.c"),
        common_dir.join("call/simple_topology_route_provider.c"),
        common_dir.join("call/simple_call_app.c"),
        common_dir.join("core/simple_bmem_backend.c"),
        common_dir.join("core/simple_ipv4_utils.c"),
        common_dir.join("core/simple_log.c"),
        common_dir.join("core/simple_mem.c"),
        common_dir.join("core/simple_session.c"),
        common_dir.join("core/simple_session_manager.c"),
        common_dir.join("core/simple_utils_log_backend.c"),
        common_dir.join("core/simple_plc_addr.c"),
        common_dir.join("media/simple_codec.c"),
        common_dir.join("media/simple_media.c"),
        common_dir.join("media/simple_endpoint_provider.c"),
        common_dir.join("media/simple_media_profile_shape.c"),
        common_dir.join("media/simple_media_runtime.c"),
        common_dir.join("media/simple_media_session_endpoint.c"),
        common_dir.join("media/simple_media_resource_policy.c"),
        common_dir.join("media/simple_session_media_plan.c"),
        common_dir.join("media/simple_topology_audio.c"),
        common_dir.join("media/simple_topology_profile.c"),
        common_dir.join("d2d/simple_d2d_bus.c"),
        common_dir.join("d2d/simple_d2d_bus_debug.c"),
        common_dir.join("d2d/simple_d2d_service.c"),
        common_dir.join("d2d/simple_d2d_unlock.c"),
        common_dir.join("d2d/simple_d2d_indoor_registry.c"),
        common_dir.join("d2d/simple_d2d_indoor_registry_bus_app.c"),
        common_dir.join("d2d/simple_d2d_indoor_heartbeat.c"),
        common_dir.join("d2d/simple_d2d_business_runtime.c"),
        common_dir.join("d2d/simple_d2d_villa_heartbeat.c"),
    ];
    let f1_support_sources = [
        shared_dir.join("codec/opus/opus_codec.c"),
        shared_dir.join("codec/f1_video_codec.c"),
        shared_dir.join("socket_adapter/udp_socket.c"),
        shared_dir.join("socket_adapter/tcp_socket.c"),
        shared_dir.join("socket_adapter/plc_socket.c"),
    ];

    let bridge_includes = [
        bridge_dir.clone(),
        manifest_dir.join("native/webrtc_bridge"),
        shared_dir.clone(),
    ];
    let f1_support_includes = [
        shared_dir.join("utils"),
        shared_dir.join("socket_adapter"),
        shared_dir.join("codec"),
    ];
    let f1_common_includes = [
        common_dir.clone(),
        common_dir.join("core"),
        common_dir.join("call"),
        common_dir.join("media"),
        common_dir.join("d2d"),
    ];
    let sdk_includes = [
        project_lib_dir.join("transport"),
        project_lib_dir.join("mtrans/call"),
        project_lib_dir.join("mtrans"),
        project_lib_dir.clone(),
        project_lib_dir.join("protocol"),
        project_lib_dir.join("topology"),
        project_lib_dir.join("codec"),
        project_lib_dir.join("opus"),
    ];

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .flag("-DOS_LINUX");

    add_includes(&mut build, &bridge_includes);
    add_includes(&mut build, &f1_support_includes);
    add_includes(&mut build, &f1_common_includes);
    add_includes(&mut build, &sdk_includes);

    add_sources(&mut build, &bridge_sources);
    add_sources(&mut build, &f1_common_sources);
    add_sources(&mut build, &f1_support_sources);
    println!(
        "cargo:rerun-if-changed={}",
        bridge_dir.join("kr_indoor_bridge.h").display()
    );

    build.compile("kr_indoor_bridge");
}

fn compile_aiot_bridge_real(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/aiot_bridge");
    let project_lib_dir = manifest_dir.join("lib");
    let source = bridge_dir.join("kr_aiot_bridge.c");

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .flag("-DOS_LINUX")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .include(&project_lib_dir)
        .file(&source);

    println!("cargo:rerun-if-changed={}", source.display());
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("goblin_aiot.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("goblin_aiot_callx.h").display()
    );

    build.compile("kr_aiot_bridge");
}

fn compile_webrtc_bridge_real(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/webrtc_bridge");
    let project_lib_dir = manifest_dir.join("lib");
    let source = bridge_dir.join("kr_webrtc_bridge.c");

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .flag("-DOS_LINUX")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .include(&project_lib_dir)
        .include(project_lib_dir.join("codec"))
        .include(project_lib_dir.join("webrtc"))
        .file(&source);

    println!("cargo:rerun-if-changed={}", source.display());
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("webrtc/webrtc_streamer.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("codec/AudioCore.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        manifest_dir
            .join("native/shared/utils/circlebuf.h")
            .display()
    );

    build.compile("kr_webrtc_bridge");
}

fn compile_sound_bridge_real(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/sound_bridge");
    let project_lib_dir = manifest_dir.join("lib");
    let shared_dir = manifest_dir.join("native/shared");
    let source = bridge_dir.join("kr_sound_bridge.c");

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .flag("-DOS_LINUX")
        .include(&bridge_dir)
        .include(&shared_dir)
        .include(shared_dir.join("utils"))
        .include(project_lib_dir.join("codec"))
        .file(&source);

    println!("cargo:rerun-if-changed={}", source.display());
    println!(
        "cargo:rerun-if-changed={}",
        bridge_dir.join("kr_sound_bridge.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("codec/AudioCore.h").display()
    );

    build.compile("kr_sound_bridge");
}

fn compile_ipc_bridge_real(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/ipc_bridge");
    let project_lib_dir = manifest_dir.join("lib");
    let source = bridge_dir.join("kr_ipc_bridge.c");

    let mut build = cc::Build::new();
    build
        .warnings(false)
        .flag("-std=gnu99")
        .flag("-D_GNU_SOURCE")
        .flag("-D_FILE_OFFSET_BITS=64")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .include(&project_lib_dir)
        .include(project_lib_dir.join("codec"))
        .file(&source);

    println!("cargo:rerun-if-changed={}", source.display());
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("IpCammer.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("codec/video/vd/VDecApi.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("codec/video/vd/VoApi.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        project_lib_dir.join("codec/video/VideoDefs.h").display()
    );

    build.compile("kr_ipc_bridge");
}

fn compile_aiot_bridge_stub(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/aiot_bridge");
    let source = bridge_dir.join("kr_aiot_bridge_stub.c");

    println!("cargo:rerun-if-changed={}", source.display());
    cc::Build::new()
        .warnings(false)
        .flag("-std=gnu99")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .file(source)
        .compile("kr_aiot_bridge");
}

fn compile_webrtc_bridge_stub(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/webrtc_bridge");
    let source = bridge_dir.join("kr_webrtc_bridge_stub.c");

    println!("cargo:rerun-if-changed={}", source.display());
    cc::Build::new()
        .warnings(false)
        .flag("-std=gnu99")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .file(source)
        .compile("kr_webrtc_bridge");
}

fn compile_ipc_bridge_stub(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/ipc_bridge");
    let source = bridge_dir.join("kr_ipc_bridge_stub.c");

    println!("cargo:rerun-if-changed={}", source.display());
    cc::Build::new()
        .warnings(false)
        .flag("-std=gnu99")
        .include(&bridge_dir)
        .include(manifest_dir.join("native/shared"))
        .file(source)
        .compile("kr_ipc_bridge");
}

fn compile_sound_bridge_stub(manifest_dir: &PathBuf) {
    let bridge_dir = manifest_dir.join("native/sound_bridge");
    let source = bridge_dir.join("kr_sound_bridge_stub.c");

    println!("cargo:rerun-if-changed={}", source.display());
    cc::Build::new()
        .warnings(false)
        .flag("-std=gnu99")
        .include(&bridge_dir)
        .file(source)
        .compile("kr_sound_bridge");
}
