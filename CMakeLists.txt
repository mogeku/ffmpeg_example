cmake_minimum_required(VERSION 3.19.0)
project(ffmpeg_example LANGUAGES CXX VERSION 0.1.0)

if(CMAKE_CL_64)
    set(arch x64)
else()
    set(arch x86)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB sdl2_libs "./3rdparty/SDL2/lib/x64/sdl2.lib")
file(GLOB ffmpeg_libs "./lib/*.lib")
message(STATUS "${ffmpeg_libs}")

add_executable(ffmpeg_example "./FFmpeg_example/main.cpp")
target_link_libraries(ffmpeg_example ${ffmpeg_libs} ${sdl2_libs})
target_include_directories(ffmpeg_example PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/include"
)

file(GLOB ffmpeg_dlls
    ${CMAKE_SOURCE_DIR}/bin/*.dll
)

add_custom_command(TARGET ffmpeg_example POST_BUILD
    VERBATIM
    COMMAND_EXPAND_LISTS
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/lib/${arch}/SDL2.dll"
    ${ffmpeg_dlls}
    "$<TARGET_FILE_DIR:ffmpeg_example>"
)
