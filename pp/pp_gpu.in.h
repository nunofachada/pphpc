/**
 * @file
 * Data structures and function headers for the predator-prey OpenCL
 * GPU simulation.
 * */

#ifndef _PP_GPU_H_
#define _PP_GPU_H_

#include "pp_common.h"

/** GPU kernel source. */
#define PP_GPU_SRC "@PP_GPU_SRC@"

/**
 * The default maximum number of agents: 16777216. Each agent requires
 * 12 bytes, thus by default 192Mb of memory will be allocated for the
 * agents buffer.
 *
 * The agent state is composed of x,y coordinates (2 + 2 bytes), alive
 * flag (1 byte), energy (2 bytes), type (1 byte) and hash (4 bytes).
 * */
#define PPG_DEFAULT_MAX_AGENTS 16777216

/** Default agent size in bits. */
#define PPG_DEFAULT_AGENT_SIZE 64

/** Default agent sort algorithm. */
#define PPG_SORT_DEFAULT "sbitonic"

/**
 * A minimal number of possibly existing agents is required in
 * order to determine minimum global worksizes of kernels.
 * */
#define PPG_MIN_AGENTS 2

/** A description of the program. */
#define PPG_DESCRIPTION "OpenCL predator-prey simulation for the GPU"

/**
 * Main command-line arguments.
 * */
typedef struct pp_g_args {

	/** Parameters file. */
	gchar* params;
	/** Stats output file. */
	gchar* stats;
	/** Profiling info. */
	gchar* prof_info;
	/** Compiler options. */ /// @todo Remove compiler_opts?
	gchar* compiler_opts;
	/** Index of device to use. */
	cl_int dev_idx;
	/** Rng seed. */
	guint32 rng_seed;
	/** Agent size in bits (32 or 64). */
	guint32 agent_size;
	/** Maximum number of agents. */
	cl_uint max_agents;

} PPGArgs;

/**
 * Algorithm selection arguments.
 * */
typedef struct pp_g_args_alg {

	/** Random number generator. */
	gchar* rng;
	/** Agent sorting algorithm. */
	gchar* sort;
	/** Sort algorithm options. */
	gchar* sort_opts;

} PPGArgsAlg;

/**
 * Local work sizes command-line arguments.
 * */
typedef struct pp_g_args_lws {
	/** Default local worksize. */
	size_t deflt;
	/** Init. cells kernel. */
	size_t init_cell;
	/** Init. agents kernel. */
	size_t init_agent;
	/** Grass kernel. */
	size_t grass;
	/** Reduce grass 1 kernel. */
	size_t reduce_grass;
	/** Reduce agent 1 kernel. */
	size_t reduce_agent;
	/** Move agent kernel. */
	size_t move_agent;
	/** Sort agent kernel local worksize. */
	size_t sort_agent;
	/** Find cell agent index local worksize. */
	size_t find_cell_idx;
	/** Agent actions local worksize. */
	size_t action_agent;
} PPGArgsLWS;

/**
 * Simulation kernels.
 * */
typedef struct pp_g_kernels {

	/** Init cells kernel. */
	CCLKernel* init_cell;
	/** Init agents kernel. */
	CCLKernel* init_agent;
	/** Grass kernel. */
	CCLKernel* grass;
	/** Reduce grass 1 kernel. */
	CCLKernel* reduce_grass1;
	/** Reduce grass 2 kernel. */
	CCLKernel* reduce_grass2;
	/** Reduce agent 1 kernel. */
	CCLKernel* reduce_agent1;
	/** Reduce agent 2 kernel. */
	CCLKernel* reduce_agent2;
	/** Move agent kernel. */
	CCLKernel* move_agent;
	/** Sort agent kernels. */
	CCLKernel* *sort_agent;
	/** Find cell agent index kernel. */
	CCLKernel* find_cell_idx;
	/** Agent actions kernel. */
	CCLKernel* action_agent;

} PPGKernels;

/**
 * Vector width command-line arguments.
 * */
typedef struct pp_g_args_vw {

	/** Width of grass kernel vector operations. */
	cl_uint grass;
	/** Width of reduce grass kernels vector operations. */
	cl_uint reduce_grass;
	/** Width of reduce agents kernels vector operations. */
	cl_uint reduce_agent;

} PPGArgsVW;

/**
 * Global work sizes for all the kernels.
 * */
typedef struct pp_g_global_work_sizes {

	/** Init cells kernel global worksize. */
	size_t init_cell;
	/** Init agents kernel global worksize. */
	size_t init_agent;
	/** Grass kernel global worksize. */
	size_t grass;
	/** Reduce grass 1 kernel global worksize. */
	size_t reduce_grass1;
	/** Reduce grass 2 kernel global worksize. */
	size_t reduce_grass2;

} PPGGlobalWorkSizes;

/**
 * Local work sizes for all the kernels.
 * */
typedef struct pp_g_local_work_sizes {

	/** Default workgroup size (defaults to maximum if not specified by
	 * user). */
	size_t deflt;
	/** Maximum workgroup size supported by device. */
	size_t max_lws;
	/** Init cells kernel local worksize. */
	size_t init_cell;
	/** Init agents kernel local worksize. */
	size_t init_agent;
	/** Grass kernel local worksize. */
	size_t grass;
	/** Reduce grass 1 kernel local worksize. */
	size_t reduce_grass1;
	/** Reduce grass 2 kernel local worksize. */
	size_t reduce_grass2;
	/** Reduce agent 1 kernel local worksize. */
	size_t reduce_agent1;
	/** Move agent kernel local worksize. */
	size_t move_agent;
	/** Sort agent kernel local worksize. */
	size_t sort_agent;
	/** Find cell agent index kernel local worksize. */
	size_t find_cell_idx;
	/** Agent actions local worksize. */
	size_t action_agent;

} PPGLocalWorkSizes;

/**
* Size of data structures.
* */
typedef struct pp_g_data_sizes {
	/** Simulation statistics. */
	size_t stats;
	/** Grass regrowth timer array. */
	size_t cells_grass;
	/** Agent index in cell array. */
	size_t cells_agents_index;
	/** Agents type and energy. */
	size_t agents_data;
	/** Local grass reduction array 1. */
	size_t reduce_grass_local1;
	/** Local grass reduction array 2. */
	size_t reduce_grass_local2;
	/** Global grass reduction array. */
	size_t reduce_grass_global;
	/** Local agent reduction array 1. */
	size_t reduce_agent_local1;
	/** Local agent reduction array 2. */
	size_t reduce_agent_local2;
	/** Global agent reduction array. */
	size_t reduce_agent_global;
	/** RNG seeds/state array. */
	size_t rng_seeds;
	/** Number of RNG seeds. */
	size_t rng_seeds_count;
} PPGDataSizes;

/**
 * Device buffers.
 * */
typedef struct pp_g_buffers_device {
	/** Simulation statistics. */
	CCLBuffer* stats;
	/** Grass regrowth timer array. */
	CCLBuffer* cells_grass;
	/** Agent index in cells array. */
	CCLBuffer* cells_agents_index;
	/** Agents type and energy. */
	CCLBuffer* agents_data;
	/** Global grass reduction array. */
	CCLBuffer* reduce_grass_global;
	/** Global agent reduction array. */
	CCLBuffer* reduce_agent_global;
	/** RNG seeds/state array. */
	CCLBuffer* rng_seeds;
} PPGBuffersDevice;

#endif
