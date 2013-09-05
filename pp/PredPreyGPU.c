/** 
 * @file
 * @brief PredPrey OpenCL GPU implementation.
 */

#include "PredPreyGPU.h"

/** The default maximum number of agents: 16777216. Each agent requires
 * 16 bytes, thus by default 256Mb of memory will be allocated for the
 * agents buffer. 
 * 
 * @todo Check if final agents will have 16 bytes
 * @todo Check the implications of having more or less memory for agents
 *  */
#define PPG_DEFAULT_MAX_AGENTS 16777216

/** A description of the program. */
#define PPG_DESCRIPTION "OpenCL predator-prey simulation for the GPU"

//#define PPG_DEBUG

/** Main command line arguments and respective default values. */
static PPGArgs args = {NULL, NULL, NULL, -1, PP_DEFAULT_SEED, NULL, PPG_DEFAULT_MAX_AGENTS};

/** Local work sizes command-line arguments*/
static PPGArgsLWS args_lws = {0, 0, 0};

/** Vector widths command line arguments. */
static PPGArgsVW args_vw = {0, 0, 0, 0, 0};

/** Main command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,        "Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",                           "FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,         "Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",                     "FILENAME"},
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts, "Extra OpenCL compiler options",                                                             "OPTS"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,       "Device index (if not given and more than one device is available, chose device from menu)", "INDEX"},
	{"rng_seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",                    "SEED"},
	{"rngen",           'n', 0, G_OPTION_ARG_STRING,   &args.rngen,         "Random number generator: " PP_RNGS,                                                         "RNG"},
	{"max_agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,    "Maximum number of agents (default is " STR(PPG_DEFAULT_MAX_AGENTS) ")",                     "SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,        NULL,                                                                                        NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Kernel local worksizes. */
