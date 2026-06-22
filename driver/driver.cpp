#include "driver.h"

static KSPIN_DISPATCH PinDispatch;
static KSDATARANGE VideoRange;
static KSPIN_INTERFACE PinInterface;
static KSPIN_DESCRIPTOR_EX PinDesc;
static KSFILTER_DESCRIPTOR FilterDesc;
static KSDEVICE_DISPATCH DeviceDispatch;
static KSDEVICE_DESCRIPTOR DeviceDesc;

static void InitDescriptors()
{
    PinDispatch.Create = PinCreate;
    PinDispatch.Process = PinProcess;

    VideoRange.FormatSize = sizeof(KSDATARANGE);
    VideoRange.MajorFormat = &KSDATAFORMAT_TYPE_VIDEO;
    VideoRange.SubFormat = &KSDATAFORMAT_SUBTYPE_NV12;
    VideoRange.Specifier = &KSDATAFORMAT_SPECIFIER_VIDEOINFO2;

    PinInterface = KSINTERFACE_STANDARD_STREAMING;

    static PKSDATARANGE ranges[] = { &VideoRange, nullptr };
    static PKSPIN_INTERFACE ifaces[] = { &PinInterface, nullptr };

    PinDesc.Dispatch = &PinDispatch;
    PinDesc.PinId = 0;
    PinDesc.InstancesPossible = KSINSTANCE_INDETERMINATE;
    PinDesc.InstancesNeeded = 1;
    PinDesc.Communication = KSCOMMUNICATION_SOURCE;
    PinDesc.Category = &PINNAME_VIDEO_CAPTURE;
    PinDesc.Name = &KSCATEGORY_VIDEO_CAMERA;
    PinDesc.DataFlow = KSPIN_DATAFLOW_OUT;
    PinDesc.DataRanges = ranges;
    PinDesc.Interfaces = ifaces;
    PinDesc.Mediums = nullptr;
    PinDesc.DataRangesCount = 1;
    PinDesc.InterfacesCount = 1;
    PinDesc.MediumsCount = 0;
    PinDesc.Flags = KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_STREAMING;

    FilterDesc.PinDescriptorsCount = 1;
    FilterDesc.PinDescriptors = &PinDesc;

    DeviceDispatch.Add = DeviceAdd;
    DeviceDispatch.Start = DeviceStart;
    DeviceDispatch.Remove = DeviceRemove;
    DeviceDispatch.Create = DeviceCreate;
    DeviceDispatch.Close = DeviceClose;
    DeviceDispatch.DeviceIoControl = DeviceControl;

    DeviceDesc.Dispatch = &DeviceDispatch;
    DeviceDesc.FilterDescriptors = &FilterDesc;
    DeviceDesc.FilterDescriptorsCount = 1;
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    InitDescriptors();
    return KsInitializeDriver(DriverObject, RegistryPath, &DeviceDesc);
}
