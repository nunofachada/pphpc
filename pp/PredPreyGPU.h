/** 
 * @file
 * @brief PredPrey OpenCL GPU data structures and function headers.
 * */

#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "PredPreyCommon.h"

/** 
 * @brief Main command-line arguments. 
 * */
typedef struct pp_g_args {
	gchar* params;        /**< Parameters file. */
	gchar* stats;         /**< Stats output file. */
	gchar* compiler_opts; /**< Compiler options. */
	cl_int dev_idx;       /**< Index of device to use. */
	guint32 rng_seed;     /**< Rng seed. */
	gchar* rngen;         /**< Random number generator. */
	cl_uint max_agents;   /**< Maximum number of agents. */
} PPGArgs;

/**
 * @brief Local work sizes command-line arguments.
 * */
typedef struct pp_g_args_lws {
	size_t init_cell;    /**< Init. cells kernel. */
	size_t grass;        /**< Grass kernel. */
	size_t reduce_grass; /**< Reduce grass 1 kernel. */
} PPGArgsLWS;

/**
 * @brief Vector width command-line arguments.
 * */
typedef struct pp_g_args_vw {
	cl_uint char_vw;  /**< Width of char vector operations. */
	cl_uint short_vw; /**< Width of short vector operations. */
	cl_uint int_vw;   /**< Width of int vector operations. */
	cl_uint float_vw; /**< Width of float vector operations. */
	cl_uint long_vw;  /**< Width of long vector operations. */
} PPGArgsVW;

/** 
 * @brief Simulation parameters to pass to device.
 * @todo Maybe this can be constant passed as compiler options? 
 * */
typedef struct pp_g_sim_params {
	cl_uint size_x;        /**< Width (number of cells) of environment.*/
	cl_uint size_y;        /**< Height (number of cells) of environment. */
	cl_uint size_xy;       /**< Dimension of environment (total number of cells). */
	cl_uint max_agents;    /**< Maximum number of agentes. */
	cl_uint grass_restart; /**< Number of iterations that the grass takes to regrow after being eaten by a sheep. */
} PPGSimParams;

/** 
 * @brief Global work sizes for all the kernels.
 * */
typedef struct pp_g_global_work_sizes {
	size_t init_cell;     /**< Init cells kernel global worksize. */
	size_t grass;         /**< Grass kernel global worksize. */
	size_t reduce_grass1; /**< Reduce grass 1 kernel global worksize. */
	size_t reduce_grass2; /**< Reduce grass 2 kernel global worksize. */
} PPGGlobalWorkSizes;

/** 
 * @brief Local work sizes for all the kernels.
 * */
typedef struct pp_g_local_work_sizes {
	size_t init_cell;     /**< Init cells kernel local worksize. */
	size_t grass;         /**< Grass kernel local worksize. */
	size_t reduce_grass1; /**< Reduce grass 1 kernel local worksize. */
	size_t reduce_grass2; /**< Reduce grass 2 kernel local worksize. */
} PPGLocalWorkSizes;

/**
 * @brief Simulation kernels.
 * */
typedef struct pp_g_kernels {
	cl_kernel init_cell;     /**< Init cells kernel. */
	cl_kernel grass;         /**< Grass kernel. */
	cl_kernel reduce_grass1; /**< Reduce grass 1 kernel. */
	cl_kernel reduce_grass2; /**< Reduce grass 2 kernel. */
} PPGKernels;

/** 
* @brief OpenCL events.
* */
typedef struct pp_g_events {
	cl_event write_rng;      /**< Write RNG seeds to device. */
	cl_event init_cell;      /**< Initialize cells. */
	cl_event *grass;         /**< Grass kernel execution. */
	cl_event *read_stats;    /**< Read stats from device to host. */
	cl_event *reduce_grass1; /**< Reduce grass kernel 1 execution. */
	cl_event *reduce_grass2; /**< Reduce grass kernel 2 execution. */
} PPGEvents;

