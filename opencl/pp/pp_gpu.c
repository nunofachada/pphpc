/*
 * PPHPC-OCL, an OpenCL implementation of the PPHPC agent-based model
 * Copyright (C) 2015 Nuno Fachada
 *
 * This file is part of PPHPC-OCL.
 *
 * PPHPC-OCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PPHPC-OCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PPHPC-OCL. If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Implementation of the predator-prey OpenCL GPU simulation.
 * */

#include "pp_gpu.h"

//~ /**
 //~ * If the constant bellow is set, then the simulation is executed in
 //~ * debug mode, i.e., a check will be performed between each OpenCL
 //~ * operation, and overlapping operations don't take place. The value of
 //~ * this constant corresponds to an interval of iterations to print
 //~ * simulation info for the current iteration.
 //~ * */
//~ #define PPG_DEBUG 250

//~ /**
 //~ * If the constant bellow is set, then simulation info will be dumped
 //~ * to a file in each iteration. The constant can assume the following
 //~ * values:
 //~ *
 //~ * 0x00 - dump all agents + all grass
 //~ * 0x01 - dump only alive agents + all grass
 //~ * 0x10 - dump all agents + only dead grass (counter>0)
 //~ * 0x11 - dump only alive agents + only dead grass (counter>0)
 //~ * */
//~ #define PPG_DUMP 0x01

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
	gchar * params;

	/** Stats output file. */
	gchar * stats;

#ifdef PP_PROFILE_OPT
	/** File where to export aggregate profiling info. */
	gchar * prof_agg_file;
#endif

	/** Compiler options. */ /// @todo Remove compiler_opts?
	gchar * compiler_opts;

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

/** Main command line arguments and respective default values. */
static PPGArgs args = {NULL, NULL,
#ifdef PP_PROFILE_OPT
	NULL,
#endif
	NULL, -1, PP_DEFAULT_SEED,
	PPG_DEFAULT_AGENT_SIZE, PPG_DEFAULT_MAX_AGENTS};

/** Algorithm selection arguments. */
static PPGArgsAlg args_alg = {NULL, NULL, NULL};

/** Local work sizes command-line arguments*/
static PPGArgsLWS args_lws = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/** Vector widths command line arguments. */
static PPGArgsVW args_vw = {0, 0, 0};

/** Main command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,
		"Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",
		"FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,
		"Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",
		"FILENAME"},
#ifdef PP_PROFILE_OPT
	{"prof-agg",        'i', 0, G_OPTION_ARG_FILENAME, &args.prof_agg_file,
		"File where to export aggregate profiling info (if omitted, info will "
		"not be exported)",
		"FILENAME"},
#endif
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts,
		"Extra OpenCL compiler options",
		"OPTS"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,
		"Device index (if not given and more than one device is available, "
		"chose device from menu)",
		"INDEX"},
	{"rng-seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,
		"Seed for random number generator (default is "
		G_STRINGIFY(PP_DEFAULT_SEED) ")",
		"SEED"},
	{"agent-size",      'a', 0, G_OPTION_ARG_INT,      &args.agent_size,
		"Agent size, 32 or 64 bits (default is "
		G_STRINGIFY(PPG_DEFAULT_AGENT_SIZE) ")",
		"BITS"},
	{"max-agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,
		"Maximum number of agents (default is "
		G_STRINGIFY(PPG_DEFAULT_MAX_AGENTS) ")",
		"SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail, NULL,
		NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/** Algorithm selection options. */
