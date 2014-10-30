/**
 * @file
 * @brief PredPrey OpenCL GPU w/ sorting implementation.
 */

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

/* Main stuff */
int main(int argc, char ** argv) {

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

	/* Events */
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
	ccl_if_err_goto(err, error_handler);

	/* Get chosen device. */
	dev = ccl_context_get_device(ctx, 0, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, PP_QUEUE_PROPERTIES, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create RNG object. */
	rng_clo = clo_rng_new("lcg", CLO_RNG_SEED_HOST_MT, NULL, MAX_AGENTS,
		0, NULL, ctx, cq, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get RNG seeds device buffer. */
	rngSeedsDevice = clo_rng_get_device_seeds(rng_clo);

	/* Get RNG kernels source. */
	src = g_strconcat(
		clo_rng_get_source(rng_clo), PP_GPU_LEGACY_SRC, NULL);

	/* Build the compiler options. */
	compilerOptsBuilder = g_string_new("");
	for (int i = 1; i < argc; i++) {
		g_string_append(compilerOptsBuilder, argv[i]);
	}
	compilerOpts = compilerOptsBuilder->str;
	printf("\n**** %s *****\n", compilerOpts);

	/* Create and build program. */
	prg = ccl_program_new_from_source(ctx, src, &err);
	ccl_if_err_goto(err, error_handler);

	ccl_program_build(prg, compilerOpts, &err);
	ccl_if_err_goto(err, error_handler);

	/* 2. Get simulation parameters */
	pp_load_params(&params, PP_DEFAULT_PARAMS_FILE, &err);
	ccl_if_err_goto(err, error_handler);

	/* 3. Compute work sizes for different kernels and print them to
	 * screen. */
	computeWorkSizes(params);
	printFixedWorkSizes();

	/* 4. obtain kernels entry points. */
	getKernelEntryPoints(zone->program, &err);
	ccl_if_err_goto(err, error_handler);

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
	ccl_if_err_goto(err, error_handler);

	agentArrayDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, agentsSizeInBytes,
		agentArrayHost, &err);
	ccl_if_err_goto(err, error_handler);

	grassMatrixDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes,
		grassMatrixHost, &err);
	ccl_if_err_goto(err, error_handler);

	iterDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint),
		&iter, &err);
	ccl_if_err_goto(err, error_handler);

	/* Stuff to get number of agents out in each iteration */
	numAgentsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_uint),
		numAgentsHost, &err);
	ccl_if_err_goto(err, error_handler);

	grassCountDevice = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		grasscount2_gws[0] * sizeof(cl_uint), NULL, &err);
	ccl_if_err_goto(err, error_handler);

	/* This size is the maximum you'll ever need for the given maximum
	 * number of agents */
	agentsCountDevice = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		(MAX_AGENTS / agentcount2_lws) * sizeof(cl_uint2), NULL, &err);
	ccl_if_err_goto(err, error_handler);

	/* Two types of agent, thus two packs of agent parameters */
	agentParamsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		2 * sizeof(PPAgentParams), agent_params, &err);
	ccl_if_err_goto(err, error_handler);

	rngSeedsDevice = ccl_buffer_new(ctx,
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes,
		rngSeedsHost, &err);
	ccl_if_err_goto(err, error_handler);

	/* 7. Set fixed kernel arguments */

	/* Sort kernel */
	ccl_kernel_set_arg(sort_kernel, 0, agentArrayDevice);

	/* Agent movement kernel */
	ccl_kernel_set_args(agentmov_kernel, agentArrayDevice,
		rngSeedsDevice, ccl_arg_priv(sim_params, PPGSSimParams),
		iterDevice, NULL);

	/* Agent grid update kernel */
	ccl_kernel_set_args(agentupdate_kernel, agentArrayDevice,
		grassMatrixDevice, ccl_arg_priv(sim_params, PPGSSimParams));

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
		params.iters * sum(tzc(nlpo2(MAX_AGENTS))),
		params.iters * (MAX_AGENTS/4) / agentcount2_lws,
		params.iters * numGrassCount2Loops);

	/* Map readNumAgents */
	numAgentsHost = (cl_uint*) ccl_buffer_enqueue_map (numAgentsDevice,
		cq, CL_FALSE, CL_MAP_READ, 0, sizeof(cl_uint), NULL,
		&map_numagents_event, &err);
	ccl_if_err_goto(err, error_handler);

	/* 8. Run the show */

	/* Guarantee all memory transfers are performed */
	ccl_queue_finish(cq, &err);
	ccl_if_err_goto(err, error_handler);

	/* Start profiling. */
	ccl_prof_start(prof);

	/* Perform simulation iterations. */
	for (iter = 1; iter <= params.iters; iter++) {

		g_debug("iter %d", iter);

		/* Determine agent kernels size for this iteration */

		/* Worst case array agent (dead or alive) occupation */
		cl_uint maxOccupiedSpace = (*numAgentsHost) * 2;

		agent_gws = LWS_GPU_PREF * ceil(((float) maxOccupiedSpace) / LWS_GPU_PREF);
		agentcount1_gws = LWS_GPU_MAX * ceil(((float) maxOccupiedSpace) / LWS_GPU_MAX);
		cl_uint effectiveNextAgentsToCount = agentcount1_gws / agentcount1_lws;
		cl_uint iterbase = iter - 1;

		/* Agent movement */
		agentaction_move_event = ccl_kernel_enqueue_ndrange(
			agentmov_kernel, cq, 1, NULL, &agent_gws, &agent_lws, NULL,
			&err);
		ccl_if_err_goto(err, error_handler);

		/* Grass growth and agent number reset */
		grass_event = ccl_kernel_enqueue_ndrange(grass_kernel, 2, NULL,
			grass_gws, grass_lws, NULL, &err);
		ccl_if_err_goto(err, error_handler);

		/* Sort agent array */
		agentsort_gws = nlpo2(maxOccupiedSpace) / 2;
		agentsort_lws = LWS_GPU_PREF;
		while (agentsort_gws % agentsort_lws != 0)
			agentsort_lws = agentsort_lws / 2;
		cl_uint totalStages = (cl_uint) tzc(agentsort_gws * 2);

		ccl_event_wait_list_add(&ewl, agentaction_move_event, NULL);
		ccl_enqueue_barrier(cq, &ewl, &err);
		ccl_if_err_goto(err, error_handler);

		for (unsigned int currentStage = 1; currentStage <= totalStages; currentStage++) {

			cl_uint step = currentStage;

			for (int currentStep = step; currentStep > 0; currentStep--) {

				agentsort_event =
					ccl_kernel_set_args_and_enqueue_ndrange(sort_kernel,
						cq, 1, NULL, &agentsort_gws,
						&agentsort_lws, NULL, &err,
						/* Kernel arguments */
						ccl_arg_skip,
						ccl_arg_priv(currentStage, cl_uint),
						ccl_arg_priv(currentStep, cl_uint), NULL);

				ccl_if_err_goto(err, error_handler);
			}

		}

		/* Update agent number in grid */
		agentupdate_event = ccl_kernel_enqueue_ndrange(
			agentupdate_kernel, cq, 1, NULL, &agent_gws, &agent_lws,
			NULL, &err);
		ccl_if_err_goto(err, error_handler);

		/* Agent actions */
		ccl_event_wait_list_add(&ewl, agentupdate_event, NULL);
		agentaction_event = ccl_kernel_enqueue_ndrange(
			agentaction_kernel, cq, 1, NULL, &agent_gws, &agent_lws,
			&ewl, &err);
		ccl_if_err_goto(err, error_handler);

		/* Gather statistics */

		/* Count agents, part 1 */
		ccl_event_wait_list_add(&ewl, agentaction_event, NULL);
		agentcount1_event = ccl_kernel_enqueue_ndrange(
			countagents1_kernel, cq, 1, NULL, &agentcount1_gws,
			&agentcount1_lws, &ewl, &err);
		ccl_if_err_goto(err, error_handler);

		/* Count grass, part 1 */
		ccl_event_wait_list_add(&ewl, agentaction_event, NULL);
		grasscount1_event  = ccl_kernel_enqueue_ndrange(
			countgrass1_kernel, cq, 1, NULL,  &grasscount1_gws,
			&grasscount1_lws, &ewl, &err);
		ccl_if_err_goto(err, error_handler);

		/* Count agents, part 2 */
		do {

			agentcount2_gws = LWS_GPU_MAX * ceil(((float) effectiveNextAgentsToCount) / LWS_GPU_MAX);

			ccl_kernel_set_arg(countagents2_kernel, 2,
				ccl_arg_priv(effectiveNextAgentsToCount, cl_uint));

			ccl_event_wait_list_add(&ewl, agentcount1_event, NULL);
			agentcount2_event = ccl_kernel_enqueue_ndrange(
				countagents2_kernel, cq, 1, NULL, &agentcount2_gws,
				&agentcount2_lws, 1, &ewl, &err);
			ccl_if_err_goto(err, error_handler);

			effectiveNextAgentsToCount = agentcount2_gws / agentcount2_lws;

			ccl_enqueue_barrier(cq, NULL, &err);
			ccl_if_err_goto(err, error_handler);

		} while (effectiveNextAgentsToCount > 1);

		/* Get total number of agents */
		readNumAgents_event = ccl_buffer_enqueue_read(numAgentsDevice,
			cq, CL_FALSE, 0, sizeof(cl_uint), numAgentsHost, NULL, &err);
		ccl_if_err_goto(err, error_handler);

		/* Count grass, part 2 */
		for (int i = 0; i < numGrassCount2Loops; i++) {

			ccl_kernel_set_arg(countgrass2_kernel, 2,
				ccl_arg_priv(effectiveNextGrassToCount[i], cl_uint));

			ccl_event_wait_list_add(&ewl, grasscount1_event, NULL);
			grasscount2_event = ccl_kernel_enqueue_ndrange(
				countgrass2_kernel, cq, 1, NULL, &grasscount2_gws[i],
				&grasscount2_lws, &ewl, &err);
			ccl_if_err_goto(err, error_handler);

			ccl_enqueue_barrier(cq, NULL, &err);
			ccl_if_err_goto(err, error_handler);

		}

		/* Confirm that number of agents have been read */
		ccl_event_wait_list_add(&ewl, readNumAgents_event, NULL);
		ccl_event_wait(&ewl, &err);
		ccl_if_err_goto(err, error_handler);

	}

	/* Unmap numAgentsHost */
	unmap_numagents_event = ccl_memobj_enqueue_unmap(
		(CCLMemObj*) numAgentsDevice, cq, numAgentsHost, NULL, &err);
	ccl_if_err_goto(err, error_handler);

	/* Guarantee all kernels have really terminated... */
	ccl_queue_finish(cq, &err);
	ccl_if_err_goto(err, error_handler);

	/* Stop profiling */
	ccl_prof_stop(prof);

	/* Get statistics */
	status = clEnqueueReadBuffer(zone->queues[0], statsArrayDevice, CL_TRUE, 0, statsSizeInBytes, statsArrayHost, 0, NULL, NULL);
	gef_if_error_create_goto(err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "statsArray read");

	// 9. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep, statsArrayHost[i].wolves, statsArrayHost[i].grass );
	fclose(fp1);

	/* Analyze events */
