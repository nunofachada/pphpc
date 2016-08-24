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
 * Implementation of the legacy predator-prey OpenCL GPU simulation.
 * */

#include "pp_gpu_legacy.h"

#define MAX_AGENTS 1048576

#define CELL_SPACE 4 /* 0-Grass state, 1-number of agents in cell, 2-pointer to first agent in cell, 3-unused */
#define CELL_GRASS_OFFSET 0
#define CELL_NUMAGENTS_OFFSET 1
#define CELL_AGINDEX_OFFSET 2

#define LWS_GPU_MAX 256 //512
#define LWS_GPU_PREF 64 //128
#define LWS_GPU_MIN 8

#define LWS_GPU_PREF_2D_X 16
#define LWS_GPU_PREF_2D_Y 8

#define MAX_GRASS_COUNT_LOOPS 5 //More than enough...

/** Agent properties. */
typedef struct pp_gs_agent {
	/** Horizontal position. */
	cl_uint x;
	/** Vertical position. */
	cl_uint y;
	/** Is agent alive? */
	cl_uint alive;
	/** Agent energy. */
	cl_ushort energy;
	/** Agent type. */
	cl_ushort type;
}  PPGSAgent __attribute__ ((aligned (16)));

/** Simulation parameters. */
typedef struct pp_gs_sim_params {
	/** Horizontal grid size. */
	cl_uint size_x;
	/** Vertical grid size. */
	cl_uint size_y;
	/** Number of cells in grid. */
	cl_uint size_xy;
	/** Maximum number of agents. */
	cl_uint max_agents;
	/** Grass restart time. */
	cl_uint grass_restart;
	/** Size of each cell grid. */
	cl_uint grid_cell_space;
} PPGSSimParams;

/* Global work sizes */
size_t agentsort_gws, agent_gws, grass_gws[2], agentcount1_gws,
	agentcount2_gws, grasscount1_gws,
	grasscount2_gws[MAX_GRASS_COUNT_LOOPS];

/* Local work sizes */
size_t agentsort_lws, agent_lws, grass_lws[2], agentcount1_lws,
	agentcount2_lws, grasscount1_lws, grasscount2_lws;
int effectiveNextGrassToCount[MAX_GRASS_COUNT_LOOPS];
int numGrassCount2Loops;

/* Kernels */
CCLKernel* grass_kernel = NULL;
CCLKernel* agentmov_kernel = NULL;
CCLKernel* agentupdate_kernel = NULL;
CCLKernel* sort_kernel = NULL;
CCLKernel* agentaction_kernel = NULL;
CCLKernel* countagents1_kernel = NULL;
CCLKernel* countagents2_kernel = NULL;
CCLKernel* countgrass1_kernel = NULL;
CCLKernel* countgrass2_kernel = NULL;

/**
 * Compute worksizes depending on the device type and number of
 * available compute units.
 *
 * @param[in] params Simulation parameters.
 * */
static void computeWorkSizes(PPParameters params) {

	/* grass growth worksizes */
	grass_lws[0] = LWS_GPU_PREF_2D_X;
	grass_gws[0] = LWS_GPU_PREF_2D_X *
		ceil(((float) params.grid_x) / LWS_GPU_PREF_2D_X);
	grass_lws[1] = LWS_GPU_PREF_2D_Y;
	grass_gws[1] = LWS_GPU_PREF_2D_Y *
		ceil(((float) params.grid_y) / LWS_GPU_PREF_2D_Y);

	/* fixed local agent kernel worksizes */
	agent_lws = LWS_GPU_PREF;
	agentcount1_lws = LWS_GPU_MAX;
	agentcount2_lws = LWS_GPU_MAX;

	/* grass count worksizes */
	grasscount1_lws = LWS_GPU_MAX;
	grasscount1_gws = LWS_GPU_MAX *
		ceil((((float) params.grid_x * params.grid_y)) / LWS_GPU_MAX);
	grasscount2_lws = LWS_GPU_MAX;
	effectiveNextGrassToCount[0] = grasscount1_gws / grasscount1_lws;
	grasscount2_gws[0] = LWS_GPU_MAX *
		ceil(((float) effectiveNextGrassToCount[0]) / LWS_GPU_MAX);

	/* Determine number of loops of secondary count required to perform
	 * complete reduction */
	numGrassCount2Loops = 1;
	while ( grasscount2_gws[numGrassCount2Loops - 1] > grasscount2_lws) {
		effectiveNextGrassToCount[numGrassCount2Loops] =
			grasscount2_gws[numGrassCount2Loops - 1] / grasscount2_lws;
		grasscount2_gws[numGrassCount2Loops] = LWS_GPU_MAX *
			ceil(((float) effectiveNextGrassToCount[numGrassCount2Loops])
				/ LWS_GPU_MAX);
		numGrassCount2Loops++;
	}
}

