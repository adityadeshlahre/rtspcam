#pragma once

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

struct FormatContextDeleter {
  void operator()(AVFormatContext *ctx) const noexcept {
    avformat_close_input(&ctx);
  }
};

using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;

struct PacketDeleter {
  void operator()(AVPacket *pkt) const noexcept { av_packet_free(&pkt); }
};

using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;

struct StreamInfo {
  int index{-1};
  AVCodecID codec_id{AV_CODEC_ID_NONE};
  int width{0};
  int height{0};
  AVRational frame_rate{};
  std::string codec_name{};
};

class RTSPClient {
public:
  explicit RTSPClient(std::string url);
  ~RTSPClient();

  RTSPClient(const RTSPClient &) = delete;
  RTSPClient &operator=(const RTSPClient &) = delete;

  bool connect(int timeout_sec = 10);
  void disconnect();

  bool read_packet();
  std::string url() const { return url_; }

  const StreamInfo &stream_info() const { return stream_info_; }

  bool is_connected() const { return ctx_ != nullptr; }

private:
  void print_stream_info();

  std::string url_;
  FormatContextPtr ctx_;
  StreamInfo stream_info_;
  int video_stream_{-1};
};
