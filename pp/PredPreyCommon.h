#ifndef PREDPREYCOMMON_H
#define PREDPREYCOMMON_H

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define __NO_STD_STRING

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include <limits.h>
#include "../utils/clutils.h"
#include "../utils/bitstuff.h"
#include "../utils/Profiler.h"

#ifdef CLPROFILER
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE
#else
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE
#endif

#define CONFIG_FILE "config.txt"

typedef struct pp_statistics {
	cl_uint sheep;
	cl_uint wolves;
	cl_uint grass;
} PPStatistics;

typedef struct pp_parameters {
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
} PPParameters;

typedef struct pp_agent_params {
	cl_uint gain_from_food;
	cl_uint reproduce_threshold;
	cl_uint reproduce_prob; /* between 1 and 100 */
} PPAgentParams;

PPParameters pp_load_params(const char * paramsFile);

#endif
