using System.Runtime.InteropServices;
using FFmpeg.AutoGen;

namespace rtspcam;

public sealed class Decoder(ILogger logger) : IDisposable
{
    private unsafe AVCodecContext* _codecCtx;
    private unsafe SwsContext* _swsCtx;
    private int _width;
    private int _height;

    public unsafe void Open(string codecName, int width, int height, nint codecPar = 0)
    {
        _width = width;
        _height = height;

        var codecId = codecName.ToLowerInvariant() switch
        {
            "h264" => AVCodecID.AV_CODEC_ID_H264,
            "hevc" => AVCodecID.AV_CODEC_ID_HEVC,
            _ => AVCodecID.AV_CODEC_ID_H264,
        };

        var codec = ffmpeg.avcodec_find_decoder(codecId);
        if (codec == null)
            throw new InvalidOperationException($"Codec {codecName} not found");

        _codecCtx = ffmpeg.avcodec_alloc_context3(codec);
        if (_codecCtx == null)
            throw new InvalidOperationException("Failed to allocate codec context");

        if (codecPar != 0)
            ffmpeg.avcodec_parameters_to_context(_codecCtx, (AVCodecParameters*)codecPar);

        ffmpeg.avcodec_open2(_codecCtx, codec, null);

        _swsCtx = ffmpeg.sws_getContext(
            width, height, AVPixelFormat.AV_PIX_FMT_YUV420P,
            width, height, AVPixelFormat.AV_PIX_FMT_NV12,
            1, null, null, null);

        logger.LogInformation("Decoder opened: {Codec} {W}x{H}", codecName, width, height);
    }

    public unsafe Nv12Frame? Decode(RtspClient.PacketData packet)
    {
        if (_codecCtx == null) return null;

        fixed (byte* pData = packet.Data)
        {
            var avpkt = ffmpeg.av_packet_alloc();
            avpkt->data = pData;
            avpkt->size = packet.Data.Length;

            var ret = ffmpeg.avcodec_send_packet(_codecCtx, avpkt);
            ffmpeg.av_packet_free(&avpkt);

            if (ret < 0) return null;

            var frame = ffmpeg.av_frame_alloc();
            ret = ffmpeg.avcodec_receive_frame(_codecCtx, frame);

            if (ret < 0 || frame == null)
            {
                ffmpeg.av_frame_free(&frame);
                return null;
            }

            var nv12 = ffmpeg.av_frame_alloc();
            nv12->width = _width;
            nv12->height = _height;
            nv12->format = (int)AVPixelFormat.AV_PIX_FMT_NV12;

            ffmpeg.av_frame_get_buffer(nv12, 0);
            ffmpeg.sws_scale(_swsCtx,
                frame->data, frame->linesize, 0, _height,
                nv12->data, nv12->linesize);

            var result = new Nv12Frame(_width, _height);
            uint pu = 0;
            for (; pu < 3; pu++)
            {
                if (nv12->data[pu] != null && nv12->linesize[pu] > 0)
                {
                    var sliceHeight = pu == 0 ? _height : _height / 2;
                    var planeBytes = (int)(nv12->linesize[pu] * sliceHeight);
                    Marshal.Copy((nint)nv12->data[pu],
                        result.Planes[(int)pu], 0, planeBytes);
                }
            }

            ffmpeg.av_frame_free(&frame);
            ffmpeg.av_frame_free(&nv12);

            return result;
        }
    }

    public unsafe void Dispose()
    {
        if (_swsCtx != null)
        {
            ffmpeg.sws_freeContext(_swsCtx);
            _swsCtx = null;
        }
        if (_codecCtx != null)
        {
            fixed (AVCodecContext** p = &_codecCtx)
                ffmpeg.avcodec_free_context(p);
            _codecCtx = null;
        }
    }
}

public sealed class Nv12Frame(int width, int height)
{
    public int Width { get; } = width;
    public int Height { get; } = height;
    public byte[][] Planes { get; } =
    [
        new byte[width * height],
        new byte[width * height / 2],
        [0]
    ];
}
