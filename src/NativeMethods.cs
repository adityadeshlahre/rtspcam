using System.Runtime.InteropServices;

namespace rtspcam;

internal static class NativeMethods
{
    private const string Kernel32 = "kernel32.dll";

    [DllImport(Kernel32, SetLastError = true, CharSet = CharSet.Auto)]
    internal static extern nint CreateFile(
        string lpFileName,
        uint dwDesiredAccess,
        uint dwShareMode,
        nint lpSecurityAttributes,
        uint dwCreationDisposition,
        uint dwFlagsAndAttributes,
        nint hTemplateFile);

    [DllImport(Kernel32, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static extern bool DeviceIoControl(
        nint hDevice,
        uint dwIoControlCode,
        nint lpInBuffer,
        uint nInBufferSize,
        nint lpOutBuffer,
        uint nOutBufferSize,
        out uint lpBytesReturned,
        nint lpOverlapped);

    [DllImport(Kernel32, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static extern bool CloseHandle(nint hObject);
}
