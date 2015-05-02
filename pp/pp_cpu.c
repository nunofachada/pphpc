/**
 * @file
 * Implementation of the predator-prey OpenCL CPU simulation.
 * */

#include "pp_cpu.h"

/**
 * The default maximum number of agents: 16777216. Each agent requires
 * 16 bytes, thus by default 256Mb of memory will be allocated for the
 * agents buffer. A higher value may lead to faster simulations
 * given that it will increase the success rate of new agent
 * allocations, but on the other hand memory allocation and
 * initialization may take longer.
 * */
#define PPC_DEFAULT_MAX_AGENTS 16777216

/**
 * Default maximum number of agents which can be shuffled in the same
 * loop.
 * */
#define PPC_DEFAULT_MAX_AGENTS_SHUF 256

/** A description of the program. */
#define PPC_DESCRIPTION "OpenCL predator-prey simulation for the CPU"

/**
 * Constant which indicates no further agents are in a cell.
 * */
#define PPC_NULL_AGENT_POINTER CL_UINT_MAX

/**
 * Minimum distance between rows, which is equal to @f$2r + 1@f$, where
 * @f$r=1@f$ is the radius of agent movement.
 * */
#define PPC_D_MIN 3

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

	/** Maximum number of agents shuffled in the same loop. */
	cl_uint max_agents_ptrs;

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

} PPCBuffersDevice;

/** Command line arguments and respective default values. */
static PPCArgs args = {NULL, NULL, NULL, 0, 0, -1, PP_DEFAULT_SEED,
		NULL, PPC_DEFAULT_MAX_AGENTS, PPC_DEFAULT_MAX_AGENTS_SHUF};

/** Valid command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,
		"Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",
		"FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,
		"Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",
		"FILENAME"},
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts,
		"Extra OpenCL compiler options",
		"OPTS"},
	{"globalsize",      'g', 0, G_OPTION_ARG_INT,      &args.gws,
		"Global work size (default is maximum possible)",
		"SIZE"},
	{"localsize",       'l', 0, G_OPTION_ARG_INT,      &args.lws,
		"Local work size (default is selected by OpenCL runtime)",
		"SIZE"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,
		"Device index (if not given and more than one device is "\
		"available, chose device from menu)",
		"INDEX"},
	{"rng_seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,
		"Seed for random number generator (default is " G_STRINGIFY(PP_DEFAULT_SEED) ")",
		"SEED"},
	{"rngen",           'n', 0, G_OPTION_ARG_STRING,   &args.rngen,
		"Random number generator: " CLO_RNG_IMPLS " (default is " PP_RNG_DEFAULT ")",
		"RNG"},
	{"max_agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,
		"Maximum number of agents (default is " G_STRINGIFY(PPC_DEFAULT_MAX_AGENTS) ")",
		"SIZE"},
	{"max_agents_shuff",'u', 0, G_OPTION_ARG_INT,      &args.max_agents_ptrs,
		"Maximum number of agents which can be shuffled in the same " \
		"loop (default is " G_STRINGIFY(PPC_DEFAULT_MAX_AGENTS_SHUF) ")",
		"SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,
		NULL, NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/**
 * Determine effective worksizes to use in simulation.
 *
 * @param[in] args Parsed command line arguments.
 * @param[in] workSizes Work sizes for kernels step1 and step2, and
 * other work/memory sizes related to the simulation.
 * @param[in] num_rows Number of rows in (height of) simulation
 * environment.
 * @param[out] err Return location for a GError.
 * */
