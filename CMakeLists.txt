cmake_minimum_required(VERSION 3.20)

project(rtspcam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CROSSCOMPILING)
    set(FFMPEG_ROOT "$ENV{FFMPEG_ROOT}" CACHE PATH "Path to cross-compiled FFmpeg")
    if(NOT FFMPEG_ROOT)
        message(FATAL_ERROR "Set FFMPEG_ROOT to cross-compiled FFmpeg, or install mingw-w64-ffmpeg")
    endif()
    list(APPEND CMAKE_PREFIX_PATH "${FFMPEG_ROOT}")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC  REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL   REQUIRED IMPORTED_TARGET libavutil)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC  REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL   REQUIRED IMPORTED_TARGET libavutil)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")
endif()

add_executable(rtspcam
    src/main.cpp
    src/rtsp/rtsp_client.cpp
)

target_link_libraries(rtspcam PRIVATE
    PkgConfig::AVFORMAT
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
)
