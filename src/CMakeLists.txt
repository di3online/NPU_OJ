AUX_SOURCE_DIRECTORY(. DIR_SRCS)
ADD_EXECUTABLE(noj ${DIR_SRCS})

target_link_libraries(noj 
    ${MYSQL_LIBRARY}
    ${Threads_LIBRARY}
)
