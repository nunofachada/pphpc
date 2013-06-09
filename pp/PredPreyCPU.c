/** 
 * @file
 * @brief PredPrey OpenCL CPU implementation.
 */
 
#include "PredPreyCPU.h"

#define MAX_AGENTS 16777216
#define NULL_AGENT_POINTER UINT_MAX

#define SHEEP_ID 0
#define WOLF_ID 1

#ifdef CLPROFILER
	#define QUEUE_PROPERTIES CL_QUEUE_PROFILING_ENABLE
#else
	#define QUEUE_PROPERTIES 0
#endif

//#define SEED 0

/** @brief A description of the program. */
static char args_doc[] = "PredPreyCPU -- OpenCL predator-prey simulation for the CPU";

/** @brief The options we understand. */
static struct argp_option args_options[] = {
	{"params",     'p', "FILE",  0, "Specify parameters file (default is " DEFAULT_PARAMS_FILE ")"},
	{"stats",      's', "FILE",  0, "Specify statistics output file (default is " DEFAULT_STATS_FILE ")" },
	{"globalsize", 'g', "SIZE",  0, "Global work size (default is maximum possible)" },
	{"localsize",  'l', "SIZE",  0, "Local work size (default is selected by OpenCL runtime)" },
	{"device",     'd', "INDEX", 0, "Device index (if not given and more than one device is available, chose device from menu)" },
	{ 0 }
};

/** @brief Argument parser. */
static struct argp argp = { args_options, ppc_opt_parse, 0, args_doc };

/** @brief OpenCL kernel files */
static const char* kernelFiles[] = {"pp/PredPreyCommon_Kernels.cl", "pp/PredPreyCPU_Kernels.cl"};

/**
 *  @brief Main program.
 * */
int main(int argc, char ** argv) {

	/* Program vars. */
	PPCWorkSizes workSizes;
	PPCKernels krnls = {NULL, NULL};
	PPCEvents evts = {NULL, NULL, NULL};
	PPCDataSizes dataSizes;
	PPCBuffersHost buffersHost = {NULL, NULL, NULL, NULL, NULL};
	PPCBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL};
	PPParameters params;
	PPCSimParams simParams;
	CLUZone zone;
	
	/* Random number generator- */
	GRand* rng;
	
	/* Status var aux */
	cl_int status_cl;
	int status_pp;
	
	/* Error management */
	GError *err = NULL;

	/* Program exit status. */
	int exit_status = PP_SUCCESS;
	
	/* Program arguments and default values. */
	PPCArgs args_values = { DEFAULT_PARAMS_FILE, DEFAULT_STATS_FILE, 0, 0, -1};
	
	/* Parse arguments. */
	argp_parse(&argp, argc, argv, 0, 0, &args_values);
	
	/* Create RNG and set seed. */
#ifdef SEED
	rng = g_rand_new_with_seed(SEED);
#else
	rng = g_rand_new();
