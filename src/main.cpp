#include "rtsp/rtsp_client.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
  std::string url;
  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], "--url") == 0) {
      url = argv[i + 1];
      break;
    }
  }

  if (url.empty()) {
    std::cerr << "Usage: rtspcam --url rtsp://...\n";
    return 1;
  }

  RTSPClient client(url);
  if (!client.connect()) {
    return 1;
  }

  std::cout << "Reading packets...\n";

  auto start = std::chrono::steady_clock::now();

  while (true) {
    if (!client.read_packet()) {
      std::cerr << "Reconnecting...\n";
      std::this_thread::sleep_for(std::chrono::seconds(2));
      if (!client.connect()) {
        std::cerr << "Reconnect failed\n";
        break;
      }
      std::cout << "Reconnected\n";
      continue;
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::minutes(10)) {
      std::cout << "Stable for 10 minutes.\n";
      break;
    }
  }

  return 0;
}
