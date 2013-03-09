#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "PredPreyCommon.h"
#include "Profiler.h"


// Simulation parameters useful for PredPreyGPU (TODO really?)
typedef struct pp_g_sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint grass_restart;
} PPGSimParams;

// Global work sizes
typedef struct pp_g_global_work_sizes {
	size_t grass;
	size_t reduce_grass1;
	size_t reduce_grass2;
} PPGGlobalWorkSizes;

// Local work sizes
typedef struct pp_g_local_work_sizes {
	size_t grass;
	size_t reduce_grass1;
	size_t reduce_grass2;
} PPGLocalWorkSizes;

// Kernels
typedef struct pp_g_kernels {
	cl_kernel grass;
	cl_kernel reduce_grass1;
	cl_kernel reduce_grass2;
} PPGKernels;

// Events
typedef struct pp_g_events {
	cl_event write_grass;
	cl_event write_rng;
	cl_event *grass;
	cl_event *read_stats;
	cl_event *reduce_grass1;
	cl_event *reduce_grass2;
} PPGEvents;

// Data sizes
typedef struct pp_g_data_sizes {
	size_t stats;
	size_t cells_grass_alive;
	size_t cells_grass_timer;
	size_t cells_agents_number;
	size_t cells_agents_index;
	size_t reduce_grass_local;
	size_t reduce_grass_global;
	size_t rng_seeds;
} PPGDataSizes;

// Host buffers
typedef struct pp_g_buffers_host {
	PPStatistics* stats;
	cl_uchar* cells_grass_alive;
	cl_ushort* cells_grass_timer;
	cl_ushort* cells_agents_number;
	cl_ushort* cells_agents_index;
	cl_ulong* rng_seeds;
} PPGBuffersHost;

// Device buffers
typedef struct pp_g_buffers_device {
	cl_mem stats;
	cl_mem cells_grass_alive;
	cl_mem cells_grass_timer;
	cl_mem cells_agents_number;
	cl_mem cells_agents_index;
	cl_mem reduce_grass_global;
	cl_mem rng_seeds;
} PPGBuffersDevice;


void simulate();
void profilingAnalysis(PPGEvents* evts, PPParameters params);
void computeWorkSizes(PPParameters params, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws);
void printWorkSizes(PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws);
void createKernels(cl_program program, PPGKernels* krnls);
void freeKernels(PPGKernels* krnls);
PPGSimParams initSimParams(PPParameters params);
void setFixedKernelArgs(PPGKernels* krnls, PPGBuffersDevice* buffersDevice, PPGSimParams simParams, PPGLocalWorkSizes lws);
void saveResults(char* filename, PPStatistics* statsArray, PPParameters params);
void getDataSizesInBytes(PPParameters params, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws);
void createHostBuffers(PPGBuffersHost* buffersHost, PPGDataSizes* dataSizes, PPParameters params, GRand* rng);
void freeHostBuffers(PPGBuffersHost* buffersHost);
void createDeviceBuffers(cl_context context, PPGBuffersHost* buffersHost, PPGBuffersDevice* buffersDevice, PPGDataSizes* dataSizes);
void freeDeviceBuffers(PPGBuffersDevice* buffersDevice);
void createEventsDataStructure(PPParameters params, PPGEvents* evts);
void freeEventsDataStructure(PPParameters params, PPGEvents* evts) ;


#endif
