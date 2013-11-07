/** 
 * @file
 * @brief PredPrey OpenCL GPU implementation.
 */

#include "pp_gpu.h"
#include "pp_gpu_sort.h"

//#define PPG_DEBUG 1 // x - interval of iterations to print info

//#define PPG_DUMP 1 // 0 - dump all, 1 - dump smart (only alive)

/** Information about the requested sorting algorithm. */
static PPGSortInfo sort_info = {NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/** Information about the requested random number generation algorithm. */
static PPRngInfo rng_info = {NULL, NULL, 0};

/** Main command line arguments and respective default values. */
static PPGArgs args = {NULL, NULL, NULL, -1, PP_DEFAULT_SEED, PPG_DEFAULT_MAX_AGENTS};

/** Algorithm selection arguments. */
static PPGArgsAlg args_alg = {NULL, NULL};

/** Local work sizes command-line arguments*/
static PPGArgsLWS args_lws = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/** Vector widths command line arguments. */
static PPGArgsVW args_vw = {0, 0, 0, 0, 0};

/** Main command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,        "Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",                           "FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,         "Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",                     "FILENAME"},
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts, "Extra OpenCL compiler options",                                                             "OPTS"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,       "Device index (if not given and more than one device is available, chose device from menu)", "INDEX"},
	{"rng_seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",                    "SEED"},
	{"max_agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,    "Maximum number of agents (default is " STR(PPG_DEFAULT_MAX_AGENTS) ")",                     "SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,        NULL,                                                                                        NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Algorithm selection options. */
static GOptionEntry entries_alg[] = {
	{"a-rng",  0, 0, G_OPTION_ARG_STRING, &args_alg.rng,  "Random number generator: " PP_RNGS,       "ALGORITHM"},
	{"a-sort", 0, 0, G_OPTION_ARG_STRING, &args_alg.sort, "Sorting:                 " PPG_SORT_ALGS, "ALGORITHM"},
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

/** Vector widths. */
static GOptionEntry entries_vw[] = {
	{"vw-char",  0, 0, G_OPTION_ARG_INT, &args_vw.char_vw,  "Vector width for char types",  "WIDTH"},
	{"vw-short", 0, 0, G_OPTION_ARG_INT, &args_vw.short_vw, "Vector width for short types", "WIDTH"},
	{"vw-int",   0, 0, G_OPTION_ARG_INT, &args_vw.int_vw,   "Vector width for int types",   "WIDTH"},
	{"vw-float", 0, 0, G_OPTION_ARG_INT, &args_vw.float_vw, "Vector width for float types", "WIDTH"},
	{"vw-long",  0, 0, G_OPTION_ARG_INT, &args_vw.long_vw,  "Vector width for long types",  "WIDTH"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/* OpenCL kernel files */
const char* kernelFiles[] = {"pp/pp_gpu.cl"};

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
	int status = PP_SUCCESS;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Predator-Prey simulation data structures. */
	PPGGlobalWorkSizes gws;
	PPGLocalWorkSizes lws;
	PPGKernels krnls = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPGEvents evts = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPGDataSizes dataSizes;
	PPGBuffersHost buffersHost = {NULL, NULL};
	PPGBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPParameters params;
	gchar* compilerOpts = NULL;
	
	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Profiler. */
	ProfCLProfile* profile = NULL;
	
	/* Random number generator */
	GRand* rng = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* Parse arguments. */
	ppg_args_parse(argc, argv, &context, &err);
	gef_if_error_goto(err, PP_UNKNOWN_ARGS, status, error_handler);

	/* Validate arguments. */
	if (!args_alg.rng) args_alg.rng = g_strdup(PP_DEFAULT_RNG);
	PP_ALG_GET(rng_info, rng_infos, args_alg.rng);
	gef_if_error_create_goto(err, PP_ERROR, !rng_info.tag, PP_INVALID_ARGS, error_handler, "Unknown random number generator '%s'.", args_alg.rng);
	
	if (!args_alg.sort) args_alg.sort = g_strdup(PPG_DEFAULT_SORT);
	PP_ALG_GET(sort_info, sort_infos, args_alg.sort);
	gef_if_error_create_goto(err, PP_ERROR, !sort_info.tag, PP_INVALID_ARGS, error_handler, "Unknown sorting algorithm '%s'.", args_alg.sort);

	/* Create RNG with specified seed. */
	rng = g_rand_new_with_seed(args.rng_seed);

	/* Profiling / Timmings. */
	profile = profcl_profile_new();
	gef_if_error_create_goto(err, PP_ERROR, profile == NULL, PP_LIBRARY_ERROR, error_handler, "Unable to create profiler object.");
	
	/* Get simulation parameters */
	status = pp_load_params(&params, args.params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);	
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_GPU, 2, QUEUE_PROPERTIES, clu_menu_device_selector, (args.dev_idx != -1 ? &args.dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Compute work sizes for different kernels. */
	status = ppg_worksizes_compute(params, zone->device_info.device_id, &gws, &lws, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Compiler options. */
	compilerOpts = ppg_compiler_opts_build(gws, lws, params, args.compiler_opts);

	/* Build program. */
	status = clu_program_create(zone, kernelFiles, 1, compilerOpts, &err);
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
	ppg_events_create(params, &evts);

	/*  Set fixed kernel arguments. */
	status = ppg_kernelargs_set(krnls, buffersDevice, dataSizes, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Print information about simulation. */
	ppg_info_print(zone, krnls, gws, lws, dataSizes, compilerOpts, &err);

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
	goto cleanup;
	
error_handler:
	/* Handle error. */
	/** @todo In some error cases, such as when RNG is unknown, status is 0. Fix this. */
	pp_error_handle(err, status);

cleanup:

	/* Free args context and associated cli args parsing buffers. */
	ppg_args_free(context);

	/* Release OpenCL kernels */
	ppg_kernels_free(&krnls);
	
	/* Release OpenCL memory objects */
	ppg_devicebuffers_free(&buffersDevice);

	/* Release OpenCL zone (program, command queue, context) */
	if (zone != NULL) clu_zone_free(zone);

	/* Free host resources */
	ppg_hostbuffers_free(&buffersHost);
	
	/* Free events */
	ppg_events_free(params, &evts); 
	
	/* Free profile data structure */
	if (profile) profcl_profile_free(profile);
	
	/* Free compiler options. */
	if (compilerOpts) g_free(compilerOpts);

	/* Free RNG */
	if (rng) g_rand_free(rng);
	
	/* Bye bye. */
	return 0;
	
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
cl_int ppg_simulate(PPParameters params, CLUZone* zone, 
	PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, 
	PPGKernels krnls, PPGEvents* evts, 
	PPGDataSizes dataSizes, 
	PPGBuffersHost buffersHost, PPGBuffersDevice buffersDevice,
	GError** err) {

	/* Aux. var. */
	cl_int status;
	
	/* Stats. */
	cl_uint *stats_pinned;
	
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
	
	/* Map stats to host. */
	stats_pinned = (cl_uint*) clEnqueueMapBuffer(
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
		&status
	);	
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map pinned stats (OpenCL error %d)", status);
	
#ifdef PPG_DEBUG
	status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: Map pinned stats (OpenCL error %d)", status);
#endif	

		
	/* Load RNG seeds into device buffers. */
	status = clEnqueueWriteBuffer(
		zone->queues[0], 
		buffersDevice.rng_seeds, 
		CL_FALSE, 
		0, 
		dataSizes.rng_seeds, 
		buffersHost.rng_seeds, 
		0, 
		NULL, 
		&evts->write_rng
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Write device buffer: rng_seeds (OpenCL error %d)", status);
	
#ifdef PPG_DEBUG
	status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after rng write (OpenCL error %d)", status);
#endif
	
	/* Init. cells */
	status = clEnqueueNDRangeKernel(
		zone->queues[0], 
		krnls.init_cell, 
		1, 
		NULL, 
		&gws.init_cell, 
		&lws.init_cell, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->init_cell
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: Init. cells (OpenCL error %d)", status);

#ifdef PPG_DEBUG
	status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after init cells (OpenCL error %d)", status);
#endif

	/* Init. agents */
	status = clEnqueueNDRangeKernel(
		zone->queues[1], 
		krnls.init_agent, 
		1, 
		NULL, 
		&gws.init_agent, 
		&lws.init_agent, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->init_agent
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: Init. agents (OpenCL error %d)", status);

#ifdef PPG_DEBUG
	status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after init agents (OpenCL error %d)", status);
#endif

#ifdef PPG_DUMP

	FILE *fp_agent_dump = fopen("dump_agents.txt", "w");
	FILE* fp_cell_dump = fopen("dump_cells.txt", "w");
	cl_ushort2 *agents_xy = (cl_ushort2*) malloc(dataSizes.agents_xy);
	cl_uint *agents_data = (cl_uint*) malloc(dataSizes.agents_data);
	cl_uint *agents_hash = (cl_uint*) malloc(dataSizes.agents_hash);
	cl_uint2 *cells_agents_index = (cl_uint2*) malloc(dataSizes.cells_agents_index);
	int blank_line;

	status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_xy, CL_TRUE, 0, dataSizes.agents_xy, agents_xy, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read xy, iteration %d (OpenCL error %d)", iter, status);
	status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_data, CL_TRUE, 0, dataSizes.agents_data, agents_data, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read data, iteration %d (OpenCL error %d)", iter, status);
	status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_hash, CL_TRUE, 0, dataSizes.agents_hash, agents_hash, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read hashes, iteration %d (OpenCL error %d)", iter, status);
	status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.cells_agents_index, CL_TRUE, 0, dataSizes.cells_agents_index, cells_agents_index, 0, NULL, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Cells dump, read agents index, iteration %d (OpenCL error %d)", iter, status);
	
	fprintf(fp_agent_dump, "\nIteration -1\n");
	for (unsigned int k = 0; k < args.max_agents; k++) {
		if ((!PPG_DUMP) || (agents_hash[k] != 0xFFFFFFFF)) {
			if (blank_line) fprintf(fp_agent_dump, "\n");
			blank_line = FALSE;
			fprintf(fp_agent_dump, "%d: %x (%d, %d)\t(%d,%d)\t%x (should be %x)\n", k, agents_data[k], agents_data[k] >> 16, agents_data[k] & 0xFFFF, agents_xy[k].s[0], agents_xy[k].s[1], agents_hash[k], ((agents_xy[k].s[0] << 16) | (agents_xy[k].s[1])));
		} else {
			blank_line = TRUE;
		}
	}	
	
	fprintf(fp_cell_dump, "\nIteration -1\n");
	for (unsigned int k = 0; k < params.grid_xy; k++) {
		fprintf(fp_cell_dump, "(%d, %d) -> (%d, %d)\n", k % params.grid_x, k / params.grid_y, cells_agents_index[k].s[0], cells_agents_index[k].s[1]);
	}

#endif


	
	/* *************** */
	/* SIMULATION LOOP */
	/* *************** */
	for (iter = 0; ; iter++) {
		
		/* ***************************************** */
		/* ********* Step 4: Gather stats ********** */
		/* ***************************************** */

		/* Step 4.1: Perform grass reduction, part I. */
		status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			krnls.reduce_grass1, 
			1, 
			NULL, 
			&gws.reduce_grass1, 
			&lws.reduce_grass1, 
			iter > 0 ? 1 : 0,
			iter > 0 ? &evts->action_agent[iter - 1] : NULL,
#ifdef CLPROFILER
			&evts->reduce_grass1[iter]
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_grass1, iteration %d (OpenCL error %d)", iter, status);

#ifdef PPG_DEBUG
		status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after reduce_grass1, iteration %d (OpenCL error %d)", iter, status);
#endif
		
	
		/* Step 4.2: Perform agents reduction, part I. */

		/* Determine worksizes. */
		/* See comment for grass reduction gws for an explanation the
		 * formulas bellow. */
		gws_reduce_agent1 = MIN( 
			lws.reduce_agent1 * lws.reduce_agent1, /* lws * number_of_workgroups */
			PP_GWS_MULT(
				PP_DIV_CEIL(max_agents_iter, args_vw.int_vw),
				lws.reduce_agent1
			)
		);

		/* The local/global worksize of reduce_agent2 must be a power of 
		 * 2 for the same reason of reduce_grass2. */
		wg_reduce_agent1 = gws_reduce_agent1 / lws.reduce_agent1;
		ws_reduce_agent2 = nlpo2(wg_reduce_agent1);
		
		/* Set variable kernel arguments in agent reduction kernels. */
		status = clSetKernelArg(krnls.reduce_agent1, 4, sizeof(cl_uint), (void *) &max_agents_iter);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 4 of reduce_agent1, iteration %d (OpenCL error %d)", iter, status);
		status = clSetKernelArg(krnls.reduce_agent2, 3, sizeof(cl_uint), (void *) &wg_reduce_agent1);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of reduce_agent2, iteration %d (OpenCL error %d)", iter, status);

		/* Run agent reduction kernel 1. */
		status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.reduce_agent1, 
			1, 
			NULL, 
			&gws_reduce_agent1, 
			&lws.reduce_agent1, 
			0, 
			NULL,
#ifdef CLPROFILER
			&evts->reduce_agent1[iter]
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_agent1, iteration %d (OpenCL error %d)", iter, status);

#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after reduce_agent1, iteration %d (OpenCL error %d)", iter, status);
#endif
		
		/* Step 4.3: Perform grass reduction, part II. */
		status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			krnls.reduce_grass2, 
			1, 
			NULL, 
			&gws.reduce_grass2, 
			&lws.reduce_grass2, 
			iter > 0 ? 1 : 0,  
			iter > 0 ? &evts->read_stats[iter - 1] : NULL,
			&evts->reduce_grass2[iter]
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_grass2, iteration %d (OpenCL error %d)", iter, status);
		
#ifdef PPG_DEBUG
		status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after reduce_grass2, iteration %d (OpenCL error %d)", iter, status);
#endif

		/* Step 4.4: Perform agents reduction, part II. */
		status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.reduce_agent2, 
			1, 
			NULL, 
			&ws_reduce_agent2, 
			&ws_reduce_agent2, 
			iter > 0 ? 1 : 0,  
			iter > 0 ? &evts->read_stats[iter - 1] : NULL,
#ifdef CLPROFILER		
			&evts->reduce_agent2[iter]
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: reduce_agent2, iteration %d (OpenCL error %d)", iter, status);
		
#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after reduce_agent2, iteration %d (OpenCL error %d)", iter, status);
#endif

		/* Step 4.5: Get statistics. */
		status = clEnqueueReadBuffer(
			zone->queues[1], 
			buffersDevice.stats, 
			CL_FALSE, 
			0, 
			sizeof(PPStatistics), 
			stats_pinned, 
			1, 
			&evts->reduce_grass2[iter],  /* Only need to wait for reduce_grass2 because reduce_agent2 is in same queue. */
			&evts->read_stats[iter]
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Read back stats, iteration %d (OpenCL error %d)", iter, status);

		
#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after read stats, iteration %d (OpenCL error %d)", iter, status);
#endif
		
		/* Stop simulation if this is the last iteration. */
		if (iter == params.iters) break;
		
		/* ***************************************** */
		/* ********* Step 1: Grass growth ********** */
		/* ***************************************** */
		
		/* Grass kernel: grow grass, set number of prey to zero */
		status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			krnls.grass, 1, 
			NULL, 
			&gws.grass, 
			&lws.grass, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&evts->grass[iter]
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: grass, iteration %d (OpenCL error %d)", iter, status);

#ifdef PPG_DEBUG
		status = clFinish(zone->queues[0]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after grass, iteration %d (OpenCL error %d)", iter, status);
#endif
		
		/* ***************************************** */
		/* ******** Step 2: Agent movement ********* */
		/* ***************************************** */

		/* Determine agent movement global worksize. */
		gws_move_agent = PP_GWS_MULT(
			max_agents_iter,
			lws.move_agent
		);
		
		status = clEnqueueNDRangeKernel(
			zone->queues[1], 
			krnls.move_agent, 
			1, 
			NULL, 
			&gws_move_agent, 
			&lws.move_agent, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&evts->move_agent[iter]
#else
			NULL
#endif
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec.: move_agent, iteration %d (OpenCL error %d)", iter, status);

#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after move_agent, iteration %d (OpenCL error %d)", iter, status);
#endif
		
		/* ***************************************** */
		/* ********* Step 3.1: Agent sort ********** */
		/* ***************************************** */

		sort_info.sort(
			&zone->queues[1], 
			krnls.sort_agent, 
			evts->sort_agent, 
			lws.sort_agent,
			max_agents_iter,
			iter,
			err
		);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
		
#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after sort_agent, iteration %d (OpenCL error %d)", iter, status);
#endif

		/* Determine the maximum number of agents there can be in the
		 * next iteration. Make sure stats are already transfered back
		 * to host. */
		status = clWaitForEvents(1, &evts->read_stats[iter]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Wait for events on host thread, iteration %d (OpenCL error %d)", iter, status);
		memcpy(&buffersHost.stats[iter], stats_pinned, sizeof(PPStatistics));
		max_agents_iter = MAX(PPG_MIN_AGENTS, buffersHost.stats[iter].wolves + buffersHost.stats[iter].sheep);
		gef_if_error_create_goto(*err, PP_ERROR, max_agents_iter > args.max_agents, PP_OUT_OF_RESOURCES, error_handler, "Agents required for next iteration above defined limit. Current iteration: %d. Required agents: %d. Agents limit: %d", iter, max_agents_iter, args.max_agents);

		/* ***************************************** */
		/* **** Step 3.2: Find cell agent index **** */
		/* ***************************************** */

		/* Determine kernel global worksize. */
		gws_find_cell_idx = PP_GWS_MULT(
			max_agents_iter,
			lws.find_cell_idx
		);

		status = clEnqueueNDRangeKernel(
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
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after find_cell_idx, iteration %d (OpenCL error %d)", iter, status);
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

		status = clEnqueueNDRangeKernel(
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
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: after action_agent, iteration %d (OpenCL error %d)", iter, status);
		if (iter % PPG_DEBUG == 0)
			fprintf(stderr, "Finishing iteration %d with max_agents_iter=%d (%d sheep, %d wolves)...\n", iter, max_agents_iter, buffersHost.stats[iter].sheep, buffersHost.stats[iter].wolves);

#endif

		/* Agent actions may, in the worst case, double the number of agents. */
		max_agents_iter = max_agents_iter * 2;


#ifdef PPG_DUMP

		status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_xy, CL_TRUE, 0, dataSizes.agents_xy, agents_xy, 0, NULL, NULL);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read xy, iteration %d (OpenCL error %d)", iter, status);
		status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_data, CL_TRUE, 0, dataSizes.agents_data, agents_data, 0, NULL, NULL);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read data, iteration %d (OpenCL error %d)", iter, status);
		status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.agents_hash, CL_TRUE, 0, dataSizes.agents_hash, agents_hash, 0, NULL, NULL);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Agents dump, read hashes, iteration %d (OpenCL error %d)", iter, status);
		status = clEnqueueReadBuffer(zone->queues[1], buffersDevice.cells_agents_index, CL_TRUE, 0, dataSizes.cells_agents_index, cells_agents_index, 0, NULL, NULL);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Cells dump, read agents index, iteration %d (OpenCL error %d)", iter, status);
		
		fprintf(fp_agent_dump, "\nIteration %d, gws_action_ag=%d, gws_mov_ag=%d\n", iter, (unsigned int) gws_action_agent, (unsigned int) gws_move_agent);
		blank_line = FALSE;
		for (unsigned int k = 0; k < args.max_agents; k++) {
			if ((!PPG_DUMP) || (agents_hash[k] != 0xFFFFFFFF)) {
				if (blank_line) fprintf(fp_agent_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_agent_dump, "%d: %x (%d, %d)\t(%d,%d)\t%x (should be %x)\n", k, agents_data[k], agents_data[k] >> 16, agents_data[k] & 0xFFFF, agents_xy[k].s[0], agents_xy[k].s[1], agents_hash[k], ((agents_xy[k].s[0] << 16) | (agents_xy[k].s[1])));
			} else {
				blank_line = TRUE;
			}
		}
		
		fprintf(fp_cell_dump, "\nIteration %d\n", iter);
		blank_line = FALSE;
		for (unsigned int k = 0; k < params.grid_xy; k++) {
			if ((!PPG_DUMP) || (cells_agents_index[k].s[0] != args.max_agents)) {
				if (blank_line) fprintf(fp_cell_dump, "\n");
				blank_line = FALSE;
				fprintf(fp_cell_dump, "(%d, %d) -> (%d, %d)\n", k % params.grid_x, k / params.grid_y, cells_agents_index[k].s[0], cells_agents_index[k].s[1]);
			} else {
				blank_line = TRUE;
			}
		}
#endif
		
	}
	/* ********************** */
	/* END OF SIMULATION LOOP */
	/* ********************** */
	
#ifdef PPG_DUMP
	fclose(fp_agent_dump);
	fclose(fp_cell_dump);
	free(agents_xy);
	free(agents_data);
	free(agents_hash);
#endif
	
	/* Post-simulation ops. */
	
	/* Get last iteration stats. */
	status = clWaitForEvents(1, &evts->read_stats[iter]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Wait for events on host thread, iteration %d (OpenCL error %d)", iter, status);
	memcpy(&buffersHost.stats[iter], stats_pinned, sizeof(PPStatistics));
	
	/* Unmap stats. */
	status = clEnqueueUnmapMemObject(
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
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap pinned stats (OpenCL error %d)", status);
		
#ifdef PPG_DEBUG
	status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 1: Unmap pinned stats (OpenCL error %d)", status);
#endif	

	/* Guarantee all activity has terminated... */
	status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Finish for queue 0 after simulation (OpenCL error %d)", status);
	status = clFinish(zone->queues[1]);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Finish for queue 1 after simulation (OpenCL error %d)", status);
	
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
cl_int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err) {

	cl_int status;
	
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
	sort_info.events_profile(evts->sort_agent, profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);


	/* Analyse event data. */
	profcl_profile_aggregate(profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	profcl_profile_overmat(profile, err);
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
cl_int ppg_worksizes_compute(PPParameters paramsSim, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err) {
	
	/* Get the maximum workgroup size. */
	cl_int status = clGetDeviceInfo(
		device, 
		CL_DEVICE_MAX_WORK_GROUP_SIZE,
		sizeof(size_t),
		&lws->max_lws,
		NULL
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_MAX_WORK_GROUP_SIZE). (OpenCL error %d)", status);
	
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

	/* Get the char vector width, if not specified by user. */
	if (args_vw.char_vw == 0) {
		status = clGetDeviceInfo(
			device, 
			CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, 
			sizeof(cl_uint), 
			&args_vw.char_vw, 
			NULL
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR). (OpenCL error %d)", status);	
	}

	/* Get the int vector width, if not specified by user. */
	if (args_vw.int_vw == 0) {
		status = clGetDeviceInfo(
			device, 
			CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, 
			sizeof(cl_uint), 
			&args_vw.int_vw, 
			NULL
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT). (OpenCL error %d)", status);	
	}

	/* Init cell worksizes. */
	lws->init_cell = args_lws.init_cell ? args_lws.init_cell : lws->deflt;
	gws->init_cell = PP_GWS_MULT(paramsSim.grid_xy, lws->init_cell);
	
	/* Init agent worksizes. */
	lws->init_agent = args_lws.init_agent ? args_lws.init_agent : lws->deflt;
	gws->init_agent = PP_GWS_MULT(args.max_agents, lws->init_agent);

	/* Grass growth worksizes. */
	lws->grass = args_lws.grass ? args_lws.grass : lws->deflt;
	gws->grass = PP_GWS_MULT(paramsSim.grid_xy, lws->grass);
	
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
			PP_DIV_CEIL(paramsSim.grid_xy, args_vw.int_vw),
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

/**
 * @brief Print information about simulation.
 * 
 * @param zone CL zone.
 * @param krnls Simulation kernels.
 * @param gws Kernel global work sizes.
 * @param lws Kernel local work sizes.
 * @param dataSizes Size of data buffers.
 * @param compilerOpts Final OpenCL compiler options.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 * */
cl_int ppg_info_print(CLUZone *zone, PPGKernels krnls, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGDataSizes dataSizes, gchar* compilerOpts, GError **err) {

	/* Status variable */
	cl_int status;
	
	/* Private memory sizes. */
	cl_ulong pm_init_cells, 
		pm_init_agents,
		pm_grass, 
		pm_reduce_grass1, 
		pm_reduce_grass2,
		pm_reduce_agent1,
		pm_reduce_agent2,
		pm_move_agent,
		pm_find_cell_idx,
		pm_action_agent;
	
	/* Determine total global memory. */
	size_t dev_mem = sizeof(PPStatistics) +
		dataSizes.cells_grass + 
		dataSizes.cells_agents_index +
		dataSizes.agents_xy +
		dataSizes.agents_data +
		dataSizes.agents_hash +
		dataSizes.reduce_grass_global + 
		dataSizes.reduce_agent_global +
		dataSizes.rng_seeds;
		
	/* Determine private memory required by kernel workitems. */
	status = clGetKernelWorkGroupInfo (krnls.init_cell, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_init_cells, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get init cell kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.init_agent, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_init_agents, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get init cell kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.grass, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_grass, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get grass kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_grass1, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_grass1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_grass1 kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_grass2, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_grass2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_grass2 kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_agent1, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_agent1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_agent1 kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_agent2, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_agent2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_agent2 kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.move_agent, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_move_agent, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get move_agent kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.find_cell_idx, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_find_cell_idx, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get find_cell_idx kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.action_agent, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_action_agent, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get action_agent kernel info (private memory). (OpenCL error %d)", status);	

	/* Print info. @todo Make ints unsigned or change %d to something nice for size_t */
	printf("\n   =========================== Simulation Info =============================\n\n");
	printf("     Required global memory    : %d bytes (%d Kb = %d Mb)\n", (int) dev_mem, (int) dev_mem / 1024, (int) dev_mem / 1024 / 1024);
	printf("     Compiler options          : %s\n", compilerOpts);
	printf("     Kernel work sizes and local memory requirements:\n");
	printf("       ----------------------------------------------------------------------\n");
	printf("       | Kernel           | GWS      | LWS   | Local memory    | Priv. mem. |\n");
	printf("       ----------------------------------------------------------------------\n");
	printf("       | init_cell        | %8d | %5d |              0b | %9ub |\n", (int) gws.init_cell, (int) lws.init_cell, (unsigned int) pm_init_cells);
	printf("       | init_agent       | %8d | %5d |              0b | %9ub |\n", (int) gws.init_agent, (int) lws.init_agent, (unsigned int) pm_init_agents);
	printf("       | grass            | %8d | %5d |              0b | %9ub |\n", (int) gws.grass, (int) lws.grass, (unsigned int) pm_grass);
	printf("       | reduce_grass1    | %8d | %5d | %6db = %3dKb | %9ub |\n", (int) gws.reduce_grass1, (int) lws.reduce_grass1, (int) dataSizes.reduce_grass_local1, (int) dataSizes.reduce_grass_local1 / 1024, (unsigned int) pm_reduce_grass1);
	printf("       | reduce_grass2    | %8d | %5d | %6db = %3dKb | %9ub |\n", (int) gws.reduce_grass2, (int) lws.reduce_grass2, (int) dataSizes.reduce_grass_local2, (int) dataSizes.reduce_grass_local2 / 1024, (unsigned int) pm_reduce_grass2);
	printf("       | reduce_agent1    |     Var. | %5d | %6db = %3dKb | %9ub |\n", (int) lws.reduce_agent1, (int) dataSizes.reduce_agent_local1, (int) dataSizes.reduce_agent_local1 / 1024, (unsigned int) pm_reduce_agent1);
	printf("       | reduce_agent2    |     Var. |  Var. | %6db = %3dKb | %9ub |\n", (int) dataSizes.reduce_agent_local2, (int) dataSizes.reduce_agent_local2 / 1024, (unsigned int) pm_reduce_agent2);
	printf("       | move_agent       |     Var. | %5d |              0b | %9ub |\n", (int) lws.move_agent, (unsigned int) pm_move_agent);
	/// @todo The sorting information should come from a specific function for each sorting approach
	printf("       | sort_agent       |     Var. | %5d |              0b |         0b |\n", (int) lws.sort_agent); 
	printf("       | find_cell_idx    |     Var. | %5d |              0b | %9ub |\n", (int) lws.find_cell_idx, (unsigned int) pm_find_cell_idx);
	printf("       | action_agent     |     Var. | %5d |              0b | %9ub |\n", (int) lws.action_agent, (unsigned int) pm_action_agent);
	printf("       ----------------------------------------------------------------------\n");

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

/** 
 * @brief Create OpenCL kernels. 
 * 
 * @param program OpenCL program.
 * @param krnls Data structure containing OpenCL kernels references.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise. 
 * */
cl_int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err) {
	
	/* Status variable. */
	cl_int status;
	
	/* Create init cells kernel. */
	krnls->init_cell = clCreateKernel(program, "initCell", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: initCell (OpenCL error %d)", status);	

	/* Create init agents kernel. */
	krnls->init_agent = clCreateKernel(program, "initAgent", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: initAgent (OpenCL error %d)", status);	

	/* Create grass kernel. */
	krnls->grass = clCreateKernel(program, "grass", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: grass (OpenCL error %d)", status);	
	
	/* Create reduce grass 1 kernel. */
	krnls->reduce_grass1 = clCreateKernel(program, "reduceGrass1", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass1 (OpenCL error %d)", status);	
	
	/* Create reduce grass 2 kernel. */
	krnls->reduce_grass2 = clCreateKernel(program, "reduceGrass2", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass2 (OpenCL error %d)", status);	
	
	/* Create reduce agent 1 kernel. */
	krnls->reduce_agent1 = clCreateKernel(program, "reduceAgent1", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceAgent1 (OpenCL error %d)", status);	
	
	/* Create reduce agent 2 kernel. */
	krnls->reduce_agent2 = clCreateKernel(program, "reduceAgent2", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceAgent2 (OpenCL error %d)", status);	
	
	/* Create move agents kernel. */
	krnls->move_agent = clCreateKernel(program, "moveAgent", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: moveAgent (OpenCL error %d)", status);
	
	/* Create sort agents kernel. */
	status = sort_info.kernels_create(&krnls->sort_agent, program, err);
	gef_if_error_goto(*err, GEF_USE_GERROR, status, error_handler);
	
	/* Find cell agent index kernel. */
	krnls->find_cell_idx = clCreateKernel(program, "findCellIdx", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: findCellIdx (OpenCL error %d)", status);
	
	/* Agent actions kernel. */
	krnls->action_agent = clCreateKernel(program, "actionAgent", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: actionAgent (OpenCL error %d)", status);

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
 * @param lws Kernel local work sizes.
 * @param dataSizes Size of data buffers.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise. 
 * 
 * */
cl_int ppg_kernelargs_set(PPGKernels krnls, PPGBuffersDevice buffersDevice, PPGDataSizes dataSizes, GError** err) {

	/* Aux. variable. */
	cl_int status;
	
	/* Cell init kernel. */
	status = clSetKernelArg(krnls.init_cell, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_cell (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls.init_cell, 1, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_cell (OpenCL error %d)", status);
	
	/* Agent init kernel. */
	status = clSetKernelArg(krnls.init_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_xy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.init_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.init_agent, 2, sizeof(cl_mem), (void*) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of init_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.init_agent, 3, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of init_agent (OpenCL error %d)", status);

	/* Grass kernel */
	status = clSetKernelArg(krnls.grass, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of grass (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.grass, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of grass (OpenCL error %d)", status);

	/* reduce_grass1 kernel */
	status = clSetKernelArg(krnls.reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_grass1, 1, dataSizes.reduce_grass_local1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_grass1, 2, sizeof(cl_mem), (void *) &buffersDevice.reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_grass1 (OpenCL error %d)", status);

	/* reduce_grass2 kernel */
	status = clSetKernelArg(krnls.reduce_grass2, 0, sizeof(cl_mem), (void *) &buffersDevice.reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass2 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_grass2, 1, dataSizes.reduce_grass_local2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass2 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_grass2, 2, sizeof(cl_mem), (void *) &buffersDevice.stats);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_grass2 (OpenCL error %d)", status);
 	
	/* reduce_agent1 kernel */
	status = clSetKernelArg(krnls.reduce_agent1, 0, sizeof(cl_mem), (void *) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_agent1 (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls.reduce_agent1, 1, sizeof(cl_mem), (void *) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_agent1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_agent1, 2, dataSizes.reduce_agent_local1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_agent1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_agent1, 3, sizeof(cl_mem), (void *) &buffersDevice.reduce_agent_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of reduce_agent1 (OpenCL error %d)", status);
	/* The 3rd argument is set on the fly. */

	/* reduce_agent2 kernel */
	status = clSetKernelArg(krnls.reduce_agent2, 0, sizeof(cl_mem), (void *) &buffersDevice.reduce_agent_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_agent2 (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls.reduce_agent2, 1, dataSizes.reduce_agent_local2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_agent2 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.reduce_agent2, 2, sizeof(cl_mem), (void *) &buffersDevice.stats);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_agent2 (OpenCL error %d)", status);

	/* Agent movement kernel. */
	status = clSetKernelArg(krnls.move_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_xy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of move_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.move_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of move_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.move_agent, 2, sizeof(cl_mem), (void*) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of move_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.move_agent, 3, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of move_agent (OpenCL error %d)", status);
	
	/* Agent sorting kernel. */
	status = sort_info.kernelargs_set(&krnls.sort_agent, buffersDevice, err);
	gef_if_error_goto(*err, GEF_USE_GERROR, status, error_handler);
	
	/* Find cell agent index kernel. */
	status = clSetKernelArg(krnls.find_cell_idx, 0, sizeof(cl_mem), (void*) &buffersDevice.agents_xy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of find_cell_idx (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.find_cell_idx, 1, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of find_cell_idx (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.find_cell_idx, 2, sizeof(cl_mem), (void*) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of find_cell_idx (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.find_cell_idx, 3, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of find_cell_idx (OpenCL error %d)", status);

	/* Agent actions kernel. */
	status = clSetKernelArg(krnls.action_agent, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of action_agent (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls.action_agent, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_agents_index);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of action_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.action_agent, 2, sizeof(cl_mem), (void*) &buffersDevice.agents_xy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of action_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.action_agent, 3, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of action_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.action_agent, 4, sizeof(cl_mem), (void*) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 4 of action_agent (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.action_agent, 5, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 5 of action_agent (OpenCL error %d)", status);

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
		fprintf(fp1, "%d\t%d\t%d\n", statsArray[i].sheep, statsArray[i].wolves, statsArray[i].grass );
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
	dataSizes->cells_grass = pp_next_multiple(params.grid_xy, args_vw.int_vw) * sizeof(cl_uint);
	dataSizes->cells_agents_index = params.grid_xy * sizeof(cl_uint2);
	
	/* Agents. */
	dataSizes->agents_xy = args.max_agents * sizeof(cl_ushort2);
	dataSizes->agents_data = args.max_agents * sizeof(cl_uint);
	dataSizes->agents_hash = args.max_agents * sizeof(cl_uint);
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local1 = lws.reduce_grass1 * args_vw.int_vw * sizeof(cl_uint);
	dataSizes->reduce_grass_global = gws.reduce_grass2 * args_vw.int_vw * sizeof(cl_uint);
	dataSizes->reduce_grass_local2 = lws.reduce_grass2 * args_vw.int_vw * sizeof(cl_uint);
	
	/* Agent reduction. */
	dataSizes->reduce_agent_local1 = 2 * lws.reduce_agent1 * args_vw.int_vw * sizeof(cl_uint); /* 2x to count sheep and wolves. */
	dataSizes->reduce_agent_global = dataSizes->reduce_agent_local1;
	dataSizes->reduce_agent_local2 = dataSizes->reduce_agent_local1;

	/* Rng seeds */
	dataSizes->rng_seeds = MAX(params.grid_xy, PPG_DEFAULT_MAX_AGENTS) * rng_info.bytes;
	dataSizes->rng_seeds_count = dataSizes->rng_seeds / sizeof(cl_ulong);

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
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes dataSizes, GError** err) {
	
	/* Status flag. */
	cl_int status;
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(PPStatistics), NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: stats (OpenCL error %d)", status);

	/* Cells */
	buffersDevice->cells_grass = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_grass, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass (OpenCL error %d)", status);

	buffersDevice->cells_agents_index = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_agents_index, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_index (OpenCL error %d)", status);
	
	/* Agents. */
	buffersDevice->agents_xy = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.agents_xy, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: agents_xy (OpenCL error %d)", status);

	buffersDevice->agents_data = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.agents_data, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: agents_data (OpenCL error %d)", status);

	buffersDevice->agents_hash = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.agents_hash, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: agents_hash (OpenCL error %d)", status);

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.reduce_grass_global, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_grass_global (OpenCL error %d)", status);

	/* Agent reduction (count) */
	buffersDevice->reduce_agent_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.reduce_agent_global, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_agent_global (OpenCL error %d)", status);

	/* RNG seeds. */
	buffersDevice->rng_seeds = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.rng_seeds, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: rng_seeds (OpenCL error %d)", status);
	
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

/**
 * @brief Free device buffers.
 * @param buffersDevice Data structure containing device data buffers.
 * */
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice) {
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->cells_grass) clReleaseMemObject(buffersDevice->cells_grass);
	if (buffersDevice->cells_agents_index) clReleaseMemObject(buffersDevice->cells_agents_index);
	if (buffersDevice->agents_xy) clReleaseMemObject(buffersDevice->agents_xy);
	if (buffersDevice->agents_data) clReleaseMemObject(buffersDevice->agents_data);
	if (buffersDevice->agents_hash) clReleaseMemObject(buffersDevice->agents_hash);
	if (buffersDevice->reduce_grass_global) clReleaseMemObject(buffersDevice->reduce_grass_global);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
}

/** 
 * @brief Initialize data structure to hold OpenCL events.
 * 
 * @param params Simulation parameters.
 * @param evts Structure to hold OpenCL events (to be modified by
 * function).
 * 
 * */
void ppg_events_create(PPParameters params, PPGEvents* evts) {

#ifdef CLPROFILER
	evts->grass = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_agent1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->move_agent = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_agent2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	sort_info.events_create(&evts->sort_agent, params.iters, NULL); // @todo Verify possible error from this function (pass err instead of NULL)
	evts->find_cell_idx = (cl_event*) calloc(params.iters, sizeof(cl_event));
#endif

	evts->read_stats = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->action_agent = (cl_event*) calloc(params.iters, sizeof(cl_event));
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
	sort_info.events_free(&evts->sort_agent);
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
gchar* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPParameters params, gchar* cliOpts) {
	gchar* compilerOptsStr;
	GString* compilerOpts = g_string_new(PP_KERNEL_INCLUDES);
	g_string_append_printf(compilerOpts, "-D VW_CHAR=%d ", args_vw.char_vw);
	g_string_append_printf(compilerOpts, "-D VW_INT=%d ", args_vw.int_vw);
	g_string_append_printf(compilerOpts, "-D VW_CHAR2INT_MUL=%d ", args_vw.char_vw / args_vw.int_vw);
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
	g_string_append_printf(compilerOpts, "-D %s ", rng_info.compiler_const);
	g_string_append_printf(compilerOpts, "-D %s ", sort_info.compiler_const);
	if (cliOpts) g_string_append_printf(compilerOpts, "%s", cliOpts);
	compilerOptsStr = compilerOpts->str;
	g_string_free(compilerOpts, FALSE);
	return compilerOptsStr;
}

/** 
 * @brief Parse command-line options. 
 * 
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 * @param context Context object for command line argument parsing.
 * @param err GLib error object for error reporting.
 * */
void ppg_args_parse(int argc, char* argv[], GOptionContext** context, GError** err) {
	
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
		"Vector widths for numerical types:", 
		"Show options which define vector widths for different numerical types", 
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
	if (args.compiler_opts) g_free(args.compiler_opts);
	if (args_alg.rng) g_free(args_alg.rng);
	if (args_alg.sort) g_free(args_alg.sort);
}
