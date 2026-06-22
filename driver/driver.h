/*
 * WDK 10.0.28000.0 ARM64 workarounds:
 * - CDECL on data declarations (e.g. "extern const GUID CDECL GUID_NULL")
 *   is valid C++ but invalid C. On ARM64 __cdecl is a no-op, so empty is safe.
 * - BOOL is not defined by ntdef.h in kernel mode on ARM64, but ks.h
 *   uses it as a function parameter type. Macro avoids redefinition conflict.
 */
#define CDECL
#define BOOL int

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

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DeviceAdd(PKSDEVICE);
NTSTATUS DeviceStart(PKSDEVICE);
void DeviceRemove(PKSDEVICE);
NTSTATUS DeviceCreate(PKSDEVICE, PIRP);
NTSTATUS DeviceClose(PKSDEVICE, PIRP);
NTSTATUS DeviceControl(PKSDEVICE, PIRP);
NTSTATUS PinCreate(PKSPIN, PIRP);
void PinProcess(PKSPIN, PKSPROCESSPIN_INDEXENTRY);