#ifdef CLPROFILER
	for (unsigned int i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, "agentaction_move", agentaction_move_event[i], NULL);
		profcl_profile_add(profile, "grass", grass_event[i], NULL);
		profcl_profile_add(profile, "agentupdate", agentupdate_event[i], NULL);
		profcl_profile_add(profile, "agentaction", agentaction_event[i], NULL);
		profcl_profile_add(profile, "agentcount1", agentcount1_event[i], NULL);
		profcl_profile_add(profile, "grasscount1", grasscount1_event[i], NULL);
		profcl_profile_add(profile, "readNumAgents", readNumAgents_event[i], NULL);
	}
	for (unsigned int i = 0; i < agentsort_event_index; i++) {
		profcl_profile_add(profile, "agentsort", agentsort_event[i], NULL);
	}
	for (unsigned int i = 0; i < grasscount2_event_index; i++) {
		profcl_profile_add(profile, "grasscount2", grasscount2_event[i], NULL);
	}
	for (unsigned int i = 0; i < agentcount2_event_index; i++) {
		profcl_profile_add(profile, "agentcount2", agentcount2_event[i], NULL);
	}
	profcl_profile_add(profile, "map_numAgents", map_numagents_event, NULL);
	profcl_profile_add(profile, "unmap_numAgents", unmap_numagents_event, NULL);
	profcl_profile_aggregate(profile, NULL);
	profcl_profile_overmat(profile, NULL);
