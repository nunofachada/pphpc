/** 
 * @file
 * @brief PredPrey OpenCL GPU implementation.
 */

#include "pp_gpu.h"

//~ /**
 //~ * If the constant bellow is set, then the simulation is executed in 
 //~ * debug mode, i.e., a check will be performed between each OpenCL
 //~ * operation, and overlapping operations don't take place. The value of
 //~ * this constant corresponds to an interval of iterations to print
 //~ * simulation info for the current iteration.
 //~ * */
//~ #define PPG_DEBUG 1000

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
//~ #define PPG_DUMP 0x11

/** Information about the requested sorting algorithm. */
static CloSortInfo sort_info = {NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL};

/** Information about the requested random number generation algorithm. */
static CloRngInfo rng_info = {NULL, NULL, 0};

/** Main command line arguments and respective default values. */
#ifdef CLPROFILER
static PPGArgs args = {NULL, NULL, NULL, NULL, -1, PP_DEFAULT_SEED, PPG_DEFAULT_AGENT_SIZE, PPG_DEFAULT_MAX_AGENTS};
#else
static PPGArgs args = {NULL, NULL, NULL, -1, PP_DEFAULT_SEED, PPG_DEFAULT_AGENT_SIZE, PPG_DEFAULT_MAX_AGENTS};
#endif

/** Algorithm selection arguments. */
static PPGArgsAlg args_alg = {NULL, NULL};

/** Local work sizes command-line arguments*/
static PPGArgsLWS args_lws = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/** Vector widths command line arguments. */
static PPGArgsVW args_vw = {0, 0, 0};

/** Main command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,        "Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",                           "FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,         "Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",                     "FILENAME"},
#ifdef CLPROFILER
	{"profinfo",        'i', 0, G_OPTION_ARG_FILENAME, &args.prof_info,     "File where to export profiling info (if omitted, prof. info will not be exported)",         "FILENAME"},
#endif
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts, "Extra OpenCL compiler options",                                                             "OPTS"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,       "Device index (if not given and more than one device is available, chose device from menu)", "INDEX"},
	{"rng-seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",                    "SEED"},
	{"agent-size",      'a', 0, G_OPTION_ARG_INT,      &args.agent_size,    "Agent size, 32 or 64 bits (default is " STR(PPG_DEFAULT_AGENT_SIZE) ")",                    "BITS"},
	{"max-agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,    "Maximum number of agents (default is " STR(PPG_DEFAULT_MAX_AGENTS) ")",                     "SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,        NULL,                                                                                        NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Algorithm selection options. */
