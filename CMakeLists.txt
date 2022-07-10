cmake_minimum_required(VERSION 3.19.0)
project(ffmpeg_example LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB sdl2_libs "./3rdparty/SDL2/lib/x64/sdl2.lib")
file(GLOB ffmpeg_libs "./lib/*.lib")
message(STATUS "${ffmpeg_libs}")

add_executable(ffmpeg_example "./FFmpeg_example/main.cpp")
target_link_libraries(ffmpeg_example ${ffmpeg_libs} ${sdl2_libs})
target_include_directories(ffmpeg_example PRIVATE
    "./include"
    "E:\\SDK\\ffmpeg\\FFmpeg_SDK\\3rdparty\\SDL2\\include"
)
