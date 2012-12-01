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

void computeWorkSizes(PARAMS params, cl_uint device_type, cl_uint cu);
void printFixedWorkSizes();
void getKernelEntryPoints(cl_program program);
void showKernelInfo();
STATS* initStatsArrayHost(PARAMS params, size_t statsSizeInBytes) ;
cl_uint* initGrassMatrixHost(PARAMS params, size_t grassSizeInBytes);
cl_ulong* initRngSeedsHost(size_t rngSeedsSizeInBytes);
SIM_PARAMS initSimParams(PARAMS params);
void setGrassKernelArgs(cl_mem grassMatrixDevice, SIM_PARAMS sim_params);
void setCountGrassKernelArgs(cl_mem grassMatrixDevice, cl_mem grassCountDevice, cl_mem statsArrayDevice, cl_mem iterDevice, SIM_PARAMS sim_params);
void releaseKernels();
void saveResults(char* filename, STATS statsArrayHost);
double printTimmings(struct timeval time0, struct timeval time1);
void showProfilingInfo(double dt);

