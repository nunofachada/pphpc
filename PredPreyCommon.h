#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define __NO_STD_STRING

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include "utils/ocl/clerrors.h"
#include "utils/ocl/clinfo.h"
#include "utils/ocl/fileutils.h"
#include "utils/ocl/bitstuff.h"

#define CONFIG_FILE "config.txt"

typedef struct clzone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint device_type;
	cl_uint cu;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
} CLZONE;


typedef struct stats {
	cl_uint sheep;
	cl_uint wolves;
	cl_uint grass;
} STATS;

typedef struct params {
	unsigned int init_sheep;
	unsigned int sheep_gain_from_food;
	unsigned int sheep_reproduce_threshold;
	unsigned int sheep_reproduce_prob;
	unsigned int init_wolves;
	unsigned int wolves_gain_from_food;
	unsigned int wolves_reproduce_threshold;
	unsigned int wolves_reproduce_prob;
	unsigned int grass_restart;
	unsigned int grid_x;
	unsigned int grid_y;
	unsigned int iters;
} PARAMS;

typedef struct agent_params {
	cl_uint gain_from_food;
	cl_uint reproduce_threshold;
	cl_uint reproduce_prob; /* between 1 and 100 */
} AGENT_PARAMS;

CLZONE getClZone(const char* vendor, const char* kernels_file, cl_uint deviceType);
void destroyClZone(CLZONE zone);
PARAMS loadParams(const char * paramsFile);

