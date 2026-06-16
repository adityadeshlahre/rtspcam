# PLAN.md

# RTSP Virtual Camera

> Goal: Expose an RTSP stream as a native Windows webcam that is detectable by browsers (WebRTC), desktop applications, and proprietary software.

---

# Philosophy

This project should be built **vertically**, not horizontally.

Avoid designing a large architecture up front.

Every milestone must produce a working executable that can be manually tested on a Windows VM.

No IPC.
No Shared Memory.
No Kernel driver.

Those are optimization steps and should only be introduced after a working virtual camera exists.

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
    ├── rtspcam.exe (Windows Service / daemon)
    │       connects to rtsp://host-ip/live
    │       exposes as virtual camera
    │
    └── Media Foundation Virtual Camera
              │
              ▼
         Chrome / Teams / OBS / any app
```

The RTSP server runs on the machine that **has the camera**.
The Windows machine runs the app — it connects to the RTSP URL and exposes it as a local webcam.

---

# Development Workflow

```
macOS (NeoVim)                        Host machine
    │                                     │
    ├── develop / test locally            ├── (optional) run mediamtx + ffmpeg
    │   (AppleClang + FFmpeg)             │   as test RTSP source
    │                                     │
    └── cross-compile rtspcam.exe ────────┘
              │
              ▼
    Windows VM (test target)
              │
              ├── rtspcam.exe --url rtsp://host-ip/live
              │
              └── verify in Chrome via getUserMedia()
```

- Write code on macOS or any dev machine.
- Run mediamtx on the host (or anywhere on network) as the RTSP source.
- Build Windows `.exe` with MinGW-w64 cross-compiler.
- Transfer `.exe` to Windows VM.
- Test `rtspcam.exe` pointing at the host's RTSP URL.
- Validate in Chrome via `navigator.mediaDevices.getUserMedia()`.

---

# Target Platforms

- Windows 11 ARM64 (run on VM)
- Windows 11 x64 (run on VM)

Build host: macOS (Apple Silicon)

Development language:

- C++20

Build system:

- CMake

Cross-compiler:

- MinGW-w64 (arm64 + x64)

IDE:

- Neo Vim (macOS)

---

# RTSP Test Server

Run [mediamtx](https://github.com/bluenviron/mediamtx) on the **host machine** (the one with the camera), or any machine on the network that has an RTSP source:

```
# On the host machine:
mediamtx
# publishes at rtsp://host-ip:8554/live

# Push a test video into it (e.g. a looped test pattern):
ffmpeg -re -f lavfi -i testsrc2=size=1920x1080:rate=30 \
       -c:v libx264 -tune zerolatency -f rtsp rtsp://host-ip:8554/live
```

Then on the Windows VM:

```
rtspcam.exe --url rtsp://host-ip:8554/live
```

Or use a real IP camera / any RTSP source on the network.

---

# Non Goals (Phase 1)

- Multi-camera support
- Configuration UI
- Audio
- Authentication manager
- Recording
- Streaming server

---

# MVP

A console application that:

```
RTSP
    │
    ▼
FFmpeg
    │
    ▼
Media Foundation Virtual Camera
    │
    ▼
Chrome / Edge / Teams / Proprietary Apps
```

If Chrome can successfully display the stream via:

```
navigator.mediaDevices.getUserMedia()
```

then Phase 1 is complete.

---

# Milestone 1

## Project Bootstrap

Repository:

```
rtspcam/

    CMakeLists.txt

    README.md

    docs/

    include/

    src/

    third_party/

    tests/
```

Deliverables:

- Builds successfully
- CI passes
- Empty executable runs

Success Criteria:

```
rtspcam.exe

