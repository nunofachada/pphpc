#include "PredPreyCommon.h"


typedef struct cell {
	cl_uint grass;
	cl_ushort numpreys_start;
	cl_ushort numpreys_end;
} CELL __attribute__ ((aligned (8)));

typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
} SIM_PARAMS;

void showKernelInfo();
