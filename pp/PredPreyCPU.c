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
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE
#else
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE
#endif

//#define SEED 0

/* OpenCL kernel files */
const char* kernelFiles[] = {"pp/PredPreyCommon_Kernels.cl", "pp/PredPreyCPU_Kernels.cl"};

/**
 *  @brief Main program.
 * */
int main(int argc, char ** argv)
{
	/* Program vars. */
	PPCGlobalWorkSizes gws;
	PPCLocalWorkSizes lws;
	PPCKernels krnls = {NULL, NULL};
	PPCDataSizes dataSizes;
	PPCBuffersHost buffersHost = {NULL, NULL, NULL, NULL, NULL};
	PPCBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL};

	/* Status var aux */
	cl_int status;	
	
	/* Error management */
	GError *err = NULL;
	
	/* Number of threads */
	size_t num_threads, lines_per_thread, num_threads_sugested, num_threads_max;

	/* Create RNG and set seed. */
#ifdef SEED
	GRand* rng = g_rand_new_with_seed(SEED);
#else
	GRand* rng = g_rand_new();
#endif	

	/* Profiling / Timmings. */
	ProfCLProfile* profile = profcl_profile_new();
	
	/* Get the required CL zone. */
	CLUZone zone;
	status = clu_zone_new(&zone, CL_DEVICE_TYPE_CPU, 1, QUEUE_PROPERTIES, &err);
	clu_if_error_goto(status, err, error);

	/* Build program. */
	status = clu_program_create(&zone, kernelFiles, 2, NULL, &err);
	clu_if_error_goto(status, err, error);

	/* Get simulation parameters */
	PPParameters params = pp_load_params(CONFIG_FILE);
	
	/* Determine number of threads to use based on compute capabilities and user arguments */
	if (ppc_numthreads_get(&num_threads, &lines_per_thread, &num_threads_sugested, &num_threads_max, zone.cu, params.grid_y, argc, argv) != 0) {
		printf("Usage: %s [num_threads]\n", argv[0]);
		goto cleanup;
	}

	/* Set simulation parameters in a format more adequate for this program. */
	PPCSimParams simParams = ppc_simparams_init(params, NULL_AGENT_POINTER, lines_per_thread);
		
	/* Print thread info to screen */
	ppc_threadinfo_print(zone.cu, num_threads, lines_per_thread, num_threads_sugested, num_threads_max);
	
	/* Create kernels. */
	status = ppc_kernels_create(zone.program, &krnls, &err);
	clu_if_error_goto(status, err, error);

	/* Determine size in bytes for host and device data structures. */
	ppc_datasizes_get(params, simParams, &dataSizes, num_threads);
	
	/* Initialize and map host/device buffers */
	status = ppc_buffers_init(zone, num_threads, &buffersHost, &buffersDevice, dataSizes, params, rng, &err);
	clu_if_error_goto(status, err, error);	
	
	/*  Set fixed kernel arguments. */
	status = ppc_kernelargs_set(&krnls, &buffersDevice, simParams, &err);
	clu_if_error_goto(status, err, error);

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Simulation!! */
	status = ppc_simulate(num_threads, lines_per_thread, params, zone, krnls, dataSizes, buffersHost, buffersDevice, &err);
	clu_if_error_goto(status, err, error);

	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  
	
	printf("Time stops now!\n");

	buffersHost.stats = (PPStatistics*) clEnqueueMapBuffer( zone.queues[0], buffersDevice.stats, CL_TRUE, CL_MAP_READ, 0, dataSizes.stats, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map buffersHost.stats");

	// 10. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", buffersHost.stats[i].sheep, buffersHost.stats[i].wolves, buffersHost.stats[i].grass );
	fclose(fp1);

	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice.stats, buffersHost.stats, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap buffersHost.stats");

	/* Show profiling info. */
	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	fprintf(stderr, "Error %d: %s\n", err->code, err->message);
	if (zone.build_log) clu_build_log_print(&zone);
	g_error_free(err);

cleanup:
	// 12. Free stuff!
	printf("Press enter to free memory...");
	getchar();
	
	/* Release OpenCL kernels */
	ppc_kernels_free(&krnls);


	//TODO Release events

	/* Free RNG */
	g_rand_free(rng);
		
	// Release memory objects
	if (buffersDevice.stats) clReleaseMemObject(buffersDevice.stats);
	if (buffersDevice.agents) clReleaseMemObject(buffersDevice.agents);
	if (buffersDevice.matrix) clReleaseMemObject(buffersDevice.matrix);
	if (buffersDevice.stats) clReleaseMemObject(buffersDevice.stats);
	if (buffersDevice.rng_seeds) clReleaseMemObject(buffersDevice.rng_seeds);

	// Release program, command queues and context
	clu_zone_free(&zone);
	
	//TODO Don't I need to free host buffers?

	printf("Press enter to bail out...");
	getchar();
	

	return 0;
}

