/** 
 * @file
 * @brief PredPrey OpenCL CPU data structures and function headers.
 */
 
#ifndef PREDPREYCPU_H
#define PREDPREYCPU_H

#include "PredPreyCommon.h"

/** 
 * @brief Constant which indicates no further agents are in a cell. 
 * */
#define NULL_AGENT_POINTER UINT_MAX


/** 
 * @brief Parsed command-line arguments. 
 * */
typedef struct pp_c_args {
	gchar* params;			/**< Parameters file. */
	gchar* stats;			/**< Stats output file. */
	gchar* compiler_opts;	/**< Compiler options. */
	size_t gws;				/**< Global work size. */
	size_t lws;				/**< Local work size. */
	cl_int dev_idx;			/**< Index of device to use. */
	guint32* rng_seed;		/**< Rng seed. */
	cl_uint max_agents;		/**< Maximum number of agents. */
} PPCArgs;

/**
 * @brief Agent object for OpenCL kernels.
 * */
typedef struct pp_c_agent {
	cl_uint energy;	/**< Agent energy. */
	cl_uint action;	/**< True if agent already acted this turn, false otherwise. */
	cl_uint type;	/**< Type of agent (sheep or wolf). */
	cl_uint next;	/**< Pointer to next agent in current cell. */
} PPCAgent __attribute__ ((aligned (16)));


/**
 * @brief Simulation parameters for OpenCL kernels.
 * */
typedef struct pp_c_sim_params {
	cl_uint size_x;				/**< Width (number of cells) of environment.*/
	cl_uint size_y;				/**< Height (number of cells) of environment. */
	cl_uint size_xy;			/**< Dimension of environment (total number of cells). */
	cl_uint max_agents;			/**< Maximum number of agentes. */
	cl_uint null_agent_pointer;	/**< Constant which indicates no further agents are in cell. */
	cl_uint grass_restart;		/**< Number of iterations that the grass takes to regrow after being eaten by a sheep. */
	cl_uint rows_per_workitem;	/**< Number of rows to be processed by each workitem. */
	cl_uint bogus;				/**< Does nothing but align the struct to 32 bytes. */
} PPCSimParams __attribute__ ((aligned (32)));

/**
 * @brief Cell object for OpenCL kernels.
 * */
typedef struct pp_c_cell {
	cl_uint grass;			/**< Number of iterations left for grass to grow (or zero if grass is alive). */
	cl_uint agent_pointer;	/**< Pointer to first agent in cell. */
} PPCCell;

/** 
 * @brief Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * */
typedef struct pp_c_work_sizes {
	size_t gws;					/**< Global worksize. */
	size_t lws;					/**< Local worksize. */
	size_t rows_per_workitem;	/**< Number of rows to be processed by each workitem. */
	size_t max_gws;				/**< Maximum global worksize for given simulation parameters. */
	size_t max_agents;			/**< Maximum number of agentes. */
} PPCWorkSizes;

/** 
* @brief OpenCL kernels.
* */
typedef struct pp_c_kernels {
	cl_kernel step1;	/**< Step 1 kernel: move agents, grow grass. */
	cl_kernel step2;	/**< Step 2 kernel: agent actions, get stats. */
} PPCKernels;

/** 
* @brief OpenCL events.
* */
typedef struct pp_c_events {
	cl_event map_stats_start;		/**< Map stats (start of simulation). */
	cl_event unmap_stats_start;	/**< Unmap stats (start of simulation). */
	cl_event map_matrix;			/**< Map matrix. */
	cl_event unmap_matrix;			/**< Unmap matrix. */
	cl_event map_agents;			/**< Map agents. */
	cl_event unmap_agents;			/**< Unmap agents. */
	cl_event map_rng_seeds;		/**< Map RNG seeds. */
	cl_event unmap_rng_seeds;		/**< Unmap RNG seeds. */
	cl_event map_agent_params;		/**< Map agent parameters. */
	cl_event unmap_agent_params;	/**< Unmap agent parameters. */
	cl_event *step1;				/**< Execution of step1 kernel. */
	cl_event *step2;				/**< Execution of step2 kernel. */
	cl_event map_stats_end;		/**< Map stats (end of simulation). */
	cl_event unmap_stats_end;		/**< Unmap stats (end of simulation). */
} PPCEvents;

