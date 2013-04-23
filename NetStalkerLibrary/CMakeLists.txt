
file(GLOB_RECURSE NS_HEADERS *.h)
file(GLOB_RECURSE NS_SOURCES *.cpp *.c)

add_definitions(-DHAS_SOCKLEN_T)
add_library(netstalker SHARED ${NS_HEADERS} ${NS_SOURCES})
if(${UNIX})
  target_link_libraries(netstalker rt)
endif()
