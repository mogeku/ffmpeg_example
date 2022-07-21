cmake_minimum_required(VERSION 3.19.0)
project(ffmpeg_example LANGUAGES CXX VERSION 0.1.0)

if(CMAKE_CL_64)
    set(arch x64)
else()
    set(arch x86)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB sdl2_libs "./3rdparty/SDL2/lib/x64/sdl2.lib")
file(GLOB sdl2_dlls "./3rdparty/SDL2/lib/x64/sdl2.dll")
file(GLOB ffmpeg_libs "./3rdparty/ffmpeg/lib/*.a")
message(STATUS "${ffmpeg_libs}")

add_executable(ffmpeg_example "./src/main.cpp")
target_link_libraries(ffmpeg_example
    PRIVATE
    ${ffmpeg_libs}
    ${sdl2_libs}
    ws2_32
    secur32
    strmiids
    mfuuid
    bcrypt
)
target_include_directories(ffmpeg_example PRIVATE
    "${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include"
    "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/include"
)

add_custom_command(TARGET ffmpeg_example
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${sdl2_dlls}
    $<TARGET_FILE_DIR:ffmpeg_example>
)
