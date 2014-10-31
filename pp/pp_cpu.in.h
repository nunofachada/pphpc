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
#define PPC_NULL_AGENT_POINTER UINT_MAX

/**
 * Parsed command-line arguments.
 * */
typedef struct pp_c_args PPCArgs;

/**
 * Agent object for OpenCL kernels.
 * */
typedef struct pp_c_agent PPCAgent;

/**
 * Simulation parameters for OpenCL kernels.
 *
 * @todo Maybe this can be constant passed as compiler options?
 * */
typedef struct pp_c_sim_params PPCSimParams;

/**
 * Cell object for OpenCL kernels.
 * */
typedef struct pp_c_cell PPCCell;

/**
 * Work sizes for kernels step1 and step2, and other work/memory
 * sizes related to the simulation.
 * */
typedef struct pp_c_work_sizes PPCWorkSizes;

/**
* Size of data structures.
* */
typedef struct pp_c_data_sizes PPCDataSizes;

/**
 * Host buffers.
 * */
typedef struct pp_c_buffers_host PPCBuffersHost;

/**
 * Device buffers.
 * */
typedef struct pp_c_buffers_device PPCBuffersDevice;

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
