# - Try to find CL-Ops
# Once done, this will define
#
#  CLOPS_FOUND - system has CL-Ops
#  CLOPS_INCLUDE_DIRS - the CL-Ops include directories
#  CLOPS_LIBRARIES - link these to use CL-Ops

include(LibFindMacros)

# Include dir
find_path(CLOPS_INCLUDE_DIR cl_ops.h)

# Finally the library itself
find_library(CLOPS_LIBRARY cl_ops)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(CLOPS_PROCESS_INCLUDES CLOPS_INCLUDE_DIR)
set(CLOPS_PROCESS_LIBS CLOPS_LIBRARY)
libfind_process(CLOPS)
