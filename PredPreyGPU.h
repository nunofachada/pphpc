#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "PredPreyCommon.h"
#include "Profiler.h"

typedef struct cell {
	cl_uchar* grass_alive;
	cl_ushort* grass_timer;
	cl_ushort* agents_number;
	cl_ushort* agents_index;
} CELL;


typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
} SIM_PARAMS;

void computeWorkSizes(PARAMS params, cl_device device);
void printWorkSizes(unsigned int numGrassCount2Loops);
void getKernelEntryPoints(cl_program program);
STATS* initStatsArray(PARAMS params, size_t statsSizeInBytes) ;
CELL* initGrassMatrixHost(PARAMS params, size_t grassSizeInBytes, STATS* statsArrayHost);
cl_ulong* initRngSeedsHost(size_t rngSeedsSizeInBytes);
SIM_PARAMS initSimParams(PARAMS params);
void setGrassKernelArgs(cl_mem grassMatrixDevice, SIM_PARAMS sim_params);
void setCountGrassKernelArgs(cl_mem grassMatrixDevice, cl_mem grassCountDevice, cl_mem statsDevice, SIM_PARAMS sim_params);
void releaseKernels();
void saveResults(char* filename, STATS* statsArrayHost, unsigned int iters);
double printTimmings(struct timeval time0, struct timeval time1);
void showProfilingInfo(double dt, unsigned int iters, cl_uint grasscount2_event_index);

#endif
