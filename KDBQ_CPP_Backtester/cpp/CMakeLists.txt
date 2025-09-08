cmake_minimum_required(VERSION 3.16)
project(kdbq_cpp_backtester)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(backtester
  engine/Backtester.cpp
)

target_include_directories(backtester PRIVATE engine)
