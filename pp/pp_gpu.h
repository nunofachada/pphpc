/** 
 * @file
 * @brief PredPrey OpenCL GPU data structures and function headers.
 * */

#ifndef PREDPREYGPU_H
#define PREDPREYGPU_H

#include "pp_common.h"

/** The default maximum number of agents: 16777216. Each agent requires
 * 12 bytes, thus by default 192Mb of memory will be allocated for the
 * agents buffer. 
 * 
 * The agent state is composed of x,y coordinates (2 + 2 bytes), alive
 * flag (1 byte), energy (2 bytes), type (1 byte) and hash (4 bytes).
 * */
#define PPG_DEFAULT_MAX_AGENTS 16777216

/** Default agent size in bits. */
#define PPG_DEFAULT_AGENT_SIZE 64

/**
 * @brief A minimal number of possibly existing agents is required in
 * order to determine minimum global worksizes of kernels. 
 * */
#define PPG_MIN_AGENTS 2

/** A description of the program. */
#define PPG_DESCRIPTION "OpenCL predator-prey simulation for the GPU"

/** 
 * @brief Main command-line arguments. 
 * */
typedef struct pp_g_args {
	gchar* params;        /**< Parameters file. */
	gchar* stats;         /**< Stats output file. */
#ifdef CLPROFILER	
	gchar* prof_info;     /**< Profiling info. */
#endif
	gchar* compiler_opts; /**< Compiler options. */
	cl_int dev_idx;       /**< Index of device to use. */
	guint32 rng_seed;     /**< Rng seed. */
	guint32 agent_size;   /**< Agent size in bits (32 or 64). */
	cl_uint max_agents;   /**< Maximum number of agents. */
} PPGArgs;

/** 
 * @brief Algorithm selection arguments. 
 * */
typedef struct pp_g_args_alg {
	gchar* rng;  /**< Random number generator. */
	gchar* sort; /**< Agent sorting algorithm. */
} PPGArgsAlg;

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
	size_t sort_agent;   /**< Sort agent kernel local worksize. */
	size_t find_cell_idx;/**< Find cell agent index local worksize. */
	size_t action_agent; /**< Agent actions local worksize. */
} PPGArgsLWS;

/**
 * @brief Vector width command-line arguments.
 * */
typedef struct pp_g_args_vw {
	cl_uint grass;        /**< Width of grass kernel vector operations. */
	cl_uint reduce_grass; /**< Width of reduce grass kernels vector operations. */
	cl_uint reduce_agent; /**< Width of reduce agents kernels vector operations. */
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
	size_t sort_agent;    /**< Sort agent kernel local worksize. */
	size_t find_cell_idx; /**< Find cell agent index kernel local worksize. */
	size_t action_agent;  /**< Agent actions local worksize. */
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
	cl_kernel *sort_agent;   /**< Sort agent kernels. */
	cl_kernel find_cell_idx; /**< Find cell agent index kernel. */
	cl_kernel action_agent;  /**< Agent actions kernel. */
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
	GArray* sort_agent;      /**< Sort agent kernel executions. */
	cl_event *find_cell_idx; /**< Find cell agent index. */
	cl_event *action_agent;  /**< Agent actions kernel execution. */
} PPGEvents;

/** 
* @brief Size of data structures.
* */
typedef struct pp_g_data_sizes {
	size_t stats;                    /**< Simulation statistics. */
	size_t cells_grass;              /**< Grass regrowth timer array. */
	size_t cells_agents_index;       /**< Agent index in cell array. */
	size_t agents_data;              /**< Agents type and energy. */
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
	cl_mem cells_grass;              /**< Grass regrowth timer array. */
	cl_mem cells_agents_index;       /**< Agent index in cells array. */
	cl_mem agents_data;	             /**< Agents type and energy. */
	cl_mem reduce_grass_global;      /**< Global grass reduction array. */
	cl_mem reduce_agent_global;      /**< Global agent reduction array. */
	cl_mem rng_seeds;                /**< RNG seeds/state array. */
} PPGBuffersDevice;

/** @brief Compute worksizes depending on the device type and number of available compute units. */
int ppg_worksizes_compute(PPParameters paramsSim, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err);

/** @brief Print information about simulation. */
void ppg_info_print(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGDataSizes dataSizes, gchar* compilerOpts);

/** @brief Build OpenCL compiler options string. */
gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPParameters params, gchar* kernelPath, gchar* cliOpts);

/** @brief Determine sizes of data buffers. */
void ppg_datasizes_get(PPParameters params, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws);

/** @brief Create OpenCL kernels. */
int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err);

/** @brief Free OpenCL kernels. */
void ppg_kernels_free(PPGKernels* krnls);

/** @brief Initialize host data buffers. */
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes dataSizes, GRand* rng);

/** @brief Free host buffers. */
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost);

/** @brief Initialize device buffers. */
int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes dataSizes, GError** err);

/** @brief Free device buffers. */
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice);

/** @brief Create data structure to hold OpenCL events. */
int ppg_events_create(PPParameters params, PPGEvents* evts, GError **err);

/** @brief Free data structure which holds OpenCL events. */
void ppg_events_free(PPParameters params, PPGEvents* evts);

/** @brief Set fixed kernel arguments. */
int ppg_kernelargs_set(PPGKernels krnls, PPGBuffersDevice buffersDevice, PPGDataSizes dataSizes, PPGLocalWorkSizes lws, GError** err);

/** @brief Perform Predator-Prey simulation. */
int ppg_simulate(PPParameters params, CLUZone* zone, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGKernels krnls, PPGEvents* evts, PPGDataSizes dataSizes, PPGBuffersHost buffersHost, PPGBuffersDevice buffersDevice, GError** err);

/** @brief Dump simulation data for current iteration. */
int ppg_dump(int iter, int dump_type, CLUZone* zone, FILE* fp_agent_dump, FILE* fp_cell_dump, size_t gws_action_agent, size_t gws_move_agent, PPParameters params, PPGDataSizes dataSizes, PPGBuffersDevice buffersDevice, void *agents_data, cl_uint2 *cells_agents_index, cl_uint *cells_grass, GError** err);

/** @brief Perform profiling analysis. */
int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err);

/** @brief Save simulation statistics. */
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params);

/** @brief Parse command-line options. */
int ppg_args_parse(int argc, char* argv[], GOptionContext** context, GError** err);

/** @brief Free command line parsing related objects. */
void ppg_args_free(GOptionContext* context);

#endif
