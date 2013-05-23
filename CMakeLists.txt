project(NPU_OJ)
set(MAINROOT noj)
cmake_minimum_required(VERSION 2.8)

message(STATUS "${CMAKE_INSTALL_PREFIX}")
message(STATUS "PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -O0 -Wall --pipe -ggdb")
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -Wno-unused-parameter -Wno-unused-result")
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
MESSAGE(STATUS "This is CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}")

find_package(MYSQL REQUIRED)
find_package(Threads REQUIRED)
find_package(PCRE REQUIRED)
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

install(DIRECTORY scripts 
    DESTINATION ${MAINROOT}
    PATTERN "scripts/*"
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ 
        GROUP_EXECUTE GROUP_READ
        WORLD_READ WORLD_EXECUTE)

install(FILES conf/noj.conf
    DESTINATION /etc/
    PERMISSIONS OWNER_READ OWNER_WRITE 
    GROUP_READ GROUP_WRITE)

