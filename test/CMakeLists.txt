cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )


project(exp CXX)


add_executable(echo_client echo/echo_client.cpp)
add_executable(echo_server echo/echo_server.cpp)

add_executable(contort contort.cc)
# target_link_libraries(contort tcmalloc)

add_executable(ref ./Ref.cpp)


# add_executable(alloc_mmap allocate_mmap.cc)
