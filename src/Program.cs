using rtspcam;
using Serilog;

var logDir = OperatingSystem.IsWindows()
    ? Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "/rtspcam"
    : ".";
Directory.CreateDirectory(logDir);

Log.Logger = new LoggerConfiguration()
    .MinimumLevel.Information()
    .WriteTo.Console()
    .WriteTo.File(
        Path.Combine(logDir, "main.log"),
        rollingInterval: RollingInterval.Infinite,
        outputTemplate: "[{Timestamp:yyyy-MM-dd HH:mm:ss}] [{Level:u3}] {Message:lj}{NewLine}{Exception}")
    .CreateLogger();

if (args.Length == 0 && OperatingSystem.IsWindows())
{
    var psi = new System.Diagnostics.ProcessStartInfo("sc", "query rtspcam")
    { RedirectStandardOutput = true };
    using var sc = System.Diagnostics.Process.Start(psi)!;
    sc.WaitForExit();

    if (sc.ExitCode != 0)
    {
        var exe = System.Diagnostics.Process.GetCurrentProcess().MainModule!.FileName;
        System.Diagnostics.Process.Start("sc",
            $"create rtspcam binPath=\"{exe}\" start=auto")!.WaitForExit();
        System.Diagnostics.Process.Start("sc", "start rtspcam")!.WaitForExit();
        Log.Information("Service installed and started.");
        return;
    }
}

if (args.Contains("--uninstall"))
{
    System.Diagnostics.Process.Start("sc", "stop rtspcam")!.WaitForExit();
    System.Diagnostics.Process.Start("sc", "delete rtspcam")!.WaitForExit();
    Log.Information("Service uninstalled.");
    return;
}

try
{
    var useConsole = args.Contains("--console") || !OperatingSystem.IsWindows();

    var builder = Host.CreateApplicationBuilder(args);
    builder.Services.AddSerilog();

    if (useConsole)
    {
        builder.Services.AddHostedService<Service>();
    }
    else
    {
        builder.Services.AddWindowsService(options =>
        {
            options.ServiceName = "Virtual RTSP Camera";
        });
        builder.Services.AddHostedService<Service>();
    }

    var host = builder.Build();
    host.Run();
}
finally
{
    Log.CloseAndFlush();
}
