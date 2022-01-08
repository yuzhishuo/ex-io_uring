cmake_minimum_required(VERSION 3.0.0)
project(ex-io_uring VERSION 0.1.0)


include_directories(./include)
aux_source_directory(./src DIR_SRCS)
add_definitions (-D_GNU_SOURCE)
add_executable(ex-io_uring ${DIR_SRCS} main.c )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

