#include "driver.h"

NTSTATUS DeviceAdd(PKSDEVICE Device)
{
    DeviceExtension* ctx = (DeviceExtension*)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(DeviceExtension), POOL_TAG);
    if (!ctx) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(ctx, sizeof(DeviceExtension));
    ctx->KsDevice = Device;
    ctx->FrameWidth = FRAME_WIDTH;
    ctx->FrameHeight = FRAME_HEIGHT;
    ctx->FrameSize = FRAME_SIZE;
    ctx->FrameBuffer = (PUCHAR)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, ctx->FrameSize, POOL_TAG);
    if (!ctx->FrameBuffer) {
        ExFreePool(ctx);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(ctx->FrameBuffer, ctx->FrameSize);
    KeInitializeSpinLock(&ctx->FrameLock);

    Device->Context = ctx;

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\VirtualRTSPCamera");
    return IoCreateSymbolicLink(&symLink, &Device->PhysicalDeviceObject->DeviceName);
}

NTSTATUS DeviceStart(PKSDEVICE Device)
{
    return STATUS_SUCCESS;
}

void DeviceRemove(PKSDEVICE Device)
{
    DeviceExtension* ctx = (DeviceExtension*)Device->Context;
    if (!ctx) return;

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\VirtualRTSPCamera");
    IoDeleteSymbolicLink(&symLink);

    if (ctx->FrameBuffer) ExFreePool(ctx->FrameBuffer);
    ExFreePool(ctx);
}

NTSTATUS DeviceCreate(PKSDEVICE Device, PIRP Irp)
{
    return STATUS_SUCCESS;
}

NTSTATUS DeviceClose(PKSDEVICE Device, PIRP Irp)
{
    return STATUS_SUCCESS;
}

NTSTATUS DeviceControl(PKSDEVICE Device, PIRP Irp)
{
    DeviceExtension* ctx = (DeviceExtension*)Device->Context;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = irpSp->Parameters.DeviceIoControl.IoControlCode;

    if (code == IOCTL_SEND_FRAME) {
        ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

        if (inLen >= ctx->FrameSize && Irp->AssociatedIrp.SystemBuffer) {
            KIRQL irql;
            KeAcquireSpinLock(&ctx->FrameLock, &irql);
            RtlCopyMemory(ctx->FrameBuffer, Irp->AssociatedIrp.SystemBuffer, ctx->FrameSize);
            ctx->FrameValid = TRUE;
            KeReleaseSpinLock(&ctx->FrameLock, &irql);

            Irp->IoStatus.Information = ctx->FrameSize;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        }
    } else {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}