#endif

	/* Print profiling info. */
	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME, NULL);

	/* If we get here, everything went Ok. */
	status = PP_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;

error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	g_assert(status != PP_SUCCESS);
	fprintf(stderr, "Error: %s\n", err->message);
	g_error_free(err);

cleanup:

	/* 11. Free stuff! */
	if (profile)
		profcl_profile_free(profile);


	/* Release events */
	if (agentaction_move_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentaction_move_event[i])
				clReleaseEvent(agentaction_move_event[i]);
		free(agentaction_move_event);
	}

	if (grass_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (grass_event[i])
				clReleaseEvent(grass_event[i]);
		free(grass_event);
	}
	if (agentupdate_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentupdate_event[i])
				clReleaseEvent(agentupdate_event[i]);
		free(agentupdate_event);
	}
	if (agentaction_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentaction_event[i])
				clReleaseEvent(agentaction_event[i]);
		free(agentaction_event);
	}
	if (agentcount1_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentcount1_event[i])
				clReleaseEvent(agentcount1_event[i]);
		free(agentcount1_event);
	}
	if (grasscount1_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (grasscount1_event[i])
				clReleaseEvent(grasscount1_event[i]);
		free(grasscount1_event);
	}
	if (readNumAgents_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (readNumAgents_event[i])
				clReleaseEvent(readNumAgents_event[i]);
		free(readNumAgents_event);
	}
	if (agentsort_event) {
		for (unsigned int i = 0; i < params.iters * sum(tzc(nlpo2(MAX_AGENTS))); i++) {
			clReleaseEvent(agentsort_event[i]);
		}
		free(agentsort_event);
	}
	if (grasscount2_event) {
		for (unsigned int i = 0; i < params.iters * numGrassCount2Loops; i++) {
			clReleaseEvent(grasscount2_event[i]);
		}
		free(grasscount2_event);
	}
	if (agentcount2_event) {
		for (unsigned int i = 0; i < (MAX_AGENTS/4) / agentcount2_lws * params.iters; i++) {
			clReleaseEvent(agentcount2_event[i]);
		}
		free(agentcount2_event);
	}


	// Free OpenCL kernels
	if (grass_kernel) clReleaseKernel(grass_kernel);
	if (agentmov_kernel) clReleaseKernel(agentmov_kernel);
	if (agentupdate_kernel) clReleaseKernel(agentupdate_kernel);
	if (sort_kernel) clReleaseKernel(sort_kernel);
	if (agentaction_kernel) clReleaseKernel(agentaction_kernel);
	if (countagents1_kernel) clReleaseKernel(countagents1_kernel);
	if (countagents2_kernel) clReleaseKernel(countagents2_kernel);
	if (countgrass1_kernel) clReleaseKernel(countgrass1_kernel);
	if (countgrass2_kernel) clReleaseKernel(countgrass2_kernel);

	// Free OpenCL memory objects
	if (statsArrayDevice) clReleaseMemObject(statsArrayDevice);
	if (agentArrayDevice) clReleaseMemObject(agentArrayDevice);
	if (grassMatrixDevice) clReleaseMemObject(grassMatrixDevice);
	if (iterDevice) clReleaseMemObject(iterDevice);
	if (numAgentsDevice) clReleaseMemObject(numAgentsDevice);
	if (grassCountDevice) clReleaseMemObject(grassCountDevice);
	if (agentsCountDevice) clReleaseMemObject(agentsCountDevice );
	if (agentParamsDevice) clReleaseMemObject(agentParamsDevice);
	if (rngSeedsDevice) clReleaseMemObject(rngSeedsDevice);

	// Free OpenCL zone
	clu_zone_free(zone);

	// Free host resources
	if (statsArrayHost) free(statsArrayHost);
	if (numAgentsHost) free(numAgentsHost);
	if (agentArrayHost) free(agentArrayHost);
	if (grassMatrixHost) free(grassMatrixHost);
	if (rngSeedsHost) free(rngSeedsHost);

	// Free strings
	if (compilerOpts) g_free(compilerOpts);
	if (kernelPath) g_free(kernelPath);
	if (path) g_free(path);
	if (compilerOptsBuilder) g_string_free(compilerOptsBuilder, FALSE);

	// Bye
	return status;

}


