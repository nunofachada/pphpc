# - Try to find cl_ops
# Once done, this will define
#
#  CL_OPS_FOUND - system has cl_ops
#  CL_OPS_INCLUDE_DIRS - the cl_ops include directories
#  CL_OPS_LIBRARIES - link these to use cl_ops

include(LibFindMacros)

# Include dir
find_path(CL_OPS_INCLUDE_DIR cl_ops.h)

# Finally the library itself
find_library(CL_OPS_LIBRARY cl_ops)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(CL_OPS_PROCESS_INCLUDES CL_OPS_INCLUDE_DIR)
set(CL_OPS_PROCESS_LIBS CL_OPS_LIBRARY)
libfind_process(CL_OPS)
