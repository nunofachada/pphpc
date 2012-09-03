#include <CL/cl.h>
#include <string.h>
#include "fileutils.h"
#include "clerrors.h"

typedef struct clzone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint device_type;
	cl_uint cu;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
} CLZONE;

CLZONE getClZone(const char* vendor, const char* kernels_file, cl_uint deviceType);
