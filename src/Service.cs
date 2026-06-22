using System.Runtime.InteropServices;

namespace rtspcam;

public sealed class Service(
    ILogger<Service> logger,
    IConfiguration config)
    : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var url = config["RTSP:Url"]
            ?? throw new InvalidOperationException("RTSP:Url not configured");

        logger.LogInformation("Starting RTSP Camera Service");
        logger.LogInformation("Target: {Url}", url);

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await RunPipeline(url, stoppingToken);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                logger.LogError(ex, "Pipeline crashed, restarting in 5s");
                await Task.Delay(5000, stoppingToken);
            }
        }

        logger.LogInformation("Service stopped");
    }

    private async Task RunPipeline(string url, CancellationToken ct)
    {
        using var rtsp = new RtspClient(logger);
        using var decoder = new Decoder(logger);
        using var driver = new DriverIO(logger);

        rtsp.Connect(url);

        var info = rtsp.StreamInfo;
        logger.LogInformation("Connected: {Codec} {Width}x{Height} @ {Fps}fps",
            info.Codec, info.Width, info.Height, info.Fps);

        decoder.Open(info.Codec, info.Width, info.Height);
        driver.Open();

        while (!ct.IsCancellationRequested)
        {
            var packet = rtsp.ReadPacket();
            if (packet is null)
            {
                logger.LogWarning("End of stream, reconnecting...");
                break;
            }

            if (packet.StreamIndex != info.StreamIndex)
                continue;

            var frame = decoder.Decode(packet);
            if (frame is not null)
            {
                driver.SendFrame(frame);
            }
        }
    }
}