/**
 * @brief Get number of threads to use.
 * 
 * @return 0 if success, -1 if invalid number of arguments was given, 2 if argument was not a positive integer.
 * */
int ppc_numthreads_get(size_t *num_threads, size_t *lines_per_thread, size_t *num_threads_sugested, size_t *num_threads_max, cl_uint cu, unsigned int num_lines, int argc, char* argv[]) {
	
	/* Check if user suggested any number of threads. */ 
	if (argc == 1) {
		/* No number of threads was suggested. */
		*num_threads_sugested = (size_t) cu;
	} else if (argc == 2) {
		/* A number of threads was suggested. */
		*num_threads_sugested = atoi(argv[1]);
		/* Argument was not a positive integer. */
		if (*num_threads_sugested < 1)
			return -2;
	} else {
		/* Invalid number of arguments. */
		return -1;
	}
	
	/* Determine maximum number of threads which can be used for current 
	 * problem (each pair threads must process lines which are separated 
	 * by two lines not being processed) */
	*num_threads_max = num_lines/ 3;
	
	/* Determine effective number of threads to use. */
	*num_threads = MIN(*num_threads_sugested, *num_threads_max);
	
	/* Determine the lines to be computed per thread. */
	*lines_per_thread = num_lines / *num_threads + (num_lines % *num_threads > 0);
	
	/* Return Ok. */
	return 0;

}

/**
 * @brief Print information about number of threads / work-items and compute units.
 * */
void ppc_threadinfo_print(cl_int cu, size_t num_threads, size_t lines_per_thread, size_t num_threads_sugested, size_t num_threads_max) {
	printf("-------- Compute Parameters --------\n");	
	printf("Compute units: %d\n", cu);
	printf("Suggested number of threads: %d\tMaximum number of threads for this problem: %d\n", (int) num_threads_sugested, (int) num_threads_max);
	printf("Effective number of threads: %d\n", (int) num_threads);
	printf("Lines per thread: %d\n", (int) lines_per_thread);
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

}

/**
 * @brief Initialize and map host/device buffers.
 * */
