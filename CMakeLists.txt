cmake_minimum_required(VERSION 2.6)

if(CMAKE_COMPILER_IS_GNUCC)
	SET(CMAKE_CXX_FLAGS "-fopenmp -fPIC -D_FILE_OFFSET_BITS=64 ${CMAKE_CXX_FLAGS}")
endif()

option(ENABLE_ROOT "ENABLE ROOT Output" off)
if (ENABLE_ROOT)
	FIND_PROGRAM(ROOT_CONFIG root-config REQUIRED ROOT_CONFIG_PATH) 
	execute_process(COMMAND ${ROOT_CONFIG} "--cflags" OUTPUT_VARIABLE ROOT_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT cflags: ${ROOT_CFLAGS}")

	execute_process(COMMAND ${ROOT_CONFIG} "--libs" OUTPUT_VARIABLE ROOT_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT libs: ${ROOT_LIBS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CFLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ROOT_LIBS}")

	add_definitions(-DQUIMBY_ROOT_ENABLED)
endif()

include_directories (include)

add_library( quimby-lib
    src/Database.cpp
    src/GadgetFile.cpp
    src/MagneticField.cpp
)
SET_TARGET_PROPERTIES(quimby-lib PROPERTIES OUTPUT_NAME "Quimby")

add_executable( quimby-tool
	tool/main
	tool/arguments
	tool/bfieldtest
	tool/paged_grid
	tool/mass
	tool/sph
	tool/sph-dump
	tool/database
	tool/lmf
)
SET_TARGET_PROPERTIES(quimby-tool PROPERTIES OUTPUT_NAME "quimby")
add_dependencies(quimby-tool quimby-lib) 
target_link_libraries (quimby-tool quimby-lib) 


INSTALL(TARGETS quimby-tool RUNTIME DESTINATION bin)
INSTALL(TARGETS quimby-lib DESTINATION lib)
INSTALL(DIRECTORY include/ DESTINATION include
          FILES_MATCHING PATTERN "*.h")
if(UNIX)
    SET(QUIMBY_PREFIX ${CMAKE_INSTALL_PREFIX})
    SET(QUIMBY_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libQuimby.a)

    SET(QUIMBY_PREFIX ${CMAKE_SOURCE_DIR})
    SET(QUIMBY_LIBRARY ${CMAKE_BINARY_DIR}/libQuimby.a)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif(UNIX)

ENABLE_TESTING()

add_executable(pg_test test/pg_test.cpp)
add_executable(mf_test test/mf_test.cpp)
add_executable(coma_profile test/coma_profile.cpp)
target_link_libraries(coma_profile quimby-lib)

add_executable(database_test test/database_test.cpp)
target_link_libraries(database_test quimby-lib)

add_executable(hcube_test test/hcube_test.cpp)
target_link_libraries(hcube_test quimby-lib)

add_executable(sph_grid_test test/sph_grid_test.cpp)
target_link_libraries(mf_test quimby-lib)
target_link_libraries(sph_grid_test quimby-lib)
ADD_TEST(mf mf_test)
ADD_TEST(pg pg_test)
ADD_TEST(sph_grid sph_grid_test)

# ----------------------------------------------------------------------------
# Python
# ----------------------------------------------------------------------------
option(ENABLE_PYTHON "Create python library via SWIG" OFF)
if(ENABLE_PYTHON)
	include(python/Python.cmake)
	include_directories(${PYTHON_INCLUDE_PATH})
	
	file(GLOB_RECURSE QUIMBY_INCLUDES include/*.h)
	set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/quimby_wrap.cxx PROPERTIES GENERATED true )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/quimby_wrap.cxx
	                COMMAND swig -c++ -python ${QUIMBY_SWIG_DEFINES} -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_CURRENT_BINARY_DIR}/quimby_wrap.cxx -outdir ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/python/quimby.i 
	                DEPENDS ${CMAKE_SOURCE_DIR}/python/quimby.i DEPENDS ${QUIMBY_INCLUDES} )
	add_library(quimby-swig MODULE ${CMAKE_CURRENT_BINARY_DIR}/quimby_wrap.cxx)
	set_target_properties(quimby-swig PROPERTIES PREFIX "")
	set_target_properties(quimby-swig PROPERTIES OUTPUT_NAME "_quimby") 
	target_link_libraries(quimby-swig quimby-lib ${PYTHON_LIBRARIES})
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/quimby.py" DESTINATION ${PYTHON_SITE_PACKAGES})
	install(TARGETS quimby-swig LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
	install(FILES python/quimby.i DESTINATION share/quimby)
endif(ENABLE_PYTHON)