#include "PredPreyCommon.hpp"

typedef struct agent {
	cl_uint x;
	cl_uint y;
	cl_uint alive;
	cl_ushort energy;
	cl_ushort type;
} AGENT __attribute__ ((aligned (16)));

typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
	cl_uint grid_cell_space;
} SIM_PARAMS;

typedef struct stats {
	unsigned int * sheep;
	unsigned int * wolves;
	unsigned int * grass;
} STATS;


void printAgentArray(cl_uint4* array, unsigned int size);
void printGrassMatrix(cl_uint4* matrix, unsigned int size_x,  unsigned int size_y);
CLZONE parseArgs(int argc, char ** argv);
void computeWorkSizes(PARAMS params, cl_uint device_type, cl_uint cu);
void printFixedWorkSizes();
void getKernelEntryPoints(cl_program program);