=> exits successfully
```

---

# Milestone 2

## RTSP Client

Implement:

```
RTSPClient
```

Responsibilities:

- connect
- reconnect
- timeout handling

Use FFmpeg.

Deliverables:

```
rtspcam.exe --url rtsp://host-ip/live
```

Test: mediamtx runs on host machine, rtspcam connects to it from Windows VM.

Output:

```
Connected
Codec: H264
Resolution: 1920x1080
FPS: 30
```

No rendering.

No decoding validation beyond receiving packets.

Success Criteria:

Stable connection for 10 minutes.

---

# Milestone 3

## Decoder

Decode frames.

Convert to:

```
NV12
```

Maintain latest frame only.

No buffering.

Console output:

```
Decoded frame #1200

1920x1080

NV12

30 fps
```

Success Criteria:

Continuous decoding.

No memory leaks.

---

# Milestone 4

## Media Foundation Virtual Camera

Register:

```
Virtual RTSP Camera
```

No RTSP integration yet.

Generate synthetic frames:

```
black background

timestamp

frame counter

moving rectangle
```

Success Criteria:

Camera appears in:

- Camera App
- Chrome
- Edge
- Teams
- OBS
- Proprietary applications

---

# Milestone 5

## Connect Decoder to Camera

Replace synthetic frames.

Pipeline:

```
RTSP

↓

Decoder

↓

Virtual Camera
```

No IPC.

No services.

Single process.

Success Criteria:

Open:

https://webrtc.github.io/samples/src/content/getusermedia/gum/

Select:

```
Virtual RTSP Camera
```

Live RTSP video must appear.

---

# Milestone 6

## Robustness

Implement:

- reconnect
- packet loss recovery
- stream restart
- timeout detection

Stress test:

```
disconnect ethernet

reconnect

camera continues automatically
```

---

# Milestone 7

## Refactor

Only after Milestone 5 succeeds.

Split into:

```
RTSP Layer

Camera Layer

Application Layer
```

Introduce interfaces.

No behavioral changes.

---

# Future Phase

Only after MVP works.

```
Windows Service

↓

RTSP Decoder

↓

Shared Memory

↓

Camera Host

↓

Windows Camera Stack
```

---

# Directory Layout

```
rtspcam/

    CMakeLists.txt
    PLAN.md

    cmake/
        toolchain-x86_64-w64-mingw32.cmake
        toolchain-aarch64-w64-mingw32.cmake

    src/

        main.cpp

        rtsp/

        decoder/

        camera/
```

Keep it intentionally small.

Avoid premature abstraction.

---

# Coding Standards

- C++20
- RAII everywhere
- No global state
- No macros except platform guards
- Prefer std::expected/std::optional where appropriate
- std::span instead of raw pointers
- std::unique_ptr ownership by default
- Exceptions only at application boundaries

---

# Build Setup (macOS → Windows cross-compile)

## Prerequisites

```sh
brew install mingw-w64 ffmpeg pkg-config
```

## Build

```sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-x86_64-w64-mingw32.cmake
cmake --build build
```

Output: `build/rtspcam.exe` → transfer to Windows VM.

---

# Dependencies

Required:

- FFmpeg (for RTSP client + decoder)
- Windows SDK
- Media Foundation

Windows-only (bundled with SDK):

- DirectShow
- DShow header `ks.h`, `ksmedia.h`

Optional (later):

- fmt
- spdlog
- nlohmann/json

## macOS (development only)

- FFmpeg via Homebrew (for building/testing RTSP and decoder locally)
- AppleClang (system compiler)

---

# Testing Strategy

Every milestone must be independently runnable.

Never merge code that cannot be manually verified.

Expected progression:

```
Milestone 1

Build

✓

Milestone 2

RTSP Connect

✓

Milestone 3

Decode

✓

Milestone 4

Virtual Camera

✓

Milestone 5

RTSP → Camera

✓

Milestone 6

Robustness

✓

Milestone 7

Refactor

✓
```

---

# Design Principle

**Working software over architecture.**

The first objective is **not** to build a perfect framework.

The first objective is to make Windows enumerate:

```
Virtual RTSP Camera
```

and have a browser display the RTSP video through:

```
navigator.mediaDevices.getUserMedia()
```

Once that is proven, services, IPC, shared memory, plugins, configuration, installers, and multi-camera support can be added incrementally without changing the validated data path.