// Compute worksizes depending on the device type and number of available compute units
void computeWorkSizes(PPParameters params) {
	// grass growth worksizes
	grass_lws[0] = LWS_GPU_PREF_2D_X;
	grass_gws[0] = LWS_GPU_PREF_2D_X * ceil(((float) params.grid_x) / LWS_GPU_PREF_2D_X);
	grass_lws[1] = LWS_GPU_PREF_2D_Y;
	grass_gws[1] = LWS_GPU_PREF_2D_Y * ceil(((float) params.grid_y) / LWS_GPU_PREF_2D_Y);
	// fixed local agent kernel worksizes
	agent_lws = LWS_GPU_PREF;
	agentcount1_lws = LWS_GPU_MAX;
	agentcount2_lws = LWS_GPU_MAX;
	// grass count worksizes
	grasscount1_lws = LWS_GPU_MAX;
	grasscount1_gws = LWS_GPU_MAX * ceil((((float) params.grid_x * params.grid_y)) / LWS_GPU_MAX);
	grasscount2_lws = LWS_GPU_MAX;
	effectiveNextGrassToCount[0] = grasscount1_gws / grasscount1_lws;
	grasscount2_gws[0] = LWS_GPU_MAX * ceil(((float) effectiveNextGrassToCount[0]) / LWS_GPU_MAX);
	// Determine number of loops of secondary count required to perform complete reduction
	numGrassCount2Loops = 1;
	while ( grasscount2_gws[numGrassCount2Loops - 1] > grasscount2_lws) {
		effectiveNextGrassToCount[numGrassCount2Loops] = grasscount2_gws[numGrassCount2Loops - 1] / grasscount2_lws;
		grasscount2_gws[numGrassCount2Loops] = LWS_GPU_MAX * ceil(((float) effectiveNextGrassToCount[numGrassCount2Loops]) / LWS_GPU_MAX);
		numGrassCount2Loops++;
	}
}

