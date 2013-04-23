
function(add_example binary_name source_path)
  file(GLOB_RECURSE SOURCES ${source_path}/*.h ${source_path}/*.cpp ${source_path}*.c) 
  add_executable(${binary_name} ${SOURCES})
  target_link_libraries(${binary_name} netstalker)
endfunction()

include_directories(../NetStalkerLibrary/include)

add_example(basicConnection-server basicConnection/server)
add_example(basicConnection-client basicConnection/client)

