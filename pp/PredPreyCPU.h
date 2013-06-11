/** 
 * @file
 * @brief PredPrey OpenCL CPU data structures and function headers.
 */
 

#ifndef PREDPREYCPU_H
#define PREDPREYCPU_H

#include "PredPreyCommon.h"

/** @brief Parsed command-line arguments. */
typedef struct pp_c_args {
	char *params;
	char *stats;
	char* compiler_opts;
	size_t gws;
	size_t lws;
	cl_uint dev_idx;
	char rng_seed_given;
	guint32 rng_seed;
	char verbose;
} PPCArgs;


typedef struct pp_c_agent {
	cl_uint energy;
	cl_uint action;
	cl_uint type;
	cl_uint next;
} PPCAgent __attribute__ ((aligned (16)));


typedef struct pp_c_sim_params {
	cl_uint size_x;
	cl_uint size_y;
	cl_uint size_xy;
	cl_uint max_agents;
	cl_uint null_agent_pointer;
	cl_uint grass_restart;
	cl_uint lines_per_thread;
	cl_uint bogus;
} PPCSimParams __attribute__ ((aligned (32)));

typedef struct pp_c_cell {
	cl_uint grass;
	cl_uint agent_pointer;
} PPCCell;

// Work sizes for kernels step1 and step2
typedef struct pp_c_work_sizes {
	size_t gws;
	size_t lws;
	size_t rows_per_workitem;
	size_t max_gws;
} PPCWorkSizes;

// Kernels
typedef struct pp_c_kernels {
	cl_kernel step1;
	cl_kernel step2;
} PPCKernels;

// Events
typedef struct pp_c_events {
	cl_event map_stats_start;
	cl_event unmap_stats_start;
	cl_event map_matrix;
	cl_event unmap_matrix;
	cl_event map_agents;
	cl_event unmap_agents;
	cl_event map_rng_seeds;
	cl_event unmap_rng_seeds;
	cl_event map_agent_params;
	cl_event unmap_agent_params;
	cl_event *step1;
	cl_event *step2;
	cl_event map_stats_end;
	cl_event unmap_stats_end;
} PPCEvents;

// Data sizes
typedef struct pp_c_data_sizes {
	size_t stats;
	size_t matrix;
	size_t agents;
	size_t rng_seeds;
	size_t agent_params;
	size_t sim_params;
} PPCDataSizes;

// Host buffers
typedef struct pp_c_buffers_host {
	PPStatistics* stats;
	PPCCell* matrix;
	PPCAgent *agents;
	cl_ulong* rng_seeds;
	PPAgentParams* agent_params;
} PPCBuffersHost;

// Device buffers
typedef struct pp_c_buffers_device {
	cl_mem stats;
	cl_mem matrix;
	cl_mem agents;
	cl_mem rng_seeds;
	cl_mem agent_params;
	cl_mem sim_params;
} PPCBuffersDevice;

/** @brief Determine effective worksizes to use in simulation. */
int ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes, cl_uint cu, unsigned int num_rows, GError **err);

/** @brief Print information about the simulation parameters. */
void ppc_simulation_info_print(cl_int cu, PPCWorkSizes workSizes, PPCArgs args);

/** @brief Get kernel entry points. */
cl_int ppc_kernels_create(cl_program program, PPCKernels* krnls, GError** err);

/** @brief Release kernels.  */
void ppc_kernels_free(PPCKernels* krnls);

/** @brief Initialize simulation parameters in host, to be sent to kernels. */
PPCSimParams ppc_simparams_init(PPParameters params, cl_uint null_agent_pointer, size_t lines_per_thread);

/** @brief Determine buffer sizes. */
void ppc_datasizes_get(PPParameters params, PPCSimParams simParams, PPCDataSizes* dataSizes, size_t num_threads);

/** @brief Initialize and map host/device buffers. */
cl_int ppc_buffers_init(CLUZone zone, size_t num_threads, PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, PPCSimParams simParams, GRand* rng, GError** err);

/** @brief Set fixed kernel arguments.  */
cl_int ppc_kernelargs_set(PPCKernels* krnls, PPCBuffersDevice* buffersDevice, PPCSimParams simParams, GError** err);

/** @brief Perform simulation! */
cl_uint ppc_simulate(PPCWorkSizes workSizes, PPParameters params, CLUZone zone, PPCKernels krnls, PPCEvents* evts, PPCDataSizes dataSizes, PPCBuffersHost buffersHost, PPCBuffersDevice buffersDevice, GError** err);

/** @brief Release OpenCL memory objects. */
void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice);
	
/** @brief Free host resources. */ 
void ppc_hostbuffers_free(PPCBuffersHost* buffersHost);

/** @brief Create events data structure. */
void ppc_events_create(PPParameters params, PPCEvents* evts);

/** @brief Free events. */
void ppc_events_free(PPParameters params, PPCEvents* evts);

/** @brief Analyze events, show profiling info. */
cl_int ppc_profiling_analyze(ProfCLProfile* profile, PPCEvents* evts, PPParameters params, GError** err);

/** @brief Get statistics. */
cl_int ppc_stats_get(char* filename, CLUZone zone, PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GError** err);

/** @brief Parse one command-line option. */
error_t ppc_opt_parse(int key, char *arg, struct argp_state *state);

#endif
