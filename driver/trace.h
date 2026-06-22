#pragma once

#define WPP_CONTROL_GUIDS                                          \
    WPP_DEFINE_CONTROL_GUID(                                       \
        RtspCamTraceGuid,                                          \
        (5d0b1e8c, 2a3f, 4b8a, 9c1d, 2e3f4a5b6c7d),              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                               \
        WPP_DEFINE_BIT(TRACE_DEVICE)                               \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                \
        WPP_DEFINE_BIT(TRACE_IOCTL)                                \
    )
