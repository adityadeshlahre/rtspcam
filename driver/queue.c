#include "driver.h"

NTSTATUS PinCreate(PKSPIN Pin, PIRP Irp)
{
    return STATUS_SUCCESS;
}

void PinProcess(PKSPIN Pin, PKSPROCESSPIN_INDEXENTRY Index)
{
    DeviceExtension* ctx = (DeviceExtension*)Pin->KsDevice->Context;

    for (ULONG i = 0; i < Index->PinCount; i++) {
        PKSPROCESSPIN processPin = &Index->Pins[i];
        if (processPin->BytesUsed) continue;

        PKSSTREAM_HEADER header = (PKSSTREAM_HEADER)processPin->StreamHeader;
        PUCHAR data = (PUCHAR)processPin->Data;

        if (processPin->FrameExtent >= ctx->FrameSize) {
            KIRQL irql;
            KeAcquireSpinLock(&ctx->FrameLock, &irql);

            if (ctx->FrameValid) {
                RtlCopyMemory(data, ctx->FrameBuffer, ctx->FrameSize);
                header->DataUsed = ctx->FrameSize;
            } else {
                RtlZeroMemory(data, ctx->FrameSize);
                header->DataUsed = ctx->FrameSize;
            }

            KeReleaseSpinLock(&ctx->FrameLock, &irql);

            header->Size = sizeof(KSSTREAM_HEADER);
            header->PresentationTime.Numerator = 1;
            header->PresentationTime.Denominator = 10000000;
            header->PresentationTime.Time = 0;
            header->OptionsFlags = 0;
            processPin->BytesUsed = ctx->FrameSize;
        }
    }
}