#endif	

	/* Profiling / Timmings. */
	ProfCLProfile* profile = profcl_profile_new();
	
	/* Get the required CL zone. */
	status_cl = clu_zone_new(&zone, CL_DEVICE_TYPE_CPU, 1, QUEUE_PROPERTIES, clu_menu_device_selector, (args_values.dev_idx != -1 ? &args_values.dev_idx : NULL), &err);
	clu_if_error_goto(status_cl, err, error);

	/* Build program. */
	status_cl = clu_program_create(&zone, kernelFiles, 2, NULL, &err);
	clu_if_error_goto(status_cl, err, error);

	/* Get simulation parameters */
	status_pp = pp_load_params(&params, args_values.params, &err);
	pp_if_error_goto(status_pp, err, error);
	
	/* Determine number of threads to use based on compute capabilities and user arguments */
	status_pp = ppc_worksizes_calc(args_values, &workSizes, zone.cu, params.grid_y, &err);
	pp_if_error_goto(status_pp, err, error);

	/* Set simulation parameters in a format more adequate for this program. */
	simParams = ppc_simparams_init(params, NULL_AGENT_POINTER, workSizes.rows_per_workitem);
		
	/* Print thread info to screen */
	ppc_worksize_info_print(zone.cu, workSizes);
	
	/* Create kernels. */
	status_cl = ppc_kernels_create(zone.program, &krnls, &err);
	clu_if_error_goto(status_cl, err, error);

	/* Determine size in bytes for host and device data structures. */
	ppc_datasizes_get(params, simParams, &dataSizes, workSizes.gws);
	
	/* Create events data structure. */
	ppc_events_create(params, &evts);

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Initialize and map host/device buffers */
	status_cl = ppc_buffers_init(zone, workSizes.gws, &buffersHost, &buffersDevice, dataSizes, &evts, params, simParams, rng, &err);
	clu_if_error_goto(status_cl, err, error);	
	
	/*  Set fixed kernel arguments. */
	status_cl = ppc_kernelargs_set(&krnls, &buffersDevice, simParams, &err);
	clu_if_error_goto(status_cl, err, error);

	/* Simulation!! */
	status_cl = ppc_simulate(workSizes, params, zone, krnls, &evts, dataSizes, buffersHost, buffersDevice, &err);
	clu_if_error_goto(status_cl, err, error);

	/* Get statistics. */
	status_cl = ppc_stats_get(args_values.stats, zone, &buffersHost, &buffersDevice, dataSizes, &evts, params, &err);
	clu_if_error_goto(status_cl, err, error);
	
	/* Guarantee all activity has terminated... */
	status_cl = clFinish(zone.queues[0]);
	clu_if_error_create_error_goto(status_cl, &err, error, "Finish for queue 0 after simulation");
	
	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  

	/* Analyze events, show profiling info. */
	status_cl = ppc_profiling_analyze(profile, &evts, params, &err);
	clu_if_error_goto(status_cl, err, error);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	/* Check what type of error we have. */
	if (err->domain == CLU_UTILS_ERROR) {
		/* It's an OpenCL error. */
		exit_status = PP_OPENCL_ERROR;
		fprintf(stderr, "OpenCL error #%d with message \"%s\"\n", err->code, err->message);
		if (zone.build_log) clu_build_log_print(&zone);
	} else {
		/* It's a PredatorPrey simulation error. */
		exit_status = err->code;
		fprintf(stderr, "%s\n", err->message);
	}
	g_error_free(err);

cleanup:

	/* Free stuff! */ //printf("Press enter to free memory..."); getchar();
	
	/* Free events */
	ppc_events_free(params, &evts); 
	
	/* Release OpenCL kernels */
	ppc_kernels_free(&krnls);

	/* Release OpenCL memory objects. This also frees host buffers 
	 * because of CL_MEM_ALLOC_HOST_PTR (I think). If we try to 
	 * free() the host buffers we will have a kind of segfault. */
	ppc_devicebuffers_free(&buffersDevice);

	/* Release program, command queues and context */
	clu_zone_free(&zone);
	
	/* Free profile data structure */
	profcl_profile_free(profile);
	
	/* Free compiler options. */
	/** @todo free(compilerOpts); */
	
	/* Free RNG */
	g_rand_free(rng);
		
	//printf("Press enter to bail out..."); getchar();

	/* See ya. */
	return exit_status;
}

/**
 * @brief Determine effective worksizes to use in simulation.
 * */
int ppc_worksizes_calc(PPCArgs args, PPCWorkSizes* workSizes, cl_uint cu, unsigned int num_rows, GError **err) {
	
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
			pp_if_error_create_error_return(PP_INVALID_ARGS, err, "Global work size is too large for model parameters. Maximum size is %d.", (int) workSizes->max_gws);
		}
	} else {
		/* User did not specify a value, thus find a good one (which will be 
		 * the largest power of 2 bellow or equal to the maximum. */
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
	if ((workSizes->lws > 0) && (workSizes->gws % workSizes->lws != 0))
		pp_if_error_create_error_return(PP_INVALID_ARGS, err, "Global work size (%d) is not multiple of local work size (%d).", (int) workSizes->gws, (int) workSizes->lws);
	
	/* Everything Ok! */
	return PP_SUCCESS;
}

/**
 * @brief Print information about number of threads / work-items and compute units.
 * */
