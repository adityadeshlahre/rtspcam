# PLAN.md

# RTSP Virtual Camera for Windows

> Goal: Expose an RTSP stream as a native Windows webcam that is detectable by browsers (WebRTC), desktop applications, and proprietary software.

---

# Philosophy

This project should be built **vertically**, not horizontally.

We avoid designing a large architecture up front.

Instead, every milestone must produce a working executable that can be manually tested.

No IPC.
No Windows Service.
No Shared Memory.

Those are optimization steps and should only be introduced after a working virtual camera exists.

---

# Target Platforms

- Windows 11 ARM64
- Windows 11 x64

Development language:

- C++20

Build system:

- CMake

IDE:

- Neo Vim

---

# Non Goals (Phase 1)

- Windows Service
- Multi-camera support
- Configuration UI
- Installer
- Audio
- Authentication manager
- Recording
- Streaming server
- Kernel mode driver

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
rtspcam.exe --url rtsp://127.0.0.1/live
```

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

src/

    main.cpp

    rtsp/

    decoder/

    camera/

include/

tests/

docs/
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

# Dependencies

Required:

- FFmpeg
- Windows SDK
- Media Foundation

Optional (later):

- fmt
- spdlog
- nlohmann/json

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
