
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )
project(liby VERSION 0.1.0)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Wconversion -Wshadow")
include_directories(./include)
aux_source_directory(./src DIR_SRCS)
add_definitions (-D_GNU_SOURCE)
add_definitions(-DFMT_HEADER_ONLY)
add_library(y SHARED 
    Emiter.cpp
    Socket.cpp
    # BufferProxy.cpp
    Notify.cpp
    InetAddress.cpp
    SocketsOps.cpp
    # Buffer.cpp
    ConnectAdaptHandle.cpp
    Connector.cpp
    Dispatcher.cpp
    AcceptAdaptHandle.cpp
    Chamber.cpp
    AcceptChannel.cpp
    )

target_link_libraries(y uring  pthread)

add_executable(libserver ${DIR_SRCS} main2.cc )
target_link_libraries(libserver y)

add_executable(libclient main1.cc )
target_link_libraries(libclient y)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

add_subdirectory(test)
