#include "rtsp_client.h"

#include <cstdlib>
#include <iostream>
#include <thread>

extern "C" {

#include <libavformat/avformat.h>
}

struct InterruptData {
  std::chrono::steady_clock::time_point start;
  int timeout_sec;
};

static int interrupt_cb(void *ctx) {
  auto *data = static_cast<InterruptData *>(ctx);
  auto elapsed = std::chrono::steady_clock::now() - data->start;
  return elapsed > std::chrono::seconds(data->timeout_sec) ? 1 : 0;
}

RTSPClient::RTSPClient(std::string url)
    : url_(std::move(url)), video_stream_(-1) {}

RTSPClient::~RTSPClient() { disconnect(); }

bool RTSPClient::connect(int timeout_sec) {
  avformat_network_init();

  AVFormatContext *raw_ctx = avformat_alloc_context();
  if (!raw_ctx) {
    std::cerr << "Failed to allocate format context\n";
    return false;
  }

  InterruptData int_data{std::chrono::steady_clock::now(), timeout_sec};
  raw_ctx->interrupt_callback.callback = interrupt_cb;
  raw_ctx->interrupt_callback.opaque = &int_data;

  AVDictionary *opts = nullptr;
  std::string timeout_us = std::to_string(timeout_sec * 1000000LL);
  av_dict_set(&opts, "timeout", timeout_us.c_str(), 0);
  av_dict_set(&opts, "rtsp_transport", "tcp", 0);
  av_dict_set(&opts, "reconnect", "1", 0);
  av_dict_set(&opts, "reconnect_at_eof", "1", 0);
  av_dict_set(&opts, "reconnect_streamed", "1", 0);
  av_dict_set(&opts, "reconnect_delay_max", "5", 0);

  int ret = avformat_open_input(&raw_ctx, url_.c_str(), nullptr, &opts);
  av_dict_free(&opts);

  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to open " << url_ << ": " << errbuf << "\n";
    return false;
  }

  ctx_.reset(raw_ctx);

  ret = avformat_find_stream_info(ctx_.get(), nullptr);
  if (ret < 0) {
    std::cerr << "Failed to find stream info\n";
    return false;
  }

  for (unsigned i = 0; i < ctx_->nb_streams; ++i) {
    if (ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_ = static_cast<int>(i);
      auto *par = ctx_->streams[i]->codecpar;
      auto *stream = ctx_->streams[i];

      stream_info_.index = video_stream_;
      stream_info_.codec_id = par->codec_id;
      stream_info_.width = par->width;
      stream_info_.height = par->height;
      stream_info_.frame_rate = stream->avg_frame_rate;
      if (auto *codec = avcodec_find_decoder(par->codec_id)) {
        stream_info_.codec_name = codec->name;
      }
      break;
    }
  }

  if (video_stream_ < 0) {
    std::cerr << "No video stream found\n";
    return false;
  }

  print_stream_info();
  return true;
}

void RTSPClient::disconnect() {
  ctx_.reset();
  video_stream_ = -1;
  stream_info_ = {};
  avformat_network_deinit();
}

bool RTSPClient::read_packet() {
  if (!ctx_)
    return false;

  AVPacket pkt;
  int ret = av_read_frame(ctx_.get(), &pkt);
  if (ret < 0) {
    if (ret == AVERROR_EOF) {
      std::cerr << "End of stream\n";
      return false;
    }
    if (ret == AVERROR_EXIT) {
      std::cerr << "Interrupted\n";
      return false;
    }
    return false;
  }

  av_packet_unref(&pkt);
  return true;
}

void RTSPClient::print_stream_info() {
  std::cout << "Connected\n";
  std::cout << "Codec: " << stream_info_.codec_name << "\n";
  std::cout << "Resolution: " << stream_info_.width << "x"
            << stream_info_.height << "\n";
  if (stream_info_.frame_rate.den > 0) {
    double fps = static_cast<double>(stream_info_.frame_rate.num) /
                 stream_info_.frame_rate.den;
    std::cout << "FPS: " << static_cast<int>(fps) << "\n";
  }
  std::cout << std::flush;
}
