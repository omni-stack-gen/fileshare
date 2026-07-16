# Project Archive Structure

This directory contains extracted archives of multiple firmware and application projects. Below is a summary of each project and their relationships.

---

## 📁 Project Overview

### 1. **EL_GW_C0_2026_07_07** (C Project)
**Type:** Audio Codec Library  
**Language:** C  
**Date:** July 7, 2026  

**Contents:**
- Multiple audio codec implementations (G.726, G.711, FDK AAC)
- Header files and codec utilities (`intmath`, `attributes`, `common`)
- Comprehensive codec processing library

**Purpose:** Provides audio encoding/decoding capabilities for communication systems.

---

### 2. **P-VCU** (C Project)
**Type:** Video Communication Unit  
**Language:** C  
**Build System:** CMake  

**Contents:**
- Main application (`application.c`, `main.c`)
- Tuya IoT SDK integration (`tuya/` directory)
- RTP streaming support (`RTP/` directory)
- Build scripts and test utilities
- Memory info monitoring tools

**Key Features:**
- Video communication protocol support
- Tuya cloud platform integration
- Doorbell functionality
- SD card album management
- Status and DP (Data Point) callbacks

**Purpose:** Video communication unit with IoT cloud connectivity (Tuya platform).

---

### 3. **TD-R34L** (C Project)
**Type:** Firmware for F1 Chip  
**Language:** C  
**Documentation:** README.md, ChangeLog.md  
**Target Chip:** F1 (7-inch display)  

**Contents:**
- Application code (`app/` directory - 13 subdirectories)
- Function libraries (`func_library/` - 7 subdirectories)
  - OTA (Over-The-Air) update functionality
  - MD5 hashing
  - Lua utility libraries
  - Third-party F1 codec APIs (video decode, audio)

**Key Features:**
- OTA update mechanism
- Lua scripting support
- Video and audio codec APIs
- PLC (Programmable Logic Controller) support

**Build Instructions:**
```bash
# Modify platform-specific paths in CMakeLists.txt
./cmake_build.sh <platform>  # e.g., NX5, F1
```

**Purpose:** Core firmware for F1-based smart home display device.

---

### 4. **f1_room_code** (Rust Project)
**Type:** UI Application for F1 Chip  
**Language:** Rust  
**Build System:** Cargo  
**UI Framework:** Slint  
**Date:** July 10, 2026  

**Contents:**
- Main Rust application (`src/` directory)
- Native code (`native/` directory)
- Codec libraries (`lib/codec/` - audio and video)
- Asset files (fonts, images)
- Build script (`build.rs`)
- AI agent memory and workflow state tracking

**Key Features:**
- Slint-based UI framework
- Font support (1000-character TTF)
- Custom codec integration
- IPC (Inter-Process Communication) support
- Comprehensive documentation (Change.log, SESSION_HANDOFF.md, AGENTS.md)

**Build Instructions:**
```bash
export SLINT_FONT_PATH=/path/to/1000.TTF
source ./lib.sh
cd target/riscv64gc-unknown-linux-gnu/release
./kr_room_ui
```

**Testing Mode:**
```bash
# Enable test mode on eth1
export KR_ROOM_TEST_CLOUD_IFACE=eth1

# Switch AIOT platform address (temporary)
export KR_ROOM_AIOT_SERVER_ADDR=43.108.59.118:30007

# Dump H.264 bridge stream
export KR_ROOM_DUMP_BRIDGE_H264=1
export KR_ROOM_DUMP_BRIDGE_H264_DIR=/tmp/kr_room_bridge_dump
```

**Purpose:** Rich UI application for F1-based smart room controller using Rust and Slint.

---

### 5. **x9_door_c_project** (C Project)
**Type:** X9 Door Control Firmware (Original/C Version)  
**Language:** C  
**Build System:** GNU Autotools (configure, make)  
**Date:** July 10, 2026  

**Contents:**
- Configuration scripts (`configure`, `configure.ac`)
- Build scripts (`build.sh`, `build_door_service.sh`)
- Documentation (`DESIGN.md`, `PROJECT_CONTEXT.md`, `TASK.md`, `ChangeLog`)
- Source files and dependencies
- Third-party libraries (Opus audio codec, Lua)
- Documentation directory with specs and design docs

**Key Components:**
- Door service implementation
- Audio codec (Opus) for communication
- Lua scripting support
- IPC (Inter-Process Communication) mechanisms

**Build Instructions:**
```bash
./autogen.sh
./configure
make
# or
./build_door_service.sh
```

**Purpose:** Door controller firmware written in C with audio/Lua support for X9 platform.

---

### 6. **x9_10_door_rust_project** (Rust Project)
**Type:** X9 Door Control Firmware (Rust Version/Modern)  
**Language:** Rust  
**Build System:** Cargo  
**Date:** July 10, 2026  

**Contents:**
- Main Rust application (`src/` directory)
  - Routes (`routes.rs`)
  - IPC communication (`ipc.rs`)
  - Controllers (clock, QR code, input)
  - Native code integration
- Asset files (screenshots: 1.png, 2.png, 3.png)
- Comprehensive documentation
  - `CLAUDE.md` - AI assistant context
  - `HANDOVER.md` - Project handover notes
  - `X9_DOOR_IPC_Rust侧开发规范.md` - Development specifications (Chinese)
  - `AGENTS.md`, `TASK.md`, `Change.log`
