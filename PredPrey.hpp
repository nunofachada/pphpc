#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "utils/clerrors.h"
#include "utils/clutils.h"
#include "utils/fileutils.h"
#include "utils/bitstuff.h"

typedef struct stats {
	unsigned int * sheep;
	unsigned int * wolves;
	unsigned int * grass;
} STATS;

typedef struct agent {
	cl_uint x;
	cl_uint y;
	cl_uint alive;
	cl_ushort energy;
	cl_ushort type;
} AGENT __attribute__ ((aligned (16)));

typedef struct agent_params {
	cl_uint gain_from_food;
	cl_uint reproduce_threshold;
	cl_uint reproduce_prob; /* between 1 and 100 */
} AGENT_PARAMS;

typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
	cl_uint grid_cell_space;
} SIM_PARAMS;

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

void printAgentArray(cl_uint4* array, unsigned int size);
void printGrassMatrix(cl_uint4* matrix, unsigned int size_x,  unsigned int size_y);
PARAMS loadParams(const char * paramsFile);
CLZONE parseArgs(int argc, char ** argv);