void ppc_worksize_info_print(cl_int cu, PPCWorkSizes workSizes) {
	printf("-------- Compute Parameters --------\n");	
	printf("Compute units in selected device: %d\n", cu);
	printf("Global work size: %d (maximum is %d)\n", (int) workSizes.gws, (int) workSizes.max_gws);
	printf("Local work size: %d%s\n", (int) workSizes.lws, (workSizes.lws == 0 ? " (let OpenCL implementation chose)" : ""));
	printf("Rows per work-item: %d\n", (int) workSizes.rows_per_workitem);
}

/**
 * @brief Get kernel entry points.
 * */
cl_int ppc_kernels_create(cl_program program, PPCKernels* krnls, GError** err) {
	
	cl_int status;
	
	krnls->step1 = clCreateKernel(program, "step1", &status);
	clu_if_error_create_error_return(status, err, "Create kernel: step1");
	
	krnls->step2 = clCreateKernel(program, "step2", &status);
	clu_if_error_create_error_return(status, err, "Create kernel: step2");
	
	return CL_SUCCESS;
}

/**
 * @brief Release kernels. 
 * */
void ppc_kernels_free(PPCKernels* krnls) {
	if (krnls->step1) clReleaseKernel(krnls->step1); 
	if (krnls->step2) clReleaseKernel(krnls->step2);
}

/**
 * @brief Initialize simulation parameters in host, to be sent to kernels.
 * */
PPCSimParams ppc_simparams_init(PPParameters params, cl_uint null_agent_pointer, size_t lines_per_thread) {
	PPCSimParams simParams;
	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = MAX_AGENTS;
	simParams.null_agent_pointer = null_agent_pointer;
	simParams.grass_restart = params.grass_restart;
	simParams.lines_per_thread = (cl_uint) lines_per_thread;	
	return simParams;
}

/**
 * @brief Determine buffer sizes. 
 * */
void ppc_datasizes_get(PPParameters params, PPCSimParams simParams, PPCDataSizes* dataSizes, size_t num_threads) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);
	
	/* Matrix */
	dataSizes->matrix = params.grid_x * params.grid_y * sizeof(PPCCell);
	
	/* Agents. */
	dataSizes->agents = MAX_AGENTS * sizeof(PPCAgent);
	
	/* Rng seeds */
	dataSizes->rng_seeds = num_threads * sizeof(cl_ulong);

	/* Agent parameters */
	dataSizes->agent_params = 2 * sizeof(PPAgentParams);

	/* Simulation parameters */
	dataSizes->sim_params = sizeof(PPCSimParams);

}

/**
 * @brief Initialize and map host/device buffers.
 * */
