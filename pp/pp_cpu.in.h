/**
 * @file
 * Data structures and function headers for the predator-prey OpenCL
 * CPU simulation.
 */

#ifndef _PP_CPU_H_
#define _PP_CPU_H_

#include "pp_common.h"

/** CPU kernel source. */
#define PP_CPU_SRC "@PP_CPU_SRC@"

/**
 * Constant which indicates no further agents are in a cell.
 * */
#define NULL_AGENT_POINTER UINT_MAX

/**
 * Parsed command-line arguments.
 * */
typedef struct pp_c_args {

	/** Parameters file. */
	gchar* params;
	/** Stats output file. */
	gchar* stats;
	/** Compiler options. */
	gchar* compiler_opts;
	/** Global work size. */
	size_t gws;
	/** Local work size. */
	size_t lws;
	/** Index of device to use. */
	cl_int dev_idx;
	/** Rng seed. */
	guint32 rng_seed;
	/** Random number generator. */
	gchar* rngen;
	/** Maximum number of agents. */
	cl_uint max_agents;

} PPCArgs;

/**
 * Agent object for OpenCL kernels.
 * */
typedef struct pp_c_agent {

	/** Agent energy. */
	cl_uint energy;

	/** True if agent already acted this turn, false otherwise. */
	cl_uint action;

	/** Type of agent (sheep or wolf). */
	cl_uint type;

	/** Pointer to next agent in current cell. */
	cl_uint next;

} PPCAgent __attribute__ ((aligned (16)));


/**
 * Simulation parameters for OpenCL kernels.
 *
 * @todo Maybe this can be constant passed as compiler options?
 * */
typedef struct pp_c_sim_params {

	/** Width (number of cells) of environment.*/
	cl_uint size_x;
	/** Height (number of cells) of environment. */
	cl_uint size_y;
	/** Dimension of environment (total number of cells). */
	cl_uint size_xy;
	/** Maximum number of agentes. */
	cl_uint max_agents;
	/** Constant which indicates no further agents are in cell. */
	cl_uint null_agent_pointer;
	/** Number of iterations that the grass takes to regrow after being
	 * eaten by a sheep. */
	cl_uint grass_restart;
	/** Number of rows to be processed by each workitem. */
	cl_uint rows_per_workitem;
	/** Does nothing but align the struct to 32 bytes. */
	cl_uint bogus;

} PPCSimParams __attribute__ ((aligned (32)));

/**
 * Cell object for OpenCL kernels.
 * */
typedef struct pp_c_cell {

	/** Number of iterations left for grass to grow (or zero if grass
	 * is alive). */
	cl_uint grass;
	/** Pointer to first agent in cell. */
	cl_uint agent_pointer;

} PPCCell;

/**
 * Work sizes for kernels step1 and step2, and other work/memory
 * sizes related to the simulation.
 * */
typedef struct pp_c_work_sizes {

	/** Global worksize. */
	size_t gws;
	/** Local worksize. */
	size_t lws;
	/** Number of rows to be processed by each workitem. */
	size_t rows_per_workitem;
	/** Maximum global worksize for given simulation parameters. */
	size_t max_gws;
	/** Maximum number of agentes. */
	size_t max_agents;

} PPCWorkSizes;

/**
* Size of data structures.
* */
typedef struct pp_c_data_sizes {

	/** Size of stats data structure. */
	size_t stats;
	/** Size of matrix data structure. */
	size_t matrix;
	/** Size of agents data structure. */
	size_t agents;
	/** Number of RNG seeds required for RNG. */
	size_t rng_seeds_count;
	/** Size of agent parameters data structure. */
	size_t agent_params;
	/** Size of simulation parameters data structure. */
	size_t sim_params;

} PPCDataSizes;

/**
 * Host buffers.
 * */
typedef struct pp_c_buffers_host {

	/** Statistics. */
	PPStatistics* stats;
	/** Matrix of environment cells. */
	PPCCell* matrix;
	/** Array of agents. */
	PPCAgent *agents;
	/** Agent parameters. */
	PPAgentParams* agent_params;
	/** Simulation parameters. */
	PPCSimParams sim_params;

} PPCBuffersHost;

/**
 * Device buffers.
 * */
 typedef struct pp_c_buffers_device {

	/** Statistics. */
	CCLBuffer* stats;
	/** Matrix of environment cells. */
	CCLBuffer* matrix;
	/** Array of agents. */
	CCLBuffer* agents;
	/** Array of RNG seeds. */
	CCLBuffer* rng_seeds;
	/** Agent parameters. */
	CCLBuffer* agent_params;
	/** Simulation parameters. */
	CCLBuffer* sim_params;

} PPCBuffersDevice;

/* Determine effective worksizes to use in simulation. */
void ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes,
	cl_uint num_rows, GError **err);

/* Print information about the simulation parameters. */
void ppc_simulation_info_print(CCLDevice* dev, PPCWorkSizes workSizes,
	PPCArgs args, GError** err);

/* Initialize simulation parameters in host, to be sent to kernels. */
PPCSimParams ppc_simparams_init(PPParameters params,
	cl_uint null_agent_pointer, PPCWorkSizes ws);

/* Determine buffer sizes. */
void ppc_datasizes_get(PPParameters params, PPCDataSizes* dataSizes,
	PPCWorkSizes ws);

/* Initialize and map host/device buffers. */
void ppc_buffers_init(CCLContext* ctx, CCLQueue* cq, PPCWorkSizes ws,
	PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice,
	PPCDataSizes dataSizes, PPParameters params, CloRng* rng_clo,
	GError** err);

/* Set fixed kernel arguments.  */
void ppc_kernelargs_set(CCLProgram* prg,
	PPCBuffersDevice* buffersDevice, GError** err);

/* Perform simulation! */
void ppc_simulate(PPCWorkSizes workSizes, PPParameters params,
	CCLQueue* cq, CCLProgram* prg, GError** err);

/* Release OpenCL memory objects. */
void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice);

/* Free host resources. */
void ppc_hostbuffers_free(PPCBuffersHost* buffersHost);

/* Save statistics. */
void ppc_stats_save(char* filename, CCLQueue* cq,
	PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice,
	PPCDataSizes dataSizes, PPParameters params, GError** err);

/* Parse command-line options. */
void ppc_args_parse(int argc, char* argv[], GOptionContext** context,
	GError** err);

/* Free command line parsing related objects. */
void ppc_args_free(GOptionContext* context);

#endif
