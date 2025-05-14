cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -Wpedantic -Werror")

project(fptlin VERSION 1.0 LANGUAGES CXX)

# main engine
set(SOURCE
  "src/fptlin.cpp"
)

add_executable(fptlin ${SOURCE})

target_include_directories(fptlin PRIVATE "include")