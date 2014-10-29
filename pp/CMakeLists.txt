# Predator-Prey implementations
set(PP_IMPLS pp_cpu pp_gpu pp_gpu_legacy)

# Common sources
set(PP_COMMON pp_common.c)

# Add executables
foreach(PP_IMPL ${PP_IMPLS})
	add_executable(${PP_IMPL} ${PP_IMPL}.c ${PP_COMMON})
	target_link_libraries(${PP_IMPL} ${OPENCL_LIBRARIES}
		${GLIB_LIBRARIES} ${GLIB_LDFLAGS} ${CF4OCL2_LIBRARIES}
		${CL_OPS_LIBRARIES} m)
endforeach()

# Copy OpenCL kernels to a location where they can be invoked by the
# sort test executable without installation
file(GLOB CL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cl)
foreach(CL_FILE ${CL_FILES})
	add_custom_command(TARGET pp_cpu POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CL_FILE}
		${CMAKE_CURRENT_BINARY_DIR}
	)
endforeach()
