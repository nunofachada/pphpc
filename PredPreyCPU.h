#include "PredPreyCommon.h"

typedef struct agent {
	cl_uint energy;
	cl_uint action;
	cl_uint type;
	cl_uint next;
} AGENT __attribute__ ((aligned (16)));


typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint null_agent_pointer;
	cl_uint grass_restart;
	cl_uint lines_per_thread;
} SIM_PARAMS;

typedef struct cell {
	cl_uint grass;
	cl_uint agent_pointer;
} CELL;