static void ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes,
	cl_uint num_rows, GError **err) {

	/* Get local work size. */
	workSizes->lws = args.lws;

	/* Determine maximum number of global work-items which can be used
	 * for current problem (each pair of work-items must process rows
	 * which are separated by two rows not being processed) */
	workSizes->max_gws = num_rows / PPC_D_MIN;

	/* Determine effective number of global work-items to use. */
	if (args.gws > 0) {

		/* User specified a value, let's see if it's possible to use
		 * it. */
		if (workSizes->max_gws >= args.gws) {

			/* Yes, it's possible. */
			workSizes->gws = args.gws;

		} else {

			/* No, specified value is too large for the given
			 * problem. Throw error and exit function. */
			ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
				PP_INVALID_ARGS, error_handler,
				"Global work size is too large for model parameters. " \
				"Maximum size is %d.", (int) workSizes->max_gws);

		}

		/* If a local work size is given, check that the global work
		 * size is a multiple of it. */
		if (workSizes->lws > 0) {
			ccl_if_err_create_goto(*err, PP_ERROR,
				workSizes->gws % workSizes->lws != 0,
				PP_INVALID_ARGS, error_handler,
				"Global work size (%d) is not multiple of local work " \
				"size (%d).",
				(int) workSizes->gws, (int) workSizes->lws);
		}


	} else {

		/* User did not specify a global work-size, as such it will
		 * depend on whether the user specified a local work size. */
		if (workSizes->lws > 0) {

			/* User specified a local work size, as such find a global
			 * work size less or equal than the maximum and which is
			 * a multiple of the local work size. */
			if (workSizes->max_gws % workSizes->lws == 0) {

				/* The maximum global work size is divisible by the
				 * local work size, as such, use it. */
				workSizes->gws = workSizes->max_gws;

			} else {

				/* The maximum global work size is not divisible by the
				 * local work size, as such find an adequate global
				 * work size that is. */
				workSizes->gws = pp_next_multiple(
					workSizes->max_gws - 2 * workSizes->lws,
					workSizes->lws);
			}

		} else {

			/* User did not specify a local work size, as such just
			 * use the maximum value for the global work size and let
			 * the OpenCL implementation figure out a good local work
			 * size. */
			workSizes->gws = workSizes->max_gws;

		}

	}

	/* Determine initial estimate of rows to be computed per thread. */
	workSizes->rows_per_workitem = num_rows / workSizes->gws;

	/* Is it viable to increment estimate? This estimate can be
	 * incremented if: 1) the number of rows is not equally divisible by
	 * the global work-size; and, 2) after incrementing it, there are
	 * enough rows for the last worker to process. */
	if ((num_rows % workSizes->gws > 0) &&
			((workSizes->gws - 1) * (workSizes->rows_per_workitem + 1)
			< num_rows - PPC_D_MIN)) {

		/* It is viable, so increment estimate. */
		workSizes->rows_per_workitem++;

	}

	/* Set maximum number of agents. */
	workSizes->max_agents = args.max_agents;

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;
}

/**
 * Print information about the simulation parameters.
 *
 * @param[in] dev Device where simulation will be executed.
 * @param[in] workSizes Work sizes for kernels step1 and step2, and
 * other work/memory sizes related to the simulation.
 * @param[in] args Parsed command line arguments.
 * @param[in] compilerOpts Compiler options.
 * @param[out] err Return location for a GError.
 * */
static void ppc_simulation_info_print(CCLDevice* dev,
	PPCWorkSizes workSizes, PPCArgs args, gchar* compiler_opts,
	GError** err) {

	/* Error reporting object. */
	GError* err_internal = NULL;

	/* Get number of device compute units. */
	cl_uint cu = ccl_device_get_info_scalar(
		dev, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* ...Header */
	printf("\n   ========================= Computational settings ======================== \n\n");
	/* ...Compute units */
	printf("     Compute units in device    : %d\n", cu);
	/* ...Global worksize */
	printf("     Global work size (max)     : %d (%d)\n",
		(int) workSizes.gws, (int) workSizes.max_gws);
	/* ...Local worksize */
	printf("     Local work size            : ");
	if (workSizes.lws == 0) printf("auto\n");
	else printf("%d\n", (int) workSizes.lws);
	/* ...Rows per workitem */
	printf("     Rows per work-item         : %d\n",
		(int) workSizes.rows_per_workitem);
	/* ...Maximum number of agents */
	printf("     Maximum number of agents   : %d\n",
		(int) workSizes.max_agents);
	/* ...RNG seed */
	printf("     Random seed                : %u\n", args.rng_seed);
	/* ...Compiler options (out of table) */
	printf("     Compiler options           : ");
	if (args.compiler_opts != NULL) printf("%s\n", compiler_opts);
	else printf("none\n");
	/* ...Finish table. */

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;

}

/**
 * Determine buffer sizes.
 *
 * @param[in] params Simulation parameters.
 * @param[in] dataSizes Sizes of simulation data structures.
 * @param[in] ws Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * */
static void ppc_datasizes_get(PPParameters params,
	PPCDataSizes* dataSizes, PPCWorkSizes ws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);

	/* Matrix */
	dataSizes->matrix = params.grid_x * params.grid_y * sizeof(PPCCell);

	/* Agents. */
	dataSizes->agents = ws.max_agents * sizeof(PPCAgent);

}

