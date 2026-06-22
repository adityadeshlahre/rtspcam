# PLAN.md

## Goal

Expose an RTSP stream as a native Windows webcam that appears in **Device Manager** and is detectable by browsers (WebRTC), desktop applications, and proprietary software.

---

## Architecture

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
    ├── rtspcam.exe (user-mode Windows Service / C# .NET)
    │       connects to rtsp://host-ip/live
    │       decodes frames → NV12
    │       sends frames to driver via IOCTL
    │       logs to %APPDATA%/rtspcam/main.log
    │
    ├── rtspcam.sys (kernel-mode AVStream driver / C)
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

The RTSP server runs on the machine that **has the camera**. The Windows machine runs rtspcam as a Windows Service + kernel driver.

---

## Development Workflow

```
macOS (NeoVim)                        Host machine
    │                                     │
    ├── develop RTSP / decoder            ├── run mediamtx + ffmpeg
    │   dotnet build (macOS SDK)          │   as test RTSP source
    │                                     │
    └── dotnet publish -r win-x64 ────────┘
              │
              ▼
    Windows VM (test target)
              │
              ├── build driver on VM (MSVC + WDK)
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

## Build Strategy

### User-mode service (C# .NET)

Built from macOS with `dotnet` SDK:

```sh
# Build (any OS)
dotnet build src/rtspcam/rtspcam.csproj

# Cross-compile for Windows
dotnet publish src/rtspcam/rtspcam.csproj -r win-x64 --self-contained -c Release -o publish/win-x64
```

Output: `publish/win-x64/rtspcam.exe` (self-contained, no .NET runtime required on target)

### Kernel driver (C / AVStream)

Built on Windows VM with MSVC + WDK. Cannot be cross-compiled from macOS.

---

## RTSP Test Server

Run [mediamtx](https://github.com/bluenviron/mediamtx) on the **host machine**:

```sh
mediamtx
# publishes at rtsp://host-ip:8554/live

# Push a test pattern:
ffmpeg -re -f lavfi -i testsrc2=size=1920x1080:rate=30 \
       -c:v libx264 -tune zerolatency -f rtsp rtsp://host-ip:8554/live
```

On the Windows VM:

```sh
rtspcam.exe --console --url rtsp://host-ip:8554/live
```

---

## Non Goals (Phase 1)

- Multi-camera support
- Configuration UI
- Audio
- Authentication manager
- Recording
- Streaming server
- WHQL certification (test signing is fine)

---

## MVP

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

If Chrome can display the stream via `navigator.mediaDevices.getUserMedia()` AND the device appears under "Cameras" in Device Manager, then the MVP is complete.

---

## Milestones

### Milestone 1 — Bootstrap ✓

- `rtspcam.slnx` — .NET solution file
- `src/rtspcam/` — worker project with FFmpeg.AutoGen NuGet package
- `Program.cs` — Windows Service entry point + DI container
- `Service.cs` — BackgroundService orchestrating the pipeline
- `NativeMethods.cs` — kernel32.dll P/Invoke (CreateFile, DeviceIoControl, CloseHandle)

### Milestone 2 — RTSP Client ✓

- `RtspClient.cs` — connects to RTSP URL, reads packets via `avformat_open_input` / `av_read_frame`
- TCP transport via URL query param `?rtsp_transport=tcp`
- Reports codec, resolution, FPS on connect

### Milestone 3 — Logger + Service ✓

- Microsoft.Extensions.Logging with file output to `%APPDATA%/rtspcam/main.log`
- `appsettings.json` — RTSP:Url and ReconnectDelay config
- Windows Service lifecycle (start/stop via ServiceController or `net start/stop`)

### Milestone 4 — Decoder ✓

- `Decoder.cs` — `avcodec` H264/H265 decode → `sws_scale` to NV12
- `Nv12Frame` record — planar YUV 4:2:0 byte arrays
- `DriverIO.cs` — `CreateFile(\\.\VirtualRTSPCamera)` + `DeviceIoControl` (driver not yet built)

### Milestone 5 — Kernel Driver (AVStream Virtual Camera) ⬚

Build a kernel-mode AVStream driver that registers a virtual camera. See `driver/` directory.

### Milestone 6 — Connect Decoder to Driver ⬚

Replace synthetic frames with decoded RTSP frames via IOCTL.

### Milestone 7 — Robustness ⬚

Auto-reconnect, packet loss recovery, watchdog, driver crash recovery.

### Milestone 8 — Windows Service Finalization ⬚

Least-privilege service account, registry config, WiX installer.

---

## Directory Layout

```
rtspcam/
    PLAN.md
    rtspcam.slnx
    publish/                       # dotnet publish output
    third_party/
        ffmpeg-win64/              # Windows FFmpeg DLLs + .lib/.h
    src/
        rtspcam/                   # user-mode service (C# .NET)
            Program.cs
            Service.cs
            RtspClient.cs
            Decoder.cs
            DriverIO.cs
            NativeMethods.cs
            rtspcam.csproj
            appsettings.json
    driver/                        # kernel-mode driver (C, AVStream)
        driver.c / driver.h
        device.c / device.h
        queue.c / queue.h
        trace.h
        inf/
            rtspcam.inf
```

---

## Dependencies

### User-mode service

- .NET 10 SDK (macOS Homebrew: `brew install dotnet`)
- FFmpeg.AutoGen 8.1.0 (NuGet)
- Windows FFmpeg DLLs in `third_party/ffmpeg-win64/`

### Kernel driver

- Windows Driver Kit (WDK) — built on Windows VM

---

## Driver Signing

Personal use only — enable test signing on the Windows VM:

```sh
bcdedit /set testsigning on
```

No production signing needed.

---

## Design Principle

**Working software over architecture.**

The first objective is to make Windows enumerate `Virtual RTSP Camera` in Device Manager and have a browser display the RTSP video through `getUserMedia()`. Once proven, additional features can be added incrementally.
