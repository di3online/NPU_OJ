project(NPU_OJ)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall --pipe -ggdb")
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-parameter -Wno-unused-result")
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE}")

MESSAGE(STATUS "This is CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}")

find_package(MYSQL REQUIRED)
find_package(Threads REQUIRED)
find_program(FTP sftp)
find_program(FTP ftp)
find_program(DIFF diff)

if (NOT FTP)
    message(FATAL_ERROR "sftp or ftp is not found")
endif(NOT FTP)

if (NOT DIFF)
    message(FATAL_ERROR "diff is not found")
endif(NOT DIFF)

add_subdirectory(src bin)
