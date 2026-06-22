/*
 * WDK 10.0.28000.0 ARM64 C compilation workarounds:
 * - CDECL: __cdecl on data (e.g. "extern const GUID CDECL GUID_NULL") is
 *   invalid C. On ARM64 __cdecl is a no-op, so empty is safe.
 * - BOOL/DWORD/BYTE/FLOAT: ntdef.h no longer pulls in windef.h in kernel
 *   mode. Define as macros to satisfy ks.h/function prototypes.
 */
#define CDECL
#define BOOL int
#define DWORD unsigned long
#define BYTE unsigned char
#define FLOAT float

#include <ntddk.h>
#include <ks.h>

/* ksmedia.h is broken in this WDK version on ARM64 (missing types,
 * anonymous struct/union issues). Define only what we need.
 * GUIDs are declared extern and resolved by ks.lib at link time. */

extern const GUID KSDATAFORMAT_TYPE_VIDEO;
extern const GUID KSDATAFORMAT_SUBTYPE_NV12;
extern const GUID KSDATAFORMAT_SPECIFIER_VIDEOINFO2;
extern const GUID PINNAME_VIDEO_CAPTURE;
extern const GUID KSCATEGORY_VIDEO_CAMERA;

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
