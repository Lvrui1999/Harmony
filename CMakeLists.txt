cmake_minimum_required(VERSION 3.10)
project(arkcore)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_STANDARD_LIBRARIES "-static -static-libgcc -static-libstdc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -Wl,--no-whole-archive")

add_executable(arkcore main.cpp data.hpp cpu.hpp instructions.hpp util.hpp memory.hpp)

target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})