/** 
* @brief Size of data structures.
* */
typedef struct pp_g_data_sizes {
	size_t stats;                    /**< Simulation statistics. */
	size_t cells_grass_alive;        /**< "Is grass alive?" array. */
	size_t cells_grass_timer;        /**< Grass regrowth timer array. */
	size_t cells_agents_index_start; /**< Agent index start array. */
	size_t cells_agents_index_end;   /**< Agent index end array. */
	size_t reduce_grass_local;       /**< Local grass reduction array. */
	size_t reduce_grass_global;      /**< Global grass reduction array. */
	size_t rng_seeds;                /**< RNG seeds/state array. */
	size_t rng_seeds_count;          /**< Number of RNG seeds. */
} PPGDataSizes;

/**
 * @brief Host buffers.
 * */
typedef struct pp_g_buffers_host {
	PPStatistics* stats; /**< Simulation statistics array. */
	cl_ulong* rng_seeds; /**< RNG seeds/states array. */
} PPGBuffersHost;

/**
 * @brief Device buffers.
 * */
typedef struct pp_g_buffers_device {
	cl_mem stats;                    /**< Simulation statistics. */
	cl_mem cells_grass_alive;        /**< "Is grass alive?" array. */
	cl_mem cells_grass_timer;        /**< Grass regrowth timer array. */
	cl_mem cells_agents_index_start; /**< Agent index start array. */
	cl_mem cells_agents_index_end;   /**< Agent index end array. */
	cl_mem reduce_grass_global;      /**< Global grass reduction array. */
	cl_mem rng_seeds;                /**< RNG seeds/state array. */
} PPGBuffersDevice;

/** @brief Initialize simulation parameters in host, to be sent to GPU. */
PPGSimParams ppg_simparams_init(PPParameters params, cl_uint max_agents);

/** @brief Compute worksizes depending on the device type and number of available compute units. */
cl_int ppg_worksizes_compute(PPParameters paramsSim, PPGSimParams paramsDev, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err);

/** @brief Print information about simulation. */
cl_int ppg_info_print(CLUZone *zone, PPGKernels krnls, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGDataSizes dataSizes, gchar* compilerOpts, GError **err);

/** @brief Build OpenCL compiler options string. */
gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGSimParams paramsDev, gchar* cliOpts);

/** @brief Determine sizes of data buffers. */
void ppg_datasizes_get(PPParameters params, PPGSimParams paramsDev, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws);

/** @brief Create OpenCL kernels. */
cl_int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err);

/** @brief Free OpenCL kernels. */
void ppg_kernels_free(PPGKernels* krnls);

/** @brief Initialize host data buffers. */
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes* dataSizes, PPParameters params, GRand* rng);

/** @brief Free host buffers. */
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost);

/** @brief Initialize device buffers. */
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes* dataSizes, GError** err);

/** @brief Free device buffers. */
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice);

/** @brief Create data structure to hold OpenCL events. */
void ppg_events_create(PPParameters params, PPGEvents* evts);

/** @brief Free data structure which holds OpenCL events. */
void ppg_events_free(PPParameters params, PPGEvents* evts);

/** @brief Set fixed kernel arguments. */
cl_int ppg_kernelargs_set(PPGKernels* krnls, PPGBuffersDevice* buffersDevice, PPGSimParams paramsDev, PPGLocalWorkSizes lws, PPGDataSizes* dataSizes, GError** err);

/** @brief Perform Predator-Prey simulation. */
cl_int ppg_simulate(PPParameters params, CLUZone* zone, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGKernels krnls, PPGEvents* evts, PPGDataSizes dataSizes, PPGBuffersHost buffersHost, PPGBuffersDevice buffersDevice, GError** err);

/** @brief Perform profiling analysis. */
cl_int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err);

/** @brief Save simulation statistics. */
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params);

/** @brief Parse command-line options. */
void ppg_args_parse(int argc, char* argv[], GOptionContext** context, GError** err);

/** @brief Free command line parsing related objects. */
void ppg_args_free(GOptionContext* context);

#endif