static GOptionEntry entries_lws[] = {
	{"l-init-cell",    0, 0, G_OPTION_ARG_INT, &args_lws.init_cell,    "Init. cells kernel",  "LWS"},
	{"l-grass",        0, 0, G_OPTION_ARG_INT, &args_lws.grass,        "Grass kernel",        "LWS"},
	{"l-reduce-grass", 0, 0, G_OPTION_ARG_INT, &args_lws.reduce_grass, "Reduce grass kernel", "LWS"},
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
const char* kernelFiles[] = {"pp/PredPreyGPU_Kernels.cl"};

/**
 * @brief Main program.
 * 
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program 
 * terminates successfully, or another value of #pp_error_codes if an
 * error occurs.
 * */
int main(int argc, char **argv)
{

	/* Status var aux */
	int status = PP_SUCCESS;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Predator-Prey simulation data structures. */
	PPGGlobalWorkSizes gws;
	PPGLocalWorkSizes lws;
	PPGKernels krnls = {NULL, NULL, NULL, NULL};
	PPGEvents evts = {NULL, NULL, NULL, NULL, NULL, NULL};
	PPGDataSizes dataSizes;
	PPGBuffersHost buffersHost = {NULL, NULL};
	PPGBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
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
	if (!args.rngen) args.rngen = g_strdup(PP_DEFAULT_RNG);
	gef_if_error_create_goto(err, PP_ERROR, !pp_rng_const_get(args.rngen), PP_INVALID_ARGS, error_handler, "Unknown random number generator '%s'.", args.rngen);

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
	ppg_hostbuffers_create(&buffersHost, dataSizes, params, rng);

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
	profcl_profile_free(profile);
	
	/* Free compiler options. */
	if (compilerOpts) g_free(compilerOpts);

	/* Free RNG */
	g_rand_free(rng);
	
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
	
	/* Current iteration. */
	cl_uint iter = 0; 
		
	/* Load data into device buffers. */
	status = clEnqueueWriteBuffer(zone->queues[0], buffersDevice.rng_seeds, CL_FALSE, 0, dataSizes.rng_seeds, buffersHost.rng_seeds, 0, NULL, &evts->write_rng);
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
			0, 
			NULL,
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
		
		/* Step 4.2: Perform grass reduction, part II. */
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

		/* Step 4.3: Perform agents reduction, part I. */

		/** @todo Agents reduction, part I. */

		/* Step 4.4: Perform agents reduction, part II. */

		/** @todo Agents reduction, part II. */

		/* Step 4.5: Get statistics. */
		status = clEnqueueReadBuffer(
			zone->queues[1], 
			buffersDevice.stats, 
			CL_FALSE, 
			0, 
			sizeof(PPStatistics), 
			&buffersHost.stats[iter], 
			1, 
			&evts->reduce_grass2[iter], 
			&evts->read_stats[iter]
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Read back stats, iteration %d (OpenCL error %d)", iter, status);

#ifdef PPG_DEBUG
		status = clFinish(zone->queues[1]);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "DEBUG queue 0: after read stats, iteration %d (OpenCL error %d)", iter, status);
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

		/** @todo Agent movement. One kernel running in queue 1. */
		
		/* ***************************************** */
		/* ********* Step 3: Agent actions ********* */
		/* ***************************************** */

		/** @todo Agent actions. Several kernels: 
		 * 1.1. calcHash, queue 1
		 * 1.2. fastRadixSort, queue 1
		 * 1.3. findCellStartAndFinish, queue 1
		 * 2. Agent actions, queue 1 */

		
	}
	/* ********************** */
	/* END OF SIMULATION LOOP */
	/* ********************** */
	
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
	profcl_profile_add(profile, "Init cells", evts->init_cell, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add(profile, "Write data", evts->write_rng, err);
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

	}
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
	
	/* Variable which will keep the maximum workgroup size */
	size_t maxWorkGroupSize;
	
	/* Get the maximum workgroup size. */
	cl_int status = clGetDeviceInfo(
		device, 
		CL_DEVICE_MAX_WORK_GROUP_SIZE,
		sizeof(size_t),
		&maxWorkGroupSize,
		NULL
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_MAX_WORK_GROUP_SIZE). (OpenCL error %d)", status);	

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

	/* init cell worksizes */
	lws->init_cell = args_lws.init_cell ? args_lws.init_cell : maxWorkGroupSize;
	gws->init_cell = PP_GWS_MULT(paramsSim.grid_xy, lws->init_cell);

	/* grass growth worksizes */
	lws->grass = args_lws.grass ? args_lws.grass : maxWorkGroupSize;
	gws->grass = PP_GWS_MULT(paramsSim.grid_xy, lws->grass);
	
	/* grass count worksizes */
	lws->reduce_grass1 = args_lws.reduce_grass ?  args_lws.reduce_grass : maxWorkGroupSize;
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
	 * step successfully (otherwise not all sums may be performed). */
	lws->reduce_grass2 = nlpo2(gws->reduce_grass1 / lws->reduce_grass1);
	gws->reduce_grass2 = lws->reduce_grass2;
	
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
	cl_ulong pm_init_cells, pm_grass, pm_reduce_grass1, pm_reduce_grass2;
	
	/* Determine total global memory. */
	size_t dev_mem = sizeof(PPStatistics) +
		dataSizes.cells_grass_alive +
		dataSizes.cells_grass_timer + 
		dataSizes.cells_agents_index_start +
		dataSizes.cells_agents_index_end +
		dataSizes.reduce_grass_global + 
		dataSizes.rng_seeds;
		
	/* Determine private memory required by kernel workitems. */
	status = clGetKernelWorkGroupInfo (krnls.init_cell, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_init_cells, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get init cell kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.grass, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_grass, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get grass kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_grass1, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_grass1, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_grass1 kernel info (private memory). (OpenCL error %d)", status);	
	status = clGetKernelWorkGroupInfo (krnls.reduce_grass2, zone->device_info.device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &pm_reduce_grass2, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get reduce_grass2 kernel info (private memory). (OpenCL error %d)", status);	

	/* Print info. */
	printf("\n   =========================== Simulation Info =============================\n\n");
	printf("     Required global memory    : %d bytes (%d Kb = %d Mb)\n", (int) dev_mem, (int) dev_mem / 1024, (int) dev_mem / 1024 / 1024);
	printf("     Compiler options          : %s\n", compilerOpts);
	printf("     Kernel work sizes and local memory requirements:\n");
	printf("       ----------------------------------------------------------------------\n");
	printf("       | Kernel           | GWS      | LWS   | Local memory    | Priv. mem. |\n");
	printf("       ----------------------------------------------------------------------\n");
	printf("       | init_cell        | %8d | %5d |               0 | %9ub |\n", (int) gws.init_cell, (int) lws.init_cell, (unsigned int) pm_init_cells);
	printf("       | grass            | %8d | %5d |               0 | %9ub |\n", (int) gws.grass, (int) lws.grass, (unsigned int) pm_grass);
	printf("       | reducegrass1     | %8d | %5d | %6db = %3dKb | %9ub |\n", (int) gws.reduce_grass1, (int) lws.reduce_grass1, (int) dataSizes.reduce_grass_local1, (int) dataSizes.reduce_grass_local1 / 1024, (unsigned int) pm_reduce_grass1);
	printf("       | reducegrass2     | %8d | %5d | %6db = %3dKb | %9ub |\n", (int) gws.reduce_grass2, (int) lws.reduce_grass2, (int) dataSizes.reduce_grass_local2, (int) dataSizes.reduce_grass_local2 / 1024, (unsigned int) pm_reduce_grass2);
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
	
	cl_int status;
	
	krnls->init_cell = clCreateKernel(program, "initCell", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: initCell (OpenCL error %d)", status);	

	krnls->grass = clCreateKernel(program, "grass", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: grass (OpenCL error %d)", status);	
	
	krnls->reduce_grass1 = clCreateKernel(program, "reduceGrass1", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass1 (OpenCL error %d)", status);	
	
	krnls->reduce_grass2 = clCreateKernel(program, "reduceGrass2", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: reduceGrass2 (OpenCL error %d)", status);	
	
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
	if (krnls->grass) clReleaseKernel(krnls->grass);
	if (krnls->reduce_grass1) clReleaseKernel(krnls->reduce_grass1); 
	if (krnls->reduce_grass2) clReleaseKernel(krnls->reduce_grass2);
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
	
	/* Cell init kernel */
	status = clSetKernelArg(krnls.init_cell, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_cell (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls.init_cell, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_grass_timer);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_cell (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.init_cell, 2, sizeof(cl_mem), (void*) &buffersDevice.rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of init_cell (OpenCL error %d)", status);

	/* Grass kernel */
	status = clSetKernelArg(krnls.grass, 0, sizeof(cl_mem), (void*) &buffersDevice.cells_grass_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of grass (OpenCL error %d)", status);

	status = clSetKernelArg(krnls.grass, 1, sizeof(cl_mem), (void*) &buffersDevice.cells_grass_timer);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of grass (OpenCL error %d)", status);

	/* reduce_grass1 kernel */
	status = clSetKernelArg(krnls.reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice.cells_grass_alive);
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
	dataSizes->cells_grass_alive = pp_next_multiple(params.grid_xy, args_vw.int_vw) * sizeof(cl_uchar);
	dataSizes->cells_grass_timer = params.grid_xy * sizeof(cl_ushort);
	dataSizes->cells_agents_index_start = params.grid_xy * sizeof(cl_uint);
	dataSizes->cells_agents_index_end = params.grid_xy * sizeof(cl_uint);
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local1 = lws.reduce_grass1 * args_vw.int_vw * sizeof(cl_uint); /** @todo Verify that GPU supports this local memory requirement */
	dataSizes->reduce_grass_global = gws.reduce_grass2 * args_vw.int_vw * sizeof(cl_uint);
	dataSizes->reduce_grass_local2 = lws.reduce_grass2 * args_vw.int_vw * sizeof(cl_uint); /** @todo Verify that GPU supports this local memory requirement */
	
	/* Rng seeds */
	dataSizes->rng_seeds = MAX(params.grid_xy, PPG_DEFAULT_MAX_AGENTS) * pp_rng_bytes_get(args.rngen);
	dataSizes->rng_seeds_count = dataSizes->rng_seeds / sizeof(cl_ulong);

}

/** 
 * @brief Initialize host data buffers. 
 * 
 * @param buffersHost Data structure containing references to the data
 * buffers.
 * @param dataSizes Sizes of simulation data arrays.
 * @param params Simulation parameters.
 * @param rng Random number generator.
 * */
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes dataSizes, PPParameters params, GRand* rng) {
	
	
	/** @todo check if params is necessary here */
	params = params;
	
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
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(PPStatistics), NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: stats (OpenCL error %d)", status);

	/* Cells */
	buffersDevice->cells_grass_alive = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_grass_alive, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass_alive (OpenCL error %d)", status);

	buffersDevice->cells_grass_timer = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_grass_timer, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass_timer (OpenCL error %d)", status);

	buffersDevice->cells_agents_index_start = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_agents_index_start, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_number (OpenCL error %d)", status);

	buffersDevice->cells_agents_index_end = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.cells_agents_index_end, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_index (OpenCL error %d)", status);

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes.reduce_grass_global, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_grass_global (OpenCL error %d)", status);

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
	if (buffersDevice->cells_grass_alive) clReleaseMemObject(buffersDevice->cells_grass_alive);
	if (buffersDevice->cells_grass_timer) clReleaseMemObject(buffersDevice->cells_grass_timer);
	if (buffersDevice->cells_agents_index_start) clReleaseMemObject(buffersDevice->cells_agents_index_start);
	if (buffersDevice->cells_agents_index_end) clReleaseMemObject(buffersDevice->cells_agents_index_end);
	if (buffersDevice->reduce_grass_global) clReleaseMemObject(buffersDevice->reduce_grass_global);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
}

/** 
 * @brief Initialize data structure to hold OpenCL events.
 * 
 * @param params Simulation parameters.
 * @param evts Structure to hold OpenCL events (to be modified by
 * function).
 * */
void ppg_events_create(PPParameters params, PPGEvents* evts) {

#ifdef CLPROFILER
	evts->grass = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
#endif

	evts->read_stats = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
}

/**
 * @brief Free data structure which holds OpenCL events.
 * 
 * @param params Simulation parameters.
 * @param evts Structure which holds OpenCL events.
 * */
void ppg_events_free(PPParameters params, PPGEvents* evts) {
	if (evts->init_cell) clReleaseEvent(evts->init_cell);
	if (evts->write_rng) clReleaseEvent(evts->write_rng);
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
	g_string_append_printf(compilerOpts, "-D VW_INT=%d ", args_vw.int_vw);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKITEMS=%d ", (unsigned int) gws.reduce_grass1);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKGROUPS=%d ", (unsigned int) (gws.reduce_grass1 / lws.reduce_grass1));
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
	g_string_append_printf(compilerOpts, "-D %s ", pp_rng_const_get(args.rngen));
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
	GOptionGroup *group_lws = g_option_group_new("lws", 
		"Kernel local work sizes:", 
		"Show options which define the kernel local work sizes", 
		NULL, NULL);
	GOptionGroup *group_vw = g_option_group_new("vw", 
		"Vector widths for numerical types:", 
		"Show options which define vector widths for different numerical types", 
		NULL, NULL);

	/* Add entries to separate option groups. */
	g_option_group_add_entries(group_lws, entries_lws);
	g_option_group_add_entries(group_vw, entries_vw);
	
	/* Add option groups to main options context. */
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
	if (args.rngen) g_free(args.rngen);
}