// Print worksizes
void printFixedWorkSizes() {
	printf("Fixed kernel sizes:\n");
	printf("grass_gws=[%d,%d]\tgrass_lws=[%d,%d]\n", (int) grass_gws[0], (int) grass_gws[1], (int) grass_lws[0], (int) grass_lws[1]);
	printf("agent_lws=%d\n", (int) agent_lws);
	printf("agentcount1_lws=%d\n", (int) agentcount1_lws);
	printf("agentcount2_lws=%d\n", (int) agentcount2_lws);
	printf("grasscount1_gws=%d\tgrasscount1_lws=%d\n", (int) grasscount1_gws, (int) grasscount1_lws);
	printf("grasscount2_lws=%d\n", (int) grasscount2_lws);
	for (int i = 0; i < numGrassCount2Loops; i++) {
		printf("grasscount2_gws[%d]=%d (effective grass to count: %d)\n", i, (int) grasscount2_gws[i], effectiveNextGrassToCount[i]);
	}
	printf("Total of %d grass count loops.\n", numGrassCount2Loops);
}

// Get kernel entry points
cl_int getKernelEntryPoints(cl_program program, GError** err) {

	/* Aux. var. */
	cl_int status;

	/* Create kernels. */
	grass_kernel = clCreateKernel( program, "Grass", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating Grass kernel");

	agentmov_kernel = clCreateKernel( program, "RandomWalk", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating RandomWalk kernel");

	agentupdate_kernel = clCreateKernel( program, "AgentsUpdateGrid", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating AgentsUpdateGrid kernel");

	sort_kernel = clCreateKernel( program, "BitonicSort", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating BitonicSort kernel");

	agentaction_kernel = clCreateKernel( program, "AgentAction", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating AgentAction kernel");

	countagents1_kernel = clCreateKernel( program, "CountAgents1", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating CountAgents1 kernel");

	countagents2_kernel = clCreateKernel( program, "CountAgents2", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating CountAgents2 kernel");

	countgrass1_kernel = clCreateKernel( program, "CountGrass1", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating CountGrass1 kernel");

	countgrass2_kernel = clCreateKernel( program, "CountGrass2", &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating CountGrass2 kernel");

	/* If we got here, everything is OK. */
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	/* Set status to error code. */
	status = (*err)->code;

finish:

	/* Return. */
	return status;
}