static GOptionEntry entries_alg[] = {
	{"a-rng",  0, 0, G_OPTION_ARG_STRING, &args_alg.rng,
		"Random number generator: " CLO_RNG_IMPLS,
		"ALGORITHM"},
	{"a-sort", 0, 0, G_OPTION_ARG_STRING, &args_alg.sort,
		"Sorting: " CLO_SORT_IMPLS,
		"ALGORITHM"},
	{"a-sort-opts", 0, 0, G_OPTION_ARG_STRING, &args_alg.sort_opts,
		"Sort algorithm options",
		"OPTIONS"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/** Kernel local worksizes. */
static GOptionEntry entries_lws[] = {
	{"l-def",          0, 0, G_OPTION_ARG_INT, &args_lws.deflt,
		"Default local worksize",
		"LWS"},
	{"l-init-cell",    0, 0, G_OPTION_ARG_INT, &args_lws.init_cell,
		"Init. cells kernel",
		"LWS"},
	{"l-init-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.init_agent,
		"Init. agents kernel",
		"LWS"},
	{"l-grass",        0, 0, G_OPTION_ARG_INT, &args_lws.grass,
		"Grass kernel",
		"LWS"},
	{"l-reduce-grass", 0, 0, G_OPTION_ARG_INT, &args_lws.reduce_grass,
		"Reduce grass kernel",
		"LWS"},
	{"l-reduce-agent", 0, 0, G_OPTION_ARG_INT, &args_lws.reduce_agent,
		"Reduce agent kernel",
		"LWS"},
	{"l-move-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.move_agent,
		"Move agent kernel",
		"LWS"},
	{"l-sort-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.sort_agent,
		"Sort agent kernel",
		"LWS"},
	{"l-find-index",   0, 0, G_OPTION_ARG_INT, &args_lws.find_cell_idx,
		"Find cell agent index kernel",
		"LWS"},
	{"l-action-agent", 0, 0, G_OPTION_ARG_INT, &args_lws.action_agent,
		"Agent actions kernel",
		"LWS"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/** Kernel vector widths. */
static GOptionEntry entries_vw[] = {
	{"vw-grass",        0, 0, G_OPTION_ARG_INT, &args_vw.grass,
		"Vector (of uints) width for grass kernel, default is 0 (auto-detect)",
		"WIDTH"},
	{"vw-reduce-grass", 0, 0, G_OPTION_ARG_INT, &args_vw.reduce_grass,
		"Vector (of uints) width for grass reduce kernel, default is 0 "
		"(auto-detect)",
		"WIDTH"},
	{"vw-reduce-agent", 0, 0, G_OPTION_ARG_INT, &args_vw.reduce_agent,
		"Vector (of uint/ulongs, depends on --agent-size) width for agent "
		"reduce kernel, default is 0 (auto-detect)",
		"WIDTH"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/** Agent size in bytes. */
static size_t agent_size_bytes;

#ifdef PPG_DUMP

/**
 * Dump simulation data for current iteration.
 *
 * @param iter Current iteration.
 * @param dump_type Type of dump (see comment for PPG_DUMP constant).
 * @param zone OpenCL zone.
 * @param fp_agent_dump Pointer of file where to dump agent info.
 * @param fp_cell_dump Pointer of file where to dump cell info.
 * @param gws_action_agent GWS for action_agent kernel (current iteration).
 * @param gws_move_agent GWS for move_agent kernel (current iteration).
 * @param params Simulation parameters.
 * @param dataSizes Size of data buffers.
 * @param buffersDevice Device data buffers.
 * @param agents_data Host buffer where to put agent data.
 * @param cells_agents_index Host buffer where to put cell data (agents
 * index).
 * @param cells_grass Host buffer where to put cell data (grass).
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function
 * terminates successfully, or an error code otherwise.
 * */
static void ppg_dump(int iter, int dump_type, CCLQueue* cq,
	FILE* fp_agent_dump, FILE* fp_cell_dump, cl_uint max_agents_iter,
	size_t gws_reduce_agent1, size_t gws_action_agent,
	size_t gws_move_agent, PPParameters params, PPGDataSizes dataSizes,
	PPGBuffersDevice buffersDevice, void *agents_data,
	cl_uint2 *cells_agents_index, cl_uint *cells_grass, GError** err) {

	/* Aux. status vars. */
	int blank_line;

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Read data from device. */
	ccl_buffer_enqueue_read(buffersDevice.agents_data, cq, CL_TRUE, 0,
		dataSizes.agents_data, agents_data, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_buffer_enqueue_read(buffersDevice.cells_agents_index, cq,
		CL_TRUE, 0, dataSizes.cells_agents_index, cells_agents_index,
		NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_buffer_enqueue_read(buffersDevice.cells_grass, cq, CL_TRUE, 0,
		dataSizes.cells_grass, cells_grass, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Export agent info. */
	fprintf(fp_agent_dump, "\nIter %d, max_agents_iter=%d, " \
		"gws_reduce_agent1=%zu, gws_action_ag=%zu, gws_mov_ag=%zu\n",
		iter, max_agents_iter, gws_reduce_agent1, gws_action_agent,
		gws_move_agent);
	blank_line = FALSE;

	if (agent_size_bytes == 8) {

		for (cl_uint k = 0; k < args.max_agents; k++) {
			cl_ulong curr_ag = ((cl_ulong*) agents_data)[k];
			if (!(dump_type & 0x01) ||
				((curr_ag & 0xFFFFFFFF00000000)!= 0xFFFFFFFF00000000)) {

				if (blank_line) fprintf(fp_agent_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_agent_dump,
					"[%4d] %016lx: (%4d, %4d) type=%d energy=%d\n",
					k, curr_ag,
					(cl_uint) (curr_ag >> 48),
					(cl_uint) ((curr_ag >> 32) & 0xFFFF),
					(cl_uint) ((curr_ag >> 16) & 0xFFFF),
					(cl_uint) (curr_ag & 0xFFFF));

			} else {
				blank_line = TRUE;
			}
		}

	} else if (agent_size_bytes == 4) {

		for (cl_uint k = 0; k < args.max_agents; k++) {
			cl_uint curr_ag = ((cl_uint*) agents_data)[k];
			if (!(dump_type & 0x01) || ((curr_ag & 0xFFFF0000)!= 0xFFFF0000)) {
				if (blank_line) fprintf(fp_agent_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_agent_dump,
					"[%4d] %08x: (%3d, %3d) type=%d energy=%d\n",
					k, curr_ag,
					(cl_uint) (curr_ag >> 22),
					(cl_uint) ((curr_ag >> 12) & 0x3FF),
					(cl_uint) ((curr_ag >> 11) & 0x1),
					(cl_uint) (curr_ag & 0x7FF));
			} else {
				blank_line = TRUE;
			}
		}
	}

	/* Export cell info. */
	fprintf(fp_cell_dump, "\nIteration %d\n", iter);
	blank_line = FALSE;
	for (cl_uint k = 0; k < params.grid_xy; k++) {
		if (!(dump_type & 0x10) || ((iter != -1)
				& (cells_agents_index[k].s[0] != args.max_agents))) {

			if (blank_line) fprintf(fp_cell_dump, "\n");
			blank_line = FALSE;
			if (iter != -1) {
				fprintf(fp_cell_dump, "(%d, %d) -> (%d, %d) %s [Grass: %d]\n",
					k % params.grid_x, k / params.grid_y,
					cells_agents_index[k].s[0], cells_agents_index[k].s[1],
					cells_agents_index[k].s[0] != cells_agents_index[k].s[1] ?
						"More than 1 agent present" : "",
					cells_grass[k]);
			} else {
				fprintf(fp_cell_dump, "(%d, %d) -> (-, -) [Grass: %d]\n",
					k % params.grid_x, k / params.grid_y,
					cells_grass[k]);
			}
		} else {
			blank_line = TRUE;
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

#endif

/**
 * Perform Predator-Prey simulation.
 *
 * @param params Simulation parameters.
 * @param zone OpenCL zone.
 * @param gws Kernels global work sizes.
 * @param lws Kernels local work sizes.
 * @param krnls OpenCL kernels.
 * @param evts OpenCL events.
 * @param dataSizes Size of data buffers.
 * @param buffersHost Host data buffers.
 * @param buffersDevice Device data buffers.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function
 * terminates successfully, or an error code otherwise.
 * */
static void ppg_simulate(PPGKernels krnls, CCLQueue * cq1, CCLQueue * cq2,
	CloSort * sorter, PPParameters params, PPGGlobalWorkSizes gws,
	PPGLocalWorkSizes lws, PPGDataSizes dataSizes,
	PPStatistics * stats_host, PPGBuffersDevice buffersDevice,
	GError ** err) {

	/* Stats. */
	PPStatistics * stats_pinned = NULL;

	/* Event wrappers. */
	CCLEvent * evt = NULL;
	CCLEvent * evt_action_agent = NULL;
	CCLEvent * evt_read_stats = NULL;
	CCLEvent * evt_reduce_grass2 = NULL;
	CCLEvent * evt_sort = NULL;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error handling object. */
	GError * err_internal = NULL;

	/* The maximum agents there can be in the next iteration. */
	cl_uint max_agents_iter =
		MAX(params.init_sheep + params.init_wolves, PPG_MIN_AGENTS);

	/* Dynamic worksizes. */
	size_t gws_reduce_agent1, ws_reduce_agent2, wg_reduce_agent1,
		gws_move_agent, gws_find_cell_idx, gws_action_agent;

	/* Current iteration. */
	cl_uint iter = 0;

	/* Map stats to host. */
	g_debug("Mapping stats to host...");
	stats_pinned = ccl_buffer_enqueue_map(buffersDevice.stats, cq2,
		CL_FALSE, CL_MAP_READ, 0, sizeof(PPStatistics), NULL, &evt,
		&err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Map: stats to host");

#ifdef PPG_DEBUG
	ccl_queue_finish(cq2, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

	/* Init. cells */
	g_debug("Initializing cells...");
	evt = ccl_kernel_enqueue_ndrange(krnls.init_cell, cq1, 1,
		NULL, &(gws.init_cell), &(lws.init_cell), NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "K: init cells");

#ifdef PPG_DEBUG
	ccl_queue_finish(cq1, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

	/* Init. agents */
	g_debug("Initializing agents...");
	evt = ccl_kernel_enqueue_ndrange(krnls.init_agent, cq2, 1,
		NULL, &(gws.init_agent), &(lws.init_agent), NULL,
		 &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "K: init agents");

#ifdef PPG_DEBUG
	ccl_queue_finish(cq2, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

#ifdef PPG_DUMP

	FILE *fp_agent_dump = fopen("dump_agents.txt", "w");
	FILE* fp_cell_dump = fopen("dump_cells.txt", "w");
	cl_ulong *agents_data =
		(cl_ulong*) malloc(dataSizes.agents_data);
	cl_uint2 *cells_agents_index =
		(cl_uint2*) malloc(dataSizes.cells_agents_index);
	cl_uint *cells_grass =
		(cl_uint*) malloc(dataSizes.cells_grass);

	ppg_dump(-1, PPG_DUMP, cq1, fp_agent_dump, fp_cell_dump, 0, 0, 0, 0,
		params, dataSizes, buffersDevice, agents_data,
		cells_agents_index, cells_grass, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

#else

	/* Ignore compiler warnings. */
	(void)dataSizes;

#endif


	/* *************** */
	/* SIMULATION LOOP */
	/* *************** */
	for (iter = 0; ; iter++) {

		/* ***************************************** */
		/* ********* Step 4: Gather stats ********** */
		/* ***************************************** */

		/* Step 4.1: Perform grass reduction, part I. Wait on agent
		 * actions from previous iteration. */
		g_debug("Iter %d: Performing grass reduction, part I...", iter);
		if (evt_action_agent != NULL)
			ccl_event_wait_list_add(&ewl, evt_action_agent, NULL);
		evt = ccl_kernel_enqueue_ndrange(krnls.reduce_grass1, cq1, 1,
			NULL, &(gws.reduce_grass1), &(lws.reduce_grass1), &ewl,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: reduce grass 1");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq1, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Step 4.2: Perform agents reduction, part I. */

		/* Determine worksizes. */
		/* See comment for grass reduction gws for an explanation the
		 * formulas bellow. */
		gws_reduce_agent1 = MIN(
			lws.reduce_agent1 * lws.reduce_agent1, /* lws * number_of_workgroups */
			CLO_GWS_MULT(
				CLO_DIV_CEIL(max_agents_iter, args_vw.reduce_agent),
				lws.reduce_agent1
			)
		);

		/* The local/global worksize of reduce_agent2 must be a power of
		 * 2 for the same reason of reduce_grass2. */
		wg_reduce_agent1 = gws_reduce_agent1 / lws.reduce_agent1;
		ws_reduce_agent2 = clo_nlpo2(wg_reduce_agent1);

		/* Set variable kernel arguments in agent reduction kernels. */
		ccl_kernel_set_arg(krnls.reduce_agent1, 3,
			ccl_arg_priv(max_agents_iter, cl_uint));

		ccl_kernel_set_arg(krnls.reduce_agent2, 3,
			ccl_arg_priv(wg_reduce_agent1, cl_uint));

		/* Run agent reduction kernel 1. */
		g_debug("Iter %d: Performing agent reduction, part I...", iter);
		evt = ccl_kernel_enqueue_ndrange(krnls.reduce_agent1, cq2, 1,
			NULL, &(gws_reduce_agent1), &(lws.reduce_agent1), NULL,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: reduce agent 1");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Step 4.3: Perform grass reduction, part II. Wait on read
		 * stats from previous iteration. */
		g_debug("Iter %d: Performing grass reduction part II...", iter);
		if (evt_read_stats != NULL)
			ccl_event_wait_list_add(&ewl, evt_read_stats, NULL);
		evt_reduce_grass2 = ccl_kernel_enqueue_ndrange(
			krnls.reduce_grass2, cq1, 1, NULL, &(gws.reduce_grass2),
			&(lws.reduce_grass2), &ewl, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt_reduce_grass2, "K: reduce grass 2");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq1, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Step 4.4: Perform agents reduction, part II. Wait on read
		 * stats from previous iteration. */
		g_debug("Iter %d: Performing agent reduction part II...", iter);
		if (evt_read_stats != NULL)
			ccl_event_wait_list_add(&ewl, evt_read_stats, NULL);
		evt = ccl_kernel_enqueue_ndrange(krnls.reduce_agent2, cq2, 1,
			NULL, &(ws_reduce_agent2), &(ws_reduce_agent2), &ewl,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: reduce agent 2");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Step 4.5: Get statistics. Wait on reduce_grass2. */
		g_debug("Iter %d: Getting statistics...", iter);
		ccl_event_wait_list_add(&ewl, evt_reduce_grass2, NULL);
		evt_read_stats = ccl_buffer_enqueue_read(buffersDevice.stats,
			cq2, CL_FALSE, 0, sizeof(PPStatistics), stats_pinned, &ewl,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt_read_stats, "Read: stats");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Stop simulation if this is the last iteration. */
		if (iter == params.iters) break;

		/* ***************************************** */
		/* ********* Step 1: Grass growth ********** */
		/* ***************************************** */

		/* Grass kernel: grow grass, set number of prey to zero */
		g_debug("Iter %d: Running grass kernel...", iter);
		evt = ccl_kernel_enqueue_ndrange(krnls.grass, cq1, 1,
			NULL, &(gws.grass), &(lws.grass), NULL, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: grass");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq1, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* ***************************************** */
		/* ******** Step 2: Agent movement ********* */
		/* ***************************************** */

		/* Determine agent movement global worksize. */
		gws_move_agent = CLO_GWS_MULT(
			max_agents_iter,
			lws.move_agent
		);

		g_debug("Iter %d: Move agents...", iter);
		evt = ccl_kernel_enqueue_ndrange(krnls.move_agent, cq2, 1,
			NULL, &(gws_move_agent), &(lws.move_agent), NULL,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: move agent");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* ***************************************** */
		/* ********* Step 3.1: Agent sort ********** */
		/* ***************************************** */

		g_debug("Iter %d: Sorting agents...", iter);
		/// @todo We should use a data_out buffer if sorting algorithm
		/// is not in-place. Also, should we keep this lws.sort_agent,
		/// or let the sorting algorithm figure it out
		evt_sort = clo_sort_with_device_data(sorter, cq2, cq2,
			buffersDevice.agents_data, NULL, max_agents_iter,
			lws.sort_agent, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* Determine the maximum number of agents there can be in the
		 * next iteration. Make sure stats are already transfered back
		 * to host. */

		ccl_event_wait(ccl_ewl(&ewl, evt_sort, NULL), &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

		memcpy(&stats_host[iter], stats_pinned, sizeof(PPStatistics));
		max_agents_iter = MAX(PPG_MIN_AGENTS,
			stats_host[iter].wolves + stats_host[iter].sheep);

		g_if_err_create_goto(*err, PP_ERROR,
			max_agents_iter > args.max_agents, PP_OUT_OF_RESOURCES,
			error_handler,
			"Agents required for next iteration above defined limit. " \
			"Current iter.: %d. Required agents: %d. Agents limit: %d",
			iter, max_agents_iter, args.max_agents);

#ifdef PPG_DEBUG
		ccl_queue_finish(cq1, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* ***************************************** */
		/* **** Step 3.2: Find cell agent index **** */
		/* ***************************************** */

		/* Determine kernel global worksize. */
		gws_find_cell_idx = CLO_GWS_MULT(
			max_agents_iter,
			lws.find_cell_idx
		);

		g_debug("Iter %d: Find cell agent indexes...", iter);
		evt = ccl_kernel_enqueue_ndrange(krnls.find_cell_idx, cq2, 1,
			NULL, &(gws_find_cell_idx), &(lws.find_cell_idx), NULL,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "K: find cell idx");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

		/* ***************************************** */
		/* ******* Step 3.3: Agent actions ********* */
		/* ***************************************** */

		/* Determine agent actions kernel global worksize. */
		gws_action_agent = CLO_GWS_MULT(
			max_agents_iter,
			lws.action_agent
		);

		/* Check if there is enough memory for all possible new
		 * agents. */
		g_if_err_create_goto(*err, PP_ERROR,
			gws_action_agent * 2 > args.max_agents, PP_OUT_OF_RESOURCES,
			error_handler,
			"Not enough memory for existing and possible new agents. " \
			"Current iter.: %d. Total possible agents: %d. Agents limit: %d",
			iter, (int) gws_action_agent * 2, (int) args.max_agents);

		/* Perform agent actions. */
		g_debug("Iter %d: Performing agent actions...", iter);
		evt_action_agent = ccl_kernel_enqueue_ndrange(
			krnls.action_agent, cq2, 1, NULL, &(gws_action_agent),
			&(lws.action_agent), NULL, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt_action_agent, "K: agent actions");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		if (iter % PPG_DEBUG == 0)
			fprintf(stderr, "Finishing iteration %d with " \
				"max_agents_iter=%d (%d sheep, %d wolves)...\n",
				iter, max_agents_iter, stats_host[iter].sheep,
				stats_host[iter].wolves);
#endif

		/* Agent actions may, in the worst case, double the number of
		 * agents. */
		max_agents_iter = MAX(max_agents_iter, lws.action_agent) * 2;

#ifdef PPG_DUMP

		ppg_dump(iter, PPG_DUMP, cq1, fp_agent_dump, fp_cell_dump,
			max_agents_iter, gws_reduce_agent1, gws_action_agent,
			gws_move_agent, params, dataSizes, buffersDevice,
			agents_data, cells_agents_index, cells_grass,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

#endif

	}
	/* ********************** */
	/* END OF SIMULATION LOOP */
	/* ********************** */

#ifdef PPG_DUMP
	fclose(fp_agent_dump);
	fclose(fp_cell_dump);
	free(agents_data);
	free(cells_agents_index);
	free(cells_grass);
#endif

	/* Post-simulation ops. */

	/* Get last iteration stats. */
	ccl_event_wait_list_add(&ewl, evt_read_stats, NULL);
	ccl_event_wait(&ewl, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	memcpy(&stats_host[iter], stats_pinned,
		sizeof(PPStatistics));

	/* Unmap stats. */
	evt = ccl_buffer_enqueue_unmap(buffersDevice.stats, cq2,
		stats_pinned, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "Unmap: stats");

#ifdef PPG_DEBUG
		ccl_queue_finish(cq2, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
#endif

	/* Guarantee all activity has terminated... */
	ccl_queue_finish(cq1, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_queue_finish(cq2, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

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
 * Compute worksizes depending on the device type and number of
 * available compute units.
 *
 * @param paramsSim Simulation parameters.
 * @param device Device where to perform simulation.
 * @param gws Kernel global worksizes (to be modified by function).
 * @param lws Kernel local worksizes (to be modified by function).
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function
 * terminates successfully, or an error code otherwise.
 * */
static void ppg_worksizes_compute(CCLProgram* prg,
	PPParameters paramsSim, PPGGlobalWorkSizes *gws,
	PPGLocalWorkSizes *lws, GError** err) {

	/* Device preferred int and long vector widths. */
	cl_uint int_vw, long_vw;

	/* Device where the simulation will take place. */
	CCLDevice* dev = NULL;

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get device where the simulation will take place. */
	dev = ccl_program_get_device(prg, 0, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the maximum workgroup size for the device. */
	lws->max_lws = ccl_device_get_info_scalar(
		dev, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine the default workgroup size. */
	if (args_lws.deflt > 0) {
		if (args_lws.deflt > lws->max_lws) {
			fprintf(stderr, "The specified default workgroup size, " \
				"%d, is higher than the device maximum, %d. Setting " \
				"default workgroup size to %d.\n", (int) args_lws.deflt,
				(int) lws->max_lws, (int) lws->max_lws);
			lws->deflt = lws->max_lws;
		} else {
			lws->deflt = args_lws.deflt;
		}
	} else {
		lws->deflt = lws->max_lws;
	}

	/* Get the device preferred int vector width. */
	int_vw = ccl_device_get_info_scalar(dev,
		CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the device preferred long vector width. */
	long_vw = ccl_device_get_info_scalar(dev,
		CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine effective grass kernel vector width. */
	if (args_vw.grass == 0)
		args_vw.grass = int_vw;

	/* Determine effective reduce grass kernels vector width. */
	if (args_vw.reduce_grass == 0)
		args_vw.reduce_grass = int_vw;

	/* Determine effective reduce agents kernel vector width. */
	if (args_vw.reduce_agent == 0) {
		if (args.agent_size == 32)
			args_vw.reduce_agent = int_vw;
		else if (args.agent_size == 64)
			args_vw.reduce_agent = long_vw;
		else
			g_assert_not_reached();
	}

	/* Init cell worksizes. */
	lws->init_cell = args_lws.init_cell ? args_lws.init_cell : lws->deflt;
	gws->init_cell = CLO_GWS_MULT(paramsSim.grid_xy, lws->init_cell);

	/* Init agent worksizes. */
	lws->init_agent = args_lws.init_agent ? args_lws.init_agent : lws->deflt;
	gws->init_agent = CLO_GWS_MULT(args.max_agents, lws->init_agent);

	/* Grass growth worksizes. */
	lws->grass = args_lws.grass ? args_lws.grass : lws->deflt;
	gws->grass = CLO_GWS_MULT(paramsSim.grid_xy / args_vw.grass, lws->grass);

	/* Grass reduce worksizes, must be power of 2 for reduction to work. */
	lws->reduce_grass1 =
		args_lws.reduce_grass ?  args_lws.reduce_grass : lws->deflt;
	if (!CLO_IS_PO2(lws->reduce_grass1)) {
		lws->reduce_grass1 =
			MIN(lws->max_lws, clo_nlpo2(lws->reduce_grass1));
		fprintf(stderr, "The workgroup size of the grass reduction " \
			"kernel must be a power of 2. Assuming a workgroup size " \
			"of %d.\n", (int) lws->reduce_grass1);
	}

	/* In order to perform reduction using just two kernel calls,
	 * reduce_grass1 and reduce_grass2, reduce_grass1 must have a number
	 * of workgroups smaller or equal to reduce_grass2's local work
	 * size. The reduce_grass2 kernel will have only one workgroup, and
	 * that single workgroup must be able to perform the final reduction.
	 * Thus, we enforce that maximum number of workgroups in
	 * reduce_grass1 by limiting the total number of workitems (i.e. the
	 * global work size).
	 * */
	gws->reduce_grass1 = MIN(
		lws->reduce_grass1 * lws->reduce_grass1, /* lws * number_of_workgroups */
		CLO_GWS_MULT(
			CLO_DIV_CEIL(paramsSim.grid_xy, args_vw.reduce_grass),
			lws->reduce_grass1
		)
	);

	/* The nlpo2() bellow is required for performing the final reduction
	 * step successfully because the implemented reduction algorithm
	 * assumes power of 2 local work sizes. */
	lws->reduce_grass2 = clo_nlpo2(gws->reduce_grass1 / lws->reduce_grass1);
	gws->reduce_grass2 = lws->reduce_grass2;

	/* Agent reduce worksizes, must be power of 2 for reduction to
	 * work. */
	lws->reduce_agent1 =
		args_lws.reduce_agent ?  args_lws.reduce_agent : lws->deflt;
	if (!CLO_IS_PO2(lws->reduce_agent1)) {
		lws->reduce_agent1 =
			MIN(lws->max_lws, clo_nlpo2(lws->reduce_agent1));
		fprintf(stderr, "The workgroup size of the agent reduction " \
			"kernel must be a power of 2. Assuming a workgroup size " \
			"of %d.\n", (int) lws->reduce_agent1);
	}

	/* The remaining agent reduction worksizes are determined on the fly
	 * because the number of agents varies during the simulation. */

	/* Agent movement local worksize. Global worksize depends on the
	 * number of existing agents. */
	lws->move_agent = args_lws.move_agent ? args_lws.move_agent : lws->deflt;

	/* Agent sort local worksize. Global worksize depends on the
	 * number of existing agents. */
	lws->sort_agent =
		args_lws.sort_agent ? args_lws.sort_agent : lws->deflt;

	/* Find cell agent index local worksize. Global worksize depends on the
	 * number of existing agents. */
	lws->find_cell_idx =
		args_lws.find_cell_idx ? args_lws.find_cell_idx : lws->deflt;

	/* Agent actions local worksize. Global worksize depends on the
	 * number of existing agents. */
	lws->action_agent =
		args_lws.action_agent ? args_lws.action_agent : lws->deflt;

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
 * Print information about simulation.
 *
 * @param gws Kernel global work sizes.
 * @param lws Kernel local work sizes.
 * @param dataSizes Size of data buffers.
 * @param compilerOpts Final OpenCL compiler options.
 * */
static void ppg_info_print(PPGGlobalWorkSizes gws,
	PPGLocalWorkSizes lws, PPGDataSizes dataSizes, CloSort* sorter,
	gchar* compilerOpts, GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Determine total global memory. */
	size_t dev_mem = sizeof(PPStatistics) +
		dataSizes.cells_grass +
		dataSizes.cells_agents_index +
		dataSizes.agents_data +
		dataSizes.reduce_grass_global +
		dataSizes.reduce_agent_global +
		dataSizes.rng_seeds;

	/* Print info. */
	/// @todo Make ints unsigned or change %d to something nice for size_t
	printf("\n   =========================== Simulation Info =============================\n\n");
	printf("     Required global memory    : %d bytes (%d Kb = %d Mb)\n",
		(int) dev_mem, (int) dev_mem / 1024, (int) dev_mem / 1024 / 1024);
	printf("     Compiler options          : %s\n", compilerOpts);
	printf("     Kernel work sizes and local memory requirements:\n");
	printf("       -------------------------------------------------------------------\n");
	printf("       | Kernel             | GWS      | LWS   | Local mem. | VW x bytes |\n");
	printf("       |                    |          |       | (bytes)    |            |\n");
	printf("       -------------------------------------------------------------------\n");
	printf("       | init_cell          | %8zu | %5zu |          0 |          0 |\n",
		gws.init_cell, lws.init_cell);
	printf("       | init_agent         | %8zu | %5zu |          0 |          0 |\n",
		gws.init_agent, lws.init_agent);
	printf("       | grass              | %8zu | %5zu |          0 |     %2d x %zu |\n",
		gws.grass, lws.grass, args_vw.grass, sizeof(cl_uint));
	printf("       | reduce_grass1      | %8zu | %5zu | %10zu |     %2d x %zu |\n",
		gws.reduce_grass1, lws.reduce_grass1, dataSizes.reduce_grass_local1,
		args_vw.reduce_grass, sizeof(cl_uint));
	printf("       | reduce_grass2      | %8zu | %5zu | %10zu |     %2d x %zu |\n",
		gws.reduce_grass2, lws.reduce_grass2, dataSizes.reduce_grass_local2,
		args_vw.reduce_grass, sizeof(cl_uint));
	printf("       | reduce_agent1      |     Var. | %5zu | %10zu |     %2d x %zu |\n",
		lws.reduce_agent1, dataSizes.reduce_agent_local1, args_vw.reduce_agent,
		agent_size_bytes);
	printf("       | reduce_agent2      |     Var. |  Var. | %10zu |     %2d x %zu |\n",
		dataSizes.reduce_agent_local2, args_vw.reduce_agent, agent_size_bytes);
	printf("       | move_agent         |     Var. | %5zu |          0 |          0 |\n",
		lws.move_agent);

	/* Get number of sort kernels. */
	cl_uint num_sort_kernels =
		clo_sort_get_num_kernels(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Show information for sort kernels. */
	for (unsigned int i = 0; i < num_sort_kernels; i++) {
		const char* kernel_name = clo_sort_get_kernel_name(
			sorter, i, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		size_t loc_mem = clo_sort_get_localmem_usage(
			sorter, i, lws.sort_agent, args.max_agents, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		printf("       | %-18.18s |     Var. | %5zu | %10zu |          0 |\n",
			kernel_name, lws.sort_agent, loc_mem);
	}

	printf("       | find_cell_idx      |     Var. | %5zu |          0 |          0 |\n",
		lws.find_cell_idx);
	printf("       | action_agent       |     Var. | %5zu |          0 |          0 |\n",
		lws.action_agent);
	printf("       -------------------------------------------------------------------\n");

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
 * Get OpenCL kernels wrappers.
 *
 * @param[in] prg Program wrapper..
 * @param[out] krnls Kernel wrappers.
 * @param[out] err Return location for a GError.
 * */
static void ppg_kernels_get(CCLProgram* prg, PPGKernels* krnls,
	GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get kernels from program. */
	krnls->init_cell = ccl_program_get_kernel(
		prg, "init_cell", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->init_agent = ccl_program_get_kernel(
		prg, "init_agent", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->reduce_grass1 = ccl_program_get_kernel(
		prg, "reduce_grass1", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->reduce_grass2 = ccl_program_get_kernel(
		prg, "reduce_grass2", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->reduce_agent1 = ccl_program_get_kernel(
		prg, "reduce_agent1", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->reduce_agent2 = ccl_program_get_kernel(
		prg, "reduce_agent2", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->grass = ccl_program_get_kernel(
		prg, "grass", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->move_agent = ccl_program_get_kernel(
		prg, "move_agent", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->find_cell_idx = ccl_program_get_kernel(
		prg, "find_cell_idx", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnls->action_agent = ccl_program_get_kernel(
		prg, "action_agent", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

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
 * Set fixed kernel arguments.
 *
 * @param[in] krnls Kernel wrappers.
 * @param[in] buffersDevice Device data buffers.
 * @param[in] dataSizes Size of data buffers.
 * @param[in] lws Local work sizes.
 * @param[out] err Return location for a GError.
 * */
static void ppg_kernelargs_set(PPGKernels krnls,
	PPGBuffersDevice buffersDevice, PPGDataSizes dataSizes) {

	/* Cell init kernel. */
	ccl_kernel_set_args(krnls.init_cell, buffersDevice.cells_grass,
		buffersDevice.rng_seeds, NULL);

	/* Agent init kernel. */
	ccl_kernel_set_args(krnls.init_agent, buffersDevice.agents_data,
		buffersDevice.rng_seeds, NULL);

	/* Grass kernel */
	ccl_kernel_set_args(krnls.grass, buffersDevice.cells_grass,
		buffersDevice.cells_agents_index, NULL);

	/* reduce_grass1 kernel */
	ccl_kernel_set_args(krnls.reduce_grass1, buffersDevice.cells_grass,
		ccl_arg_full(NULL, dataSizes.reduce_grass_local1),
		buffersDevice.reduce_grass_global, NULL);

	/* reduce_grass2 kernel */
	ccl_kernel_set_args(krnls.reduce_grass2,
		buffersDevice.reduce_grass_global,
		ccl_arg_full(NULL, dataSizes.reduce_grass_local2),
		buffersDevice.stats, NULL);

	/* reduce_agent1 kernel */
	ccl_kernel_set_args(krnls.reduce_agent1, buffersDevice.agents_data,
		ccl_arg_full(NULL, dataSizes.reduce_agent_local1),
		buffersDevice.reduce_agent_global, NULL);
	/* The 3rd argument is set on the fly. */

	/* reduce_agent2 kernel */
	ccl_kernel_set_args(krnls.reduce_agent2,
		buffersDevice.reduce_agent_global,
		ccl_arg_full(NULL, dataSizes.reduce_agent_local2),
		buffersDevice.stats, NULL);

	/* Agent movement kernel. */
	ccl_kernel_set_args(krnls.move_agent, buffersDevice.agents_data,
		buffersDevice.rng_seeds, NULL);

	/* Find cell agent index kernel. */
	ccl_kernel_set_args(krnls.find_cell_idx, buffersDevice.agents_data,
		buffersDevice.cells_agents_index, NULL);

	/* Agent actions kernel. */
	ccl_kernel_set_args(krnls.action_agent, buffersDevice.cells_grass,
		buffersDevice.cells_agents_index, buffersDevice.agents_data,
		buffersDevice.agents_data, buffersDevice.rng_seeds, NULL);

}

/**
 * Determine sizes of data buffers.
 *
 * @param[in] params Simulation parameters.
 * @param[out] dataSizes Size of data buffers.
 * @param[in] gws Kernel global work sizes.
 * @param[in] lws Kernel local work sizes.
 * */
static void ppg_datasizes_get(PPGDataSizes * dataSizes,
	PPParameters params, CloRng * rng_clo, CloSort * sorter,
	PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {

	/// @todo Sorter should be used to query for total required device
	/// memory for sorting.
	(void)sorter;

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);

	/* Environment cells */
	dataSizes->cells_grass =
		pp_next_multiple(params.grid_xy, args_vw.grass)
		* sizeof(cl_uint);
	dataSizes->cells_agents_index = params.grid_xy * sizeof(cl_uint2);

	/* Agents. */
	dataSizes->agents_data = args.max_agents * agent_size_bytes;

	/* Grass reduction. */
	dataSizes->reduce_grass_local1 =
		2 * lws.reduce_grass1 * args_vw.reduce_grass * sizeof(cl_uint);
	dataSizes->reduce_grass_global =
		2 * gws.reduce_grass2 * args_vw.reduce_grass * sizeof(cl_uint);
	dataSizes->reduce_grass_local2 =
		2 * lws.reduce_grass2 * args_vw.reduce_grass * sizeof(cl_uint);

	/* Agent reduction. */
	dataSizes->reduce_agent_local1 = 4 * lws.reduce_agent1
		* args_vw.reduce_agent * agent_size_bytes; /* 4x to count sheep pop, wolves pop, sheep en, wolves en. */
	dataSizes->reduce_agent_global = dataSizes->reduce_agent_local1;
	dataSizes->reduce_agent_local2 = dataSizes->reduce_agent_local1;

	/* RNG */
	dataSizes->rng_seeds = clo_rng_get_size(rng_clo);

}

/**
 * Initialize device buffers.
 *
 * @param[in] ctx Context wrapper.
 * @param[in] rng_clo CL_Ops RNG object.
 * @param[out] buffersDevice Data structure containing device data
 * buffers.
 * @param[in] dataSizes Size of data buffers.
 * @param[out] err Return location for a GError.
 * */
static void ppg_devicebuffers_create(CCLContext* ctx, CloRng* rng_clo,
	PPGBuffersDevice* buffersDevice, PPGDataSizes dataSizes,
	GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Statistics */
	buffersDevice->stats = ccl_buffer_new(ctx,
		CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(PPStatistics),
		NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Cells */
	buffersDevice->cells_grass = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		dataSizes.cells_grass, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	buffersDevice->cells_agents_index = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE, dataSizes.cells_agents_index, NULL,
		&err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Agents. */
	buffersDevice->agents_data = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		dataSizes.agents_data, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE, dataSizes.reduce_grass_global, NULL,
		&err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Agent reduction (count) */
	buffersDevice->reduce_agent_global = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE, dataSizes.reduce_agent_global, NULL,
		&err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* RNG seeds. */
	buffersDevice->rng_seeds = clo_rng_get_device_seeds(rng_clo);

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
 * Free device buffers.
 *
 * @param[in] buffersDevice Data structure containing device data
 * buffers.
 * */
static void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice) {

	if (buffersDevice->stats)
		ccl_buffer_destroy(buffersDevice->stats);
	if (buffersDevice->cells_grass)
		ccl_buffer_destroy(buffersDevice->cells_grass);
	if (buffersDevice->cells_agents_index)
		ccl_buffer_destroy(buffersDevice->cells_agents_index);
	if (buffersDevice->agents_data)
		ccl_buffer_destroy(buffersDevice->agents_data);
	if (buffersDevice->reduce_agent_global)
		ccl_buffer_destroy(buffersDevice->reduce_agent_global);
	if (buffersDevice->reduce_grass_global)
		ccl_buffer_destroy(buffersDevice->reduce_grass_global);

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
static gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws,
	PPGLocalWorkSizes lws, PPParameters params, gchar* cliOpts) {

	gchar* compilerOptsStr;

	GString* compilerOpts = g_string_new("");
	g_string_append_printf(compilerOpts, "-D VW_GRASS=%d ",
		args_vw.grass);
	g_string_append_printf(compilerOpts, "-D VW_GRASSREDUCE=%d ",
		args_vw.reduce_grass);
	g_string_append_printf(compilerOpts, "-D VW_AGENTREDUCE=%d ",
		args_vw.reduce_agent);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKGROUPS=%d ",
		(unsigned int) (gws.reduce_grass1 / lws.reduce_grass1));
	g_string_append_printf(compilerOpts, "-D MAX_LWS=%d ",
		(unsigned int) lws.max_lws);
	g_string_append_printf(compilerOpts, "-D MAX_AGENTS=%d ",
		args.max_agents);
	g_string_append_printf(compilerOpts, "-D CELL_NUM=%d ",
		params.grid_xy);
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
	g_string_append_printf(compilerOpts, "-D %s ",
		args.agent_size == 64 ? "PPG_AG_64" : "PPG_AG_32");
	if (cliOpts) g_string_append_printf(compilerOpts, "%s", cliOpts);
	compilerOptsStr = compilerOpts->str;

	g_string_free(compilerOpts, FALSE);

	return compilerOptsStr;
}

/**
 * Parse command-line options.
 *
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Command line arguments.
 * @param[in] context Context object for command line argument parsing.
 * @param[out] err Return location for a GError.
 * */
static void ppg_args_parse(int argc, char* argv[],
	GOptionContext** context, GError** err) {

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Create context and add main entries. */
	*context = g_option_context_new (" - " PPG_DESCRIPTION);
	g_option_context_add_main_entries(*context, entries, NULL);

	/* Create separate option groups. */
	GOptionGroup *group_alg = g_option_group_new("alg",
		"Algorithm selection:",
		"Show options for algorithm selection",
		NULL, NULL);
	GOptionGroup *group_lws = g_option_group_new("lws",
		"Kernel local work sizes:",
		"Show options which define the kernel local work sizes",
		NULL, NULL);
	GOptionGroup *group_vw = g_option_group_new("vw",
		"Vector widths for kernels:",
		"Show options which define vector widths for different kernels",
		NULL, NULL);

	/* Add entries to separate option groups. */
	g_option_group_add_entries(group_alg, entries_alg);
	g_option_group_add_entries(group_lws, entries_lws);
	g_option_group_add_entries(group_vw, entries_vw);

	/* Add option groups to main options context. */
	g_option_context_add_group(*context, group_alg);
	g_option_context_add_group(*context, group_lws);
	g_option_context_add_group(*context, group_vw);

	/* Parse all options. */
	g_option_context_parse(*context, &argc, &argv, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine random number generator. */
	if (!args_alg.rng) args_alg.rng = g_strdup(PP_RNG_DEFAULT);

	/* Determine sorting algorithm. */
	if (!args_alg.sort) args_alg.sort = g_strdup(PPG_SORT_DEFAULT);

	/* ** Validate arguments. ** */

	/* Validate agent size. */
	g_if_err_create_goto(*err, PP_ERROR,
		(args.agent_size != 32) && (args.agent_size != 64),
		PP_INVALID_ARGS, error_handler,
		"The -a (--agent-size) parameter must be either 32 or 64.");
	agent_size_bytes = args.agent_size == 64
		? sizeof(cl_ulong) : sizeof(cl_uint);

	/* Validate vector sizes. */
	g_if_err_create_goto(*err, PP_ERROR,
		(clo_ones32(args_vw.grass) > 1) || (args_vw.grass > 16),
		PP_INVALID_ARGS, error_handler,
		"The -vw-grass parameter must be either 0 (auto-detect), 1, 2, 4, "
		"8 or 16.");

	g_if_err_create_goto(*err, PP_ERROR,
		(clo_ones32(args_vw.reduce_grass) > 1) || (args_vw.reduce_grass > 16),
		PP_INVALID_ARGS, error_handler,
		"The -vw-reduce-grass parameter must be either 0 (auto-detect), 1, 2, "
		"4, 8 or 16.");

	g_if_err_create_goto(*err, PP_ERROR,
		(clo_ones32(args_vw.reduce_agent) > 1) || (args_vw.reduce_agent > 16),
		PP_INVALID_ARGS, error_handler,
		"The -vw-reduce-agent parameter must be either 0 (auto-detect), 1, 2, "
		"4, 8 or 16.");

	/* If we got here, everything is OK. */
	g_assert (*err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:

	/* Return. */
	return;

}

/**
 * @brief Free command line parsing related objects.
 *
 * @param context Context object for command line argument parsing.
 * */
static void ppg_args_free(GOptionContext* context) {
	if (context) {
		g_option_context_free(context);
	}
	if (args.params) g_free(args.params);
	if (args.stats) g_free(args.stats);
#ifdef PP_PROFILE_OPT
	if (args.prof_agg_file) g_free(args.prof_agg_file);
#endif
	if (args.compiler_opts) g_free(args.compiler_opts);
	if (args_alg.rng) g_free(args_alg.rng);
	if (args_alg.sort) g_free(args_alg.sort);
	if (args_alg.sort_opts) g_free(args_alg.sort_opts);
}

/**
 * @brief Main program.
 *
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return ::PP_SUCCESS if program terminates successfully, or another
 * value of ::pp_error_codes if an error occurs.
 * */
int main(int argc, char **argv) {

	/* Auxiliary status variable. */
	int status;

	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;

	/* Predator-Prey simulation data structures. */
	PPGGlobalWorkSizes gws;
	PPGLocalWorkSizes lws;
	PPGDataSizes dataSizes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	PPGBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPParameters params;
	PPGKernels krnls =
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPStatistics * stats_host = NULL;
	gchar* compilerOpts = NULL;

	/* OpenCL wrappers. */
	CCLContext * ctx = NULL;
	CCLDevice * dev = NULL;
	CCLQueue * cq1 = NULL;
	CCLQueue * cq2 = NULL;
	CCLProgram * prg = NULL;

	/* Profiler. */
	CCLProf * prof = NULL;

	/* Device random number generator. */
	CloRng * rng_clo = NULL;

	/* CL_Ops sorter object. */
	CloSort * sorter = NULL;

	/* Complete OCL program source code. */
	gchar * src = NULL;

	/* Device selection filters. */
	CCLDevSelFilters filters = NULL;

	/* Host random number generator. */
	GRand * rng = NULL;

	/* Error management object. */
	GError * err = NULL;

	/* Type of elements (agents) to sort and type of keys to sort. */
	CloType ag_sort_elem_type, ag_sort_key_type;

	/* One-line OpenCL code to extract key from element (agent) in order
	 * to sort it. */
	const char * get_key;

	/* Parse and validate arguments. */
	ppg_args_parse(argc, argv, &context, &err);
	g_if_err_goto(err, error_handler);

	/* Create host RNG with specified seed. */
	rng = g_rand_new_with_seed(args.rng_seed);

	/* Profiling / Timmings. */
	prof = ccl_prof_new();

	/* Get simulation parameters */
	pp_load_params(&params, args.params, &err);
	g_if_err_goto(err, error_handler);

	/* Specify device filters and create context from them. */
	ccl_devsel_add_indep_filter(
		&filters, ccl_devsel_indep_type_gpu, NULL);
	ccl_devsel_add_dep_filter(
		&filters, ccl_devsel_dep_menu, (void *) &args.dev_idx);

	ctx = ccl_context_new_from_filters(&filters, &err);
	g_if_err_goto(err, error_handler);

	/* Get chosen device. */
	dev = ccl_context_get_device(ctx, 0, &err);
	g_if_err_goto(err, error_handler);

	/* Create command queues. */
	cq1 = ccl_queue_new(ctx, dev, PP_QUEUE_PROPERTIES, &err);
	g_if_err_goto(err, error_handler);

	cq2 = ccl_queue_new(ctx, dev, PP_QUEUE_PROPERTIES, &err);
	g_if_err_goto(err, error_handler);

	/* Create RNG object. */
	rng_clo = clo_rng_new(
		args_alg.rng, CLO_RNG_SEED_HOST_MT, NULL,
		MAX(args.max_agents, params.grid_xy), args.rng_seed, NULL,
		ctx, cq1, &err);
	g_if_err_goto(err, error_handler);

	/* Create sorter object. */
	if (args.agent_size == 64) {
		ag_sort_elem_type = CLO_ULONG;
		get_key = "((x) >> 17)";
	} else {
		ag_sort_elem_type = CLO_UINT;
		get_key = "((x) >> 12)";
	}
	ag_sort_key_type = CLO_ULONG;
	sorter = clo_sort_new(args_alg.sort, args_alg.sort_opts, ctx,
		&ag_sort_elem_type, &ag_sort_key_type, NULL, get_key,
		args.compiler_opts, &err);
	g_if_err_goto(err, error_handler);

	/* Concatenate complete source: RNG kernels source + common source
	 * + GPU source. */
	src = g_strconcat(clo_rng_get_source(rng_clo), PP_COMMON_SRC,
		PP_GPU_SRC, NULL);

	/* Create program. */
	prg = ccl_program_new_from_source(ctx, src, &err);
	g_if_err_goto(err, error_handler);

	/* Compute work sizes for different kernels. */
	ppg_worksizes_compute(prg, params, &gws, &lws, &err);
	g_if_err_goto(err, error_handler);

	/* Compiler options. */
	compilerOpts = ppg_compiler_opts_build(gws, lws, params,
		args.compiler_opts);

	/* Build program. */
	ccl_program_build(prg, compilerOpts, &err);
	pp_build_log(prg);
	g_if_err_goto(err, error_handler);

	/* Populate kernels struct. */
	ppg_kernels_get(prg, &krnls, &err);
	g_if_err_goto(err, error_handler);

	/* Determine size in bytes for host and device data structures. */
	ppg_datasizes_get(&dataSizes, params, rng_clo, sorter, gws, lws);

	/* Initialize host statistics buffer. */
	stats_host = (PPStatistics*) g_slice_alloc0(dataSizes.stats);

	/* Create device buffers */
	ppg_devicebuffers_create(ctx, rng_clo, &buffersDevice,
		dataSizes, &err);
	g_if_err_goto(err, error_handler);

	/*  Set fixed kernel arguments. */
	ppg_kernelargs_set(krnls, buffersDevice, dataSizes);

	/* Print information about simulation. */
	ppg_info_print(gws, lws, dataSizes, sorter, compilerOpts, &err);
	g_if_err_goto(err, error_handler);

	/* Start basic timming / profiling. */
	ccl_prof_start(prof);

	/* Simulation!! */
	ppg_simulate(krnls, cq1, cq2, sorter, params, gws, lws, dataSizes,
		stats_host, buffersDevice, &err);
	g_if_err_goto(err, error_handler);

	/* Stop basic timing / profiling. */
	ccl_prof_stop(prof);

	/* Output results to file */
	pp_stats_save(args.stats, stats_host, params, &err);
	g_if_err_goto(err, error_handler);

#ifdef PP_PROFILE_OPT
	/* Analyze events */
	ccl_prof_add_queue(prof, "Queue 1", cq1);
	ccl_prof_add_queue(prof, "Queue 2", cq2);
	ccl_prof_calc(prof, &err);
	g_if_err_goto(err, error_handler);

	/* Export aggregate profile info if so requested by user. */
	if (args.prof_agg_file) {
		pp_export_prof_agg_info(args.prof_agg_file, prof);
	}

	/* Print profiling summary. */
	ccl_prof_print_summary(prof);
#else

	/* Print ellapsed time. */
	printf("Ellapsed time: %.4es\n", ccl_prof_time_elapsed(prof));

#endif

	/* If we get here, no need for error checking, jump to cleanup. */
	status = PP_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;

error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	status = err->code;
	fprintf(stderr, "Error: %s\n", err->message);
#ifdef PPG_DEBUG
	fprintf(stderr, "Error code (domain): %d (%s)\n",
		err->code, g_quark_to_string(err->domain));
	fprintf(stderr, "Exit status: %d\n", status);
#endif
	g_error_free(err);

cleanup:

	/* Release OpenCL memory objects */
	ppg_devicebuffers_free(&buffersDevice);

	/* Free host statistics buffer. */
	g_slice_free1(dataSizes.stats, stats_host);

	/* Free compiler options. */
	if (compilerOpts) g_free(compilerOpts);

	/* Free profiler object. */
	if (prof) ccl_prof_destroy(prof);

	/* Free CL_Ops RNG object. */
	if (rng_clo) clo_rng_destroy(rng_clo);

	/* Free CL_Ops sorter object. */
	if (sorter) clo_sort_destroy(sorter);

	/* Free host RNG */
	if (rng) g_rand_free(rng);

	/* Free complete program source. */
	if (src) g_free(src);

	/* Free remaining OpenCL wrappers. */
	if (prg) ccl_program_destroy(prg);
	if (cq1) ccl_queue_destroy(cq1);
	if (cq2) ccl_queue_destroy(cq2);
	if (ctx) ccl_context_destroy(ctx);

	/* Free context and associated cli args parsing buffers. */
	ppg_args_free(context);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

	/* Bye. */
	return status;

}
