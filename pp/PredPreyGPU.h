/** 
 * @file
 * @brief PredPrey OpenCL GPU data structures and function headers.
 * */

#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "PredPreyCommon.h"

/** 
 * @brief Parsed command-line arguments. 
 * */
typedef struct pp_g_args {
	gchar* params;        /**< Parameters file. */
	gchar* stats;         /**< Stats output file. */
	gchar* compiler_opts; /**< Compiler options. */
	size_t gws;           /**< Global work size. */
	size_t lws;           /**< Local work size. */
	cl_int dev_idx;       /**< Index of device to use. */
	guint32 rng_seed;     /**< Rng seed. */
	cl_uint max_agents;   /**< Maximum number of agents. */
	cl_uint vwint;        /**< Vector width for integers */
} PPGArgs;

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
	cl_event write_grass_alive;
	cl_event write_grass_timer;
	cl_event write_agents_number;
	cl_event write_agents_index;
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

PPGSimParams ppg_simparams_init(PPParameters params, cl_uint max_agents);
cl_int ppg_worksizes_compute(PPParameters params, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err);
void ppg_worksizes_print(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws);
char* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGSimParams simParams, gchar* cliOpts);
void ppg_datasizes_get(PPParameters params, PPGSimParams simParams, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws);
cl_int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err);
void ppg_kernels_free(PPGKernels* krnls);
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes* dataSizes, PPParameters params, GRand* rng);
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost);
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes* dataSizes, GError** err);
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice);
void ppg_events_create(PPParameters params, PPGEvents* evts);
void ppg_events_free(PPParameters params, PPGEvents* evts);
cl_int ppg_kernelargs_set(PPGKernels* krnls, PPGBuffersDevice* buffersDevice, PPGSimParams simParams, PPGLocalWorkSizes lws, PPGDataSizes* dataSizes, GError** err);
cl_int ppg_simulate(PPParameters params, CLUZone* zone, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGKernels krnls, PPGEvents* evts, PPGDataSizes dataSizes, PPGBuffersHost buffersHost, PPGBuffersDevice buffersDevice, GError** err);
cl_int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err);
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params);

/** @brief Parse command-line options. */
void ppg_args_parse(int argc, char* argv[], GOptionContext** context, GError** err);

/** @brief Free command line parsing related objects. */
void ppg_args_free(GOptionContext* context);

#endif
