using System.Runtime.InteropServices;
using FFmpeg.AutoGen;

namespace rtspcam;

public sealed class RtspClient(ILogger logger) : IDisposable
{
    private unsafe AVFormatContext* _ctx;
    private int _videoStream = -1;

    public record StreamInfoData(int StreamIndex, string Codec, int Width, int Height, double Fps, nint CodecPar);

    public StreamInfoData? StreamInfo { get; private set; }

    public unsafe void Connect(string url, string transport = "tcp", int timeoutSec = 10)
    {
        ffmpeg.avformat_network_init();

        _ctx = ffmpeg.avformat_alloc_context();
        if (_ctx == null)
            throw new InvalidOperationException("Failed to alloc format context");

        var query = url.Contains('?') ? '&' : '?';
        var fullUrl = $"{url}{query}rtsp_transport={transport}&timeout={timeoutSec * 1000000}u&reconnect=1";

        fixed (AVFormatContext** pCtxPtr = &_ctx)
        {
            var ret = ffmpeg.avformat_open_input(pCtxPtr, fullUrl, null, null);
            if (ret < 0)
                throw new InvalidOperationException(
                    $"Failed to open RTSP stream (error {ret})");
        }

        ffmpeg.avformat_find_stream_info(_ctx, null);

        for (var i = 0; i < _ctx->nb_streams; i++)
        {
            var par = _ctx->streams[i]->codecpar;
            if (par->codec_type == AVMediaType.AVMEDIA_TYPE_VIDEO)
            {
                _videoStream = i;
                var codec = ffmpeg.avcodec_find_decoder(par->codec_id);
                var codecName = codec != null
                    ? Marshal.PtrToStringAnsi((nint)codec->name) ?? "?"
                    : "?";
                var avgFps = _ctx->streams[i]->avg_frame_rate;
                var fps = avgFps.den > 0
                    ? avgFps.num / (double)avgFps.den
                    : 30.0;

                StreamInfo = new StreamInfoData(i, codecName,
                    par->width, par->height, fps, (nint)par);
                break;
            }
        }

        if (StreamInfo == null)
            throw new InvalidOperationException("No video stream found");
    }

    public unsafe PacketData? ReadPacket()
    {
        if (_ctx == null) return null;

        var pkt = ffmpeg.av_packet_alloc();
        try
        {
            var ret = ffmpeg.av_read_frame(_ctx, pkt);
            if (ret < 0) return null;

            var data = new byte[pkt->size];
            Marshal.Copy((nint)pkt->data, data, 0, pkt->size);

            return new PacketData(data, pkt->stream_index);
        }
        finally
        {
            ffmpeg.av_packet_free(&pkt);
        }
    }

    public unsafe void Disconnect()
    {
        if (_ctx != null)
        {
            fixed (AVFormatContext** p = &_ctx)
                ffmpeg.avformat_close_input(p);
            _ctx = null;
        }
        ffmpeg.avformat_network_deinit();
    }

    public void Dispose() => Disconnect();

    public record PacketData(byte[] Data, int StreamIndex);
}