/**
 * Print worksizes.
 * */
static void printFixedWorkSizes() {
	printf("Fixed kernel sizes:\n");
	printf("grass_gws=[%d,%d]\tgrass_lws=[%d,%d]\n", (int) grass_gws[0],
		(int) grass_gws[1], (int) grass_lws[0], (int) grass_lws[1]);
	printf("agent_lws=%d\n", (int) agent_lws);
	printf("agentcount1_lws=%d\n", (int) agentcount1_lws);
	printf("agentcount2_lws=%d\n", (int) agentcount2_lws);
	printf("grasscount1_gws=%d\tgrasscount1_lws=%d\n",
		(int) grasscount1_gws, (int) grasscount1_lws);
	printf("grasscount2_lws=%d\n", (int) grasscount2_lws);
	for (int i = 0; i < numGrassCount2Loops; i++) {
		printf("grasscount2_gws[%d]=%d (effective grass to count: %d)\n",
			i, (int) grasscount2_gws[i], effectiveNextGrassToCount[i]);
	}
	printf("Total of %d grass count loops.\n", numGrassCount2Loops);
}

/**
 * Get kernel wrappers from program wrapper.
 *
 * @param[in] prg Program wrapper.
 * @param[out] err Return location for a GError.
 * */
static void getKernelsFromProgram(CCLProgram* prg, GError** err) {

	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get kernels. */
	grass_kernel = ccl_program_get_kernel(prg, "Grass", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	agentmov_kernel =
		ccl_program_get_kernel(prg, "RandomWalk", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	agentupdate_kernel =
		ccl_program_get_kernel(prg, "AgentsUpdateGrid", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	sort_kernel =
		ccl_program_get_kernel(prg, "BitonicSort", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	agentaction_kernel =
		ccl_program_get_kernel(prg, "AgentAction", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	countagents1_kernel =
		ccl_program_get_kernel(prg, "CountAgents1", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	countagents2_kernel =
		ccl_program_get_kernel(prg, "CountAgents2", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	countgrass1_kernel =
		ccl_program_get_kernel(prg, "CountGrass1", &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	countgrass2_kernel =
		ccl_program_get_kernel(prg, "CountGrass2", &err_internal);
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
 * Main function of the legacy predator-prey OpenCL GPU simulation.
 *
 * @param[in] argc See `argv`.
 * @param[in] argv All input parameters will be considered compiler
 * options.
 * @return ::PP_SUCCESS if simulation terminates properly, or another
 * ::pp_error_codes error code otherwise.
 * */
int main(int argc, char* argv[]) {

	/* Status var aux */
	int status;

	/* Error management */
	GError *err = NULL;

	/* Host memory buffers */
	PPStatistics* statsArrayHost = NULL;
	cl_uint* numAgentsHost = NULL;
	PPGSAgent* agentArrayHost = NULL;
	cl_uint* grassMatrixHost = NULL;
	cl_ulong* rngSeedsHost = NULL;

	/* Device memory buffers */
	CCLBuffer* statsArrayDevice = NULL;
	CCLBuffer* agentArrayDevice = NULL;
	CCLBuffer* grassMatrixDevice = NULL;
	CCLBuffer* iterDevice = NULL;
	CCLBuffer* numAgentsDevice = NULL;
	CCLBuffer* grassCountDevice = NULL;
	CCLBuffer* agentsCountDevice = NULL;
	CCLBuffer* agentParamsDevice = NULL;
	CCLBuffer* rngSeedsDevice = NULL;

	/* Other OpenCL wrappers. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;

	/* cl_ops RNG. */
	CloRng* rng_clo = NULL;

	/* Parameters */
	PPParameters params;

	/* Event wait list and events */
	CCLEventWaitList ewl = NULL;
	CCLEvent* agentaction_move_event = NULL;
	CCLEvent* grass_event = NULL;
	CCLEvent* agentaction_event = NULL;
	CCLEvent* agentsort_event = NULL;
	CCLEvent* agentcount1_event = NULL;
	CCLEvent* agentcount2_event = NULL;
	CCLEvent* agentupdate_event = NULL;
	CCLEvent* grasscount1_event = NULL;
	CCLEvent* grasscount2_event = NULL;
	CCLEvent* readNumAgents_event = NULL;
	CCLEvent* map_numagents_event = NULL;
	CCLEvent* unmap_numagents_event = NULL;
	CCLEvent* barrier_event = NULL;
	CCLEvent* readStats_event = NULL;

	/* Complete OCL program source code. */
	gchar* src = NULL;

	/* Device selection filters. */
	CCLDevSelFilters filters = NULL;

	/* Random number generator */
	GRand* rng = g_rand_new();

	/* Compiler options. */
	gchar* compilerOpts = NULL;
	GString* compilerOptsBuilder = NULL;

	/* Profiling / Timmings */
	CCLProf* prof = ccl_prof_new();

	/* Specify device filters and create context from them. */
	ccl_devsel_add_indep_filter(
		&filters, ccl_devsel_indep_type_gpu, NULL);
	ccl_devsel_add_dep_filter(&filters, ccl_devsel_dep_menu, NULL);

	ctx = ccl_context_new_from_filters(&filters, &err);
	g_if_err_goto(err, error_handler);

	/* Get chosen device. */
	dev = ccl_context_get_device(ctx, 0, &err);
	g_if_err_goto(err, error_handler);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, PP_QUEUE_PROPERTIES, &err);
	g_if_err_goto(err, error_handler);

	/* Create RNG object. */
	rng_clo = clo_rng_new("lcg", CLO_RNG_SEED_HOST_MT, NULL, MAX_AGENTS,
		0, NULL, ctx, cq, &err);
	g_if_err_goto(err, error_handler);

	/* Get RNG seeds device buffer. */
	rngSeedsDevice = clo_rng_get_device_seeds(rng_clo);

	/* Concatenate complete source: RNG kernels source + common source
	 * + GPU legacy source. */
	src = g_strconcat(clo_rng_get_source(rng_clo), PP_COMMON_SRC,
		PP_GPU_LEGACY_SRC, NULL);

	/* Build the compiler options. */
	compilerOptsBuilder = g_string_new("");
	for (int i = 1; i < argc; i++) {
		g_string_append(compilerOptsBuilder, argv[i]);
	}
	compilerOpts = compilerOptsBuilder->str;
	printf("\nCompiler options: \"%s\"\n", compilerOpts);

	/* Create and build program. */
	prg = ccl_program_new_from_source(ctx, src, &err);
	g_if_err_goto(err, error_handler);

	ccl_program_build(prg, compilerOpts, &err);
	g_if_err_goto(err, error_handler);

	/* 2. Get simulation parameters */
	pp_load_params(&params, PP_DEFAULT_PARAMS_FILE, &err);
	g_if_err_goto(err, error_handler);

	/* 3. Compute work sizes for different kernels and print them to
	 * screen. */
	computeWorkSizes(params);
	printFixedWorkSizes();

	/* 4. obtain kernels entry points. */
	getKernelsFromProgram(prg, &err);
	g_if_err_goto(err, error_handler);

	printf("-------- Simulation start --------\n");

	/* 5. Create and initialize host buffers */

	/* Statistics */
	size_t statsSizeInBytes = (params.iters + 1) * sizeof(PPStatistics);
	statsArrayHost = (PPStatistics *) malloc(statsSizeInBytes);
	statsArrayHost[0].sheep = params.init_sheep;
	statsArrayHost[0].wolves = params.init_wolves;
	statsArrayHost[0].grass = 0;

	/* Number of agents after each iteration - this will be mapped
	 * with device, so CPU can auto-adjust GPU worksize in each
	 * iteration */
	numAgentsHost = (cl_uint*) malloc(sizeof(cl_uint));
	(*numAgentsHost) = params.init_sheep + params.init_wolves;

	/* Current iteration */
	cl_uint iter = 0;

	/* Agent array */
	size_t agentsSizeInBytes = MAX_AGENTS * sizeof(PPGSAgent);
	agentArrayHost = (PPGSAgent *) malloc(agentsSizeInBytes);
	for(unsigned int i = 0; i < MAX_AGENTS; i++) {

		agentArrayHost[i].x = g_rand_int_range(rng, 0, params.grid_x);
		agentArrayHost[i].y = g_rand_int_range(rng, 0, params.grid_y);

		if (i < params.init_sheep) {
			agentArrayHost[i].energy = g_rand_int_range(
				rng, 1, params.sheep_gain_from_food * 2);
			agentArrayHost[i].type = 0; /* Sheep */
			agentArrayHost[i].alive = 1;
		} else if (i < params.init_sheep + params.init_wolves) {
			agentArrayHost[i].energy = g_rand_int_range(
				rng, 1, params.wolves_gain_from_food * 2);
			agentArrayHost[i].type = 1; /* Wolves */
			agentArrayHost[i].alive = 1;
		} else {
			agentArrayHost[i].energy = 0;
			agentArrayHost[i].type = 0;
			agentArrayHost[i].alive = 0;
		}
	}

	/* Grass matrix */
	size_t grassSizeInBytes =
		CELL_SPACE * params.grid_x * params.grid_y * sizeof(cl_uint);
	grassMatrixHost = (cl_uint*) malloc(grassSizeInBytes);

	for(unsigned int i = 0; i < params.grid_x; i++) {
		for (unsigned int j = 0; j < params.grid_y; j++) {

			unsigned int gridIndex = (i + j*params.grid_x) * CELL_SPACE;
			grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] =
				g_rand_boolean(rng)
					? 0
					: g_rand_int_range(rng, 1, params.grass_restart);
			if (grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] == 0)
				statsArrayHost[0].grass++;
		}
	}

	/* Agent parameters */
	PPAgentParams agent_params[2];
	agent_params[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	agent_params[SHEEP_ID].reproduce_threshold =
		params.sheep_reproduce_threshold;
	agent_params[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	agent_params[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	agent_params[WOLF_ID].reproduce_threshold =
		params.wolves_reproduce_threshold;
	agent_params[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;

	/* Sim parameters */
	PPGSSimParams sim_params;
	sim_params.size_x = params.grid_x;
	sim_params.size_y = params.grid_y;
	sim_params.size_xy = params.grid_x * params.grid_y;
	sim_params.max_agents = MAX_AGENTS;
	sim_params.grass_restart = params.grass_restart;
	sim_params.grid_cell_space = CELL_SPACE;

	/* RNG seeds */
	size_t rngSeedsSizeInBytes = MAX_AGENTS * sizeof(cl_ulong);
	rngSeedsHost = (cl_ulong*) malloc(rngSeedsSizeInBytes);
	for (int i = 0; i < MAX_AGENTS; i++) {
		rngSeedsHost[i] = g_rand_int(rng);
	}

	/* 6. Create OpenCL buffers */

	statsArrayDevice = ccl_buffer_new(ctx,
		CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, statsSizeInBytes,
		statsArrayHost, &err);
	g_if_err_goto(err, error_handler);

	agentArrayDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, agentsSizeInBytes,
		agentArrayHost, &err);
	g_if_err_goto(err, error_handler);

	grassMatrixDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes,
		grassMatrixHost, &err);
	g_if_err_goto(err, error_handler);

	iterDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint),
		&iter, &err);
	g_if_err_goto(err, error_handler);

	/* Stuff to get number of agents out in each iteration */
	numAgentsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_uint),
		numAgentsHost, &err);
	g_if_err_goto(err, error_handler);

	grassCountDevice = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		grasscount2_gws[0] * sizeof(cl_uint), NULL, &err);
	g_if_err_goto(err, error_handler);

	/* This size is the maximum you'll ever need for the given maximum
	 * number of agents */
	agentsCountDevice = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		(MAX_AGENTS / agentcount2_lws) * sizeof(cl_uint2), NULL, &err);
	g_if_err_goto(err, error_handler);

	/* Two types of agent, thus two packs of agent parameters */
	agentParamsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		2 * sizeof(PPAgentParams), agent_params, &err);
	g_if_err_goto(err, error_handler);

	rngSeedsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes,
		rngSeedsHost, &err);
	g_if_err_goto(err, error_handler);

	/* 7. Set fixed kernel arguments */

	/* Sort kernel */
	ccl_kernel_set_arg(sort_kernel, 0, agentArrayDevice);

	/* Agent movement kernel */
	ccl_kernel_set_args(agentmov_kernel, agentArrayDevice,
		rngSeedsDevice, ccl_arg_priv(sim_params, PPGSSimParams),
		iterDevice, NULL);

	/* Agent grid update kernel */
	ccl_kernel_set_args(agentupdate_kernel, agentArrayDevice,
		grassMatrixDevice, ccl_arg_priv(sim_params, PPGSSimParams),
		NULL);

	/* Grass kernel */
	ccl_kernel_set_args(grass_kernel, grassMatrixDevice,
		ccl_arg_priv(sim_params, PPGSSimParams), NULL);

	/* Agent actions kernel */
	ccl_kernel_set_args(agentaction_kernel, agentArrayDevice,
		grassMatrixDevice, ccl_arg_priv(sim_params, PPGSSimParams),
		agentParamsDevice, rngSeedsDevice, numAgentsDevice, NULL);

	/* Count agents */
	ccl_kernel_set_args(countagents1_kernel,
		agentArrayDevice, agentsCountDevice,
		ccl_arg_local(agentcount1_lws, cl_uint2), NULL);

	ccl_kernel_set_args(countagents2_kernel, agentsCountDevice,
		ccl_arg_local(agentcount2_lws, cl_uint2), ccl_arg_skip,
		numAgentsDevice, statsArrayDevice, iterDevice, NULL);

	/* Count grass */
	ccl_kernel_set_args(countgrass1_kernel, grassMatrixDevice,
		grassCountDevice, ccl_arg_local(grasscount1_lws, cl_uint),
		ccl_arg_priv(sim_params, PPGSSimParams), NULL);

	ccl_kernel_set_args(countgrass2_kernel, grassCountDevice,
		ccl_arg_local(grasscount2_gws[0], cl_uint), ccl_arg_skip,
		statsArrayDevice, iterDevice, NULL);

	/* Output debug info. */
	g_debug("sort: %d\t agc2: %d\t gc2: %d\n",
		(int) (params.iters * clo_sum(clo_tzc(clo_nlpo2(MAX_AGENTS)))),
		(int) (params.iters * (MAX_AGENTS/4) / agentcount2_lws),
		(int) (params.iters * numGrassCount2Loops));

	/* Start profiling. */
	ccl_prof_start(prof);

	/* Map readNumAgents */
	numAgentsHost = (cl_uint*) ccl_buffer_enqueue_map(numAgentsDevice,
		cq, CL_FALSE, CL_MAP_READ, 0, sizeof(cl_uint), NULL,
		&map_numagents_event, &err);
	g_if_err_goto(err, error_handler);
	ccl_event_set_name(map_numagents_event, "Map readNumAgents");

	/* 8. Run the show */

	/* Guarantee all memory transfers are performed */
	ccl_queue_finish(cq, &err);
	g_if_err_goto(err, error_handler);

	/* Perform simulation iterations. */
	for (iter = 1; iter <= params.iters; iter++) {

		g_debug("iter %d", iter);

		/* Determine agent kernels size for this iteration */

		/* Worst case array agent (dead or alive) occupation */
		cl_uint maxOccupiedSpace = (*numAgentsHost) * 2;

		agent_gws = LWS_GPU_PREF *
			ceil(((float) maxOccupiedSpace) / LWS_GPU_PREF);
		agentcount1_gws = LWS_GPU_MAX *
			ceil(((float) maxOccupiedSpace) / LWS_GPU_MAX);
		cl_uint effectiveNextAgentsToCount =
			agentcount1_gws / agentcount1_lws;

		/* Agent movement */
		agentaction_move_event = ccl_kernel_enqueue_ndrange(
			agentmov_kernel, cq, 1, NULL, &agent_gws, &agent_lws, NULL,
			&err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(agentaction_move_event, "K: Agent move");

		/* Grass growth and agent number reset */
		grass_event = ccl_kernel_enqueue_ndrange(grass_kernel, cq, 2,
			NULL, grass_gws, grass_lws, NULL, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(grass_event, "K: Grass");

		/* Sort agent array */
		agentsort_gws = clo_nlpo2(maxOccupiedSpace) / 2;
		agentsort_lws = LWS_GPU_PREF;
		while (agentsort_gws % agentsort_lws != 0)
			agentsort_lws = agentsort_lws / 2;
		cl_uint totalStages = (cl_uint) clo_tzc(agentsort_gws * 2);

		ccl_event_wait_list_add(&ewl, agentaction_move_event, NULL);
		barrier_event = ccl_enqueue_barrier(cq, &ewl, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(barrier_event, "Barrier Move-Sort");

		for (unsigned int currentStage = 1; currentStage <= totalStages;
			currentStage++) {

			cl_uint step = currentStage;

			for (int currentStep = step; currentStep > 0;
				currentStep--) {

				agentsort_event =
					ccl_kernel_set_args_and_enqueue_ndrange(sort_kernel,
						cq, 1, NULL, &agentsort_gws,
						&agentsort_lws, NULL, &err,
						/* Kernel arguments */
						ccl_arg_skip,
						ccl_arg_priv(currentStage, cl_uint),
						ccl_arg_priv(currentStep, cl_uint), NULL);

				g_if_err_goto(err, error_handler);
				ccl_event_set_name(agentsort_event, "K: sort");
			}

		}

		/* Update agent number in grid */
		agentupdate_event = ccl_kernel_enqueue_ndrange(
			agentupdate_kernel, cq, 1, NULL, &agent_gws, &agent_lws,
			NULL, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(agentupdate_event, "K: Agent update");

		/* Agent actions */
		ccl_event_wait_list_add(&ewl, agentupdate_event, NULL);
		agentaction_event = ccl_kernel_enqueue_ndrange(
			agentaction_kernel, cq, 1, NULL, &agent_gws, &agent_lws,
			&ewl, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(agentaction_event, "K: Agent actions");

		/* Gather statistics */

		/* Count agents, part 1 */
		ccl_event_wait_list_add(&ewl, agentaction_event, NULL);
		agentcount1_event = ccl_kernel_enqueue_ndrange(
			countagents1_kernel, cq, 1, NULL, &agentcount1_gws,
			&agentcount1_lws, &ewl, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(agentcount1_event, "K: agentcount1");

		/* Count grass, part 1 */
		ccl_event_wait_list_add(&ewl, agentaction_event, NULL);
		grasscount1_event  = ccl_kernel_enqueue_ndrange(
			countgrass1_kernel, cq, 1, NULL,  &grasscount1_gws,
			&grasscount1_lws, &ewl, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(grasscount1_event, "K: grasscount1");

		/* Count agents, part 2 */
		do {

			agentcount2_gws = LWS_GPU_MAX *
				ceil(((float) effectiveNextAgentsToCount) / LWS_GPU_MAX);

			ccl_kernel_set_arg(countagents2_kernel, 2,
				ccl_arg_priv(effectiveNextAgentsToCount, cl_uint));

			ccl_event_wait_list_add(&ewl, agentcount1_event, NULL);
			agentcount2_event = ccl_kernel_enqueue_ndrange(
				countagents2_kernel, cq, 1, NULL, &agentcount2_gws,
				&agentcount2_lws, &ewl, &err);
			g_if_err_goto(err, error_handler);
			ccl_event_set_name(agentcount2_event, "K: agentcount2");

			effectiveNextAgentsToCount =
				agentcount2_gws / agentcount2_lws;

			barrier_event = ccl_enqueue_barrier(cq, NULL, &err);
			g_if_err_goto(err, error_handler);
			ccl_event_set_name(barrier_event, "Barrier agentcount2");

		} while (effectiveNextAgentsToCount > 1);

		/* Get total number of agents */
		readNumAgents_event = ccl_buffer_enqueue_read(numAgentsDevice,
			cq, CL_FALSE, 0, sizeof(cl_uint), numAgentsHost, NULL, &err);
		g_if_err_goto(err, error_handler);
		ccl_event_set_name(readNumAgents_event, "Read number of agents");

		/* Count grass, part 2 */
		for (int i = 0; i < numGrassCount2Loops; i++) {

			ccl_kernel_set_arg(countgrass2_kernel, 2,
				ccl_arg_priv(effectiveNextGrassToCount[i], cl_uint));

			ccl_event_wait_list_add(&ewl, grasscount1_event, NULL);
			grasscount2_event = ccl_kernel_enqueue_ndrange(
				countgrass2_kernel, cq, 1, NULL, &grasscount2_gws[i],
				&grasscount2_lws, &ewl, &err);
			g_if_err_goto(err, error_handler);
			ccl_event_set_name(grasscount2_event, "K: grasscount2");

			barrier_event = ccl_enqueue_barrier(cq, NULL, &err);
			g_if_err_goto(err, error_handler);
			ccl_event_set_name(barrier_event, "Barrier grasscount2");

		}

		/* Confirm that number of agents have been read */
		ccl_event_wait_list_add(&ewl, readNumAgents_event, NULL);
		ccl_event_wait(&ewl, &err);
		g_if_err_goto(err, error_handler);

	}

	/* Unmap numAgentsHost */
	unmap_numagents_event = ccl_memobj_enqueue_unmap(
		(CCLMemObj*) numAgentsDevice, cq, numAgentsHost, NULL, &err);
	g_if_err_goto(err, error_handler);
	ccl_event_set_name(unmap_numagents_event, "Unmap numagents");

	/* Guarantee all kernels have really terminated... */
	ccl_queue_finish(cq, &err);
	g_if_err_goto(err, error_handler);

	/* Get statistics */
	readStats_event = ccl_buffer_enqueue_read(statsArrayDevice, cq,
		CL_TRUE, 0, statsSizeInBytes, statsArrayHost, NULL, &err);
	g_if_err_goto(err, error_handler);
	ccl_event_set_name(readStats_event, "Read stats");

	/* Stop profiling */
	ccl_prof_stop(prof);

	/* 9. Output results to file */
	FILE * fp1 = fopen("stats.txt", "w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep,
			statsArrayHost[i].wolves, statsArrayHost[i].grass);
	fclose(fp1);

#ifdef PP_PROFILE_OPT
	/* Analyze events */
	ccl_prof_add_queue(prof, "Queue 1", cq);
	ccl_prof_calc(prof, &err);
	g_if_err_goto(err, error_handler);

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

	/* 11. Free stuff! */

	/* Free profiler object. */
	if (prof) ccl_prof_destroy(prof);

	/* Free RNG object. */
	if (rng_clo) clo_rng_destroy(rng_clo);

	/* Free complete program source. */
	if (src) g_free(src);

	/* Free OpenCL memory object wrappers */
	if (statsArrayDevice) ccl_buffer_destroy(statsArrayDevice);
	if (agentArrayDevice) ccl_buffer_destroy(agentArrayDevice);
	if (grassMatrixDevice) ccl_buffer_destroy(grassMatrixDevice);
	if (iterDevice) ccl_buffer_destroy(iterDevice);
	if (numAgentsDevice) ccl_buffer_destroy(numAgentsDevice);
	if (grassCountDevice) ccl_buffer_destroy(grassCountDevice);
	if (agentsCountDevice) ccl_buffer_destroy(agentsCountDevice );
	if (agentParamsDevice) ccl_buffer_destroy(agentParamsDevice);
	if (rngSeedsDevice) ccl_buffer_destroy(rngSeedsDevice);

	/* Free remaining OpenCL wrappers. */
	if (prg) ccl_program_destroy(prg);
	if (cq) ccl_queue_destroy(cq);
	if (ctx) ccl_context_destroy(ctx);

	/* Free host resources */
	if (statsArrayHost) free(statsArrayHost);
	if (numAgentsHost) free(numAgentsHost);
	if (agentArrayHost) free(agentArrayHost);
	if (grassMatrixHost) free(grassMatrixHost);
	if (rngSeedsHost) free(rngSeedsHost);

	/* Free strings */
	if (compilerOpts) g_free(compilerOpts);
	if (compilerOptsBuilder) g_string_free(compilerOptsBuilder, FALSE);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

	/* Bye */
	return status;

}
