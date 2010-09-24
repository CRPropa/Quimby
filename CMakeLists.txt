cmake_minimum_required(VERSION 2.6)

SET(CMAKE_CXX_FLAGS "-fopenmp ${CMAKE_CXX_FLAGS}")
#SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
#SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")

include_directories (include)

add_library( gadget-lib SHARED
    src/GadgetFile.cpp
)
SET_TARGET_PROPERTIES(gadget-lib PROPERTIES OUTPUT_NAME "Gadget")

add_executable( gadget-tool
	tool/main
	tool/arguments
	tool/bfieldtest
	tool/paged_grid
	tool/mass
)
SET_TARGET_PROPERTIES(gadget-tool PROPERTIES OUTPUT_NAME "gadget")
add_dependencies(gadget-tool gadget-lib) 
target_link_libraries (gadget-tool gadget-lib) 


INSTALL(TARGETS gadget-tool RUNTIME DESTINATION bin)
INSTALL(TARGETS gadget-lib DESTINATION lib)
INSTALL(DIRECTORY include/ DESTINATION include
          FILES_MATCHING PATTERN "*.hpp")
if(UNIX)          
    configure_file(${CMAKE_SOURCE_DIR}/gadget-config.in ${CMAKE_CURRENT_BINARY_DIR}/gadget-config @ONLY)
    INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gadget-config DESTINATION bin PERMISSIONS)
endif(UNIX)

ENABLE_TESTING()

add_executable(pg_test test/pg_test.cpp)
ADD_TEST(pg pg_test)