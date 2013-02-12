cmake_minimum_required(VERSION 2.6)

SET(CMAKE_CXX_FLAGS "-fopenmp -D_FILE_OFFSET_BITS=64 ${CMAKE_CXX_FLAGS}")

option(ENABLE_ROOT "ENABLE ROOT Output" off)
if (ENABLE_ROOT)
	FIND_PROGRAM(ROOT_CONFIG root-config REQUIRED ROOT_CONFIG_PATH) 
	execute_process(COMMAND ${ROOT_CONFIG} "--cflags" OUTPUT_VARIABLE ROOT_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT cflags: ${ROOT_CFLAGS}")

	execute_process(COMMAND ${ROOT_CONFIG} "--libs" OUTPUT_VARIABLE ROOT_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT libs: ${ROOT_LIBS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CFLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ROOT_LIBS}")

	add_definitions(-DGADGET_ROOT_ENABLED)
endif()

include_directories (include)

add_library( gadget-lib
    src/Database.cpp
    src/GadgetFile.cpp
    src/MagneticField.cpp
    src/MultiResolutionMagneticField.cpp
)
SET_TARGET_PROPERTIES(gadget-lib PROPERTIES OUTPUT_NAME "Gadget")

add_executable( gadget-tool
	tool/main
	tool/arguments
	tool/bfieldtest
	tool/paged_grid
	tool/mass
	tool/sph
	tool/sph-dump
	tool/database
	tool/mrmf
	tool/lmf
)
SET_TARGET_PROPERTIES(gadget-tool PROPERTIES OUTPUT_NAME "gadget")
add_dependencies(gadget-tool gadget-lib) 
target_link_libraries (gadget-tool gadget-lib) 


INSTALL(TARGETS gadget-tool RUNTIME DESTINATION bin)
INSTALL(TARGETS gadget-lib DESTINATION lib)
INSTALL(DIRECTORY include/ DESTINATION include
          FILES_MATCHING PATTERN "*.h")
if(UNIX)
    SET(GADGET_PREFIX ${CMAKE_INSTALL_PREFIX})
    SET(GADGET_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libGadget.a)
    configure_file(${CMAKE_SOURCE_DIR}/gadget-config.in ${CMAKE_CURRENT_BINARY_DIR}/gadget-config.system @ONLY)
    INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gadget-config.system DESTINATION bin PERMISSIONS RENAME gadget-config)

    SET(GADGET_PREFIX ${CMAKE_SOURCE_DIR})
    SET(GADGET_LIBRARY ${CMAKE_BINARY_DIR}/libGadget.a)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    configure_file(${CMAKE_SOURCE_DIR}/gadget-config.in ${CMAKE_BINARY_DIR}/bin/gadget-config @ONLY)
endif(UNIX)

ENABLE_TESTING()

add_executable(pg_test test/pg_test.cpp)
add_executable(mf_test test/mf_test.cpp)
add_executable(coma_profile test/coma_profile.cpp)
target_link_libraries(coma_profile gadget-lib)

add_executable(database_test test/database_test.cpp)
target_link_libraries(database_test gadget-lib)

add_executable(hcube_test test/hcube_test.cpp)
target_link_libraries(hcube_test gadget-lib)

add_executable(sph_grid_test test/sph_grid_test.cpp)
target_link_libraries(mf_test gadget-lib)
target_link_libraries(sph_grid_test gadget-lib)
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
	
	file(GLOB_RECURSE GADGET_INCLUDES include/*.h)
	set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/gadget_wrap.cxx PROPERTIES GENERATED true )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gadget_wrap.cxx
	                COMMAND swig -c++ -python ${GADGET_SWIG_DEFINES} -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_CURRENT_BINARY_DIR}/gadget_wrap.cxx -outdir ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/python/gadget.i 
	                DEPENDS ${CMAKE_SOURCE_DIR}/python/gadget.i DEPENDS ${GADGET_INCLUDES} )
	add_library(gadget-swig MODULE ${CMAKE_CURRENT_BINARY_DIR}/gadget_wrap.cxx)
	set_target_properties(gadget-swig PROPERTIES PREFIX "")
	set_target_properties(gadget-swig PROPERTIES OUTPUT_NAME "_gadget") 
	target_link_libraries(gadget-swig gadget-lib ${PYTHON_LIBRARIES})
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gadget.py" DESTINATION ${PYTHON_SITE_PACKAGES})
	install(TARGETS gadget-swig LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
	install(FILES python/gadget.i DESTINATION share/gadget)
endif(ENABLE_PYTHON)