static GOptionEntry entries_alg[] = {
	{"a-rng",  0, 0, G_OPTION_ARG_STRING, &args_alg.rng,  "Random number generator: " CLO_RNGS,       "ALGORITHM"},
	{"a-sort", 0, 0, G_OPTION_ARG_STRING, &args_alg.sort, "Sorting: " CLO_SORT_ALGS,                  "ALGORITHM"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Kernel local worksizes. */
static GOptionEntry entries_lws[] = {
	{"l-def",          0, 0, G_OPTION_ARG_INT, &args_lws.deflt,         "Default local worksize",       "LWS"},
	{"l-init-cell",    0, 0, G_OPTION_ARG_INT, &args_lws.init_cell,     "Init. cells kernel",           "LWS"},
	{"l-init-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.init_agent,    "Init. agents kernel",          "LWS"},
	{"l-grass",        0, 0, G_OPTION_ARG_INT, &args_lws.grass,         "Grass kernel",                 "LWS"},
	{"l-reduce-grass", 0, 0, G_OPTION_ARG_INT, &args_lws.reduce_grass,  "Reduce grass kernel",          "LWS"},
	{"l-reduce-agent", 0, 0, G_OPTION_ARG_INT, &args_lws.reduce_agent,  "Reduce agent kernel",          "LWS"},
	{"l-move-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.move_agent,    "Move agent kernel",            "LWS"},
	{"l-sort-agent",   0, 0, G_OPTION_ARG_INT, &args_lws.sort_agent,    "Sort agent kernel",            "LWS"},
	{"l-find-index",   0, 0, G_OPTION_ARG_INT, &args_lws.find_cell_idx, "Find cell agent index kernel", "LWS"},
	{"l-action-agent", 0, 0, G_OPTION_ARG_INT, &args_lws.action_agent,  "Agent actions kernel",         "LWS"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Kernel vector widths. */
static GOptionEntry entries_vw[] = {
	{"vw-grass",        0, 0, G_OPTION_ARG_INT, &args_vw.grass,        "Vector (of uints) width for grass kernel, default is 0 (auto-detect)",                                       "WIDTH"},
	{"vw-reduce-grass", 0, 0, G_OPTION_ARG_INT, &args_vw.reduce_grass, "Vector (of uints) width for grass reduce kernel, default is 0 (auto-detect)",                                "WIDTH"},
	{"vw-reduce-agent", 0, 0, G_OPTION_ARG_INT, &args_vw.reduce_agent, "Vector (of uint/ulongs, depends on --agent-size) width for agent reduce kernel, default is 0 (auto-detect)", "WIDTH"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Agent size in bytes. */
static size_t agent_size_bytes;

/* OpenCL kernel files */
static char* kernelFile = "pp_gpu.cl";

/**
 * @brief Main program.
 * 
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program 
 * terminates successfully, or another value of #pp_error_codes if an
 * error occurs.
 * */
int main(int argc, char **argv) {

	/* Auxiliary status variable. */
	int status;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Predator-Prey simulation data structures. */
	PPGGlobalWorkSizes gws;
	PPGLocalWorkSizes lws;
	PPGKernels krnls = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPGEvents evts = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPGDataSizes dataSizes;
	PPGBuffersHost buffersHost = {NULL, NULL};
	PPGBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPParameters params;
	gchar* compilerOpts = NULL;
	gchar* kernelPath = NULL;
	
	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Profiler. */
	ProfCLProfile* profile = NULL;
	
	/* Random number generator */
	GRand* rng = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* Parse and validate arguments. */
	status = ppg_args_parse(argc, argv, &context, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Create RNG with specified seed. */
	rng = g_rand_new_with_seed(args.rng_seed);

	/* Profiling / Timmings. */
	profile = profcl_profile_new();
	gef_if_error_create_goto(err, PP_ERROR, profile == NULL, status = PP_LIBRARY_ERROR, error_handler, "Unable to create profiler object.");
	
	/* Get simulation parameters */
	status = pp_load_params(&params, args.params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);	
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_GPU, 2, QUEUE_PROPERTIES, clu_menu_device_selector, (args.dev_idx != -1 ? &args.dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Compute work sizes for different kernels. */
	status = ppg_worksizes_compute(params, zone->device_info.device_id, &gws, &lws, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Get full kernel path. */
	kernelPath = clo_kernelpath_get(kernelFile, argv[0]);
	
	/* Compiler options. */
	compilerOpts = ppg_compiler_opts_build(gws, lws, params, kernelPath, args.compiler_opts);

	/* Build program. */
	status = clu_program_create(zone, &kernelPath, 1, compilerOpts, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);

	/* Create kernels. */
	status = ppg_kernels_create(zone->program, &krnls, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Determine size in bytes for host and device data structures. */
	ppg_datasizes_get(params, &dataSizes, gws, lws);

	/* Initialize host buffers. */
	ppg_hostbuffers_create(&buffersHost, dataSizes, rng);

	/* Create device buffers */
	status = ppg_devicebuffers_create(zone->context, &buffersDevice, dataSizes, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Create events data structure. */
	ppg_events_create(params, &evts, &err);

	/*  Set fixed kernel arguments. */
	status = ppg_kernelargs_set(krnls, buffersDevice, dataSizes, lws, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Print information about simulation. */
	ppg_info_print(gws, lws, dataSizes, compilerOpts);

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Simulation!! */
	status = ppg_simulate(params, zone, gws, lws, krnls, &evts, dataSizes, buffersHost, buffersDevice, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  
	
	/* Output results to file */
	ppg_results_save(args.stats, buffersHost.stats, params);

	/* Analyze events, show profiling info. */
	status = ppg_profiling_analyze(profile, &evts, params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* If we get here, no need for error checking, jump to cleanup. */
	status = PP_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;
	
error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	fprintf(stderr, "Error: %s\n", err->message);
#ifdef PPG_DEBUG
	fprintf(stderr, "Error code (domain): %d (%s)\n", err->code, g_quark_to_string(err->domain));
	fprintf(stderr, "Exit status: %d\n", status);
#endif
	g_error_free(err);	

cleanup:

	/* Free args context and associated cli args parsing buffers. */
	ppg_args_free(context);

	/* Free events */
	ppg_events_free(params, &evts); 
	
	/* Release OpenCL kernels */
	ppg_kernels_free(&krnls);
	
	/* Release OpenCL memory objects */
	ppg_devicebuffers_free(&buffersDevice);

	/* Release OpenCL zone (program, command queue, context) */
	if (zone != NULL) clu_zone_free(zone);

	/* Free host resources */
	ppg_hostbuffers_free(&buffersHost);
	
	/* Free profile data structure */
	if (profile) profcl_profile_free(profile);
	
	/* Free compiler options. */
	if (compilerOpts) g_free(compilerOpts);
	
	/* Free kernel path. */
	if (kernelPath) g_free(kernelPath);
	
	/* Free RNG */
	if (rng) g_rand_free(rng);
	
	/* Bye bye. */
	return status;
	
}

/** 
 * @brief Perform Predator-Prey simulation.
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
int ppg_simulate(PPParameters params, CLUZone* zone, 
	PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, 
	PPGKernels krnls, PPGEvents* evts, 
	PPGDataSizes dataSizes, 
	PPGBuffersHost buffersHost, PPGBuffersDevice buffersDevice,
	GError** err) {

	/* Aux. var. */
	int status, ocl_status;
	
	/* Stats. */
	PPStatistics *stats_pinned;
	
	/* The maximum agents there can be in the next iteration. */
	cl_uint max_agents_iter = MAX(params.init_sheep + params.init_wolves, PPG_MIN_AGENTS);
	
	/* Dynamic worksizes. */
	size_t gws_reduce_agent1, 
		ws_reduce_agent2, 
		wg_reduce_agent1, 
		gws_move_agent,
		gws_find_cell_idx,
		gws_action_agent;
	
	/* Current iteration. */
	cl_uint iter = 0;
	
	/* Parse sort algorithm specific options, if any. */
	gchar* sort_options = g_strrstr(args_alg.sort, ".");
	if (sort_options != NULL) sort_options = sort_options + 1; /* Remove prefix dot. */
	
	/* Map stats to host. */
	g_debug("Mapping stats to host...");
	stats_pinned = (PPStatistics*) clEnqueueMapBuffer(
		zone->queues[1],
		buffersDevice.stats,
		CL_FALSE,
		CL_MAP_READ,
		0,
		sizeof(PPStatistics),
		0,
		NULL,
#ifdef CLPROFILER
		&evts->map_stats,
#else
		NULL,
#endif
		&ocl_status
	);	
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Map pinned stats: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
#ifdef PPG_DEBUG
	ocl_status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: Map pinned stats: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
#endif	

		
	/* Load RNG seeds into device buffers. */
	g_debug("Loading RNG seeds into device buffers...");
	ocl_status = clEnqueueWriteBuffer(
		zone->queues[0], 
		buffersDevice.rng_seeds, 
		CL_FALSE, 
		0, 
		dataSizes.rng_seeds, 
		buffersHost.rng_seeds, 
		0, 
		NULL, 
		&(evts->write_rng)
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Write device buffer: rng_seeds: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
#ifdef PPG_DEBUG
	ocl_status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after rng write: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
#endif
	
	/* Init. cells */
	g_debug("Initializing cells...");
	ocl_status =clEnqueueNDRangeKernel(
		zone->queues[0], 
		krnls.init_cell, 
		1, 
		NULL, 
		&(gws.init_cell),
		&(lws.init_cell), 
		0, 
		NULL, 
#ifdef CLPROFILER
		&(evts->init_cell)
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: Init. cells: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
	ocl_status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after init cells: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
#endif

	/* Init. agents */
	g_debug("Initializing agents...");
	ocl_status = clEnqueueNDRangeKernel(
		zone->queues[1], 
		krnls.init_agent, 
		1, 
		NULL, 
		&(gws.init_agent), 
		&(lws.init_agent), 
		0, 
		NULL, 
#ifdef CLPROFILER
		&(evts->init_agent)
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: Init. agents: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
	ocl_status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after init agents: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
#endif

#ifdef PPG_DUMP

	FILE *fp_agent_dump = fopen("dump_agents.txt", "w");
	FILE* fp_cell_dump = fopen("dump_cells.txt", "w");
	cl_ulong *agents_data = (cl_ulong*) malloc(dataSizes.agents_data);
	cl_uint2 *cells_agents_index = (cl_uint2*) malloc(dataSizes.cells_agents_index);
	cl_uint *cells_grass = (cl_uint*) malloc(dataSizes.cells_grass);
	
	ppg_dump(-1, PPG_DUMP, zone, fp_agent_dump, fp_cell_dump, 0, 0, 0, 0, params, dataSizes, buffersDevice, agents_data, cells_agents_index, cells_grass, err);
	
#endif

	
	/* *************** */
	/* SIMULATION LOOP */
	/* *************** */
	for (iter = 0; ; iter++) {
		
		/* ***************************************** */
		/* ********* Step 4: Gather stats ********** */
		/* ***************************************** */

		/* Step 4.1: Perform grass reduction, part I. */
		g_debug("Iteration %d: Performing grass reduction, part I...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			(zone->queues)[0], 
			krnls.reduce_grass1, 
			1, 
			NULL, 
			&(gws.reduce_grass1), 
			&(lws.reduce_grass1), 
			iter > 0 ? 1 : 0,
			iter > 0 ? &((evts->action_agent)[iter - 1]) : NULL,
#ifdef CLPROFILER
			&((evts->reduce_grass1)[iter])
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_grass1, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after reduce_grass1, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
		
	
		/* Step 4.2: Perform agents reduction, part I. */

		/* Determine worksizes. */
		/* See comment for grass reduction gws for an explanation the
		 * formulas bellow. */
		gws_reduce_agent1 = MIN( 
			lws.reduce_agent1 * lws.reduce_agent1, /* lws * number_of_workgroups */
			PP_GWS_MULT(
				PP_DIV_CEIL(max_agents_iter, args_vw.reduce_agent),
				lws.reduce_agent1
			)
		);

		/* The local/global worksize of reduce_agent2 must be a power of 
		 * 2 for the same reason of reduce_grass2. */
		wg_reduce_agent1 = gws_reduce_agent1 / lws.reduce_agent1;
		ws_reduce_agent2 = nlpo2(wg_reduce_agent1);
		
		/* Set variable kernel arguments in agent reduction kernels. */
		ocl_status = clSetKernelArg(krnls.reduce_agent1, 3, sizeof(cl_uint), (void *) &max_agents_iter);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of reduce_agent1, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		ocl_status = clSetKernelArg(krnls.reduce_agent2, 3, sizeof(cl_uint), (void *) &wg_reduce_agent1);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of reduce_agent2, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

		/* Run agent reduction kernel 1. */
		g_debug("Iteration %d: Performing agent reduction, part I...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.reduce_agent1, 
			1, 
			NULL, 
			&gws_reduce_agent1, 
			&lws.reduce_agent1, 
			0, 
			NULL,
#ifdef CLPROFILER
			&(evts->reduce_agent1[iter])
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_agent1, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after reduce_agent1, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
		
		/* Step 4.3: Perform grass reduction, part II. */
		g_debug("Iteration %d: Performing grass reduction, part II...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			(zone->queues)[0], 
			krnls.reduce_grass2, 
			1, 
			NULL, 
			&gws.reduce_grass2, 
			&lws.reduce_grass2, 
			iter > 0 ? 1 : 0,  
			iter > 0 ? &(evts->read_stats[iter - 1]) : NULL,
			&(evts->reduce_grass2[iter])
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_grass2, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		
#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after reduce_grass2, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif

		/* Step 4.4: Perform agents reduction, part II. */
		g_debug("Iteration %d: Performing agent reduction, part II...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.reduce_agent2, 
			1, 
			NULL, 
			&ws_reduce_agent2, 
			&ws_reduce_agent2, 
			iter > 0 ? 1 : 0,  
			iter > 0 ? &(evts->read_stats[iter - 1]) : NULL,
#ifdef CLPROFILER		
			&(evts->reduce_agent2[iter])
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_agent2, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		
#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after reduce_agent2, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif

		/* Step 4.5: Get statistics. */
		g_debug("Iteration %d: Getting statistics...", iter);
		ocl_status = clEnqueueReadBuffer(
			zone->queues[1], 
			buffersDevice.stats, 
			CL_FALSE, 
			0, 
			sizeof(PPStatistics), 
			stats_pinned, 
			1, 
			&(evts->reduce_grass2[iter]),  /* Only need to wait for reduce_grass2 because reduce_agent2 is in same queue. */
			&(evts->read_stats[iter])
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Read back stats, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

		
#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after read stats, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
		
		/* Stop simulation if this is the last iteration. */
		if (iter == params.iters) break;
		
		/* ***************************************** */
		/* ********* Step 1: Grass growth ********** */
		/* ***************************************** */
		
		/* Grass kernel: grow grass, set number of prey to zero */
		g_debug("Iteration %d: Running grass kernel...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			krnls.grass, 1, 
			NULL, 
			&gws.grass, 
			&lws.grass, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&(evts->grass[iter])
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: grass, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after grass, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
		
		/* ***************************************** */
		/* ******** Step 2: Agent movement ********* */
		/* ***************************************** */

		/* Determine agent movement global worksize. */
		gws_move_agent = PP_GWS_MULT(
			max_agents_iter,
			lws.move_agent
		);
		
		g_debug("Iteration %d: Move agents...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.move_agent, 
			1, 
			NULL, 
			&gws_move_agent, 
			&lws.move_agent, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&(evts->move_agent[iter])
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Kernel exec.: move_agent, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after move_agent, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
		
		/* ***************************************** */
		/* ********* Step 3.1: Agent sort ********** */
		/* ***************************************** */

		g_debug("Iteration %d: Sorting agents...", iter);
		sort_info.sort(
			&zone->queues[1], 
			krnls.sort_agent, 
			lws.sort_agent,
			max_agents_iter,
			sort_options,
			evts->sort_agent, 
			PP_PROFILE,
			err
		);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
		
#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after sort_agent, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif

		/* Determine the maximum number of agents there can be in the
		 * next iteration. Make sure stats are already transfered back
		 * to host. */
		ocl_status = clWaitForEvents(1, &evts->read_stats[iter]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Wait for events on host thread, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		memcpy(&buffersHost.stats[iter], stats_pinned, sizeof(PPStatistics));
		max_agents_iter = MAX(PPG_MIN_AGENTS, buffersHost.stats[iter].wolves + buffersHost.stats[iter].sheep);
		gef_if_error_create_goto(*err, PP_ERROR, max_agents_iter > args.max_agents, PP_OUT_OF_RESOURCES, error_handler, "Agents required for next iteration above defined limit. Current iteration: %d. Required agents: %d. Agents limit: %d", iter, max_agents_iter, args.max_agents);

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after copying pinned stats to host, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after copying pinned stats to host, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif

		/* ***************************************** */
		/* **** Step 3.2: Find cell agent index **** */
		/* ***************************************** */

		/* Determine kernel global worksize. */
		gws_find_cell_idx = PP_GWS_MULT(
			max_agents_iter,
			lws.find_cell_idx
		);

		g_debug("Iteration %d: Find cell agent indexes...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[1],
			krnls.find_cell_idx,
			1,
			NULL,
			&gws_find_cell_idx,
			&lws.find_cell_idx,
			0,
			NULL,
#ifdef CLPROFILER
			&evts->find_cell_idx[iter]
#else
			NULL
#endif
		);
		
#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after find_cell_idx, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
#endif
	

		/* ***************************************** */
		/* ******* Step 3.3: Agent actions ********* */
		/* ***************************************** */

		/* Determine agent actions kernel global worksize. */
		gws_action_agent = PP_GWS_MULT(
			max_agents_iter,
			lws.action_agent
		);

		/* Check if there is enough memory for all possible new agents. */
		gef_if_error_create_goto(*err, PP_ERROR, gws_action_agent * 2 > args.max_agents, PP_OUT_OF_RESOURCES, error_handler, "Not enough memory for existing and possible new agents. Current iteration: %d. Total possible agents: %d. Agents limit: %d", iter, (int) gws_action_agent * 2, (int) args.max_agents);

		g_debug("Iteration %d: Performing agent actions...", iter);
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[1],
			krnls.action_agent,
			1,
			NULL,
			&gws_action_agent,
			&lws.action_agent,
			0,
			NULL,
			&evts->action_agent[iter]
		);

#ifdef PPG_DEBUG
		ocl_status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after action_agent, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
		if (iter % PPG_DEBUG == 0)
			fprintf(stderr, "Finishing iteration %d with max_agents_iter=%d (%d sheep, %d wolves)...\n", iter, max_agents_iter, buffersHost.stats[iter].sheep, buffersHost.stats[iter].wolves);

#endif

		/* Agent actions may, in the worst case, double the number of agents. */
		max_agents_iter = MAX(max_agents_iter, lws.action_agent) * 2;


#ifdef PPG_DUMP
		ppg_dump(iter, PPG_DUMP, zone, fp_agent_dump, fp_cell_dump, max_agents_iter, gws_reduce_agent1, gws_action_agent, gws_move_agent, params, dataSizes, buffersDevice, agents_data, cells_agents_index, cells_grass, err);
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
	ocl_status = clWaitForEvents(1, &evts->read_stats[iter]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Wait for events on host thread, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
	memcpy(&buffersHost.stats[iter], stats_pinned, sizeof(PPStatistics));
	
	/* Unmap stats. */
	ocl_status = clEnqueueUnmapMemObject(
		zone->queues[1],
		buffersDevice.stats,
		(void*) stats_pinned,
		0,
		NULL,
#ifdef CLPROFILER
		&evts->unmap_stats
#else
		NULL
#endif	
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Unmap pinned stats: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
#ifdef PPG_DEBUG
	ocl_status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: Unmap pinned stats: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
#endif	

	/* Guarantee all activity has terminated... */
	ocl_status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Finish for queue 0 after simulation: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	ocl_status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Finish for queue 1 after simulation: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
	
}

/** 
 * @brief Dump simulation data for current iteration. 
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
 * @param cells_agents_index Host buffer where to put cell data (agents index).
 * @param cells_grass Host buffer where to put cell data (grass).
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * */
int ppg_dump(int iter, int dump_type, CLUZone* zone, 
	FILE* fp_agent_dump, FILE* fp_cell_dump, cl_uint max_agents_iter,
	size_t gws_reduce_agent1, size_t gws_action_agent, size_t gws_move_agent, 
	PPParameters params, PPGDataSizes dataSizes, PPGBuffersDevice buffersDevice, 
	void *agents_data, cl_uint2 *cells_agents_index, cl_uint *cells_grass, 
	GError** err) {

	/* Aux. status vars. */
	int status, ocl_status, blank_line;
	
	/* Read data from device. */
	ocl_status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_data, CL_TRUE, 0, dataSizes.agents_data, agents_data, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Agents dump, read data, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
	ocl_status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.cells_agents_index, CL_TRUE, 0, dataSizes.cells_agents_index, cells_agents_index, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Cells dump, read agents index, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));
	ocl_status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.cells_grass, CL_TRUE, 0, dataSizes.cells_grass, cells_grass, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Cells dump, read grass, iteration %d: OpenCL error %d (%s).", iter, ocl_status, clerror_get(ocl_status));

	/* Export agent info. */
	fprintf(fp_agent_dump, "\nIteration %d, max_agents_iter=%d, gws_reduce_agent1=%zu, gws_action_ag=%zu, gws_mov_ag=%zu\n", iter, max_agents_iter, gws_reduce_agent1, gws_action_agent, gws_move_agent);
	blank_line = FALSE;
	if (agent_size_bytes == 8) {
		for (unsigned int k = 0; k < args.max_agents; k++) {
			cl_ulong curr_ag = ((cl_ulong*) agents_data)[k];
			if (!(dump_type & 0x01) || ((curr_ag & 0xFFFFFFFF00000000)!= 0xFFFFFFFF00000000)) {
				if (blank_line) fprintf(fp_agent_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_agent_dump, "[%4d] %016lx: (%4d, %4d) type=%d energy=%d\n", 
					k, 
					curr_ag, 
					(cl_uint) (curr_ag >> 48),
					(cl_uint) ((curr_ag >> 32) & 0xFFFF), 
					(cl_uint) ((curr_ag >> 16) & 0xFFFF), 
					(cl_uint) (curr_ag & 0xFFFF)
					);
			} else {
				blank_line = TRUE;
			}
		}
	} else if (agent_size_bytes == 4) {
		for (unsigned int k = 0; k < args.max_agents; k++) {
			cl_uint curr_ag = ((cl_uint*) agents_data)[k];
			if (!(dump_type & 0x01) || ((curr_ag & 0xFFFF0000)!= 0xFFFF0000)) {
				if (blank_line) fprintf(fp_agent_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_agent_dump, "[%4d] %08x: (%3d, %3d) type=%d energy=%d\n", 
					k, 
					curr_ag, 
					(cl_uint) (curr_ag >> 22),
					(cl_uint) ((curr_ag >> 12) & 0x3FF), 
					(cl_uint) ((curr_ag >> 11) & 0x1), 
					(cl_uint) (curr_ag & 0x7FF)
					);
			} else {
				blank_line = TRUE;
			}
		}
	}

	
	/* Export cell info. */
	fprintf(fp_cell_dump, "\nIteration %d\n", iter);
	blank_line = FALSE;
	for (unsigned int k = 0; k < params.grid_xy; k++) {
		if (!(dump_type & 0x10) || ((iter != -1) & (cells_agents_index[k].s[0] != args.max_agents))) {
			if (blank_line) fprintf(fp_cell_dump, "\n");
			blank_line = FALSE;
			if (iter != -1) {
				fprintf(fp_cell_dump, "(%d, %d) -> (%d, %d) %s [Grass: %d]\n", 
					k % params.grid_x, 
					k / params.grid_y, 
					cells_agents_index[k].s[0], 
					cells_agents_index[k].s[1],
					cells_agents_index[k].s[0] != cells_agents_index[k].s[1] ? "More than 1 agent present" : "",
					cells_grass[k]);
			} else {
				fprintf(fp_cell_dump, "(%d, %d) -> (-, -) [Grass: %d]\n", 
					k % params.grid_x, 
					k / params.grid_y, 
					cells_grass[k]);
			}
		} else {
			blank_line = TRUE;
		}
	}

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;	

}

/**
 * @brief Perform profiling analysis. 
 * 
 * @param profile Profiling object.
 * @param evts OpenCL events to profile.
 * @param params Simulation parameters.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * */
int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err) {

	/* Aux. variable. */
	int status;
	
#ifdef CLPROFILER
	
	/* Perfomed detailed analysis onfy if profiling flag is set. */
	
	/* One time events. */
	profcl_profile_add(profile, "Write RNG seeds", evts->write_rng, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);	
	
	profcl_profile_add(profile, "Init cells", evts->init_cell, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add(profile, "Init agents", evts->init_agent, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add(profile, "Map stats", evts->map_stats, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	profcl_profile_add(profile, "Unmap stats", evts->unmap_stats, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
		
	/* Simulation loop events. */
	for (guint i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, "Grass", evts->grass[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Read stats", evts->read_stats[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Reduce grass 1", evts->reduce_grass1[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Reduce grass 2", evts->reduce_grass2[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Reduce agent 1", evts->reduce_agent1[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Reduce agent 2", evts->reduce_agent2[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Move agent", evts->move_agent[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Find cell idx", evts->find_cell_idx[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
		profcl_profile_add(profile, "Agent action", evts->action_agent[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}
	for (guint i = 0; i < (evts->sort_agent)->len; i++) {
		ProfCLEvName ev_name;
		ev_name = g_array_index(evts->sort_agent, ProfCLEvName, i);
		profcl_profile_add(profile, ev_name.eventName, ev_name.event, err);
	}
	
	/* Analyse event data. */
	profcl_profile_aggregate(profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	profcl_profile_overmat(profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Export profiling data. */
	if (args.prof_info) {
		profcl_export_info_file(profile, args.prof_info, err);
	}
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

#else
	/* Avoid compiler warnings. */
	evts = evts;
	params = params;
	
#endif

	/* Show profiling info. */
	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
}

/**
 * @brief Compute worksizes depending on the device type and number of 
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
int ppg_worksizes_compute(PPParameters paramsSim, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err) {
	
	/* Device preferred int and long vector widths. */
	cl_uint int_vw, long_vw;
	
	/* Aux vars. */
	int status, ocl_status;
	
	/* Get the maximum workgroup size. */
	ocl_status = clGetDeviceInfo(
		device, 
		CL_DEVICE_MAX_WORK_GROUP_SIZE,
		sizeof(size_t),
		&lws->max_lws,
		NULL
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_MAX_WORK_GROUP_SIZE).: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Determine the default workgroup size. */
	if (args_lws.deflt > 0) {
		if (args_lws.deflt > lws->max_lws) {
			fprintf(stderr, "The specified default workgroup size, %d, is higher than the device maximum, %d. Setting default workgroup size to %d.\n", (int) args_lws.deflt, (int) lws->max_lws, (int) lws->max_lws);
			lws->deflt = lws->max_lws;
		} else {
			lws->deflt = args_lws.deflt;
		}
	} else {
		lws->deflt = lws->max_lws;
	}

	/* Get the device preferred int vector width. */
	ocl_status = clGetDeviceInfo(
		device, 
		CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, 
		sizeof(cl_uint), 
		&int_vw, 
		NULL
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT).: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	

	/* Get the device preferred long vector width. */
	ocl_status = clGetDeviceInfo(
		device, 
		CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, 
		sizeof(cl_uint), 
		&long_vw, 
		NULL
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG).: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
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
	gws->init_cell = PP_GWS_MULT(paramsSim.grid_xy, lws->init_cell);
	
	/* Init agent worksizes. */
	lws->init_agent = args_lws.init_agent ? args_lws.init_agent : lws->deflt;
	gws->init_agent = PP_GWS_MULT(args.max_agents, lws->init_agent);

	/* Grass growth worksizes. */
	lws->grass = args_lws.grass ? args_lws.grass : lws->deflt;
	gws->grass = PP_GWS_MULT(paramsSim.grid_xy / args_vw.grass, lws->grass);
	
	/* Grass reduce worksizes, must be power of 2 for reduction to work. */
	lws->reduce_grass1 = args_lws.reduce_grass ?  args_lws.reduce_grass : lws->deflt;
	if (!is_po2(lws->reduce_grass1)) {
		lws->reduce_grass1 = MIN(lws->max_lws, nlpo2(lws->reduce_grass1));
		fprintf(stderr, "The workgroup size of the grass reduction kernel must be a power of 2. Assuming a workgroup size of %d.\n", (int) lws->reduce_grass1);
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
		PP_GWS_MULT(
			PP_DIV_CEIL(paramsSim.grid_xy, args_vw.reduce_grass),
			lws->reduce_grass1
		)
	);

	/* The nlpo2() bellow is required for performing the final reduction
	 * step successfully because the implemented reduction algorithm 
	 * assumes power of 2 local work sizes. */
	lws->reduce_grass2 = nlpo2(gws->reduce_grass1 / lws->reduce_grass1);
	gws->reduce_grass2 = lws->reduce_grass2;
	
	/* Agent reduce worksizes, must be power of 2 for reduction to work. */
	lws->reduce_agent1 = args_lws.reduce_agent ?  args_lws.reduce_agent : lws->deflt;
	if (!is_po2(lws->reduce_agent1)) {
		lws->reduce_agent1 = MIN(lws->max_lws, nlpo2(lws->reduce_agent1));
		fprintf(stderr, "The workgroup size of the agent reduction kernel must be a power of 2. Assuming a workgroup size of %d.\n", (int) lws->reduce_agent1);
	}
	
	/* The remaining agent reduction worksizes are determined on the fly 
	 * because the number of agents varies during the simulation. */
	 
	/* Agent movement local worksize. Global worksize depends on the 
	 * number of existing agents. */
	lws->move_agent = args_lws.move_agent ? args_lws.move_agent : lws->deflt;
	
	/* Agent sort local worksize. Global worksize depends on the 
	 * number of existing agents. */
	lws->sort_agent = args_lws.sort_agent ? args_lws.sort_agent : lws->deflt;
	
	/* Find cell agent index local worksize. Global worksize depends on the 
	 * number of existing agents. */
	lws->find_cell_idx = args_lws.find_cell_idx ? args_lws.find_cell_idx : lws->deflt;
	
	/* Agent actions local worksize. Global worksize depends on the 
	 * number of existing agents. */
	lws->action_agent = args_lws.action_agent ? args_lws.action_agent : lws->deflt;
	
	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
}

/**
 * @brief Print information about simulation.
 * 
 * @param gws Kernel global work sizes.
 * @param lws Kernel local work sizes.
 * @param dataSizes Size of data buffers.
 * @param compilerOpts Final OpenCL compiler options.
 * */
void ppg_info_print(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGDataSizes dataSizes, gchar* compilerOpts) {

	/* Determine total global memory. */
	size_t dev_mem = sizeof(PPStatistics) +
		dataSizes.cells_grass + 
		dataSizes.cells_agents_index +
		dataSizes.agents_data +
		dataSizes.reduce_grass_global + 
		dataSizes.reduce_agent_global +
		dataSizes.rng_seeds;
		
	/* Print info. @todo Make ints unsigned or change %d to something nice for size_t */
	printf("\n   =========================== Simulation Info =============================\n\n");
	printf("     Required global memory    : %d bytes (%d Kb = %d Mb)\n", (int) dev_mem, (int) dev_mem / 1024, (int) dev_mem / 1024 / 1024);
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
		gws.reduce_grass1, lws.reduce_grass1, dataSizes.reduce_grass_local1, args_vw.reduce_grass, sizeof(cl_uint));
	printf("       | reduce_grass2      | %8zu | %5zu | %10zu |     %2d x %zu |\n", 
		gws.reduce_grass2, lws.reduce_grass2, dataSizes.reduce_grass_local2, args_vw.reduce_grass, sizeof(cl_uint));
	printf("       | reduce_agent1      |     Var. | %5zu | %10zu |     %2d x %zu |\n", 
		lws.reduce_agent1, dataSizes.reduce_agent_local1, args_vw.reduce_agent, agent_size_bytes);
	printf("       | reduce_agent2      |     Var. |  Var. | %10zu |     %2d x %zu |\n", 
		dataSizes.reduce_agent_local2, args_vw.reduce_agent, agent_size_bytes);
	printf("       | move_agent         |     Var. | %5zu |          0 |          0 |\n", 
		lws.move_agent);

	for (unsigned int i = 0; i < sort_info.num_kernels; i++) {
		const char* kernel_name = sort_info.kernelname_get(i);
		printf("       | %-18.18s |     Var. | %5zu | %10zu |          0 |\n", 
			kernel_name, lws.sort_agent, 
			sort_info.localmem_usage(kernel_name, lws.sort_agent, agent_size_bytes, args.max_agents)); 
	}

	printf("       | find_cell_idx      |     Var. | %5zu |          0 |          0 |\n", 
		lws.find_cell_idx);
	printf("       | action_agent       |     Var. | %5zu |          0 |          0 |\n", 
		lws.action_agent);
	printf("       -------------------------------------------------------------------\n");

}

/** 
 * @brief Create OpenCL kernels. 
 * 
 * @param program OpenCL program.
 * @param krnls Data structure containing OpenCL kernels references.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise. 
 * */
int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err) {
	
	/* Status variable. */
	int status, ocl_status;
	
	/* Create init cells kernel. */
	krnls->init_cell = clCreateKernel(program, "initCell", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: initCell: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	

	/* Create init agents kernel. */
	krnls->init_agent = clCreateKernel(program, "initAgent", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: initAgent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	

	/* Create grass kernel. */
	krnls->grass = clCreateKernel(program, "grass", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: grass: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	
	
	/* Create reduce grass 1 kernel. */
	krnls->reduce_grass1 = clCreateKernel(program, "reduceGrass1", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	
	
	/* Create reduce grass 2 kernel. */
	krnls->reduce_grass2 = clCreateKernel(program, "reduceGrass2", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	
	
	/* Create reduce agent 1 kernel. */
	krnls->reduce_agent1 = clCreateKernel(program, "reduceAgent1", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceAgent1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	
	
	/* Create reduce agent 2 kernel. */
	krnls->reduce_agent2 = clCreateKernel(program, "reduceAgent2", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceAgent2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));	
	
	/* Create move agents kernel. */
	krnls->move_agent = clCreateKernel(program, "moveAgent", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: moveAgent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Create sort agents kernel. */
	status = sort_info.kernels_create(&krnls->sort_agent, program, err);
	gef_if_error_goto(*err, GEF_USE_GERROR, status, error_handler);
	
	/* Find cell agent index kernel. */
	krnls->find_cell_idx = clCreateKernel(program, "findCellIdx", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: findCellIdx: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Agent actions kernel. */
	krnls->action_agent = clCreateKernel(program, "actionAgent", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: actionAgent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
}

/** 
 * @brief Free OpenCL kernels. 
 * 
 * @param krnls Data structure containing OpenCL kernels references.
 * */
void ppg_kernels_free(PPGKernels* krnls) {
	if (krnls->init_cell) clReleaseKernel(krnls->init_cell);
	if (krnls->init_agent) clReleaseKernel(krnls->init_agent);
	if (krnls->grass) clReleaseKernel(krnls->grass);
	if (krnls->reduce_grass1) clReleaseKernel(krnls->reduce_grass1); 
	if (krnls->reduce_grass2) clReleaseKernel(krnls->reduce_grass2);
	if (krnls->reduce_agent1) clReleaseKernel(krnls->reduce_agent1);
	if (krnls->reduce_agent2) clReleaseKernel(krnls->reduce_agent2);
	if (krnls->move_agent) clReleaseKernel(krnls->move_agent);
	if (krnls->sort_agent) sort_info.kernels_free(&krnls->sort_agent);
	if (krnls->find_cell_idx) clReleaseKernel(krnls->find_cell_idx);
	if (krnls->action_agent) clReleaseKernel(krnls->action_agent);
}

/**
 * @brief Set fixed kernel arguments.
 * 
 * @param krnls OpenCL kernels.
 * @param buffersDevice Device data buffers.
 * @param dataSizes Size of data buffers.
 * @param lws Local work sizes.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise. 
 * 
 * */
int ppg_kernelargs_set(PPGKernels krnls, PPGBuffersDevice buffersDevice, PPGDataSizes dataSizes, PPGLocalWorkSizes lws, GError** err) {

	/* Aux. variable. */
	int status, ocl_status;
	
	/* Cell init kernel. */
	ocl_status = clSetKernelArg(krnls.init_cell, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_cell: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(krnls.init_cell, 1, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_cell: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Agent init kernel. */
	ocl_status = clSetKernelArg(krnls.init_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.init_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Grass kernel */
	ocl_status = clSetKernelArg(krnls.grass, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of grass: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.grass, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of grass: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* reduce_grass1 kernel */
	ocl_status = clSetKernelArg(krnls.reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_grass1, 1, dataSizes.reduce_grass_local1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_grass1, 2, sizeof(cl_mem), (void *) &buffersDevice.reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_grass1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* reduce_grass2 kernel */
	ocl_status = clSetKernelArg(krnls.reduce_grass2, 0, sizeof(cl_mem), (void *) &buffersDevice.reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_grass2, 1, dataSizes.reduce_grass_local2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_grass2, 2, sizeof(cl_mem), (void *) &buffersDevice.stats);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_grass2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
 	
	/* reduce_agent1 kernel */
	ocl_status = clSetKernelArg(krnls.reduce_agent1, 0, sizeof(cl_mem), (void *) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_agent1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(krnls.reduce_agent1, 1, dataSizes.reduce_agent_local1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_agent1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_agent1, 2, sizeof(cl_mem), (void *) &buffersDevice.reduce_agent_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_agent1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	/* The 3rd argument is set on the fly. */

	/* reduce_agent2 kernel */
	ocl_status = clSetKernelArg(krnls.reduce_agent2, 0, sizeof(cl_mem), (void *) &buffersDevice.reduce_agent_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_agent2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(krnls.reduce_agent2, 1, dataSizes.reduce_agent_local2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_agent2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.reduce_agent2, 2, sizeof(cl_mem), (void *) &buffersDevice.stats);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_agent2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Agent movement kernel. */
	ocl_status = clSetKernelArg(krnls.move_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of move_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.move_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of move_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Agent sorting kernel. */
	status = sort_info.kernelargs_set(&krnls.sort_agent, buffersDevice.agents_data, lws.sort_agent, agent_size_bytes, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Find cell agent index kernel. */
	ocl_status = clSetKernelArg(krnls.find_cell_idx, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of find_cell_idx: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.find_cell_idx, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of find_cell_idx: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Agent actions kernel. */
	ocl_status = clSetKernelArg(krnls.action_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of action_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(krnls.action_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of action_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.action_agent, 2, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of action_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.action_agent, 3, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of action_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(krnls.action_agent, 4, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 4 of action_agent: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);

finish:
	
	/* Return. */
	return status;
	
}

/** 
 * @brief Save simulation statistics. 
 * 
 * @param filename Name of file where to save statistics.
 * @param statsArray Statistics information array.
 * @param params Simulation parameters.
 * */
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params) {
	
	/* Get definite file name. */
	gchar* realFilename = (filename != NULL) ? filename : PP_DEFAULT_STATS_FILE;	

	FILE * fp1 = fopen(realFilename,"w");
	
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArray[i].sheep, statsArray[i].wolves, statsArray[i].grass);
	fclose(fp1);
}

/**
 * @brief Determine sizes of data buffers.
 * 
 * @param params Simulation parameters.
 * @param dataSizes Size of data buffers (to be modified by function).
 * @param gws Kernel global work sizes.
 * @param lws Kernel local work sizes.
 * */
void ppg_datasizes_get(PPParameters params, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);
	
	/* Environment cells */
	dataSizes->cells_grass = pp_next_multiple(params.grid_xy, args_vw.grass) * sizeof(cl_uint);
	dataSizes->cells_agents_index = params.grid_xy * sizeof(cl_uint2);
	
	/* Agents. */
	dataSizes->agents_data = args.max_agents * agent_size_bytes;
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local1 = lws.reduce_grass1 * args_vw.reduce_grass * sizeof(cl_uint);
	dataSizes->reduce_grass_global = gws.reduce_grass2 * args_vw.reduce_grass * sizeof(cl_uint);
	dataSizes->reduce_grass_local2 = lws.reduce_grass2 * args_vw.reduce_grass * sizeof(cl_uint);
	
	/* Agent reduction. */
	dataSizes->reduce_agent_local1 = 2 * lws.reduce_agent1 * args_vw.reduce_agent * agent_size_bytes; /* 2x to count sheep and wolves. */
	dataSizes->reduce_agent_global = dataSizes->reduce_agent_local1;
	dataSizes->reduce_agent_local2 = dataSizes->reduce_agent_local1;

	/* Rng seeds */
	dataSizes->rng_seeds = MAX(params.grid_xy, PPG_DEFAULT_MAX_AGENTS) * rng_info.bytes;
	dataSizes->rng_seeds_count = dataSizes->rng_seeds / sizeof(cl_ulong); /// @todo should this not be ulong or uint depending on what rng algorithm we're using?

}

/** 
 * @brief Initialize host data buffers. 
 * 
 * @param buffersHost Data structure containing references to the data
 * buffers.
 * @param dataSizes Sizes of simulation data arrays.
 * @param rng Random number generator.
 * */
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes dataSizes, GRand* rng) {
	
	/* Statistics */
	buffersHost->stats = (PPStatistics*) malloc(dataSizes.stats);

	/* RNG seeds */
	buffersHost->rng_seeds = (cl_ulong*) malloc(dataSizes.rng_seeds);
	for (unsigned int i = 0; i < dataSizes.rng_seeds_count; i++) {
		buffersHost->rng_seeds[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}	
	
}

/**
 * @brief Free host buffers.
 * 
 * @param buffersHost Data structure containing references to the data
 * buffers.
 * */
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost) {
	free(buffersHost->stats);
	free(buffersHost->rng_seeds);
}

/**
 * @brief Initialize device buffers.
 * 
 * @param context OpenCL context.
 * @param buffersDevice Data structure containing device data buffers.
 * @param dataSizes Size of data buffers.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * */
int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes dataSizes, GError** err) {
	
	/* Status flag. */
	int status, ocl_status;
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(PPStatistics), NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: stats: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Cells */
	buffersDevice->cells_grass = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_grass, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	buffersDevice->cells_agents_index = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_agents_index, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_index: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Agents. */
	buffersDevice->agents_data = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.agents_data, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: agents_data: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.reduce_grass_global, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_grass_global: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Agent reduction (count) */
	buffersDevice->reduce_agent_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.reduce_agent_global, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_agent_global: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* RNG seeds. */
	buffersDevice->rng_seeds = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.rng_seeds, NULL, &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create device buffer: rng_seeds: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;

}

/**
 * @brief Free device buffers.
 * @param buffersDevice Data structure containing device data buffers.
 * */
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice) {
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->cells_grass) clReleaseMemObject(buffersDevice->cells_grass);
	if (buffersDevice->cells_agents_index) clReleaseMemObject(buffersDevice->cells_agents_index);
	if (buffersDevice->agents_data) clReleaseMemObject(buffersDevice->agents_data);
	if (buffersDevice->reduce_agent_global) clReleaseMemObject(buffersDevice->reduce_agent_global);
	if (buffersDevice->reduce_grass_global) clReleaseMemObject(buffersDevice->reduce_grass_global);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
}

/** 
 * @brief Initialize data structure to hold OpenCL events.
 * 
 * @param params Simulation parameters.
 * @param evts Structure to hold OpenCL events (to be modified by
 * function).
 * @param lws Kernel local work sizes.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * 
 * */
int ppg_events_create(PPParameters params, PPGEvents* evts, GError **err) {
	
	/* Aux. status var. */
	int status;

#ifdef CLPROFILER

	/* Create events for grass kernel. */
	evts->grass = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->grass == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for grass kernel events.");	

	/* Create events for reduce_grass1 kernel. */
	evts->reduce_grass1 = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->reduce_grass1 == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for reduce_grass1 kernel events.");	

	/* Create events for reduce_agent1 kernel. */
	evts->reduce_agent1 = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->reduce_agent1 == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for reduce_agent1 kernel events.");	

	/* Create events for move_agent kernel. */
	evts->move_agent = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->move_agent == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for move_agent kernel events.");	

	/* Create events for reduce_agent2kernel. */
	evts->reduce_agent2 = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->reduce_agent2 == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for reduce_agent2 kernel events.");	

	/* Create events for sort kernels. */
	evts->sort_agent = g_array_sized_new(FALSE, FALSE, sizeof(ProfCLEvName), params.iters);

	/* Create events for find_cell_idx kernel. */
	evts->find_cell_idx = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->find_cell_idx == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for find_cell_idx kernel events.");	

#endif

	/* Create events for read stats. */
	evts->read_stats = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->read_stats == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for read_stats events.");	

	/* Create events for reduce_grass2 kernel. */
	evts->reduce_grass2 = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->reduce_grass2 == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for reduce_grass2 kernel events.");	

	/* Create events for action_agent kernel. */
	evts->action_agent = (cl_event*) calloc(params.iters + 1, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, evts->action_agent == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for action_agent kernel events.");	

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
}

/**
 * @brief Free data structure which holds OpenCL events.
 * 
 * @param params Simulation parameters.
 * @param evts Structure which holds OpenCL events.
 * */
void ppg_events_free(PPParameters params, PPGEvents* evts) {
	if (evts->write_rng) clReleaseEvent(evts->write_rng);
	if (evts->init_cell) clReleaseEvent(evts->init_cell);
	if (evts->init_agent) clReleaseEvent(evts->init_agent);
	if (evts->map_stats) clReleaseEvent(evts->map_stats);
	if (evts->unmap_stats) clReleaseEvent(evts->unmap_stats);
	if (evts->grass) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->grass[i]) clReleaseEvent(evts->grass[i]);
		}
		free(evts->grass);
	}
	if (evts->read_stats) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->read_stats[i]) clReleaseEvent(evts->read_stats[i]);
		}
		free(evts->read_stats);
	}
	if (evts->reduce_grass1) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->reduce_grass1[i]) clReleaseEvent(evts->reduce_grass1[i]);
		}
		free(evts->reduce_grass1);
	}
	if (evts->reduce_grass2) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->reduce_grass2[i]) clReleaseEvent(evts->reduce_grass2[i]);
		}
		free(evts->reduce_grass2);
	}
	if (evts->reduce_agent1) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->reduce_agent1[i]) clReleaseEvent(evts->reduce_agent1[i]);
		}
		free(evts->reduce_agent1);
	}
	if (evts->reduce_agent2) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->reduce_agent2[i]) clReleaseEvent(evts->reduce_agent2[i]);
		}
		free(evts->reduce_agent2);
	}
	if (evts->move_agent) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->move_agent[i]) clReleaseEvent(evts->move_agent[i]);
		}
		free(evts->move_agent);
	}
	if (evts->sort_agent) {
		for (guint i = 0; i < (evts->sort_agent)->len; i++) {
			ProfCLEvName ev_name;
			ev_name = g_array_index(evts->sort_agent, ProfCLEvName, i);
			clReleaseEvent(ev_name.event);
		}
		g_array_free(evts->sort_agent, TRUE);
	}
	if (evts->find_cell_idx) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->find_cell_idx[i]) clReleaseEvent(evts->find_cell_idx[i]);
		}
		free (evts->find_cell_idx);
	}
	if (evts->action_agent) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->action_agent[i]) clReleaseEvent(evts->action_agent[i]);
		}
		free (evts->action_agent);
	}
}

/**
 * @brief Build OpenCL compiler options string. 
 * 
 * @param gws Kernel global work sizes.
 * @param lws Kernel local work sizes.
 * @param params Simulation parameters.
 * @param cliOpts Compiler options specified through the command-line.
 * @return The final OpenCL compiler options string.
 */
gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPParameters params, gchar* kernelPath, gchar* cliOpts) {

	gchar* compilerOptsStr;
	gchar *path = g_path_get_dirname(kernelPath);

	GString* compilerOpts = g_string_new(PP_KERNEL_INCLUDES);
	g_string_append_printf(compilerOpts, "-I %s ", path);
	g_string_append_printf(compilerOpts, "-D VW_GRASS=%d ", args_vw.grass);
	g_string_append_printf(compilerOpts, "-D VW_GRASSREDUCE=%d ", args_vw.reduce_grass);
	g_string_append_printf(compilerOpts, "-D VW_AGENTREDUCE=%d ", args_vw.reduce_agent);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKGROUPS=%d ", (unsigned int) (gws.reduce_grass1 / lws.reduce_grass1));
	g_string_append_printf(compilerOpts, "-D MAX_LWS=%d ", (unsigned int) lws.max_lws);
	g_string_append_printf(compilerOpts, "-D MAX_AGENTS=%d ", args.max_agents);
	g_string_append_printf(compilerOpts, "-D CELL_NUM=%d ", params.grid_xy);
	g_string_append_printf(compilerOpts, "-D INIT_SHEEP=%d ", params.init_sheep);
	g_string_append_printf(compilerOpts, "-D SHEEP_GAIN_FROM_FOOD=%d ", params.sheep_gain_from_food);
	g_string_append_printf(compilerOpts, "-D SHEEP_REPRODUCE_THRESHOLD=%d ", params.sheep_reproduce_threshold);
	g_string_append_printf(compilerOpts, "-D SHEEP_REPRODUCE_PROB=%d ", params.sheep_reproduce_prob);
	g_string_append_printf(compilerOpts, "-D INIT_WOLVES=%d ", params.init_wolves);
	g_string_append_printf(compilerOpts, "-D WOLVES_GAIN_FROM_FOOD=%d ", params.wolves_gain_from_food);
	g_string_append_printf(compilerOpts, "-D WOLVES_REPRODUCE_THRESHOLD=%d ", params.wolves_reproduce_threshold);
	g_string_append_printf(compilerOpts, "-D WOLVES_REPRODUCE_PROB=%d ", params.wolves_reproduce_prob);
	g_string_append_printf(compilerOpts, "-D GRASS_RESTART=%d ", params.grass_restart);
	g_string_append_printf(compilerOpts, "-D GRID_X=%d ", params.grid_x);
	g_string_append_printf(compilerOpts, "-D GRID_Y=%d ", params.grid_y);
	g_string_append_printf(compilerOpts, "-D ITERS=%d ", params.iters);
	g_string_append_printf(compilerOpts, "-D %s ", args.agent_size == 64 ? "PPG_AG_64" : "PPG_AG_32");
	g_string_append_printf(compilerOpts, "-D %s ", rng_info.compiler_const);
	g_string_append_printf(compilerOpts, "-D %s ", sort_info.compiler_const);
	if (cliOpts) g_string_append_printf(compilerOpts, "%s", cliOpts);
	compilerOptsStr = compilerOpts->str;

	g_string_free(compilerOpts, FALSE);
	g_free(path);

	return compilerOptsStr;
}

/** 
 * @brief Parse command-line options. 
 * 
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 * @param context Context object for command line argument parsing.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * */
int ppg_args_parse(int argc, char* argv[], GOptionContext** context, GError** err) {
	
	/* Status variable. */
	int status;

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
	g_option_context_parse(*context, &argc, &argv, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* ** Validate arguments. ** */
	
	/* Validate random number generator. */
	if (!args_alg.rng) args_alg.rng = g_strdup(CLO_DEFAULT_RNG);
	CLO_ALG_GET(rng_info, rng_infos, args_alg.rng);
	gef_if_error_create_goto(*err, PP_ERROR, !rng_info.tag, status = PP_INVALID_ARGS, error_handler, "Unknown random number generator '%s'.", args_alg.rng);
	
	/* Validate sorting algorithm. */
	if (!args_alg.sort) args_alg.sort = g_strdup(CLO_DEFAULT_SORT);
	CLO_ALG_GET(sort_info, sort_infos, args_alg.sort);
	gef_if_error_create_goto(*err, PP_ERROR, !sort_info.tag, status = PP_INVALID_ARGS, error_handler, "Unknown sorting algorithm '%s'.", args_alg.sort);
	
	/* Validate agent size. */
	gef_if_error_create_goto(*err, PP_ERROR, (args.agent_size != 32) && (args.agent_size != 64), status = PP_INVALID_ARGS, error_handler, "The -a (--agent-size) parameter must be either 32 or 64.");
	agent_size_bytes = args.agent_size == 64 ? sizeof(cl_ulong) : sizeof(cl_uint);
	
	/* Validate vector sizes. */
	gef_if_error_create_goto(*err, PP_ERROR, (ones32(args_vw.grass) > 1) || (args_vw.grass > 16), status = PP_INVALID_ARGS, error_handler, "The -vw-grass parameter must be either 0 (auto-detect), 1, 2, 4, 8 or 16.");
	gef_if_error_create_goto(*err, PP_ERROR, (ones32(args_vw.reduce_grass) > 1) || (args_vw.reduce_grass > 16), status = PP_INVALID_ARGS, error_handler, "The -vw-reduce-grass parameter must be either 0 (auto-detect), 1, 2, 4, 8 or 16.");
	gef_if_error_create_goto(*err, PP_ERROR, (ones32(args_vw.reduce_agent) > 1) || (args_vw.reduce_agent > 16), status = PP_INVALID_ARGS, error_handler, "The -vw-reduce-agent parameter must be either 0 (auto-detect), 1, 2, 4, 8 or 16.");

	/* If we got here, everything is OK. */
	g_assert (err == NULL || *err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;
	
}

/**
 * @brief Free command line parsing related objects.
 * 
 * @param context Context object for command line argument parsing.
 * */
void ppg_args_free(GOptionContext* context) {
	if (context) {
		g_option_context_free(context);
	}
	if (args.params) g_free(args.params);
	if (args.stats) g_free(args.stats);
#ifdef CLPROFILER	
	if (args.prof_info) g_free(args.prof_info);
#endif
	if (args.compiler_opts) g_free(args.compiler_opts);
	if (args_alg.rng) g_free(args_alg.rng);
	if (args_alg.sort) g_free(args_alg.sort);
}
