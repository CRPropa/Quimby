SET(CMAKE_CXX_FLAGS "-fopenmp")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")


add_executable( gadget
	GadgetFile.cpp
	GadgetFile.hpp
	Octree.hpp
	main.cpp
)

INSTALL(TARGETS gadget RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)