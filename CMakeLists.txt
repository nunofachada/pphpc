cmake_minimum_required(VERSION 2.8)
project(pphpc-opencl)

# Compiler options for GCC
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99 ")
	set(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -ggdb3 ")
endif()

# Find required libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
find_package(PkgConfig REQUIRED)
find_package(OpenCL REQUIRED)
find_package(Cf4ocl REQUIRED)
find_package(ClOps REQUIRED)
find_package(Doxygen 1.8.3 QUIET)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.32.1)

# Library include directories
include_directories(${GLIB_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} ${CF4OCL_INCLUDE_DIRS} ${CLOPS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/pp)

# Add source folder
add_subdirectory(pp)

# add a target to generate API documentation with Doxygen
if(DOXYGEN_FOUND)
	set(DOCS_DIRECTORY "doc")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	# Convert GitHub README.md into Doxygen MD format
	file(READ ${CMAKE_CURRENT_SOURCE_DIR}/README.md README_CONTENTS)
	string(REPLACE "```c" "~~~~~~~~~~~~~~~{.c}" README_CONTENTS_MOD1 ${README_CONTENTS})
	string(REPLACE "```" "~~~~~~~~~~~~~~~" README_CONTENTS_MOD2 ${README_CONTENTS_MOD1})
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/README.doxy.md ${README_CONTENTS_MOD2})
	# Add a build docs target
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif()

