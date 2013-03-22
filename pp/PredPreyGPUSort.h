#ifndef PREDPREYGPUSORT_H
#define PREDPREYGPUSORT_H

#include "PredPreyCommon.h"

typedef struct pp_gs_agent {
	cl_uint x;
	cl_uint y;
	cl_uint alive;
	cl_ushort energy;
	cl_ushort type;
} PPGSAgent __attribute__ ((aligned (16)));

typedef struct pp_gs_sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
	cl_uint grid_cell_space;
} PPGSSimParams;

void computeWorkSizes(PPParameters params, cl_uint device_type, cl_uint cu);
void printFixedWorkSizes();
cl_int getKernelEntryPoints(cl_program program, GError** err);

#endif
