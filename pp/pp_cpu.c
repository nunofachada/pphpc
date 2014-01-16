/** 
 * @file
 * @brief PredPrey OpenCL CPU implementation.
 */
 
#include "pp_cpu.h"

/** The default maximum number of agents: 16777216. Each agent requires
 * 16 bytes, thus by default 256Mb of memory will be allocated for the
 * agents buffer. A higher value may lead to faster simulations
 * given that it will increase the success rate of new agent allocations,
 * but on the other hand memory allocation and initialization may take
 * longer. */
#define PPC_DEFAULT_MAX_AGENTS 16777216

/** A description of the program. */
#define PPC_DESCRIPTION "OpenCL predator-prey simulation for the CPU"

/** Command line arguments and respective default values. */
static PPCArgs args = {NULL, NULL, NULL, 0, 0, -1, PP_DEFAULT_SEED, NULL, PPC_DEFAULT_MAX_AGENTS};

/** Valid command line options. */
static GOptionEntry entries[] = {
	{"params",          'p', 0, G_OPTION_ARG_FILENAME, &args.params,        "Specify parameters file (default is " PP_DEFAULT_PARAMS_FILE ")",                           "FILENAME"},
	{"stats",           's', 0, G_OPTION_ARG_FILENAME, &args.stats,         "Specify statistics output file (default is " PP_DEFAULT_STATS_FILE ")",                     "FILENAME"},
	{"compiler",        'c', 0, G_OPTION_ARG_STRING,   &args.compiler_opts, "Extra OpenCL compiler options",                                                             "OPTS"},
	{"globalsize",      'g', 0, G_OPTION_ARG_INT,      &args.gws,           "Global work size (default is maximum possible)",                                            "SIZE"},
	{"localsize",       'l', 0, G_OPTION_ARG_INT,      &args.lws,           "Local work size (default is selected by OpenCL runtime)",                                   "SIZE"},
	{"device",          'd', 0, G_OPTION_ARG_INT,      &args.dev_idx,       "Device index (if not given and more than one device is available, chose device from menu)", "INDEX"},
	{"rng_seed",        'r', 0, G_OPTION_ARG_INT,      &args.rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",                    "SEED"},
	{"rngen",           'n', 0, G_OPTION_ARG_STRING,   &args.rngen,         "Random number generator: " PP_RNGS,                                                         "RNG"},
	{"max_agents",      'm', 0, G_OPTION_ARG_INT,      &args.max_agents,    "Maximum number of agents (default is " STR(PPC_DEFAULT_MAX_AGENTS) ")",                     "SIZE"},
	{G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_CALLBACK, pp_args_fail,       NULL,                                                                                         NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** OpenCL kernel files. */
static const char* kernelFiles[] = {"pp/pp_cpu.cl"};

/** Information about the requested random number generation algorithm. */
static PPRngInfo rng_info = {NULL, NULL, 0};

/**
 * @brief Main program.
 * 
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program 
 * terminates successfully, or another value of #pp_error_codes if an
 * error occurs.
 * */
int main(int argc, char ** argv) {

	/* Status var aux */
	int status = PP_SUCCESS;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Predator-Prey simulation data structures. */
	PPCWorkSizes workSizes;
	PPCKernels krnls = {NULL, NULL};
	PPCEvents evts = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPCDataSizes dataSizes;
	PPCBuffersHost buffersHost = {NULL, NULL, NULL, NULL, NULL, {0, 0, 0, 0, 0, 0, 0, 0}};
	PPCBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL};
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
	ppc_args_parse(argc, argv, &context, &err);
	gef_if_error_goto(err, PP_UNKNOWN_ARGS, status, error_handler);
	
	/* Validate arguments. */
	if (!args.rngen) args.rngen = g_strdup(PP_DEFAULT_RNG);
	PP_ALG_GET(rng_info, rng_infos, args.rngen);
	gef_if_error_create_goto(err, PP_ERROR, !rng_info.tag, PP_INVALID_ARGS, error_handler, "Unknown random number generator '%s'.", args.rngen);
	
	/* Create RNG with specified seed. */
	rng = g_rand_new_with_seed(args.rng_seed);

	/* Profiling / Timmings. */
	profile = profcl_profile_new();
	gef_if_error_create_goto(err, PP_ERROR, profile == NULL, PP_LIBRARY_ERROR, error_handler, "Unable to create profiler object.");
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_CPU, 1, QUEUE_PROPERTIES, clu_menu_device_selector, (args.dev_idx != -1 ? &args.dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Build compiler options. */
	compilerOpts = ppc_compiler_opts_build(args.compiler_opts);
	
	/* Build program. */
	status = clu_program_create(zone, kernelFiles, 1, compilerOpts, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);

	/* Get simulation parameters */
	status = pp_load_params(&params, args.params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/* Determine number of threads to use based on compute capabilities and user arguments */
	status = ppc_worksizes_calc(args, &workSizes, params.grid_y, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Set simulation parameters for passing to the OpenCL kernels. */
	buffersHost.sim_params = ppc_simparams_init(params, NULL_AGENT_POINTER, workSizes);
		
	/* Print simulation info to screen */
	ppc_simulation_info_print(zone->cu, workSizes, args, compilerOpts);
	
	/* Create kernels. */
	status = ppc_kernels_create(zone->program, &krnls, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Determine size in bytes for host and device data structures. */
	ppc_datasizes_get(params, &dataSizes, workSizes);

#ifdef CLPROFILER	
	/* Create events data structure. */
	ppc_events_create(params, &evts);
#endif

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Initialize and map host/device buffers */
	status = ppc_buffers_init(*zone, workSizes, &buffersHost, &buffersDevice, dataSizes, &evts, params, rng, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);
	
	/*  Set fixed kernel arguments. */
	status = ppc_kernelargs_set(&krnls, &buffersDevice, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Simulation!! */
	status = ppc_simulate(workSizes, params, *zone, krnls, &evts, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Get statistics. */
	status = ppc_stats_save(args.stats, *zone, &buffersHost, &buffersDevice, dataSizes, &evts, params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

	/* Guarantee all activity has terminated... */
	status = clFinish(zone->queues[0]);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Finish for queue 0 after simulation");
	
	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  

	/* Analyze events, show profiling info. */
	status = ppc_profiling_analyze(profile, &evts, params, &err);
	gef_if_error_goto(err, GEF_USE_STATUS, status, error_handler);

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

	/* Free stuff! */  
	
#ifdef CLPROFILER
	/* Free events */
	ppc_events_free(params, &evts); 
#endif
	
	/* Release OpenCL kernels */
	ppc_kernels_free(&krnls);

	/* Release OpenCL memory objects. This also frees host buffers 
	 * because of CL_MEM_ALLOC_HOST_PTR (I think). If we try to 
	 * free() the host buffers we will have a kind of segfault. */
	ppc_devicebuffers_free(&buffersDevice);
	
	/* Free context and associated cli args parsing buffers. */
	ppc_args_free(context);

	/* Release program, command queues and context */
	if (zone != NULL) clu_zone_free(zone);
	
	/* Free profile data structure */
	if (profile != NULL) profcl_profile_free(profile);
	
	/* Free RNG */
	if (rng != NULL) g_rand_free(rng);
		
	/* See ya. */
	return status;
}

/**
 * @brief Determine effective worksizes to use in simulation.
 * 
 * @param args Parsed command line arguments.
 * @param workSizes Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * @param num_rows Number of rows in (height of) simulation environment.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_INVALID_ARGS @endlink if parsed command line 
 * arguments are not valid for the current simulation.
 * */
int ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes, unsigned int num_rows, GError **err) {
	
	/* Aux. var. */
	int status = PP_SUCCESS;
	
	/* Determine maximum number of global work-items which can be used for current 
	 * problem (each pair of work-items must process rows which are separated 
	 * by two rows not being processed) */
	workSizes->max_gws = num_rows / 3;
	
	/* Determine effective number of global work-items to use. */
	if (args.gws > 0) {
		/* User specified a value, let's see if it's possible to use it. */
		if (workSizes->max_gws >= args.gws) {
			/* Yes, it's possible. */
			workSizes->gws = args.gws;
		} else {
			/* No, specified value is too large for the given problem. */
			gef_if_error_create_goto(*err, PP_ERROR, 1, PP_INVALID_ARGS, error_handler, "Global work size is too large for model parameters. Maximum size is %d.", (int) workSizes->max_gws);
		}
	} else {
		/* User did not specify a value, thus find a good one (which will be 
		 * the largest power of 2 bellow or equal to the maximum. */
		 /** @todo Should not be nlpo but in increments of LWS. */
		unsigned int maxgws = nlpo2(workSizes->max_gws);
		if (maxgws > workSizes->max_gws)
			workSizes->gws = maxgws / 2;
		else
			workSizes->gws = maxgws;
	}
	/* Determine the rows to be computed per thread. */
	workSizes->rows_per_workitem = num_rows / workSizes->gws + (num_rows % workSizes->gws > 0);
	
	/* Get local work size. */
	workSizes->lws = args.lws;
	
	/* Check that the global work size is a multiple of the local work size. */
	if ((workSizes->lws > 0) && (workSizes->gws % workSizes->lws != 0)) {
		gef_if_error_create_goto(*err, PP_ERROR, 1, PP_INVALID_ARGS, error_handler, "Global work size (%d) is not multiple of local work size (%d).", (int) workSizes->gws, (int) workSizes->lws);
	}
	
	/* Set maximum number of agents. */
	workSizes->max_agents = args.max_agents;
	
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
 * @brief Create compiler options string. 
 * 
 * @param cliOpts Compiler options passed through the command-line.
 * @return Final compiler options to be passed to OpenCL compiler.
 * */
gchar* ppc_compiler_opts_build(gchar* cliOpts) {
	gchar *compilerOptsStr;
	GString* compilerOpts = g_string_new(PP_KERNEL_INCLUDES);
	g_string_append_printf(compilerOpts, "-D %s ", rng_info.compiler_const);
	if (cliOpts) g_string_append_printf(compilerOpts, "%s", cliOpts);
	compilerOptsStr = compilerOpts->str;
	g_string_free(compilerOpts, FALSE);
	return compilerOptsStr;
}

/**
 * @brief Print information about the simulation parameters.
 * 
 * @param cu Number of computer units available in selected device.
 * @param workSizes Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * @param args Parsed command line arguments.
 * @param compilerOpts Processed compiler options.
 * */
void ppc_simulation_info_print(cl_int cu, PPCWorkSizes workSizes, PPCArgs args, gchar* compilerOpts) {
	/* ...Header */
	printf("\n   ========================= Computational settings ======================== \n\n");	
	/* ...Compute units */
	printf("     Compute units in device    : %d\n", cu);	
	/* ...Global worksize */
	printf("     Global work size (max)     : %d (%d)\n", (int) workSizes.gws, (int) workSizes.max_gws);
	/* ...Local worksize */
	printf("     Local work size            : ");
	if (workSizes.lws == 0) printf("auto\n");
	else printf("%d\n", (int) workSizes.lws);
	/* ...Rows per workitem */
	printf("     Rows per work-item         : %d\n", (int) workSizes.rows_per_workitem);
	/* ...Maximum number of agents */
	printf("     Maximum number of agents   : %d\n", (int) workSizes.max_agents);
	/* ...RNG seed */
	printf("     Random seed                : %u\n", args.rng_seed);
	/* ...Compiler options (out of table) */
	printf("     Compiler options           : ");
	if (compilerOpts != NULL) printf("%s\n", compilerOpts);
	else printf("none\n");
	/* ...Finish table. */
	
}

/**
 * @brief Get kernel entry points.
 * 
 * @param program OpenCL program object.
 * @param krnls OpenCL simulation kernels.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_kernels_create(cl_program program, PPCKernels* krnls, GError** err) {
	
	/* Aux. var. */
	int status;
	
	/* Create kernels. */
	krnls->step1 = clCreateKernel(program, "step1", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: step1");	
	
	krnls->step2 = clCreateKernel(program, "step2", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: step2");	

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
 * @brief Release kernels. 
 * 
 * @param krnls OpenCL simulation kernels.
 * */
void ppc_kernels_free(PPCKernels* krnls) {
	if (krnls->step1) clReleaseKernel(krnls->step1); 
	if (krnls->step2) clReleaseKernel(krnls->step2);
}

/**
 * @brief Initialize simulation parameters to be sent to kernels.
 * 
 * @param params Simulation parameters.
 * @param null_agent_pointer Constant which indicates no further agents are in cell.
 * @param ws Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * @return Simulation parameters to be sent to OpenCL kernels.
 * */
PPCSimParams ppc_simparams_init(PPParameters params, cl_uint null_agent_pointer, PPCWorkSizes ws) {
	PPCSimParams simParams;
	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = ws.max_agents;
	simParams.null_agent_pointer = null_agent_pointer;
	simParams.grass_restart = params.grass_restart;
	simParams.rows_per_workitem = (cl_uint) ws.rows_per_workitem;	
	return simParams;
}

/**
 * @brief Determine buffer sizes.
 * 
 * @param params Simulation parameters.
 * @param dataSizes Sizes of simulation data structures.
 * @param ws Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * */
void ppc_datasizes_get(PPParameters params, PPCDataSizes* dataSizes, PPCWorkSizes ws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);
	
	/* Matrix */
	dataSizes->matrix = params.grid_x * params.grid_y * sizeof(PPCCell);
	
	/* Agents. */
	dataSizes->agents = ws.max_agents * sizeof(PPCAgent);
	
	/* Rng seeds */
	dataSizes->rng_seeds = ws.gws * rng_info.bytes;
	dataSizes->rng_seeds_count = dataSizes->rng_seeds / sizeof(cl_ulong);

	/* Agent parameters */
	dataSizes->agent_params = 2 * sizeof(PPAgentParams);

	/* Simulation parameters */
	dataSizes->sim_params = sizeof(PPCSimParams);

}

/**
 * @brief Initialize and map host/device buffers.
 * 
 * @param zone Required objects (context, queues, etc.) for an OpenCL execution session on a specific device.
 * @param ws Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * @param buffersHost Host buffers.
 * @param buffersDevice Device buffers.
 * @param dataSizes Sizes of simulation data structures.
 * @param evts OpenCL events, to be used if profiling is on.
 * @param params Simulation parameters.
 * @param rng Random number generator.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_buffers_init(CLUZone zone, PPCWorkSizes ws, PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GRand* rng, GError** err) {

	/* Avoid compiler warning (unused parameter) when profiling is off. */
#ifndef CLPROFILER
	evts = evts;
#endif	
	
	/* Aux. variable */
	cl_int status = PP_SUCCESS;
	
	/* ************************* */
	/* Initialize device buffers */
	/* ************************* */
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.stats, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->stats");	
	
	/* Grass matrix */
	buffersDevice->matrix = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.matrix, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->matrix");	

	/* Agent array */
	buffersDevice->agents = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.agents, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->agents");	

	/* Random number generator array of seeds */
	buffersDevice->rng_seeds = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.rng_seeds, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->rng_seeds");	

	/* Agent parameters */
	buffersDevice->agent_params = clCreateBuffer(zone.context, CL_MEM_READ_ONLY  | CL_MEM_ALLOC_HOST_PTR, dataSizes.agent_params, NULL, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->agent_params");	

	/* Simulation parameters */
	buffersDevice->sim_params = clCreateBuffer(zone.context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, dataSizes.sim_params, &buffersHost->sim_params, &status );
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Creating buffersDevice->sim_params");	

	/* *********************************************************** */
	/* Initialize host buffers, which are mapped to device buffers */
	/* *********************************************************** */

	/* Initialize statistics buffer */
	buffersHost->stats = (PPStatistics*) clEnqueueMapBuffer(
		zone.queues[0], 
		buffersDevice->stats, 
		CL_TRUE, 
		CL_MAP_WRITE, 
		0, 
		dataSizes.stats, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_stats_start,
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost->stats");	

	buffersHost->stats[0].sheep = params.init_sheep;
	buffersHost->stats[0].wolves = params.init_wolves;
	buffersHost->stats[0].grass = 0;
	for (unsigned int i = 1; i <= params.iters; i++) {
		buffersHost->stats[i].sheep = 0;
		buffersHost->stats[i].wolves = 0;
		buffersHost->stats[i].grass = 0;
	}

	/* Initialize grass matrix */
	buffersHost->matrix = (PPCCell *) clEnqueueMapBuffer(
		zone.queues[0], 
		buffersDevice->matrix, 
		CL_TRUE, 
		CL_MAP_WRITE | CL_MAP_READ, 
		0, 
		dataSizes.matrix, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_matrix, 
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost->matrix");	

	for(cl_uint i = 0; i < params.grid_x; i++) {
		for (cl_uint j = 0; j < params.grid_y; j++) {
			
			/* Determine grid index. */
			cl_uint gridIndex = (i + j*params.grid_x);
			
			/* Initialize grass. */
			cl_uint grassState = g_rand_int_range(rng, 0, 2) == 0 
				? 0 
				: g_rand_int_range(rng, 1, params.grass_restart + 1);
			buffersHost->matrix[gridIndex].grass = grassState;
			if (grassState == 0)
				buffersHost->stats[0].grass++;
			
			/* Initialize agent pointer. */
			buffersHost->matrix[gridIndex].agent_pointer = NULL_AGENT_POINTER;
		}
	}

	/* Unmap stats buffer from device */ 
	status = clEnqueueUnmapMemObject( 
		zone.queues[0], 
		buffersDevice->stats, 
		buffersHost->stats, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->unmap_stats_start
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost->stats");	

	/* Initialize agent array */
	buffersHost->agents = (PPCAgent *) clEnqueueMapBuffer( 
		zone.queues[0], 
		buffersDevice->agents, 
		CL_TRUE, 
		CL_MAP_WRITE, 
		0, 
		dataSizes.agents, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_agents, 
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost->agents");	

	for(cl_uint i = 0; i < ws.max_agents; i++) 	{
		/* Check if there are still agents to initialize. */
		if (i < params.init_sheep + params.init_wolves) {
			
			/* There are still agents to initialize, chose a place to put next agent. */
			cl_uint x = g_rand_int_range(rng, 0, params.grid_x);
			cl_uint y = g_rand_int_range(rng, 0, params.grid_y);
			
			/* Initialize generic agent parameters. */
			buffersHost->agents[i].action = 0;
			buffersHost->agents[i].next = NULL_AGENT_POINTER;
			cl_uint gridIndex = (x + y*params.grid_x);
			if (buffersHost->matrix[gridIndex].agent_pointer == NULL_AGENT_POINTER) {
				/* This cell had no agent, put it there. */
				buffersHost->matrix[gridIndex].agent_pointer = i;
			} else {
				/* Cell already has agent, put it at the end of the list. */
				cl_uint agindex = buffersHost->matrix[gridIndex].agent_pointer;
				while (buffersHost->agents [agindex].next != NULL_AGENT_POINTER)
					agindex = buffersHost->agents [agindex].next;
				buffersHost->agents [agindex].next = i;
			}
			
			/* Perform agent specific initialization. */
			if (i < params.init_sheep) {
				/* Initialize sheep specific parameters. */
				buffersHost->agents[i].energy = g_rand_int_range(rng, 1, params.sheep_gain_from_food * 2 + 1);
				buffersHost->agents[i].type = SHEEP_ID;
			} else {
				/* Initialize wolf specific parameters. */
				buffersHost->agents[i].energy = g_rand_int_range(rng, 1, params.wolves_gain_from_food * 2 + 1);
				buffersHost->agents[i].type = WOLF_ID;
			}

		} else {
			/* No more agents to initialize, initialize array position only. 
			 * OpenCL implementations seem to initialize everything to zero,
			 * but to avoid any bugs on untested implementations we must at 
			 * least initialize agents energy to zero. */
			buffersHost->agents[i].energy = 0;
		}

	}

	/* Unmap agents buffer from device */ 
	status = clEnqueueUnmapMemObject(
		zone.queues[0], 
		buffersDevice->agents, 
		buffersHost->agents, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->unmap_agents
#else
		NULL
#endif
	);	
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost->agents");	

	/* Unmap matrix buffer from device */ 
	status = clEnqueueUnmapMemObject(
		zone.queues[0], 
		buffersDevice->matrix, 
		buffersHost->matrix, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->unmap_matrix
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost->matrix");	

	/* Initialize RNG seeds */
	buffersHost->rng_seeds = (cl_ulong *) clEnqueueMapBuffer(
		zone.queues[0], 
		buffersDevice->rng_seeds, 
		CL_TRUE, 
		CL_MAP_WRITE, 
		0, 
		dataSizes.rng_seeds, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_rng_seeds, 
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost->rng_seeds");	

	for (unsigned int i = 0; i < dataSizes.rng_seeds_count; i++) {
		buffersHost->rng_seeds[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}

	/* Unmap RNG seeds buffer from device */
	status = clEnqueueUnmapMemObject( 
		zone.queues[0], 
		buffersDevice->rng_seeds, 
		buffersHost->rng_seeds, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->unmap_rng_seeds
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost->rng_seeds");	

	/* Initialize agent parameters */
	buffersHost->agent_params = (PPAgentParams *) clEnqueueMapBuffer( 
		zone.queues[0], 
		buffersDevice->agent_params, 
		CL_TRUE, 
		CL_MAP_WRITE, 
		0, 
		dataSizes.agent_params, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_agent_params, 
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost->agent_params");	

	buffersHost->agent_params[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	buffersHost->agent_params[SHEEP_ID].reproduce_threshold = params.sheep_reproduce_threshold;
	buffersHost->agent_params[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	buffersHost->agent_params[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	buffersHost->agent_params[WOLF_ID].reproduce_threshold = params.wolves_reproduce_threshold;
	buffersHost->agent_params[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;

	/* Unmap agent parameters buffer from device. */
	status = clEnqueueUnmapMemObject( 
		zone.queues[0], 
		buffersDevice->agent_params, 
		buffersHost->agent_params, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->unmap_agent_params
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost->agent_params");	

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
 * @brief Set fixed kernel arguments. 
 * 
 * @param krnls OpenCL simulation kernels.
 * @param buffersDevice Device buffers.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_kernelargs_set(PPCKernels* krnls, PPCBuffersDevice* buffersDevice, GError** err) {
	
	/* Aux. var. */
	int status;
	
	/* Step1 kernel - Move agents, grow grass */
	status = clSetKernelArg(krnls->step1, 0, sizeof(cl_mem), (void *) &buffersDevice->agents);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 0 of step1_kernel");	

	status = clSetKernelArg(krnls->step1, 1, sizeof(cl_mem), (void *) &buffersDevice->matrix);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 1 of step1_kernel");	

	status = clSetKernelArg(krnls->step1, 2, sizeof(cl_mem), (void *) &buffersDevice->rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 2 of step1_kernel");	

	status = clSetKernelArg(krnls->step1, 4, sizeof(cl_mem), (void *) &buffersDevice->sim_params);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 4 of step1_kernel");	

	/* Step2 kernel - Agent actions, get stats */
	status = clSetKernelArg(krnls->step2, 0, sizeof(cl_mem), (void *) &buffersDevice->agents);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 0 of step2_kernel");	

	status = clSetKernelArg(krnls->step2, 1, sizeof(cl_mem), (void *) &buffersDevice->matrix);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 1 of step2_kernel");	

	status = clSetKernelArg(krnls->step2, 2, sizeof(cl_mem), (void *) &buffersDevice->rng_seeds);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 2 of step2_kernel");	

	status = clSetKernelArg(krnls->step2, 3, sizeof(cl_mem), (void *) &buffersDevice->stats);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 3 of step2_kernel");	

	status = clSetKernelArg(krnls->step2, 6, sizeof(cl_mem), (void *) &buffersDevice->sim_params);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 6 of step2_kernel");	

	status = clSetKernelArg(krnls->step2, 7, sizeof(cl_mem), (void *) &buffersDevice->agent_params);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 7 of step2_kernel");	

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
 * @brief Perform simulation!
 * 
 * @param workSizes Work sizes for kernels step1 and step2, and other work/memory sizes related to the simulation.
 * @param params Simulation parameters.
 * @param zone Required objects (context, queues, etc.) for an OpenCL execution session on a specific device.
 * @param krnls OpenCL simulation kernels.
 * @param evts OpenCL events, to be used if profiling is on.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_simulate(PPCWorkSizes workSizes, PPParameters params, CLUZone zone, PPCKernels krnls, PPCEvents* evts, GError** err) {

	/* Avoid compiler warning (unused parameter) when profiling is off. */
#ifndef CLPROFILER
	evts = evts;
#endif	
	
	/* Aux. vars. */
	int status;	

	/* Current iteration. */
	cl_uint iter;

    /* If local work group size is not given or is 0, set it to NULL and let OpenCL decide. */ 
	size_t *local_size = (workSizes.lws > 0 ? &workSizes.lws : NULL);
	
	/* Simulation loop! */
	for (iter = 1; iter <= params.iters; iter++) {
		
		/* Step 1:  Move agents, grow grass */
		for (cl_uint turn = 0; turn < workSizes.rows_per_workitem; turn++ ) {
			
			/* Set turn on step1_kernel */
			status = clSetKernelArg(krnls.step1, 3, sizeof(cl_uint), (void *) &turn);
			gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 3 of step1_kernel");	

			/* Run kernel */
			status = clEnqueueNDRangeKernel( 
				zone.queues[0], 
				krnls.step1, 
				1, 
				NULL, 
				&workSizes.gws, 
				local_size, 
				0, 
				NULL, 
#ifdef CLPROFILER
				&evts->step1[iter - 1]
#else
				NULL
#endif
			);
			gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "step1_kernel");	

		}

		/* Step 2:  Agent actions, get stats */
		status = clSetKernelArg(krnls.step2, 4, sizeof(cl_uint), (void *) &iter);
		gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 4 of step2_kernel");	

		for (cl_uint turn = 0; turn < workSizes.rows_per_workitem; turn++ ) {

			/* Set turn on step2_kernel */
			status = clSetKernelArg(krnls.step2, 5, sizeof(cl_uint), (void *) &turn);
			gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Arg 5 of step2_kernel");	
			
			/* Run kernel */
			status = clEnqueueNDRangeKernel(
				zone.queues[0], 
				krnls.step2, 
				1, 
				NULL, 
				&workSizes.gws, 
				local_size, 
				0, 
				NULL, 
#ifdef CLPROFILER
				&evts->step2[iter - 1]
#else
				NULL
#endif
			);
			gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "step2_kernel");	
			
		}

	}

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
 * @brief Release OpenCL memory objects.
 * 
 * @param buffersDevice Device buffers.
 * */
void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice) {
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->agents) clReleaseMemObject(buffersDevice->agents);
	if (buffersDevice->matrix) clReleaseMemObject(buffersDevice->matrix);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
	if (buffersDevice->sim_params) clReleaseMemObject(buffersDevice->sim_params);
}

#ifdef CLPROFILER
/** 
 * @brief Create events data structure. 
 * 
 * @param params Simulation parameters.
 * @param evts OpenCL events, to be used if profiling is on.
 * */
void ppc_events_create(PPParameters params, PPCEvents* evts) {

	evts->step1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->step2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
}

/** 
 * @brief Free events data structure. 
 * 
 * @param params Simulation parameters.
 * @param evts OpenCL events, to be used if profiling is on.
 * */
void ppc_events_free(PPParameters params, PPCEvents* evts) {
	
	if (evts->map_stats_start) clReleaseEvent(evts->map_stats_start);
	if (evts->unmap_stats_start) clReleaseEvent(evts->unmap_stats_start);
	if (evts->map_matrix) clReleaseEvent(evts->map_matrix);
	if (evts->unmap_matrix) clReleaseEvent(evts->unmap_matrix);
	if (evts->map_agents) clReleaseEvent(evts->map_agents);
	if (evts->unmap_agents) clReleaseEvent(evts->unmap_agents);
	if (evts->map_rng_seeds) clReleaseEvent(evts->map_rng_seeds);
	if (evts->unmap_rng_seeds) clReleaseEvent(evts->unmap_rng_seeds);
	if (evts->map_agent_params) clReleaseEvent(evts->map_agent_params);
	if (evts->unmap_agent_params) clReleaseEvent(evts->unmap_agent_params);
	if (evts->map_stats_end) clReleaseEvent(evts->map_stats_end);
	if (evts->unmap_stats_end) clReleaseEvent(evts->unmap_stats_end);
	if (evts->step1) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->step1[i]) clReleaseEvent(evts->step1[i]);
		}
		free(evts->step1);
	}
	if (evts->step2) {
		for (guint i = 0; i < params.iters; i++) {
			if (evts->step2[i]) clReleaseEvent(evts->step2[i]);
		}
		free(evts->step2);
	}
}
#endif

/** 
 * @brief Analyze events, show profiling info. 
 * 
 * @param profile Profiling information object.
 * @param evts OpenCL events, to be used if profiling is on.
 * @param params Simulation parameters.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_profiling_analyze(ProfCLProfile* profile, PPCEvents* evts, PPParameters params, GError** err) {

	/* Status var. */
	int status;
		
#ifdef CLPROFILER
	/* Perfomed detailed analysis onfy if profiling flag is set. */
	
	/* One time events. */
	profcl_profile_add_composite(profile, "Map/unmap stats start", evts->map_stats_start, evts->unmap_stats_start, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add_composite(profile, "Map/unmap matrix", evts->map_matrix, evts->unmap_matrix, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add_composite(profile, "Map/unmap agents", evts->map_agents, evts->unmap_agents, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add_composite(profile, "Map/unmap rng_seeds", evts->map_rng_seeds, evts->unmap_rng_seeds, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add_composite(profile, "Map/unmap agent_params", evts->map_agent_params, evts->unmap_agent_params, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	profcl_profile_add_composite(profile, "Map/unmap stats end", evts->map_stats_end, evts->unmap_stats_end, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

	/* Simulation loop events. */
	for (guint i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, "Step1", evts->step1[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);

		profcl_profile_add(profile, "Step2", evts->step2[i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}
	
	/* Analyse event data. */
	profcl_profile_aggregate(profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
	profcl_profile_overmat(profile, err);
	gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	
#else
	/* Avoid compiler warning (unused parameter) when profiling is off. */
	evts = evts;
	params = params;
	err = err;
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
 * @brief Get statistics.
 * 
 * @param filename File where to save simulation output (statistics).
 * @param zone Required objects (context, queues, etc.) for an OpenCL execution session on a specific device.
 * @param buffersHost Host buffers.
 * @param buffersDevice Device buffers.
 * @param dataSizes Sizes of simulation data structures.
 * @param evts OpenCL events, to be used if profiling is on.
 * @param params Simulation parameters.
 * @param err GLib error object for error reporting.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or @link pp_error_codes::PP_LIBRARY_ERROR @endlink if OpenCL API instructions
 * yield an error.
 * */
int ppc_stats_save(gchar* filename, CLUZone zone, PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GError** err) {
	
	/* Avoid compiler warning (unused parameter) when profiling is off. */
#ifndef CLPROFILER
	evts = evts;
#endif	

	/* Aux. vars. */
	cl_int status;	
	
	/* Stats file. */
	FILE * fp;
	
	/* Get definite file name. */
	gchar* realFilename = (filename != NULL) ? filename : PP_DEFAULT_STATS_FILE;
	
	/* Map stats host buffer in order to get statistics */
	buffersHost->stats = (PPStatistics*) clEnqueueMapBuffer( 
		zone.queues[0], 
		buffersDevice->stats, 
		CL_TRUE, 
		CL_MAP_READ, 
		0, 
		dataSizes.stats, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts->map_stats_end,
#else
		NULL,
#endif
		&status
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Map buffersHost.stats");	

	/* Output results to file */
	fp = fopen(realFilename, "w");
	gef_if_error_create_goto(*err, PP_ERROR, fp == NULL, PP_UNABLE_SAVE_STATS, error_handler, "Unable to open file \"%s\"", realFilename);

	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp, "%d\t%d\t%d\n", buffersHost->stats[i].sheep, buffersHost->stats[i].wolves, buffersHost->stats[i].grass );
	fclose(fp);

	/* Unmap stats host buffer. */
	status = clEnqueueUnmapMemObject( 
		zone.queues[0], 
		buffersDevice->stats, 
		buffersHost->stats, 
		0,
		NULL,
#ifdef CLPROFILER
		&evts->unmap_stats_end
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Unmap buffersHost.stats");	

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
 * @brief Parse command-line options. 
 * 
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 * @param context Context object for command line argument parsing.
 * @param err GLib error object for error reporting.
 * */
void ppc_args_parse(int argc, char* argv[], GOptionContext** context, GError** err) {
	*context = g_option_context_new (" - " PPC_DESCRIPTION);
	g_option_context_add_main_entries(*context, entries, NULL);
	g_option_context_parse(*context, &argc, &argv, err);
}

/**
 * @brief Free command line parsing related objects.
 * 
 * @param context Context object for command line argument parsing.
 * */
void ppc_args_free(GOptionContext* context) {
	if (context) {
		g_option_context_free(context);
	}
	if (args.params) g_free(args.params);
	if (args.stats) g_free(args.stats);
	if (args.compiler_opts) g_free(args.compiler_opts);
	if (args.rngen) g_free(args.rngen);
}