/** 
* @brief Size of data structures.
* */
typedef struct pp_c_data_sizes {
	size_t stats;			/**< Size of stats data structure. */
	size_t matrix;			/**< Size of matrix data structure. */
	size_t agents;			/**< Size of agents data structure. */
	size_t rng_seeds;		/**< Size of RNG seeds data structure. */
	size_t agent_params;	/**< Size of agent parameters data structure. */
	size_t sim_params;		/**< Size of simulation parameters data structure. */
} PPCDataSizes;

/**
 * @brief Host buffers.
 * */
typedef struct pp_c_buffers_host {
	PPStatistics* stats;			/**< Statistics. */
	PPCCell* matrix;				/**< Matrix of environment cells. */
	PPCAgent *agents;				/**< Array of agents. */
	cl_ulong* rng_seeds;			/**< Array of RNG seeds. */
	PPAgentParams* agent_params;	/**< Agent parameters. */
	PPCSimParams sim_params;		/**< Simulation parameters. */
} PPCBuffersHost;

/**
 * @brief Device buffers.
 * */
 typedef struct pp_c_buffers_device {
	cl_mem stats;			/**< Statistics. */
	cl_mem matrix;			/**< Matrix of environment cells. */
	cl_mem agents;			/**< Array of agents. */
	cl_mem rng_seeds;		/**< Array of RNG seeds. */
	cl_mem agent_params;	/**< Agent parameters. */
	cl_mem sim_params;		/**< Simulation parameters. */
} PPCBuffersDevice;

/** @brief Determine effective worksizes to use in simulation. */
int ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes, unsigned int num_rows, GError **err);

/** @brief Print information about the simulation parameters. */
void ppc_simulation_info_print(cl_int cu, PPCWorkSizes workSizes, PPCArgs args);

/** @brief Get kernel entry points. */
int ppc_kernels_create(cl_program program, PPCKernels* krnls, GError** err);

/** @brief Release kernels.  */
void ppc_kernels_free(PPCKernels* krnls);

/** @brief Initialize simulation parameters in host, to be sent to kernels. */
PPCSimParams ppc_simparams_init(PPParameters params, cl_uint null_agent_pointer, PPCWorkSizes ws);

/** @brief Determine buffer sizes. */
void ppc_datasizes_get(PPParameters params, PPCDataSizes* dataSizes, PPCWorkSizes ws);

/** @brief Initialize and map host/device buffers. */
int ppc_buffers_init(CLUZone zone, PPCWorkSizes ws, PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GRand* rng, GError** err);

/** @brief Set fixed kernel arguments.  */
int ppc_kernelargs_set(PPCKernels* krnls, PPCBuffersDevice* buffersDevice, GError** err);

/** @brief Perform simulation! */
int ppc_simulate(PPCWorkSizes workSizes, PPParameters params, CLUZone zone, PPCKernels krnls, PPCEvents* evts, GError** err);

/** @brief Release OpenCL memory objects. */
void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice);
	
/** @brief Free host resources. */ 
void ppc_hostbuffers_free(PPCBuffersHost* buffersHost);

#ifdef CLPROFILER
/** @brief Create events data structure. */
void ppc_events_create(PPParameters params, PPCEvents* evts);

/** @brief Free events. */
void ppc_events_free(PPParameters params, PPCEvents* evts);
#endif

/** @brief Analyze events, show profiling info. */
int ppc_profiling_analyze(ProfCLProfile* profile, PPCEvents* evts, PPParameters params, GError** err);

/** @brief Save statistics. */
int ppc_stats_save(char* filename, CLUZone zone, PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GError** err);

/** @brief Parse command-line options. */
void ppc_args_parse(int argc, char* argv[], GOptionContext** context, GError** err);

/** @brief Free command line parsing related objects. */
void ppc_args_free(GOptionContext* context);

#endif
