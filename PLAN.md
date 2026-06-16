# PLAN.md

# RTSP Virtual Camera

> Goal: Expose an RTSP stream as a native Windows webcam that appears in **Device Manager** and is detectable by browsers (WebRTC), desktop applications, and proprietary software.

---

# Philosophy

This project should be built **vertically**, not horizontally.

Avoid designing a large architecture up front.

Every milestone must produce a working executable that can be manually tested on a Windows VM.

---

# Architecture

```
Host machine (camera source)
    │
    ├── mediamtx (RTSP server)
    │       publishes rtsp://host-ip/live
    │
    └── camera / screen capture / any RTSP source
              │
              │  RTSP over network
              ▼
Windows machine (runs rtspcam)
    │
    ├── rtspcam.exe (user-mode Windows Service)
    │       connects to rtsp://host-ip/live
    │       decodes frames → NV12
    │       sends frames to driver via IOCTL
    │       logs to %APPDATA%/rtspcam/main.log
    │
    ├── rtspcam.sys (kernel-mode AVStream driver)
    │       registers as "Virtual RTSP Camera"
    │       appears in Device Manager under "Cameras"
    │       receives NV12 frames from user-mode service
    │       exposes them as a standard video capture device
    │
    └── Any app (Chrome / Teams / OBS / Camera app)
              │
              ▼
         sees it as a real camera in Device Manager
```

The RTSP server runs on the machine that **has the camera**.
The Windows machine runs rtspcam as a Windows Service + kernel driver.

---

# Development Workflow

```
macOS (NeoVim)                        Host machine
    │                                     │
    ├── develop RTSP / decoder            ├── run mediamtx + ffmpeg
    │   test locally (AppleClang)         │   as test RTSP source
    │                                     │
    └── cross-compile rtspcam.exe ────────┘
              │
              ▼
    Windows VM (test target)
              │
              ├── build driver on VM (MSVC + WDK)
              │   or cross-compile .sys from macOS
              │
              ├── enable test signing:
              │   bcdedit /set testsigning on
              │
              ├── install driver:
              │   sc create rtspcam ... + devcon install
              │
              ├── install service:
              │   rtspcam.exe --install --url rtsp://host-ip/live
              │
              ├── verify in Device Manager:
              │   "Virtual RTSP Camera" under "Cameras"
              │
              └── check logs:
                  %APPDATA%/rtspcam/main.log
```

---

# Build Strategy

## Windows Driver (rtspcam.sys)

Must be built with MSVC + WDK (Windows Driver Kit).

Build on the Windows VM directly, or use MSVC cross-compiler from macOS.

Requires:
- Visual Studio 2022 Build Tools
- Windows Driver Kit (WDK)
- Enterprise WDK (for command-line builds)

```
# On Windows VM:
cmake -B build-driver -DDRIVER=ON
cmake --build build-driver
# Output: build-driver/rtspcam.sys
```

## User-mode service (rtspcam.exe)

Cross-compile from macOS with MinGW-w64:

```
brew install mingw-w64 ffmpeg pkg-config

cmake -B build-x64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-x86_64-w64-mingw32.cmake
cmake --build build-x64
# Output: build-x64/rtspcam.exe
```

## Local macOS dev build (RTSP/decoder only)

```
cmake -B build && cmake --build build
./build/rtspcam --console --url rtsp://host-ip/live
```

---

# Target Platforms

- Windows 11 ARM64
- Windows 11 x64

Build host (user-mode): macOS (Apple Silicon) with MinGW-w64
Build host (driver): Windows VM with MSVC + WDK

Languages: C++20 (user-mode), C (kernel-mode driver)

---

# RTSP Test Server