cl_int ppc_buffers_init(CLUZone zone, size_t num_threads, PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, PPCSimParams simParams, GRand* rng, GError** err) {
	
	/* Aux. variable */
	cl_int status;
	
	/* ************************* */
	/* Initialize device buffers */
	/* ************************* */
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.stats, NULL, &status );
	clu_if_error_create_error_return(status, err, "Creating buffersDevice->stats");
	
	/* Grass matrix */
	buffersDevice->matrix = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.matrix, NULL, &status );
	clu_if_error_create_error_return(status, err, "Creating buffersDevice->matrix");

	/* Agent array */
	buffersDevice->agents = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.agents, NULL, &status );
	clu_if_error_create_error_return(status, err, "Creating buffersDevice->agents");

	/* Random number generator array of seeds */
	buffersDevice->rng_seeds = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSizes.rng_seeds, NULL, &status );
	clu_if_error_create_error_return(status, err, "Creating buffersDevice->rng_seeds");

	/* Agent parameters */
	buffersDevice->agent_params = clCreateBuffer(zone.context, CL_MEM_READ_ONLY  | CL_MEM_ALLOC_HOST_PTR, dataSizes.agent_params, NULL, &status );
	clu_if_error_create_error_return(status, err, "buffersDevice->agent_params");

	/* Simulation parameters */
	buffersDevice->sim_params = clCreateBuffer(zone.context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, dataSizes.sim_params, &simParams, &status );
	clu_if_error_create_error_return(status, err, "buffersDevice->sim_params");

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
	clu_if_error_create_error_return(status, err, "Map buffersHost->stats");

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
	clu_if_error_create_error_return(status, err, "Map buffersHost->matrix");

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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->stats");

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
	clu_if_error_create_error_return(status, err, "Map buffersHost->agents");

	for(cl_uint i = 0; i < MAX_AGENTS; i++) 	{
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
			/* No more agents to initialize, initialize array position only. */
			buffersHost->agents[i].energy = 0;
			buffersHost->agents[i].type = 0;
			buffersHost->agents[i].action = 0;
			buffersHost->agents[i].next = NULL_AGENT_POINTER;
			break;
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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->agents");

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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->matrix");

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
	clu_if_error_create_error_return(status, err, "Map buffersHost->rng_seeds");

	for (unsigned int i = 0; i < num_threads; i++) {
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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->rng_seeds");

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
	clu_if_error_create_error_return(status, err, "Map buffersHost->agent_params");

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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->agent_params"); 

	return status;
	
}

/**
 * @brief Set fixed kernel arguments. 
 * */
cl_int ppc_kernelargs_set(PPCKernels* krnls, PPCBuffersDevice* buffersDevice, PPCSimParams simParams, GError** err) {
	
	/* Aux. var. */
	cl_int status;
	
	/* Step1 kernel - Move agents, grow grass */
	status = clSetKernelArg(krnls->step1, 0, sizeof(cl_mem), (void *) &buffersDevice->agents);
	clu_if_error_create_error_return(status, err, "Arg 0 of step1_kernel");

	status = clSetKernelArg(krnls->step1, 1, sizeof(cl_mem), (void *) &buffersDevice->matrix);
	clu_if_error_create_error_return(status, err, "Arg 1 of step1_kernel");

	status = clSetKernelArg(krnls->step1, 2, sizeof(cl_mem), (void *) &buffersDevice->rng_seeds);
	clu_if_error_create_error_return(status, err, "Arg 2 of step1_kernel");

	status = clSetKernelArg(krnls->step1, 4, sizeof(cl_mem), (void *) &buffersDevice->sim_params);
	clu_if_error_create_error_return(status, err, "Arg 4 of step1_kernel");

	/* Step2 kernel - Agent actions, get stats */
	status = clSetKernelArg(krnls->step2, 0, sizeof(cl_mem), (void *) &buffersDevice->agents);
	clu_if_error_create_error_return(status, err, "Arg 0 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 1, sizeof(cl_mem), (void *) &buffersDevice->matrix);
	clu_if_error_create_error_return(status, err, "Arg 1 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 2, sizeof(cl_mem), (void *) &buffersDevice->rng_seeds);
	clu_if_error_create_error_return(status, err, "Arg 2 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 3, sizeof(cl_mem), (void *) &buffersDevice->stats);
	clu_if_error_create_error_return(status, err, "Arg 3 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 6, sizeof(cl_mem), (void *) &buffersDevice->sim_params);
	clu_if_error_create_error_return(status, err, "Arg 6 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 7, sizeof(cl_mem), (void *) &buffersDevice->agent_params);
	clu_if_error_create_error_return(status, err, "Arg 7 of step2_kernel");

	/* Everything Ok. */
	return CL_SUCCESS;
}

/**
 * @brief Perform simulation!
 * */
cl_uint ppc_simulate(PPCWorkSizes workSizes, PPParameters params, CLUZone zone, PPCKernels krnls, PPCEvents* evts, PPCDataSizes dataSizes, PPCBuffersHost buffersHost, PPCBuffersDevice buffersDevice, GError** err) {
	
	/* Aux. vars. */
	cl_int status;	

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
			clu_if_error_create_error_return(status, err,  "Arg 3 of step1_kernel");
			
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
			clu_if_error_create_error_return(status, err, "step1_kernel");

		}

		/* Step 2:  Agent actions, get stats */
		status = clSetKernelArg(krnls.step2, 4, sizeof(cl_uint), (void *) &iter);
		clu_if_error_create_error_return(status, err, "Arg 4 of step2_kernel");

		for (cl_uint turn = 0; turn < workSizes.rows_per_workitem; turn++ ) {

			/* Set turn on step2_kernel */
			status = clSetKernelArg(krnls.step2, 5, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_return(status, err, "Arg 5 of step2_kernel");
			
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
			clu_if_error_create_error_return(status, err, "step2_kernel");
			
		}

	}

	/* Everything Ok. */
	return CL_SUCCESS;
}

/** 
 * @brief Release OpenCL memory objects.
 * */
void ppc_devicebuffers_free(PPCBuffersDevice* buffersDevice) {
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->agents) clReleaseMemObject(buffersDevice->agents);
	if (buffersDevice->matrix) clReleaseMemObject(buffersDevice->matrix);
	if (buffersDevice->stats) clReleaseMemObject(buffersDevice->stats);
	if (buffersDevice->rng_seeds) clReleaseMemObject(buffersDevice->rng_seeds);
	if (buffersDevice->sim_params) clReleaseMemObject(buffersDevice->sim_params);
}

/** 
 * @brief Create events data structure. 
 * */
void ppc_events_create(PPParameters params, PPCEvents* evts) {

#ifdef CLPROFILER
	evts->step1 = (cl_event*) calloc(params.iters, sizeof(cl_event));
	evts->step2 = (cl_event*) calloc(params.iters, sizeof(cl_event));
#endif

}

/** 
 * @brief Free events data structure. 
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

/** 
 * @brief Analyze events, show profiling info. 
 * */
cl_int ppc_profiling_analyze(ProfCLProfile* profile, PPCEvents* evts, PPParameters params, GError** err) {

#ifdef CLPROFILER
	
	/* Perfomed detailed analysis onfy if profiling flag is set. */
	cl_int status;
	
	/* One time events. */
	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap stats start", evts->map_stats_start, evts->unmap_stats_start, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_stats_start");

	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap matrix", evts->map_matrix, evts->unmap_matrix, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_matrix");

	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap agents", evts->map_agents, evts->unmap_agents, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_agents");

	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap rng_seeds", evts->map_rng_seeds, evts->unmap_rng_seeds, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_rng_seeds");

	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap agent_params", evts->map_agent_params, evts->unmap_agent_params, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_agent_params");

	profcl_profile_add(profile, profcl_evinfo_composite_get("Map/unmap stats end", evts->map_stats_end, evts->unmap_stats_end, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: map/unmap_stats_end");


	/* Simulation loop events. */
	for (guint i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("Step1", evts->step1[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: step1[%d]", i);

		profcl_profile_add(profile, profcl_evinfo_get("Step2", evts->step2[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: step2[%d]", i);
	}
	/* Analyse event data. */
	profcl_profile_aggregate(profile);
	profcl_profile_overmat(profile);	
#endif

	/* Show profiling info. */
	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME);
	
	/* Success. */
	return CL_SUCCESS;
}

/**
 * @brief Get statistics.
 * */
cl_int ppc_stats_get(char* filename, CLUZone zone, PPCBuffersHost* buffersHost, PPCBuffersDevice* buffersDevice, PPCDataSizes dataSizes, PPCEvents* evts, PPParameters params, GError** err) {
	
	/* Aux. vars. */
	cl_int status;	
	
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
	clu_if_error_create_error_return(status, err, "Map buffersHost.stats");

	/* Output results to file */
	FILE * fp1 = fopen(filename, "w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", buffersHost->stats[i].sheep, buffersHost->stats[i].wolves, buffersHost->stats[i].grass );
	fclose(fp1);

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
	clu_if_error_create_error_return(status, err, "Unmap buffersHost.stats");

	return CL_SUCCESS;
}

/** 
 * @brief Parse one command-line option. 
 * */
error_t ppc_opt_parse(int key, char *arg, struct argp_state *state) {

	/* The input argument from argp_parse is a pointer to a PPCArgs structure. */
	PPCArgs *args = state->input;
     
	switch (key) {
	case 'p':
		args->params = arg;
		break;
	case 's':
		args->stats = arg;
		break;
	case 'g':
		args->gws = (size_t) atoi(arg);
		break;
	case 'l':
		args->lws = (size_t) atoi(arg);
		break;
	case 'd':
		args->dev_idx = (cl_uint) atoi(arg);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
