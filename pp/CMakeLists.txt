# Predator-Prey implementations
set(PP_IMPLS pp_gpu_legacy pp_cpu pp_gpu)

# Common sources
set(PP_COMMON pp_common.c)

# Add executables
foreach(PP_IMPL ${PP_IMPLS})
	add_executable(${PP_IMPL} ${PP_IMPL}.c ${PP_COMMON})
	target_link_libraries(${PP_IMPL} ${OPENCL_LIBRARIES}
		${GLIB_LIBRARIES} ${GLIB_LDFLAGS} ${CF4OCL2_LIBRARIES}
		${CL_OPS_LIBRARIES} m)
endforeach()

# Put OpenCL kernels in header
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/pp_cpu.cl
	PP_CPU_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" PP_CPU_SRC ${PP_CPU_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pp_cpu.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/pp_cpu.h @ONLY)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/pp_gpu.cl
	PP_GPU_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" PP_GPU_SRC ${PP_GPU_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pp_gpu.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/pp_gpu.h @ONLY)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/pp_gpu_legacy.cl
	PP_GPU_LEGACY_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" PP_GPU_LEGACY_SRC
	${PP_GPU_LEGACY_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pp_gpu_legacy.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/pp_gpu_legacy.h @ONLY)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/pp_common.cl
	PP_COMMON_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" PP_COMMON_SRC
	${PP_COMMON_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pp_common.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/pp_common.h @ONLY)
