#pragma once

#include <ntddk.h>
#include <ks.h>
#include <ksmedia.h>

#define POOL_TAG 'tprR'
#define FRAME_WIDTH  1280
#define FRAME_HEIGHT 720
#define FRAME_SIZE   (FRAME_WIDTH * FRAME_HEIGHT * 3 / 2)

#define IOCTL_SEND_FRAME \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct DeviceExtension {
    PKSDEVICE           KsDevice;
    PUCHAR              FrameBuffer;
    ULONG               FrameWidth;
    ULONG               FrameHeight;
    ULONG               FrameSize;
    BOOLEAN             FrameValid;
    KSPIN_LOCK          FrameLock;
};

extern "C" {
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DeviceAdd(PKSDEVICE);
NTSTATUS DeviceStart(PKSDEVICE);
void DeviceRemove(PKSDEVICE);
NTSTATUS DeviceCreate(PKSDEVICE, PIRP);
NTSTATUS DeviceClose(PKSDEVICE, PIRP);
NTSTATUS DeviceControl(PKSDEVICE, PIRP);
NTSTATUS PinCreate(PKSPIN, PIRP);
void PinProcess(PKSPIN, PKSPROCESSPIN_INDEXENTRY);
}
