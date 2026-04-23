cmake_minimum_required(VERSION 3.16)
project(nanoclone CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather C++ sources from src/ and src/git/
file(GLOB CXX_SOURCES
    src/*.cpp
    src/git/*.cpp
)

# SQLite amalgamation compiled as C
set(SQLITE_SOURCE src/sqlite/sqlite3.c)
set_source_files_properties(${SQLITE_SOURCE} PROPERTIES
    LANGUAGE C
    COMPILE_FLAGS "-DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0"
)

add_executable(nanoclone
    ${CXX_SOURCES}
    src/markdown_preview.cpp
    ${SQLITE_SOURCE}
)

target_include_directories(nanoclone PRIVATE src)

find_package(Threads REQUIRED)
target_link_libraries(nanoclone PRIVATE Threads::Threads)