cl_int ppc_buffers_init(CLUZone zone, size_t num_threads, PPCBuffersHost *buffersHost, PPCBuffersDevice *buffersDevice, PPCDataSizes dataSizes, PPParameters params, GRand* rng, GError** err) {
	
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

	/* *********************************************************** */
	/* Initialize host buffers, which are mapped to device buffers */
	/* *********************************************************** */

	/* Initialize statistics buffer */
	buffersHost->stats = (PPStatistics*) clEnqueueMapBuffer( zone.queues[0], buffersDevice->stats, CL_TRUE, CL_MAP_WRITE, 0, dataSizes.stats, 0, NULL, NULL, &status);
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
	buffersHost->matrix = (PPCCell *) clEnqueueMapBuffer( zone.queues[0], buffersDevice->matrix, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, dataSizes.matrix, 0, NULL, NULL, &status);
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
	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice->stats, buffersHost->stats, 0, NULL, NULL);
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->stats");

	/* Initialize agent array */
	buffersHost->agents = (PPCAgent *) clEnqueueMapBuffer( zone.queues[0], buffersDevice->agents, CL_TRUE, CL_MAP_WRITE, 0, dataSizes.agents, 0, NULL, NULL, &status);
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
		}

	}

	/* Unmap agents buffer from device */ 
	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice->agents, buffersHost->agents, 0, NULL, NULL);
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->agents");

	/* Unmap matrix buffer from device */ 
	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice->matrix, buffersHost->matrix, 0, NULL, NULL);
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->matrix");

	/* Initialize RNG seeds */
	buffersHost->rng_seeds = (cl_ulong *) clEnqueueMapBuffer( zone.queues[0], buffersDevice->rng_seeds, CL_TRUE, CL_MAP_WRITE, 0, dataSizes.rng_seeds, 0, NULL, NULL, &status);
	clu_if_error_create_error_return(status, err, "Map buffersHost->rng_seeds");

	for (unsigned int i = 0; i < num_threads; i++) {
		buffersHost->rng_seeds[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}

	/* Unmap RNG seeds buffer from device */
	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice->rng_seeds, buffersHost->rng_seeds, 0, NULL, NULL);
	clu_if_error_create_error_return(status, err, "Unmap buffersHost->rng_seeds");

	/* Initialize agent parameters */
	buffersHost->agent_params = (PPAgentParams *) clEnqueueMapBuffer( zone.queues[0], buffersDevice->agent_params, CL_TRUE, CL_MAP_WRITE, 0, dataSizes.agent_params, 0, NULL, NULL, &status);
	clu_if_error_create_error_return(status, err, "Map buffersHost->agent_params");

	buffersHost->agent_params[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	buffersHost->agent_params[SHEEP_ID].reproduce_threshold = params.sheep_reproduce_threshold;
	buffersHost->agent_params[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	buffersHost->agent_params[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	buffersHost->agent_params[WOLF_ID].reproduce_threshold = params.wolves_reproduce_threshold;
	buffersHost->agent_params[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;

	/* Unmap agent parameters buffer from device. */
	status = clEnqueueUnmapMemObject( zone.queues[0], buffersDevice->agent_params, buffersHost->agent_params, 0, NULL, NULL);
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

	status = clSetKernelArg(krnls->step1, 4, sizeof(PPCSimParams), (void *) &simParams);
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

	status = clSetKernelArg(krnls->step2, 6, sizeof(PPCSimParams), (void *) &simParams);
	clu_if_error_create_error_return(status, err, "Arg 6 of step2_kernel");

	status = clSetKernelArg(krnls->step2, 7, sizeof(cl_mem), (void *) &buffersDevice->agent_params);
	clu_if_error_create_error_return(status, err, "Arg 7 of step2_kernel");

	/* Everything Ok. */
	return CL_SUCCESS;
}

/**
 * @brief Perform simulation!
 * */
cl_uint ppc_simulate(size_t num_threads, size_t lines_per_thread, PPParameters params, CLUZone zone, PPCKernels krnls, PPCDataSizes dataSizes, PPCBuffersHost buffersHost, PPCBuffersDevice buffersDevice, GError** err) {
	
	/* Aux. vars. */
	cl_int status;	

	/* Current iteration. */
	cl_uint iter;

    /* Assume workgroup size is 1 */ 
    /** @todo Don't really assume this please. */
	size_t num_work_items = 1;
	
	/* Guarantee all memory transfers are performed */
    /** @todo This is not necessary if queue is created with in-order execution. */
	clFinish(zone.queues[0]); 

	/* Simulation loop! */
	for (iter = 1; iter <= params.iters; iter++) {
		
		/* Step 1:  Move agents, grow grass */
		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {
			
			/* Set turn on step1_kernel */
			status = clSetKernelArg(krnls.step1, 3, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_return(status, err,  "Arg 3 of step1_kernel");
			
			/* Run kernel */
			status = clEnqueueNDRangeKernel( zone.queues[0], krnls.step1, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_return(status, err, "step1_kernel");

			/* Barrier */
			/** @todo This is not necessary if queue is created with in-order execution. */
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_return(status, err, "barrier in sim loop 1");
		}

		/* Step 2:  Agent actions, get stats */
		status = clSetKernelArg(krnls.step2, 4, sizeof(cl_uint), (void *) &iter);
		clu_if_error_create_error_return(status, err, "Arg 4 of step2_kernel");

		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {

			/* Set turn on step2_kernel */
			status = clSetKernelArg(krnls.step2, 5, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_return(status, err, "Arg 5 of step2_kernel");
			
			/* Run kernel */
			status = clEnqueueNDRangeKernel( zone.queues[0], krnls.step2, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_return(status, err, "step2_kernel");
			
			/* Barrier */
			/** @todo This is not necessary if queue is created with in-order execution. */
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_return(status, err, "barrier in sim loop 2");
		}

	}
	
	/* Guarantee all kernels have really terminated... */
    /** @todo This is not necessary if queue is created with in-order execution. */
	clFinish(zone.queues[0]); 

	/* Everything Ok. */
	return CL_SUCCESS;
}
