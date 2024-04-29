cmake_minimum_required(VERSION 3.22.1)
project("hello")

add_library(${CMAKE_PROJECT_NAME} SHARED hello.c)

find_library(log-lib log)
find_library(GLESv3-lib GLESv3)

target_link_libraries(${CMAKE_PROJECT_NAME} ${log-lib} ${GLESv3-lib})