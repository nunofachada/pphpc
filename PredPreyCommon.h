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
#include <limits.h>
#include "utils/clerrors.h"
#include "utils/clinfo.h"
#include "utils/fileutils.h"
#include "utils/bitstuff.h"

#define CONFIG_FILE "config.txt"

#define MAX_PLATFORMS 10
#define MAX_DEVICES_PER_PLATFORM 10
#define MAX_DEVICES_TOTAL 20
#define MAX_AUX_BUFF 500

typedef struct cl_zone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint device_type;
	cl_uint cu;
	cl_context context;
	cl_command_queue * queues;
	cl_program program;
	unsigned int numQueues;
} CLZone;

typedef struct device_info {
	cl_device_id id;
	char name[MAX_AUX_BUFF];
	cl_platform_id platformId;
	char platformName[MAX_AUX_BUFF];
} DeviceInfo;

typedef struct statistics {
	cl_uint sheep;
	cl_uint wolves;
	cl_uint grass;
} Statistics;

typedef struct parameters {
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
} Parameters;

typedef struct agent_params {
	cl_uint gain_from_food;
	cl_uint reproduce_threshold;
	cl_uint reproduce_prob; /* between 1 and 100 */
} AgentParams;

CLZone getClZone(const char* kernels_file, cl_uint deviceType, cl_uint numQueues, char profile);

void destroyClZone(CLZone zone);
PARAMS loadParams(const char * paramsFile);

#endif