Run [mediamtx](https://github.com/bluenviron/mediamtx) on the **host machine**:

```
# On the host:
mediamtx
# publishes at rtsp://host-ip:8554/live

# Push a test pattern:
ffmpeg -re -f lavfi -i testsrc2=size=1920x1080:rate=30 \
       -c:v libx264 -tune zerolatency -f rtsp rtsp://host-ip:8554/live
```

On the Windows VM:

```
rtspcam.exe --console --url rtsp://host-ip:8554/live
```

Or use any real IP camera as the RTSP source.

---

# Non Goals (Phase 1)

- Multi-camera support
- Configuration UI
- Audio
- Authentication manager
- Recording
- Streaming server
- WHQL certification (test signing is fine)

---

# MVP

A Windows Service + Kernel Driver that:

```
RTSP
    │
    ▼
FFmpeg (receive + decode)
    │
    ▼
NV12 frame
    │  IOCTL
    ▼
AVStream Kernel Driver
    │
    ├── Visible in Device Manager
    │
    ▼
Chrome / Edge / Teams / OBS / Camera App
```

If Chrome can display the stream via:

```
navigator.mediaDevices.getUserMedia()
```

AND the device appears under "Cameras" in Device Manager, then the MVP is complete.

---

# Milestone 1 — Bootstrap ✓

## Done

- `CMakeLists.txt` — C++20, compiles cleanly
- `src/main.cpp` — exits with 0
- Cross-compile toolchain files for MinGW-w64

---

# Milestone 2 — RTSP Client ✓

## Done

- `RTSPClient` class using FFmpeg
- Connects to `rtsp://...`, reads packets
- Reconnect with backoff
- Prints codec, resolution, FPS to stdout

---

# Milestone 3 — Logger + Service Skeleton

## Logger

Implement a simple file logger:

```
src/log/logger.h
src/log/logger.cpp
```

Writes to `%APPDATA%/rtspcam/main.log` (or `./rtspcam.log` on macOS for dev).

Format:

```
[2026-06-20 14:30:01] [INFO]  Connected to rtsp://host-ip/live
[2026-06-20 14:30:01] [INFO]  Codec: H264, 1920x1080 @ 30fps
[2026-06-20 14:30:05] [ERROR] Connection lost, reconnecting...
[2026-06-20 14:30:07] [INFO]  Reconnected
```

Levels: DEBUG, INFO, WARN, ERROR.

## Windows Service Skeleton

Wrap the app in a Windows Service entry point:

```
src/service/service.h
src/service/service.cpp
src/service/service_install.cpp
```

- `ServiceMain` — entry point called by SCM
- `ServiceCtrlHandler` — handle stop/shutdown
- CLI: `--install`, `--uninstall`, `--console`, `--url`
- On `--install`: registers with `CreateService()`
- On start: reads URL from registry or CLI, runs the pipeline
- Logs every service event (start, stop, crash)
- Logger flushes on every write (crash-safe)

Deliverables:

```
rtspcam.exe --install --url rtsp://host-ip/live
net start rtspcam
# service starts, logs to %APPDATA%/rtspcam/main.log

rtspcam.exe --console --url rtsp://host-ip/live
# runs in foreground for development
```

Success Criteria:

- Service installs, starts, stops cleanly
- `main.log` contains startup entries
- Uninstall removes the service

---

# Milestone 4 — Decoder

Decode RTSP packets into raw NV12 frames.

Maintain latest frame only. No buffering.

```
src/decoder/decoder.h
src/decoder/decoder.cpp
```

- Uses FFmpeg `avcodec` for H264/H265 decoding
- Converts decoded frame to NV12 via `sws_scale`
- Thread-safe: one writer (decoder), one reader (driver)
- Logs frame count, resolution, fps every second

Console output:

```
Decoded frame #1200
1920x1080
NV12
30 fps
```

Success Criteria:

- Continuous decoding for 10 minutes
- No memory leaks
- All decode errors logged to `main.log`

---

# Milestone 5 — Kernel Driver (AVStream Virtual Camera)

Build a kernel-mode AVStream driver that registers a virtual camera.

This is the core differentiator — the camera appears in **Device Manager**.

```
driver/
    driver.h
    driver.c
    device.h
    device.c
    queue.h
    queue.c
    trace.h
    inf/
        rtspcam.inf       # driver installation file
        rtspcam.cat       # signed catalog (test cert for now)
```

## Driver Responsibilities

- Registers as a capture device using AVStream framework
- Appears under "Cameras" in Device Manager as "Virtual RTSP Camera"
- Creates a device interface that user-mode service can open
- Accepts NV12 frames from user-mode via IOCTL
- Provides frames to any app that opens the camera (Chrome, Teams, etc.)
- Handles power management, PnP events, multiple concurrent viewers

## User-mode → Driver Communication

```
rtspcam.exe                  rtspcam.sys
    │                            │
    ├── CreateFile() ──────────► ├── IRP_MJ_CREATE
    ├── DeviceIoControl() ─────► ├── IOCTL_SEND_FRAME
    │   (NV12 buffer)           │   (copy to AVStream pipeline)
    └── CloseHandle() ─────────► ├── IRP_MJ_CLOSE
```

## Test Signing

```
# On Windows VM (one time):
bcdedit /set testsigning on

# Sign driver with test cert:
certmgr.exe ...
inf2cat.exe ...

# Install:
devcon.exe install rtspcam.inf root\rtspcam
```

## Synthetic Frames First

Start with a test mode that generates synthetic frames internally
(no RTSP needed yet) — just to prove the driver works:

```
black background + timestamp + frame counter
```

Success Criteria:

- Device appears under "Cameras" in Device Manager
- Camera App can open it (shows test pattern)
- Chrome can enumerate it via `navigator.mediaDevices.enumerateDevices()`
- All driver load/unload events logged to `main.log`

---

# Milestone 6 — Connect Decoder to Driver

Replace synthetic frames with decoded RTSP frames.

Pipeline:

```
rtspcam.exe (user-mode)
    RTSP → Decoder → NV12 frame → IOCTL → rtspcam.sys (kernel)
                                              │
                                              ▼
                                         AVStream pipeline
                                              │
                                              ▼
                                         Chrome / Teams / etc
```

- Decoder writes latest NV12 frame
- Service sends it to driver via `DeviceIoControl`
- Driver enqueues into AVStream capture pipeline
- Frame queue in driver: latest frame only (no accumulation)

Success Criteria:

- Device Manager shows "Virtual RTSP Camera"
- Chrome displays live RTSP video via `getUserMedia()`
- Teams detects it as a camera
- All pipeline stats logged to `main.log`

---

# Milestone 7 — Robustness

- Auto-reconnect on RTSP disconnect (already partially done)
- Packet loss recovery (FFmpeg's built-in error concealment)
- Stream restart if decoder crashes
- Timeout detection with interrupt callback
- Watchdog: if no frames for 10 seconds, log and restart pipeline
- Driver crash recovery: if `.sys` faults, service detects and reloads
- Graceful service shutdown: sends black frame, then closes driver handle

Stress test:

```
disconnect host network → wait → reconnect
→ camera continues automatically
→ Device Manager still shows the device
```

---

# Milestone 8 — Windows Service Finalization

- Service runs as `NT AUTHORITY\LocalService` (least privilege)
- On startup: loads driver via `sc start`, waits for device arrival
- On shutdown: sends black frames, stops driver cleanly
- Registry: read URL, log level, reconnect params from `HKLM\Software\rtspcam`
- Installer (WiX) bundles `.exe` + `.sys` + `.inf` + `.cat`
- Uninstaller removes driver, service, and log files

---

# Driver Signing (Production)

For Device Manager visibility without test mode:

```
Development:   bcdedit /set testsigning on
               Self-signed test certificate

Production:    EV Code Signing Certificate (~$300/year)
               Microsoft Hardware Dev Center submission
               Attested signing via Azure DevOps (cheaper)
```

Phase 1 uses test signing only.

---

# Directory Layout

```
rtspcam/
    CMakeLists.txt
    PLAN.md

    cmake/
        toolchain-x86_64-w64-mingw32.cmake
        toolchain-aarch64-w64-mingw32.cmake

    src/                          # user-mode service
        main.cpp

        log/
            logger.h
            logger.cpp

        rtsp/
            rtsp_client.h
            rtsp_client.cpp

        decoder/
            decoder.h
            decoder.cpp

        service/
            service.h
            service.cpp
            service_install.cpp

    driver/                       # kernel-mode driver
        driver.c
        driver.h
        device.c
        device.h
        queue.c
        queue.h
        trace.h
        inf/
            rtspcam.inf
```

Keep it intentionally small. Add files only when a milestone requires them.

---

# Coding Standards

User-mode (C++20):
- RAII everywhere
- No global state
- No macros except platform guards
- Prefer `std::expected` / `std::optional` where appropriate
- `std::span` instead of raw pointers
- `std::unique_ptr` ownership by default
- Exceptions only at application boundaries
- Every error path must be logged before returning

Kernel-mode (C, with MSVC):
- WDK coding conventions (__drv_ annotations, SAL)
- No C++ exceptions
- No STL
- Pool tagging for memory tracking
- Driver Verifier compliance from day one

---

# Dependencies

User-mode service:
- FFmpeg (RTSP client + decoder)
- Windows SDK

Kernel driver:
- Windows Driver Kit (WDK)
- AVStream library (built into WDK)

Tools:
- MinGW-w64 (cross-compiler on macOS)
- Visual Studio 2022 Build Tools (on Windows VM)
- devcon.exe (driver installation testing)

macOS (development only):
- FFmpeg via Homebrew
- AppleClang (system compiler)

---

# Logging Specification

```
Path:  %APPDATA%/rtspcam/main.log
       (falls back to ./rtspcam.log on macOS / dev)

Format:
[YYYY-MM-DD HH:MM:SS] [LEVEL] message

Levels:
DEBUG   - frame stats, verbose info
INFO    - connection, reconnection, startup/shutdown
WARN    - packet loss, decode glitches, minor errors
ERROR   - connection failures, decode failures, crashes
FATAL   - unhandled exceptions, service termination

Behavior:
- Append mode (never truncate)
- Flush on every write (kernel buffer, no user-mode buffering)
- Max file size: 10 MB (rotate to main.1.log, keep 3 archives)
- Log level controlled by --verbose flag or registry
```

---

# Testing Strategy

Every milestone must be independently runnable on the Windows VM.

Never merge code that cannot be manually verified.

Expected progression:

```
M1  Bootstrap                 ✓
M2  RTSP Client               ✓
M3  Logger + Service          ⬚
M4  Decoder                   ⬚
M5  Kernel Driver (AVStream)  ⬚
M6  Decoder → Driver          ⬚
M7  Robustness                ⬚
M8  Service Finalization      ⬚
```

---

# Design Principle

**Working software over architecture.**

The first objective is **not** to build a perfect framework.

The first objective is to make Windows enumerate in Device Manager:

```
Virtual RTSP Camera
```

and have a browser display the RTSP video through:

```
navigator.mediaDevices.getUserMedia()
```

Once that is proven, services, IPC, shared memory, plugins, configuration, installers, and multi-camera support can be added incrementally without changing the validated data path.
