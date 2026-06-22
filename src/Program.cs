using rtspcam;

var builder = Host.CreateApplicationBuilder(args);

builder.Services.AddWindowsService(options =>
{
    options.ServiceName = "Virtual RTSP Camera";
});

builder.Services.AddHostedService<Service>();

var host = builder.Build();

host.Run();
