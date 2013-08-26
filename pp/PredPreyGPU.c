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

/** @todo This is only currently used for the size of RNG seeds array 
 * but this size should be determined by the maximum GWS used by the 
 * different kernels which require RNG. */
#define MAX_GWS 1048576

#define PPG_LIMITS_SHOW
//#define PPG_DEBUG

#define LWS_INIT_CELL 32
#define LWS_GRASS 32
#define LWS_REDUCEGRASS1 32

/** Command line arguments and respective default values. */
static PPGArgs args = {NULL, NULL, NULL, 0, 0, -1, PP_DEFAULT_SEED, NULL, PPG_DEFAULT_MAX_AGENTS, 0};

/** Valid command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,        "Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",                           "FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,         "Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",                     "FILENAME"},
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts, "Extra OpenCL compiler options",                                                             "OPTS"},
	{"globalsize",      'g', 0, G_OPTION_ARG_INT,      &args.gws,           "Global work size (default is maximum possible)",                                            "SIZE"},
	{"localsize",       'l', 0, G_OPTION_ARG_INT,      &args.lws,           "Local work size (default is selected by OpenCL runtime)",                                   "SIZE"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,       "Device index (if not given and more than one device is available, chose device from menu)", "INDEX"},
	{"rng_seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",                    "SEED"},
	{"rngen",           'n', 0, G_OPTION_ARG_STRING,   &args.rng_seed,      "Random number generator: " PP_RNGS,                                                         "RNG"},
	{"max_agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,    "Maximum number of agents (default is " STR(PPG_DEFAULT_MAX_AGENTS) ")",                     "SIZE"},
	{"vw-int",           0,  0, G_OPTION_ARG_INT,      &args.vwint,         "Vector width for int's (default is given by device)",                                       "SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,        NULL,                                                                                        NULL},
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
	PPGSimParams simParams;
	char* compilerOpts = NULL;
	
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
	
	/* Set simulation parameters in a format more adequate for this program. */
	simParams = ppg_simparams_init(params, args.max_agents);

	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_GPU, 2, QUEUE_PROPERTIES, clu_menu_device_selector, (args.dev_idx != -1 ? &args.dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Compute work sizes for different kernels and print them to screen. */
	status = ppg_worksizes_compute(params, zone->device_info.device_id, &gws, &lws, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	ppg_worksizes_print(gws, lws);

	/* Compiler options. */
	compilerOpts = ppg_compiler_opts_build(gws, lws, simParams, args.compiler_opts);
	printf("%s\n", compilerOpts);

	/* Build program. */
	status = clu_program_create(zone, kernelFiles, 1, compilerOpts, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);

	/* Create kernels. */
	status = ppg_kernels_create(zone->program, &krnls, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Determine size in bytes for host and device data structures. */
	ppg_datasizes_get(params, simParams, &dataSizes, gws, lws);

	/* Initialize host buffers. */
	ppg_hostbuffers_create(&buffersHost, &dataSizes, params, rng);

	/* Create device buffers */
	status = ppg_devicebuffers_create(zone->context, &buffersDevice, &dataSizes, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Create events data structure. */
	ppg_events_create(params, &evts);

	/*  Set fixed kernel arguments. */
	status = ppg_kernelargs_set(&krnls, &buffersDevice, simParams, lws, &dataSizes, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
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
	clu_zone_free(zone);

	/* Free host resources */
	ppg_hostbuffers_free(&buffersHost);
	
	/* Free events */
	ppg_events_free(params, &evts); 
	
	/* Free profile data structure */
	profcl_profile_free(profile);
	
	/* Free compiler options. */
	free(compilerOpts);

	/* Free RNG */
	g_rand_free(rng);
	
	/* Bye bye. */
	return 0;
	
}

/** 
 * @brief Perform simulation
 * 
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

/* Perform profiling analysis */
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

/* Compute worksizes depending on the device type and number of available compute units */
cl_int ppg_worksizes_compute(PPParameters params, cl_device_id device, PPGGlobalWorkSizes *gws, PPGLocalWorkSizes *lws, GError** err) {
	
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
	if (args.vwint == 0) {
		status = clGetDeviceInfo(
			device, 
			CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, 
			sizeof(cl_uint), 
			&args.vwint, 
			NULL
		);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Get device info (CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT). (OpenCL error %d)", status);	
	}

	/* init cell worksizes */
#ifdef LWS_INIT_CELL
	lws->init_cell = LWS_INIT_CELL;
#else
	lws->init_cell = maxWorkGroupSize; /** @todo Allow user to define. */
#endif
	gws->init_cell = lws->init_cell * ceil(((float) (params.grid_x * params.grid_y)) / lws->init_cell); 
	/** @todo Use sim_params to avoid mult. */
	/** @todo Use efficient function to calculate GWS depending on LWS. */

	/* grass growth worksizes */
#ifdef LWS_GRASS
	lws->grass = LWS_GRASS;
#else
	lws->grass = maxWorkGroupSize;
#endif
	gws->grass = lws->grass * ceil(((float) (params.grid_x * params.grid_y)) / lws->grass); /** @todo Use sim_params to avoid mult. */
	
	/* grass count worksizes */
#ifdef LWS_REDUCEGRASS1 //TODO This should depend on number of cells, vector width, etc.
	lws->reduce_grass1 = LWS_REDUCEGRASS1; 
#else
	lws->reduce_grass1 = maxWorkGroupSize;
#endif	
	gws->reduce_grass1 = MIN(lws->reduce_grass1 * lws->reduce_grass1, lws->reduce_grass1 * ceil(((float) (params.grid_x * params.grid_y)) / args.vwint / lws->reduce_grass1));
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

// Print worksizes
void ppg_worksizes_print(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {
	printf("Kernel work sizes:\n");
	printf("init_cell_gws=%d\tinit_cell_lws=%d\n", (int) gws.init_cell, (int) lws.init_cell);
	printf("grass_gws=%d\tgrass_lws=%d\n", (int) gws.grass, (int) lws.grass);
	printf("reducegrass1_gws=%d\treducegrass1_lws=%d\n", (int) gws.reduce_grass1, (int) lws.reduce_grass1);
	printf("grasscount2_lws/gws=%d\n", (int) lws.reduce_grass2);
}

// Get kernel entry points
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

// Release kernels
void ppg_kernels_free(PPGKernels* krnls) {
	if (krnls->init_cell) clReleaseKernel(krnls->init_cell);
	if (krnls->grass) clReleaseKernel(krnls->grass);
	if (krnls->reduce_grass1) clReleaseKernel(krnls->reduce_grass1); 
	if (krnls->reduce_grass2) clReleaseKernel(krnls->reduce_grass2);
}


// Initialize simulation parameters in host, to be sent to GPU
PPGSimParams ppg_simparams_init(PPParameters params, cl_uint max_agents) {
	PPGSimParams simParams;
	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = max_agents;
	simParams.grass_restart = params.grass_restart;
	return simParams;
}

//  Set fixed kernel arguments.
cl_int ppg_kernelargs_set(PPGKernels* krnls, PPGBuffersDevice* buffersDevice, PPGSimParams simParams, PPGLocalWorkSizes lws, PPGDataSizes* dataSizes, GError** err) {

	/* Aux. variable. */
	cl_int status;
	
	/* Cell init kernel */
	status = clSetKernelArg(krnls->init_cell, 0, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of init_cell (OpenCL error %d)", status);
	
	status = clSetKernelArg(krnls->init_cell, 1, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_timer);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of init_cell (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->init_cell, 2, sizeof(cl_mem), (void*) &buffersDevice->rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of init_cell (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->init_cell, 3, sizeof(PPGSimParams), (void*) &simParams);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of init_cell (OpenCL error %d)", status);

	/* Grass kernel */
	status = clSetKernelArg(krnls->grass, 0, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of grass (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->grass, 1, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_timer);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of grass (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->grass, 2, sizeof(PPGSimParams), (void*) &simParams);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of grass (OpenCL error %d)", status);
	
	/* reduce_grass1 kernel */
	status = clSetKernelArg(krnls->reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice->cells_grass_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->reduce_grass1, 1, dataSizes->reduce_grass_local, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass1 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->reduce_grass1, 2, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of reduce_grass1 (OpenCL error %d)", status);

	/* reduce_grass2 kernel */
	status = clSetKernelArg(krnls->reduce_grass2, 0, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of reduce_grass2 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->reduce_grass2, 1, lws.reduce_grass2 *args.vwint*sizeof(cl_uint), NULL);//TODO Put this size in dataSizes
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of reduce_grass2 (OpenCL error %d)", status);

	status = clSetKernelArg(krnls->reduce_grass2, 2, sizeof(cl_mem), (void *) &buffersDevice->stats);
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

// Save results
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params) {
	
	/* Get definite file name. */
	gchar* realFilename = (filename != NULL) ? filename : PP_DEFAULT_STATS_FILE;	

	FILE * fp1 = fopen(realFilename,"w");
	
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArray[i].sheep, statsArray[i].wolves, statsArray[i].grass );
	fclose(fp1);
}

// Determine buffer sizes
void ppg_datasizes_get(PPParameters params, PPGSimParams simParams, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);
	
	/* Environment cells */
	dataSizes->cells_grass_alive = (simParams.size_xy + args.vwint) * sizeof(cl_uchar); /** @todo verify that the extra args.vwint size is properly summed (reduced) as zero */
	dataSizes->cells_grass_timer = simParams.size_xy * sizeof(cl_ushort);
	dataSizes->cells_agents_index_start = simParams.size_xy * sizeof(cl_uint);
	dataSizes->cells_agents_index_end = simParams.size_xy * sizeof(cl_uint);
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local = lws.reduce_grass1 * args.vwint * sizeof(cl_uint); /** @todo Verify that GPU supports this local memory requirement */
	dataSizes->reduce_grass_global = gws.reduce_grass2 * args.vwint * sizeof(cl_uint);
	
	/* Rng seeds */
	dataSizes->rng_seeds = MAX_GWS * sizeof(cl_ulong);

}

// Initialize host buffers
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes* dataSizes, PPParameters params, GRand* rng) {
	
	
	/** @todo check if params is necessary here */
	params = params;
	
	/* Statistics */
	buffersHost->stats = (PPStatistics*) malloc(dataSizes->stats);

	/* RNG seeds */
	buffersHost->rng_seeds = (cl_ulong*) malloc(dataSizes->rng_seeds);
	for (int i = 0; i < MAX_GWS; i++) {
		buffersHost->rng_seeds[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}	
	
}

// Free host buffers
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost) {
	free(buffersHost->stats);
	free(buffersHost->rng_seeds);
}

// Initialize device buffers
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersDevice* buffersDevice, PPGDataSizes* dataSizes, GError** err) {
	
	cl_int status;
	
#ifdef PPG_LIMITS_SHOW
	size_t dev_mem = sizeof(PPStatistics) +
		dataSizes->cells_grass_alive +
		dataSizes->cells_grass_timer + 
		dataSizes->cells_agents_index_start +
		dataSizes->cells_agents_index_end +
		dataSizes->reduce_grass_global + 
		dataSizes->rng_seeds;
	printf("Required global memory: %d bytes (%d Kb = %d Mb)\n", (int) dev_mem, (int) dev_mem / 1024, (int) dev_mem / 1024 / 1024);
	printf("Required local memory for reduceGrass kernels: %d bytes (%d Kb)\n", (int) dataSizes->reduce_grass_local, (int) dataSizes->reduce_grass_local / 1024);
#endif	
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(PPStatistics), NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: stats (OpenCL error %d)", status);

	/* Cells */
	buffersDevice->cells_grass_alive = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_grass_alive, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass_alive (OpenCL error %d)", status);

	buffersDevice->cells_grass_timer = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_grass_timer, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_grass_timer (OpenCL error %d)", status);

	buffersDevice->cells_agents_index_start = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_agents_index_start, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_number (OpenCL error %d)", status);

	buffersDevice->cells_agents_index_end = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_agents_index_end, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: cells_agents_index (OpenCL error %d)", status);

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->reduce_grass_global, NULL, &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer: reduce_grass_global (OpenCL error %d)", status);

	/* RNG seeds. */
	buffersDevice->rng_seeds = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->rng_seeds, NULL, &status );
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

// Free device buffers
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice) {
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->cells_grass_alive) clReleaseMemObject(buffersDevice->cells_grass_alive);
	if (buffersDevice->cells_grass_timer) clReleaseMemObject(buffersDevice->cells_grass_timer);
	if (buffersDevice->cells_agents_index_start) clReleaseMemObject(buffersDevice->cells_agents_index_start);
	if (buffersDevice->cells_agents_index_end) clReleaseMemObject(buffersDevice->cells_agents_index_end);
	if (buffersDevice->reduce_grass_global) clReleaseMemObject(buffersDevice->reduce_grass_global);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
}

// Create event data structure
void ppg_events_create(PPParameters params, PPGEvents* evts) {

#ifdef CLPROFILER
	evts->grass = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
#endif

	evts->read_stats = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->reduce_grass2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
}

// Free event data structure
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

/* Create compiler options string. */
char* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGSimParams simParams, gchar* cliOpts) {
	char* compilerOptsStr;
	GString* compilerOpts = g_string_new(PP_KERNEL_INCLUDES);
	g_string_append_printf(compilerOpts, "-D VW_INT=%d ", args.vwint);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKITEMS=%d ", (unsigned int) gws.reduce_grass1);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKGROUPS=%d ", (unsigned int) (gws.reduce_grass1 / lws.reduce_grass1));
	g_string_append_printf(compilerOpts, "-D CELL_NUM=%d ", simParams.size_xy);
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
	*context = g_option_context_new (" - " PPG_DESCRIPTION);
	g_option_context_add_main_entries(*context, entries, NULL);
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
