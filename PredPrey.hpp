#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "../clutils/clerrors.h"
#include "../clutils/fileutils.h"

typedef struct clzone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint cu;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
} CLZONE;

typedef struct stats {
	unsigned int * sheep;
	unsigned int * wolves;
	unsigned int * grass;
} STATS;

typedef struct agent {
	cl_uint x;
	cl_uint y;
	cl_ushort energy;
	cl_ushort type;
	cl_uint alive;
} AGENT;

typedef struct agent_params {
	cl_uint gain_from_food;
	cl_uint reproduce_threshold;
	cl_uint reproduce_prob; /* between 1 and 100 */
} AGENT_PARAMS;

typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint max_agents;
	cl_uint grass_restart;
	cl_uint grid_cell_space;
} SIM_PARAMS;

CLZONE getClZone(const char* vendor, cl_uint deviceType);
void printAgentArray(cl_uint4* array, unsigned int size);
void printGrassMatrix(cl_uint4* matrix, unsigned int size_x,  unsigned int size_y);

