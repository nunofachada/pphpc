#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "PredPreyCommon.h"
#include "Profiler.h"


// Simulation parameters useful for PredPreyGPU (TODO really?)
typedef struct sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
} SimParams;

// Global work sizes
typedef struct global_work_sizes {
	size_t grass;
	size_t reduce_grass1;
	size_t reduce_grass2;
} GlobalWorkSizes;

// Local work sizes
typedef struct local_work_sizes {
	size_t grass;
	size_t reduce_grass1;
	size_t reduce_grass2;
} LocalWorkSizes;

// Kernels
typedef struct kernels {
	cl_kernel grass;
	cl_kernel reduce_grass1;
	cl_kernel reduce_grass2;
} Kernels;

// Events
typedef struct events {
	cl_event write_grass;
	cl_event write_rng;
	cl_event *grass;
	cl_event *read_stats;
	cl_event *reduce_grass1;
	cl_event *reduce_grass2;
} Events;

// Data sizes
typedef struct data_sizes {
	size_t stats;
	size_t cells_grass_alive;
	size_t cells_grass_timer;
	size_t cells_agents_number;
	size_t cells_agents_index;
	size_t reduce_grass_local;
	size_t reduce_grass_global;
	site_t rng_seeds;
} DataSizes;

// Host buffers
typedef struct buffers_host {
	Statistics* stats;
	cl_uchar* cells_grass_alive;
	cl_ushort* cells_grass_timer;
	cl_ushort* cells_agents_number;
	cl_ushort* cells_agents_index;
	cl_ulong* rng_seeds;
} BuffersHost;

// Device buffers
typedef struct buffers_device {
	cl_mem stats;
	cl_mem cells_grass_alive;
	cl_mem cells_grass_timer;
	cl_mem cells_agents_number;
	cl_mem cells_agents_index;
	cl_mem reduce_grass_global;
	cl_mem rng_seeds;
} BuffersDevice;

void computeWorkSizes(PARAMS params, cl_device device, GlobalWorkSizes *gws, LocalWorkSizes *lws);
void printWorkSizes(GlobalWorkSizes *gws, LocalWorkSizes *lws);
void createKernels(cl_program program, Kernels* krnls);
SimParams initSimParams(Parameters params);
void setGrassKernelArgs(cl_mem grassMatrixDevice, SimParams sim_params);
void setCountGrassKernelArgs(cl_mem grassMatrixDevice, cl_mem grassCountDevice, cl_mem statsDevice, SIM_PARAMS sim_params);
void releaseKernels();
void saveResults(char* filename, STATS* statsArrayHost, unsigned int iters);
void getDataSizesInBytes(params, &dataSizes)

#endif
