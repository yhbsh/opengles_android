cmake_minimum_required(VERSION 3.22.1)
project("engine")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(${CMAKE_PROJECT_NAME} SHARED triangle.c)

find_library(LOG_LIB log)
find_library(LIBC c)
find_library(ZLIB z)
find_library(GLES3_LIB GLESv3)

set(FFMPEG_LIBS_DIR "${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/lib")
set(FFMPEG_LIBS
    "${FFMPEG_LIBS_DIR}/libavformat.so"
    "${FFMPEG_LIBS_DIR}/libavcodec.so"
    "${FFMPEG_LIBS_DIR}/libavdevice.so"
    "${FFMPEG_LIBS_DIR}/libavfilter.so"
    "${FFMPEG_LIBS_DIR}/libavutil.so"
    "${FFMPEG_LIBS_DIR}/libswresample.so"
    "${FFMPEG_LIBS_DIR}/libswscale.so"
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    ${LIBC}
    ${LOG_LIB}
    ${ZLIB}
    ${GLES3_LIB}
    ${FFMPEG_LIBS}
)