/**
 * Initialize and map host/device buffers.
 *
 * @param[in] ctx Context wrapper.
 * @param[in] cq Command-queue wrapper.
 * @param[in] ws Work sizes for kernels step1 and step2, and other
 * work/memory sizes related to the simulation.
 * @param[out] buffersHost Host buffers.
 * @param[out] buffersDevice Device buffers.
 * @param[in] dataSizes Sizes of simulation data structures.
 * @param[in] params Simulation parameters.
 * @param[in] rng_clo CL_Ops RNG object.
 * @param[out] err Return location for a GError.
 * */
static void ppc_buffers_init(CCLContext* ctx, CCLQueue* cq,
	PPCWorkSizes ws, PPCBuffersHost *buffersHost,
	PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes,
	PPParameters params, CloRng* rng_clo, GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Event wrapper. */
	CCLEvent* evt = NULL;

	/* Initialize RNG. */
	GRand* rng = g_rand_new_with_seed(args.rng_seed);

	/* ************************* */
	/* Initialize device buffers */
	/* ************************* */

	/* Statistics */
	buffersDevice->stats = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.stats,
		NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Grass matrix */
	buffersDevice->matrix = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.matrix,
		NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get RNG seeds from the CL_Ops RNG. */
	buffersDevice->rng_seeds = clo_rng_get_device_seeds(rng_clo);

	/* Agent array */
	buffersDevice->agents = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.agents,
		NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* *********************************************************** */
	/* Initialize host buffers, which are mapped to device buffers */
	/* *********************************************************** */

	/* Map stats buffer */
	buffersHost->stats = (PPStatistics*) ccl_buffer_enqueue_map(
		buffersDevice->stats, cq, CL_TRUE, CL_MAP_WRITE, 0,
		dataSizes.stats, NULL, &evt, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Map: stats");

	/* Set all stats to zero. */
	memset(buffersHost->stats, 0, params.iters * sizeof(PPStatistics));

	/* Set initial stats to initial agent counts. */
	buffersHost->stats[0].sheep = params.init_sheep;
	buffersHost->stats[0].wolves = params.init_wolves;

	/* Map grass matrix. */
	buffersHost->matrix = (PPCCell*) ccl_buffer_enqueue_map(
		buffersDevice->matrix, cq, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ,
		0, dataSizes.matrix, NULL, &evt, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Map: matrix");

	/* Initialize grass matrix. */
	for(cl_uint i = 0; i < params.grid_x; i++) {
		for (cl_uint j = 0; j < params.grid_y; j++) {

			/* Determine grid index. */
			cl_uint gridIndex = (i + j * params.grid_x);

			/* Determine grass state. */
			cl_uint grassState = g_rand_int_range(rng, 0, 2) == 0
				? 0
				: g_rand_int_range(rng, 1, params.grass_restart + 1);

			/* Initialize grass. */
			buffersHost->matrix[gridIndex].grass = grassState;

			/* If grass is alive, update initial grass count stats. */
			if (grassState == 0)
				buffersHost->stats[0].grass++;

			/* Update initial grass energy stats. */
			buffersHost->stats[0].grass_en += grassState;

			/* Initialize agent pointer. */
			buffersHost->matrix[gridIndex].agent_pointer =
				PPC_NULL_AGENT_POINTER;
		}
	}

	/* Unmap stats buffer. */
	evt = ccl_buffer_enqueue_unmap(buffersDevice->stats, cq,
		buffersHost->stats, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Unmap: stats");

	/* Map agent array. */
	buffersHost->agents = (PPCAgent *) ccl_buffer_enqueue_map(
		buffersDevice->agents, cq, CL_TRUE, CL_MAP_WRITE, 0,
		dataSizes.agents, NULL, &evt, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Map: agents");

	/* Initialize agent array. */
	for(cl_uint i = 0; i < ws.max_agents; i++) 	{

		/* Check if there are still agents to initialize. */
		if (i < params.init_sheep + params.init_wolves) {

			/* There are still agents to initialize... */

			/* Initialize generic agent parameters. */
			buffersHost->agents[i].action = 0;

			/* Initialize specific agent parameters. */
			if (i < params.init_sheep) {

				/* Initialize sheep specific parameters. */
				buffersHost->agents[i].energy = g_rand_int_range(
					rng, 1, params.sheep_gain_from_food * 2 + 1);
				buffersHost->agents[i].type = SHEEP_ID;
				buffersHost->stats[0].sheep_en +=
					buffersHost->agents[i].energy;

			} else {

				/* Initialize wolf specific parameters. */
				buffersHost->agents[i].energy = g_rand_int_range(
					rng, 1, params.wolves_gain_from_food * 2 + 1);
				buffersHost->agents[i].type = WOLF_ID;
				buffersHost->stats[0].wolves_en +=
					buffersHost->agents[i].energy;

			}

			/* Chose a place to put next agent. */
			cl_uint x = g_rand_int_range(rng, 0, params.grid_x);
			cl_uint y = g_rand_int_range(rng, 0, params.grid_y);
			cl_uint gridIndex = (x + y * params.grid_x);

			/* Put agent in grid. */
			if (buffersHost->matrix[gridIndex].agent_pointer
					== PPC_NULL_AGENT_POINTER) {

				/* This cell had no agent, put it there. */
				buffersHost->matrix[gridIndex].agent_pointer = i;
				buffersHost->agents[i].next = PPC_NULL_AGENT_POINTER;

			} else {

				/* Cell already has agent, put it in the beginning
				 * of the list. */
				buffersHost->agents[i].next =
					buffersHost->matrix[gridIndex].agent_pointer;

				buffersHost->matrix[gridIndex].agent_pointer = i;

			}

		} else {

			/* No more agents to initialize, initialize array position
			 * only. OpenCL implementations seem to initialize
			 * everything to zero, but to avoid any bugs on untested
			 * implementations we must at least initialize agents energy
			 * to zero. */
			buffersHost->agents[i].energy = 0;

		}

	}

	/* Unmap agents buffer. */
	evt = ccl_buffer_enqueue_unmap(buffersDevice->agents, cq,
		buffersHost->agents, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Unmap: agents");

	/* Unmap matrix buffer. */
	evt = ccl_buffer_enqueue_unmap(buffersDevice->matrix, cq,
		buffersHost->matrix, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Unmap: matrix");

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Release RNG. */
	g_rand_free(rng);

	/* Return. */
	return;

}

/**
 * Set fixed kernel arguments.
 *
 * @param[in] prg Program wrapper.
 * @param[in] buffersDevice Device buffers.
 * @param[out] err Return location for a GError.
 * */
static void ppc_kernelargs_set(CCLProgram* prg,
	PPCBuffersDevice* buffersDevice, GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Kernel wrappers. */
	CCLKernel* step1_krnl = NULL;
	CCLKernel* step2_krnl = NULL;

	/* Get kernels. */
	step1_krnl = ccl_program_get_kernel(prg, "step1", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	step2_krnl = ccl_program_get_kernel(prg, "step2", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Step1 kernel - Move agents, grow grass */
	ccl_kernel_set_args(step1_krnl, buffersDevice->agents,
		buffersDevice->matrix, buffersDevice->rng_seeds, ccl_arg_skip,
		NULL);

	/* Step2 kernel - Agent actions, get stats */
	ccl_kernel_set_args(step2_krnl, buffersDevice->agents,
		buffersDevice->matrix, buffersDevice->rng_seeds,
		buffersDevice->stats, ccl_arg_skip, ccl_arg_skip, NULL);

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;
}

/**
 * Perform simulation!
 *
 * @param[in] workSizes Work sizes for kernels step1 and step2, and
 * other work/memory sizes related to the simulation.
 * @param[in] params Simulation parameters.
 * @param[in] cq Command queue wrapper.
 * @param[in] prg Program wrapper.
 * @param[out] err Return location for a GError.
 * */
static void ppc_simulate(PPCWorkSizes workSizes, PPParameters params,
	CCLQueue* cq, CCLProgram* prg, GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Kernel wrappers. */
	CCLKernel* step1_krnl = NULL;
	CCLKernel* step2_krnl = NULL;

	/* Event wrapper. */
	CCLEvent* evt = NULL;

	/* Current iteration. */
	cl_uint iter;

    /* If local work group size is not given or is 0, set it to NULL and
     * let OpenCL decide. */
	size_t *local_size = (workSizes.lws > 0 ? &workSizes.lws : NULL);

	/* Get step1 kernel. */
	step1_krnl = ccl_program_get_kernel(prg, "step1", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get step2 kernel. */
	step2_krnl = ccl_program_get_kernel(prg, "step2", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Simulation loop. */
	for (iter = 1; iter <= params.iters; iter++) {

		/* Step 1:  Move agents, grow grass */
		for (cl_uint t = 0; t < workSizes.rows_per_workitem; ++t) {

			/* Set turn on step1_kernel */
			ccl_kernel_set_arg(step1_krnl, 3, ccl_arg_priv(t, cl_uint));

			/* Run kernel */
			evt = ccl_kernel_enqueue_ndrange(step1_krnl, cq, 1, NULL,
				&workSizes.gws, local_size, NULL, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			ccl_event_set_name(evt, "K: step1");

		}

		/* Step 2:  Agent actions, get stats */
		ccl_kernel_set_arg(step2_krnl, 4, ccl_arg_priv(iter, cl_uint));

		for (cl_uint t = 0; t < workSizes.rows_per_workitem; ++t ) {

			/* Set turn on step2_kernel */
			ccl_kernel_set_arg(step2_krnl, 5, ccl_arg_priv(t, cl_uint));

			/* Run kernel */
			evt = ccl_kernel_enqueue_ndrange(step2_krnl, cq, 1, NULL,
				&workSizes.gws, local_size, NULL, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			ccl_event_set_name(evt, "K: step2");

		}

	}

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;
}

/**
 * Release OpenCL memory objects.
 *
 * @param[in] buffersDevice Device buffers.
 * */
static void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice) {
	if (buffersDevice->stats)
		ccl_buffer_destroy(buffersDevice->stats);
	if (buffersDevice->agents)
		ccl_buffer_destroy(buffersDevice->agents);
	if (buffersDevice->matrix)
		ccl_buffer_destroy(buffersDevice->matrix);
}

/**
 * Save statistics.
 *
 * @param[in] filename File where to save simulation statistics.
 * @param[in] cq Command queue wrapper.
 * @param[in] buffersHost Host buffers.
 * @param[in] buffersDevice Device buffers.
 * @param[in] dataSizes Sizes of simulation data structures.
 * @param[in] params Simulation parameters.
 * @param[out] err Return location for a GError.
 * */
static void ppc_stats_save(char* filename, CCLQueue* cq,
	PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice,
	PPCDataSizes dataSizes, PPParameters params, GError** err) {

	/* Stats file. */
	FILE * fp;

	/* Event wrapper. */
	CCLEvent* evt = NULL;

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get definite file name. */
	gchar* realFilename =
		(filename != NULL) ? filename : PP_DEFAULT_STATS_FILE;

	/* Map stats host buffer in order to get statistics */
	buffersHost->stats = ccl_buffer_enqueue_map(
		buffersDevice->stats, cq, CL_TRUE, CL_MAP_READ, 0,
		dataSizes.stats, NULL, &evt, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Map: stats");

	/* Output results to file */
	fp = fopen(realFilename, "w");
	ccl_if_err_create_goto(*err, PP_ERROR, fp == NULL,
		PP_UNABLE_SAVE_STATS, error_handler,
		"Unable to open file \"%s\"", realFilename);

	for (cl_uint i = 0; i <= params.iters; ++i)
		fprintf(fp, "%d\t%d\t%d\t%f\t%f\t%f\n",
			buffersHost->stats[i].sheep, buffersHost->stats[i].wolves,
			buffersHost->stats[i].grass,
			buffersHost->stats[i].sheep_en /
				(float) buffersHost->stats[i].sheep,
			buffersHost->stats[i].wolves_en /
				(float) buffersHost->stats[i].wolves,
			buffersHost->stats[i].grass_en /
				(float) params.grid_xy);

	fclose(fp);

	/* Unmap stats host buffer. */
	evt = ccl_buffer_enqueue_unmap(buffersDevice->stats, cq,
		buffersHost->stats, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Unmap: stats");

	/* Guarantee all activity has terminated... */
	ccl_queue_finish(cq, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;
}

/**
 * Parse command-line options.
 *
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Command line arguments.
 * @param[in] context Context object for command line argument parsing.
 * @param[out] err Return location for a GError.
 * */
static void ppc_args_parse(int argc, char* argv[],
	GOptionContext** context, GError** err) {

	*context = g_option_context_new (" - " PPC_DESCRIPTION);
	g_option_context_add_main_entries(*context, entries, NULL);
	g_option_context_parse(*context, &argc, &argv, err);

}

/**
 * Free command line parsing related objects.
 *
 * @param[in] context Context object for command line argument parsing.
 * */
static void ppc_args_free(GOptionContext* context) {

	if (context) {
		g_option_context_free(context);
	}
	if (args.params) g_free(args.params);
	if (args.stats) g_free(args.stats);
	if (args.compiler_opts) g_free(args.compiler_opts);
	if (args.rngen) g_free(args.rngen);
}

/**
 * Build OpenCL compiler options string.
 *
 * @param[in] gws Kernel global work sizes.
 * @param[in] lws Kernel local work sizes.
 * @param[in] params Simulation parameters.
 * @param[in] cliOpts Compiler options specified through the
 * command-line.
 * @return The final OpenCL compiler options string.
 */
static gchar* ppc_compiler_opts_build(PPCArgs args, PPParameters params,
	cl_uint rows_per_workitem, gchar* cliOpts) {

	gchar* compilerOptsStr;

	GString* compilerOpts = g_string_new("");
	g_string_append_printf(compilerOpts, "-D MAX_AGENTS=%d ",
		args.max_agents);
	g_string_append_printf(compilerOpts, "-D MAX_AGENT_PTRS=%d ",
		args.max_agents_ptrs);
	g_string_append_printf(compilerOpts, "-D ROWS_PER_WORKITEM=%d ",
		rows_per_workitem);
	g_string_append_printf(compilerOpts, "-D INIT_SHEEP=%d ",
		params.init_sheep);
	g_string_append_printf(compilerOpts, "-D SHEEP_GAIN_FROM_FOOD=%d ",
		params.sheep_gain_from_food);
	g_string_append_printf(compilerOpts, "-D SHEEP_REPRODUCE_THRESHOLD=%d ",
		params.sheep_reproduce_threshold);
	g_string_append_printf(compilerOpts, "-D SHEEP_REPRODUCE_PROB=%d ",
		params.sheep_reproduce_prob);
	g_string_append_printf(compilerOpts, "-D INIT_WOLVES=%d ",
		params.init_wolves);
	g_string_append_printf(compilerOpts, "-D WOLVES_GAIN_FROM_FOOD=%d ",
		params.wolves_gain_from_food);
	g_string_append_printf(compilerOpts, "-D WOLVES_REPRODUCE_THRESHOLD=%d ",
		params.wolves_reproduce_threshold);
	g_string_append_printf(compilerOpts, "-D WOLVES_REPRODUCE_PROB=%d ",
		params.wolves_reproduce_prob);
	g_string_append_printf(compilerOpts, "-D GRASS_RESTART=%d ",
		params.grass_restart);
	g_string_append_printf(compilerOpts, "-D GRID_X=%d ",
		params.grid_x);
	g_string_append_printf(compilerOpts, "-D GRID_Y=%d ",
		params.grid_y);
	g_string_append_printf(compilerOpts, "-D ITERS=%d ",
		params.iters);

	if (cliOpts) g_string_append_printf(compilerOpts, "%s", cliOpts);
	compilerOptsStr = compilerOpts->str;

	g_string_free(compilerOpts, FALSE);

	return compilerOptsStr;
}


/**
 * PredPrey CPU simulation main program.
 *
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Vector of command line arguments.
 * @return ::PP_SUCCESS if program terminates successfully, or another
 * value of ::pp_error_codes if an error occurs.
 * */
int main(int argc, char ** argv) {

	/* Status var aux */
	int status;

	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;

	/* Predator-Prey simulation data structures. */
	PPCWorkSizes workSizes;
	PPCDataSizes dataSizes;
	PPCBuffersHost buffersHost = {NULL, NULL, NULL};
	PPCBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL};
	PPParameters params;
	gchar* compilerOpts = NULL;

	/* OpenCL object wrappers. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;

	/* Complete OCL program source code. */
	gchar* src = NULL;

	/* Device selection filters. */
	CCLDevSelFilters filters = NULL;

	/* Profiler. */
	CCLProf* prof = NULL;

	/* CL_Ops RNG. */
	CloRng* rng_clo = NULL;

	/* Error management object. */
	GError *err = NULL;

	/* Parse arguments. */
	ppc_args_parse(argc, argv, &context, &err);
	ccl_if_err_goto(err, error_handler);
	if (!args.rngen) args.rngen = g_strdup(PP_RNG_DEFAULT);

	/* Add CPU filter to device selection. */
	ccl_devsel_add_indep_filter(
		&filters, ccl_devsel_indep_type_cpu, NULL);

	/* Add menu filter to device selection. */
	ccl_devsel_add_dep_filter(&filters, ccl_devsel_dep_menu,
		&args.dev_idx);

	/* Create context with device specified by user. */
	ctx = ccl_context_new_from_filters(&filters, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get chosen device. */
	dev = ccl_context_get_device(ctx, 0, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, PP_QUEUE_PROPERTIES, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create RNG with specified seed. */
	rng_clo = clo_rng_new(args.rngen, CLO_RNG_SEED_HOST_MT, NULL,
		args.max_agents, args.rng_seed, NULL, ctx, cq, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get simulation parameters */
	pp_load_params(&params, args.params, &err);
	ccl_if_err_goto(err, error_handler);

	/* Determine number of threads to use based on compute capabilities
	 * and user arguments */
	ppc_worksizes_calc(args, &workSizes, params.grid_y, &err);
	ccl_if_err_goto(err, error_handler);

	/* Concatenate complete source: RNG kernels source + common source
	 * + CPU kernel source. */
	src = g_strconcat(clo_rng_get_source(rng_clo), PP_COMMON_SRC,
		PP_CPU_SRC, NULL);

	/* Create aprogram. */
	prg = ccl_program_new_from_source(ctx, src, &err);
	ccl_if_err_goto(err, error_handler);

	/* Compiler options. */
	compilerOpts = ppc_compiler_opts_build(args, params,
		workSizes.rows_per_workitem, args.compiler_opts);

	/* Build program. */
	ccl_program_build(prg, compilerOpts, &err);
	ccl_if_err_goto(err, error_handler);

	/* Profiling / Timings. */
	prof = ccl_prof_new();

	/* Print simulation info to screen */
	ppc_simulation_info_print(dev, workSizes, args, compilerOpts, &err);
	ccl_if_err_goto(err, error_handler);

	/* Determine size in bytes for host and device data structures. */
	ppc_datasizes_get(params, &dataSizes, workSizes);

	/* Start basic timming / profiling. */
	ccl_prof_start(prof);

	/* Initialize and map host/device buffers */
	ppc_buffers_init(ctx, cq, workSizes, &buffersHost, &buffersDevice,
		dataSizes, params, rng_clo, &err);
	ccl_if_err_goto(err, error_handler);

	/*  Set fixed kernel arguments. */
	ppc_kernelargs_set(prg, &buffersDevice, &err);
	ccl_if_err_goto(err, error_handler);

	/* Simulation!! */
	ppc_simulate(workSizes, params, cq, prg, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get statistics. */
	ppc_stats_save(args.stats, cq, &buffersHost,
		&buffersDevice, dataSizes, params, &err);
	ccl_if_err_goto(err, error_handler);

	/* Stop basic timing / profiling. */
	ccl_prof_stop(prof);

#ifdef PP_PROFILE_OPT
	/* Analyze events */
	ccl_prof_add_queue(prof, "Queue 1", cq);
	ccl_prof_calc(prof, &err);
	ccl_if_err_goto(err, error_handler);

	/* Print profiling summary. */
	ccl_prof_print_summary(prof);
#else

	/* Print ellapsed time. */
	printf("Ellapsed time: %.4es\n", ccl_prof_time_elapsed(prof));

#endif

	/* If we get here, everything went Ok. */
	g_assert(err == NULL);
	status = PP_SUCCESS;
	goto cleanup;

error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	fprintf(stderr, "Error: %s\n", err->message);
	status = err->code;
	g_error_free(err);

cleanup:

	/* Free stuff! */

	/* Release OpenCL memory objects. This also frees host buffers
	 * because of CL_MEM_ALLOC_HOST_PTR (I think). If we try to
	 * free() the host buffers we will have a kind of segfault. */
	ppc_devicebuffers_free(&buffersDevice);

	/* Free profiler object. */
	if (prof) ccl_prof_destroy(prof);

	/* Free compiler options. */
	if (compilerOpts) g_free(compilerOpts);

	/* Free CL_Ops RNG object. */
	if (rng_clo) clo_rng_destroy(rng_clo);

	/* Free complete program source. */
	if (src) g_free(src);

	/* Free remaining OpenCL wrappers. */
	if (prg) ccl_program_destroy(prg);
	if (cq) ccl_queue_destroy(cq);
	if (ctx) ccl_context_destroy(ctx);

	/* Free context and associated cli args parsing buffers. */
	ppc_args_free(context);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

	/* Bye. */
	return status;
}
