cmake_minimum_required(VERSION 3.22.1)
project("hello")

add_library(${CMAKE_PROJECT_NAME} SHARED hello.c)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_library(log-lib log)
find_library(GLESv3-lib GLESv3)

set(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/lib)
link_directories(${FFMPEG_LIB_DIR})
set(FFMPEG_LIBS
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
)

foreach (lib ${FFMPEG_LIBS})
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_LIB_DIR}/lib${lib}.a)
endforeach ()

target_link_libraries(${CMAKE_PROJECT_NAME}
        ${log-lib}
        ${GLESv3-lib}
        ${FFMPEG_LIBRARIES}
)
