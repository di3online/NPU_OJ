aux_source_directory(. DIR_SRCS)
#message(STAUS, "PCRE INCLUDE: ${PCRE_INCLUDE_DIR}")
#MESSAGE(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
include_directories(${PCRE_INCLUDE_DIR})
include_directories(${MYSQL_INCLUDE_DIR})
add_executable(noj ${DIR_SRCS})

target_link_libraries(noj 
    ${MYSQL_LIBRARIES}
    ${Threads_LIBRARY}
    ${PCRE_LIBRARIES}
)

install(TARGETS noj
    DESTINATION ${MAINROOT}/bin)