- Git repository included

**Key Components:**
- Rust-based door control system
- IPC for inter-process communication
- QR code support
- Clock and input handling
- Comprehensive specifications document

**Purpose:** Modern Rust implementation of X9 door controller with improved architecture and development practices.

---

## 🔗 Project Relationships & Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│                    SHARED COMPONENTS                         │
│  (Audio Codecs, Tuya SDK, Lua, Opus, etc.)                  │
└────────────┬──────────────────────────┬──────────────────────┘
             │                          │
    ┌────────▼──────────┐      ┌────────▼──────────┐
    │   F1 Chip Layer   │      │   X9 Chip Layer   │
    └────────┬──────────┘      └────────┬──────────┘
             │                          │
    ┌────────▼──────────┐      ┌────────▼──────────────────┐
    │  Firmware Layer   │      │  Firmware Layer           │
    │  ┌──────────────┐ │      │  ┌──────────────────────┐ │
    │  │   TD-R34L    │ │      │  │ x9_door_c_project    │ │
    │  │ (C Firmware) │ │      │  │ (C - Original)       │ │
    │  └──────┬───────┘ │      │  │                      │ │
    │         │         │      │  │ x9_10_door_rust_... │ │
    │         │         │      │  │ (Rust - Modern)      │ │
    │         │         │      │  └──────────────────────┘ │
    │  ┌──────▼───────┐ │      │                            │
    │  │ f1_room_code │ │      │  (Parallel Versions)      │
    │  │ (Rust UI)    │ │      └────────────────────────────┘
    │  └──────────────┘ │
    │                   │
    └───────────────────┘
             │
    ┌────────▼──────────┐
    │   Communication   │
    │  ┌──────────────┐ │
    │  │  P-VCU       │ │
    │  │  (Video Unit)│ │
    │  │ + IPC + Tuya │ │
    │  └──────────────┘ │
    │                   │
    │  ┌──────────────┐ │
    │  │ EL_GW_C0     │ │
    │  │ (Codecs)     │ │
    │  └──────────────┘ │
    └───────────────────┘
```

### Layer Description:

1. **Shared Components Layer:**
   - Audio/Video codecs (EL_GW_C0_2026_07_07)
   - Communication APIs and protocols

2. **Hardware Target Layer:**
   - **F1 Platform:** 7-inch display smart controller
   - **X9 Platform:** Door control system

3. **Firmware Layer:**
   - **TD-R34L:** Core F1 firmware
   - **x9_door_c_project:** Original X9 door firmware (C)
   - **x9_10_door_rust_project:** Modern X9 door firmware (Rust rewrite)

4. **Application Layer:**
   - **f1_room_code:** Rust-based UI for F1 smart room
   - **P-VCU:** Video communication unit with IoT cloud integration

---

## 📊 Quick Comparison

| Project | Language | Platform | Type | Status |
|---------|----------|----------|------|--------|
| EL_GW_C0_2026_07_07 | C | Shared | Audio Codec Lib | Core Component |
| P-VCU | C | Shared | Video Comm Unit | Core Component |
| TD-R34L | C | F1 | Firmware | Active |
| f1_room_code | Rust | F1 | Application/UI | Active |
| x9_door_c_project | C | X9 | Firmware | Legacy |
| x9_10_door_rust_project | Rust | X9 | Firmware | Modern |

---

## 🛠️ Technology Stack

- **Languages:** C, Rust
- **Build Systems:** CMake, GNU Autotools, Cargo
- **Communication:** RTP, IPC protocols, IoT (Tuya)
- **UI Framework:** Slint (Rust)
- **Scripting:** Lua
- **Audio:** Opus, G.726, G.711, FDK AAC
- **Video:** H.264 encoding/decoding
- **Hardware:** F1 chip (7"), X9 platform
- **Cloud Integration:** Tuya IoT platform

---

## 📝 Key Integration Points

1. **Audio/Video Processing:**
   - EL_GW_C0_2026_07_07 (codecs) → Used by P-VCU, TD-R34L, door projects
   - Opus codec integrated into X9 door projects

2. **IPC Communication:**
   - P-VCU uses IPC for video communication
   - x9_10_door_rust_project (routes.rs, ipc.rs) uses IPC
   - f1_room_code integrates IPC mechanisms

3. **IoT Integration:**
   - P-VCU connects to Tuya platform
   - f1_room_code can switch AIOT platforms
   - x9 projects may use similar cloud connectivity

4. **Firmware Updates:**
   - TD-R34L includes OTA mechanism
   - Likely shared by other firmware projects

---

## 📂 Total Archive Contents

- **6 Top-level folders** (after extraction)
- **2 F1-based projects** (TD-R34L firmware + f1_room_code UI)
- **2 X9-based projects** (C and Rust versions)
- **2 Shared component libraries** (codecs + video unit)
- **Multiple language implementations:** C, Rust, Python scripts

---

## 🔍 For More Information

Each project folder contains:
- Detailed documentation (DESIGN.md, README.md, TASK.md)
- Build scripts and configuration
- Source code with inline comments
- Change logs and development notes
- Agent/AI assistant context files (.claude/, AGENTS.md)

Start with individual project README.md and documentation files for deep dives into specific components.
