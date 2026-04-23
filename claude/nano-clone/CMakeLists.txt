cmake_minimum_required(VERSION 3.16)
project(nanoclone CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(nanoclone
    src/main.cpp
    src/terminal.cpp
    src/buffer.cpp
    src/database.cpp
    src/filepanel.cpp
    src/highlighters.cpp
    src/editor.cpp
)

target_include_directories(nanoclone PRIVATE src)
