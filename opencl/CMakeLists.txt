cmake_minimum_required(VERSION 2.8.3)
project(PPHPC-OpenCL)

# Set modules directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Set log domain
add_definitions(-DG_LOG_DOMAIN=\"${PROJECT_NAME}\")

# Is profiling on or off?
option(PROFILING "Use OpenCL profiling?" ON)

if (PROFILING)
	add_definitions(-DPP_PROFILE_OPT)
endif()

# #################### #
# DEPENDENCIES SECTION #
# #################### #

# Find required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(CF4OCL2 REQUIRED cf4ocl2>=2.0.0)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.32.1)

# Find cl_ops
find_package(cl_ops REQUIRED)

# Find optional executables for creating docs
find_package(Git QUIET)
find_package(Doxygen 1.8.3 QUIET)
find_package(LATEX QUIET)

# ################ #
# COMPILER OPTIONS #
# ################ #

# Compiler options for GCC/Clang
# -Wno-comment because of comment within comment in OpenCL headers
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -Wno-comment -std=c99")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")

# ########################### #
# SET LIBRARY INCLUDE FOLDERS #
# ########################### #

# Library include directories
include_directories(
	${GLIB_INCLUDE_DIRS}
	${CF4OCL2_INCLUDE_DIRS}
	${CL_OPS_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/pp
	${CMAKE_BINARY_DIR}/${PROJECT_NAME})

# ########################### #
# BUILD DOCUMENTATION SECTION #
# ########################### #

# add a target to generate API documentation with Doxygen
if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND AND LATEX_COMPILER)

	# Doc build is enabled
	set(DOC_BUILD "ON" CACHE INTERNAL "Build documentation")

	# Set doc generation directory
	set(DOCS_DIRECTORY "doc")

	# Document generation options
	option(DOC_GENERATE_HTML "Generate HTML documentation?" ON)
	option(DOC_GENERATE_LATEX "Generate Latex documentation?" OFF)
	if (DOC_GENERATE_LATEX AND CMAKE_MAKE_PROGRAM
		AND PDFLATEX_COMPILER AND MAKEINDEX_COMPILER)
		option(DOC_GENERATE_PDF "Generate PDF documentation?" OFF)
	else()
		unset(DOC_GENERATE_PDF CACHE)
	endif()

	# Set doxygen document generation options
	if (DOC_GENERATE_HTML)
		set(DOX_GENERATE_HTML "YES")
	else()
		set(DOX_GENERATE_HTML "NO")
	endif()
	if (DOC_GENERATE_LATEX)
		set(DOX_GENERATE_LATEX "YES")
	else()
		set(DOX_GENERATE_LATEX "NO")
	endif()

	# If git is found, set document file version filter to last
	# commit where file was changed
	if (GIT_FOUND)
		set(DOX_FILE_VERSION_FILTER
			"${GIT_EXECUTABLE} log -n 1 --pretty=%h --")
		execute_process(
			COMMAND ${GIT_EXECUTABLE} log --pretty=git@%h -n 1
			OUTPUT_VARIABLE DOX_PROJECT_VERSION)
	endif()

	# Copy and configure doxigen configuration file
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

	# Add a build docs target
	if (DOC_GENERATE_HTML OR DOC_GENERATE_LATEX)
		add_custom_target(doc
			${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating documentation with Doxygen" VERBATIM)
	else()
		add_custom_target(doc
			${CMAKE_COMMAND} -E echo "Enable at least one document format."
			COMMENT "All document generation formats are disabled!" VERBATIM)
	endif()

	# Generate PDF from latex files
	if (DOC_GENERATE_PDF)
		add_custom_command(TARGET doc POST_BUILD
			COMMAND ${CMAKE_MAKE_PROGRAM}
			COMMAND ${CMAKE_COMMAND} -E make_directory ../pdf
			COMMAND ${CMAKE_COMMAND} -E copy refman.pdf ../pdf
			WORKING_DIRECTORY ${DOCS_DIRECTORY}/latex)
	endif()

else()

	# No doc builds
	unset(DOC_BUILD CACHE)

	# Add doc target which warns that some document build dependencies
	# are missing
	add_custom_target(doc
		${CMAKE_COMMAND} -E echo "Documentation generation is disabled."
		COMMENT "One or more tools required for documentation generation were not found!"
		VERBATIM)

endif()

# ############### #
# PROCESS SUBDIRS #
# ############### #

# Add source folder
add_subdirectory(pp)
