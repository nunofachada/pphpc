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
	size_t deflt;        /**< Default local worksize. */
	size_t init_cell;    /**< Init. cells kernel. */
	size_t init_agent;   /**< Init. agents kernel. */
	size_t grass;        /**< Grass kernel. */
	size_t reduce_grass; /**< Reduce grass 1 kernel. */
	size_t reduce_agent; /**< Reduce agent 1 kernel. */
	size_t move_agent;   /**< Move agent kernel. */
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
 * @brief Global work sizes for all the kernels.
 * */
typedef struct pp_g_global_work_sizes {
	size_t init_cell;     /**< Init cells kernel global worksize. */
	size_t init_agent;    /**< Init agents kernel global worksize. */
	size_t grass;         /**< Grass kernel global worksize. */
	size_t reduce_grass1; /**< Reduce grass 1 kernel global worksize. */
	size_t reduce_grass2; /**< Reduce grass 2 kernel global worksize. */
} PPGGlobalWorkSizes;

/** 
 * @brief Local work sizes for all the kernels.
 * */
typedef struct pp_g_local_work_sizes {
	size_t deflt;         /**< Default workgroup size (defaults to maximum if not specified by user). */
	size_t max_lws;       /**< Maximum workgroup size supported by device. */
	size_t init_cell;     /**< Init cells kernel local worksize. */
	size_t init_agent;    /**< Init agents kernel local worksize. */
	size_t grass;         /**< Grass kernel local worksize. */
	size_t reduce_grass1; /**< Reduce grass 1 kernel local worksize. */
	size_t reduce_grass2; /**< Reduce grass 2 kernel local worksize. */
	size_t reduce_agent1; /**< Reduce agent 1 kernel local worksize. */
	size_t move_agent;    /**< Move agent kernel local worksize. */
} PPGLocalWorkSizes;

/**
 * @brief Simulation kernels.
 * */
typedef struct pp_g_kernels {
	cl_kernel init_cell;     /**< Init cells kernel. */
	cl_kernel init_agent;    /**< Init agents kernel. */
	cl_kernel grass;         /**< Grass kernel. */
	cl_kernel reduce_grass1; /**< Reduce grass 1 kernel. */
	cl_kernel reduce_grass2; /**< Reduce grass 2 kernel. */
	cl_kernel reduce_agent1; /**< Reduce agent 1 kernel. */
	cl_kernel reduce_agent2; /**< Reduce agent 2 kernel. */
	cl_kernel move_agent;    /**< Move agent kernel. */
} PPGKernels;

/** 
* @brief OpenCL events.
* */
typedef struct pp_g_events {
	cl_event write_rng;      /**< Write RNG seeds to device. */
	cl_event init_cell;      /**< Initialize cells. */
	cl_event init_agent;     /**< Initialize agents. */
	cl_event map_stats;      /**< Map stats. */
	cl_event unmap_stats;    /**< Unmap stats. */
	cl_event *grass;         /**< Grass kernel execution. */
	cl_event *read_stats;    /**< Read stats from device to host. */
	cl_event *reduce_grass1; /**< Reduce grass kernel 1 execution. */
	cl_event *reduce_grass2; /**< Reduce grass kernel 2 execution. */
	cl_event *reduce_agent1; /**< Reduce agent kernel 1 execution. */
	cl_event *reduce_agent2; /**< Reduce agent kernel 2 execution. */
	cl_event *move_agent;    /**< Move agent kernel execution. */
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
	size_t agents_x;                 /**< Agents x-position. */
	size_t agents_y;                 /**< Agents y-position. */
	size_t agents_alive;             /**< Agents alive or dead flag. */
	size_t agents_energy;            /**< Agents energy. */
	size_t agents_type;              /**< Agents type (wolf or sheep). */
	size_t agents_hash;              /**< Agents hash (for sorting). */
	size_t reduce_grass_local1;      /**< Local grass reduction array 1. */
	size_t reduce_grass_local2;      /**< Local grass reduction array 2. */
	size_t reduce_grass_global;      /**< Global grass reduction array. */
	size_t reduce_agent_local1;      /**< Local agent reduction array 1. */
	size_t reduce_agent_local2;      /**< Local agent reduction array 2. */
	size_t reduce_agent_global;      /**< Global agent reduction array. */
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
	cl_mem agents_x;                 /**< Agents x-position. */
	cl_mem agents_y;                 /**< Agents y-position. */
	cl_mem agents_alive;             /**< Agents alive or dead flag. */
	cl_mem agents_energy;            /**< Agents energy. */
	cl_mem agents_type;	             /**< Agents type (wolf or sheep). */
	cl_mem agents_hash;              /**< Agents hash (for sorting). */
	cl_mem reduce_grass_global;      /**< Global grass reduction array. */
	cl_mem reduce_agent_global;      /**< Global agent reduction array. */
	cl_mem rng_seeds;                /**< RNG seeds/state array. */
} PPGBuffersDevice;

/** @brief Compute worksizes depending on the device type and number of available compute units. */
cl_int ppg_worksizes_compute(PPParameters paramsSim, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err);

/** @brief Print information about simulation. */
cl_int ppg_info_print(CLUZone *zone, PPGKernels krnls, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGDataSizes dataSizes, gchar* compilerOpts, GError **err);

/** @brief Build OpenCL compiler options string. */
gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPParameters params, gchar* cliOpts);

/** @brief Determine sizes of data buffers. */
void ppg_datasizes_get(PPParameters params, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws);

/** @brief Create OpenCL kernels. */
cl_int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err);

/** @brief Free OpenCL kernels. */
void ppg_kernels_free(PPGKernels* krnls);

/** @brief Initialize host data buffers. */
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes dataSizes, GRand* rng);

/** @brief Free host buffers. */
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost);

/** @brief Initialize device buffers. */
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes dataSizes, GError** err);

/** @brief Free device buffers. */
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice);

/** @brief Create data structure to hold OpenCL events. */
void ppg_events_create(PPParameters params, PPGEvents* evts);

/** @brief Free data structure which holds OpenCL events. */
void ppg_events_free(PPParameters params, PPGEvents* evts);

/** @brief Set fixed kernel arguments. */
cl_int ppg_kernelargs_set(PPGKernels krnls, PPGBuffersDevice buffersDevice, PPGDataSizes dataSizes, GError** err);

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
