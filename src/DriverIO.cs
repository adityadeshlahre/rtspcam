using System.Runtime.InteropServices;

namespace rtspcam;

public sealed class DriverIO(ILogger logger) : IDisposable
{
    private const string DevicePath = @"\\.\VirtualRTSPCamera";
    private const uint IoctlSendFrame = 0x80002000;
    private const uint GenericReadWrite = 0xC0000000;
    private const uint OpenExisting = 3;
    private const uint FileAttributeNormal = 0x80;

    private nint _device;

    public void Open()
    {
        _device = NativeMethods.CreateFile(
            DevicePath,
            GenericReadWrite,
            0,
            nint.Zero,
            OpenExisting,
            FileAttributeNormal,
            nint.Zero);

        if (_device == new nint(-1))
        {
            logger.LogWarning("Driver not available — device not found. Frames will not be sent.");
            _device = nint.Zero;
            return;
        }

        logger.LogInformation("Driver opened");
    }

    public unsafe void SendFrame(Nv12Frame frame)
    {
        if (_device == nint.Zero) return;

        var totalSize = frame.Planes[0].Length + frame.Planes[1].Length + frame.Planes[2].Length;
        var buffer = new byte[totalSize];

        Buffer.BlockCopy(frame.Planes[0], 0, buffer, 0, frame.Planes[0].Length);
        Buffer.BlockCopy(frame.Planes[1], 0, buffer, frame.Planes[0].Length, frame.Planes[1].Length);
        Buffer.BlockCopy(frame.Planes[2], 0, buffer,
            frame.Planes[0].Length + frame.Planes[1].Length, frame.Planes[2].Length);

        fixed (byte* pBuf = buffer)
        {
            var success = NativeMethods.DeviceIoControl(
                _device,
                IoctlSendFrame,
                (nint)pBuf,
                (uint)buffer.Length,
                nint.Zero,
                0,
                out _,
                nint.Zero);

            if (!success)
            {
                logger.LogWarning("Driver IOCTL failed: {Error}",
                    Marshal.GetLastPInvokeError());
            }
        }
    }

    public void Dispose()
    {
        if (_device != nint.Zero)
        {
            NativeMethods.CloseHandle(_device);
            _device = nint.Zero;
        }
        logger.LogInformation("Driver closed");
    }
}
