using System.Runtime.InteropServices;

namespace rtspcam;

public sealed class DriverIO(ILogger logger) : IDisposable
{
    private const string DevicePath = @"\\.\VirtualRTSPCamera";
    private const uint IoctlSendFrame = 0x80002000;

    private SafeHandle? _device;

    public void Open()
    {
        _device = NativeMethods.CreateFile(
            DevicePath,
            0xC0000000, // GENERIC_READ | GENERIC_WRITE
            0,
            nint.Zero,
            3, // OPEN_EXISTING
            0x80, // FILE_ATTRIBUTE_NORMAL
            nint.Zero);

        if (_device?.IsInvalid == true)
        {
            logger.LogWarning("Driver not available (device not found). Frames will not be sent.");
            _device = null;
            return;
        }

        logger.LogInformation("Driver opened");
    }

    public unsafe void SendFrame(Nv12Frame frame)
    {
        if (_device == null) return;

        var totalSize = frame.Planes[0].Length + frame.Planes[1].Length + frame.Planes[2].Length;
        var buffer = new byte[totalSize];

        Buffer.BlockCopy(frame.Planes[0], 0, buffer, 0, frame.Planes[0].Length);
        Buffer.BlockCopy(frame.Planes[1], 0, buffer, frame.Planes[0].Length, frame.Planes[1].Length);
        Buffer.BlockCopy(frame.Planes[2], 0, buffer, frame.Planes[0].Length + frame.Planes[1].Length, frame.Planes[2].Length);

        fixed (byte* pBuf = buffer)
        {
            var success = NativeMethods.DeviceIoControl(
                _device,
                IoctlSendFrame,
                (nint)pBuf,
                buffer.Length,
                nint.Zero,
                0,
                out _,
                nint.Zero);

            if (!success)
            {
                var err = Marshal.GetLastPInvokeError();
                logger.LogWarning("Driver IOCTL failed: {Error}", err);
            }
        }
    }

    public void Dispose()
    {
        _device?.Close();
        logger.LogInformation("Driver closed");
    }
}
