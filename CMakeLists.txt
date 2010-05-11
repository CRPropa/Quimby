cmake_minimum_required(VERSION 2.6)

SET(CMAKE_CXX_FLAGS "-fopenmp")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")

include_directories (include)

add_library( Gadget SHARED
    src/GadgetFile.cpp
)

add_executable( gadget
	main
	arguments
	paged_grid
)
add_dependencies(gadget Gadget) 

target_link_libraries (gadget Gadget) 

INSTALL(TARGETS gadget RUNTIME DESTINATION bin)
INSTALL(TARGETS Gadget LIBRARY DESTINATION lib)
