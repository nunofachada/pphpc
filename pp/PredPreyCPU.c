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
	PPCKernels krnls = {NULL, NULL};

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
	
	/* Host buffers */
	PPStatistics* statsArrayHost = NULL;
	PPCCell* cellMatrixHost = NULL;
	PPCAgent* agentsArrayHost = NULL;
	cl_ulong* rngSeedsHost = NULL;
	
	/* Device buffers */	
	cl_mem statsArrayDevice = NULL,
		cellMatrixDevice = NULL,
		agentsArrayDevice = NULL,
		rngSeedsDevice = NULL,
		agentParamsDevice = NULL;
	
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
	
	/* Print thread info to screen */
	ppc_threadinfo_print(zone.cu, num_threads, lines_per_thread, num_threads_sugested, num_threads_max);
	
	/* Create kernels. */
	status = ppc_kernels_create(zone.program, &krnls, &err);
	clu_if_error_goto(status, err, error);

	// 6. Create memory objects

	// Statistics
	size_t statsSizeInBytes = (params.iters + 1) * sizeof(PPStatistics);

	statsArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, statsSizeInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating statsArrayDevice");

	// Grass matrix
	size_t cellMatrixSizeInBytes = params.grid_x * params.grid_y * sizeof(PPCCell);

	cellMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, cellMatrixSizeInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating cellMatrixDevice");

	// Agent array
	size_t agentsSizeInBytes = MAX_AGENTS * sizeof(PPCAgent);

	agentsArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, agentsSizeInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating agentArrayDevice");

	// Random number generator array of seeds
	size_t rngSeedsSizeInBytes = num_threads * sizeof(cl_ulong);

	rngSeedsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, rngSeedsSizeInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating rngSeedsDevice");

	// Agent parameters
	size_t agentParamsSizeInBytes = 2 * sizeof(PPAgentParams);

	agentParamsDevice = clCreateBuffer(zone.context, CL_MEM_READ_ONLY  | CL_MEM_ALLOC_HOST_PTR, agentParamsSizeInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating agentParamsDevice");

	// 7. Initialize memory objects

	// Statistics
	statsArrayHost = (PPStatistics*) clEnqueueMapBuffer( zone.queues[0], statsArrayDevice, CL_TRUE, CL_MAP_WRITE, 0, statsSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map statsArrayHost");

	statsArrayHost[0].sheep = params.init_sheep;
	statsArrayHost[0].wolves = params.init_wolves;
	statsArrayHost[0].grass = 0;
	for (unsigned int i = 1; i <= params.iters; i++) {
		statsArrayHost[i].sheep = 0;
		statsArrayHost[i].wolves = 0;
		statsArrayHost[i].grass = 0;
	}

	// Grass matrix
	cellMatrixHost = (PPCCell *) clEnqueueMapBuffer( zone.queues[0], cellMatrixDevice, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, cellMatrixSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map cellMatrixHost");

	for(cl_uint i = 0; i < params.grid_x; i++)
	{
		for (cl_uint j = 0; j < params.grid_y; j++)
		{
			cl_uint gridIndex = (i + j*params.grid_x);
			/* Initialize grass. */
			cl_uint grassState = g_rand_int_range(rng, 0, 2) == 0 
				? 0 
				: g_rand_int_range(rng, 1, params.grass_restart + 1);
			
			
			cellMatrixHost[gridIndex].grass = grassState;
			if (grassState == 0)
				statsArrayHost[0].grass++;
			/* Initialize agent pointer. */
			cellMatrixHost[gridIndex].agent_pointer = NULL_AGENT_POINTER;
		}
	}

	status = clEnqueueUnmapMemObject( zone.queues[0], statsArrayDevice, statsArrayHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap statsArrayHost");

	// Agent array
	agentsArrayHost = (PPCAgent *) clEnqueueMapBuffer( zone.queues[0], agentsArrayDevice, CL_TRUE, CL_MAP_WRITE, 0, agentsSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map agentsArrayHost");

	for(cl_uint i = 0; i < MAX_AGENTS; i++)
	{
		/* Check if there are still agents to initialize. */
		if (i < params.init_sheep + params.init_wolves)
		{
			/* There are still agents to initialize, chose a place to put next agent. */
			cl_uint x = g_rand_int_range(rng, 0, params.grid_x);
			cl_uint y = g_rand_int_range(rng, 0, params.grid_y);
			/* Initialize generic agent stuff. */
			agentsArrayHost[i].action = 0;
			agentsArrayHost[i].next = NULL_AGENT_POINTER;
			cl_uint gridIndex = (x + y*params.grid_x);
			if (cellMatrixHost[gridIndex].agent_pointer == NULL_AGENT_POINTER) {
				/* This cell had no agent, put it there. */
				cellMatrixHost[gridIndex].agent_pointer = i;
			} else {
				/* Cell already has agent, put it at the end of the list. */
				cl_uint agindex = cellMatrixHost[gridIndex].agent_pointer;
				while (agentsArrayHost[agindex].next != NULL_AGENT_POINTER)
					agindex = agentsArrayHost[agindex].next;
				agentsArrayHost[agindex].next = i;
			}
			/* Perform agent specific initialization. */
			if (i < params.init_sheep)
			{
				/* Initialize sheep specific stuff. */
				agentsArrayHost[i].energy = g_rand_int_range(rng, 1, params.sheep_gain_from_food * 2 + 1);
				agentsArrayHost[i].type = 0; // Sheep
			}
			else
			{
				/* Initialize wolf specific stuff. */
				agentsArrayHost[i].energy = g_rand_int_range(rng, 1, params.wolves_gain_from_food * 2 + 1);
				agentsArrayHost[i].type = 1; // Wolf
			}

		} 
		else 
		{
			/* No more agents to initialize, initialize array position only. */
			agentsArrayHost[i].energy = 0;
			agentsArrayHost[i].type = 0;
			agentsArrayHost[i].action = 0;
			agentsArrayHost[i].next = NULL_AGENT_POINTER;
		}

	}

	status = clEnqueueUnmapMemObject( zone.queues[0], agentsArrayDevice, agentsArrayHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap agentsArrayHost");

	status = clEnqueueUnmapMemObject( zone.queues[0], cellMatrixDevice, cellMatrixHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap cellMatrixHost");

	// RNG seeds
	rngSeedsHost = (cl_ulong *) clEnqueueMapBuffer( zone.queues[0], rngSeedsDevice, CL_TRUE, CL_MAP_WRITE, 0, rngSeedsSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map rngSeedsHost");

	for (unsigned int i = 0; i < num_threads; i++) {
		rngSeedsHost[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}

	status = clEnqueueUnmapMemObject( zone.queues[0], rngSeedsDevice, rngSeedsHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap rngSeedsHost");

	// Agent parameters
	PPAgentParams* agentParamsHost = (PPAgentParams *) clEnqueueMapBuffer( zone.queues[0], agentParamsDevice, CL_TRUE, CL_MAP_WRITE, 0, agentParamsSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map agentParamsHost");

	agentParamsHost[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	agentParamsHost[SHEEP_ID].reproduce_threshold = params.sheep_reproduce_threshold;
	agentParamsHost[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	agentParamsHost[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	agentParamsHost[WOLF_ID].reproduce_threshold = params.wolves_reproduce_threshold;
	agentParamsHost[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;

	status = clEnqueueUnmapMemObject( zone.queues[0], agentParamsDevice, agentParamsHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap agentParamsHost");

	// Sim parameters
	PPCSimParams simParams;

	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = MAX_AGENTS;
	simParams.null_agent_pointer = NULL_AGENT_POINTER;
	simParams.grass_restart = params.grass_restart;
	simParams.lines_per_thread = lines_per_thread;

	// 8. Set fixed kernel arguments

	// MoveAgentGrowGrass (step1) kernel
	status = clSetKernelArg(krnls.step1, 0, sizeof(cl_mem), (void *) &agentsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of step1_kernel");

	status = clSetKernelArg(krnls.step1, 1, sizeof(cl_mem), (void *) &cellMatrixDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of step1_kernel");

	status = clSetKernelArg(krnls.step1, 2, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 2 of step1_kernel");

	status = clSetKernelArg(krnls.step1, 4, sizeof(PPCSimParams), (void *) &simParams);
	clu_if_error_create_error_goto(status, &err, error, "Arg 4 of step1_kernel");

	// AgentActionsGetStats (step2) kernel
	status = clSetKernelArg(krnls.step2, 0, sizeof(cl_mem), (void *) &agentsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of step2_kernel");

	status = clSetKernelArg(krnls.step2, 1, sizeof(cl_mem), (void *) &cellMatrixDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of step2_kernel");

	status = clSetKernelArg(krnls.step2, 2, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 2 of step2_kernel");

	status = clSetKernelArg(krnls.step2, 3, sizeof(cl_mem), (void *) &statsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 3 of step2_kernel");

	status = clSetKernelArg(krnls.step2, 6, sizeof(PPCSimParams), (void *) &simParams);
	clu_if_error_create_error_goto(status, &err, error, "Arg 6 of step2_kernel");

	status = clSetKernelArg(krnls.step2, 7, sizeof(cl_mem), (void *) &agentParamsDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 7 of step2_kernel");


// TEST STUFF
	/*FILE * fpa = fopen("agents.txt","w");
	fprintf(fpa, "Id\tEnergy\tAction\tType\tNext\n");
	for (unsigned int i = 0; i < MAX_AGENTS; i++)
		fprintf(fpa, "%d\t%d\t%d\t%d\t%d\n", i, agentsArrayHost[i].energy, agentsArrayHost[i].action, agentsArrayHost[i].type, agentsArrayHost[i].next );
	fclose(fpa);

	FILE * fpc = fopen("matrix.txt","w");
	fprintf(fpc, "Id\tGrass\tAgent\n");
	for (unsigned int i = 0; i < simParams.size_xy; i++)
		fprintf(fpc, "%d\t%d\t%d\n", i, cellMatrixHost[i].grass, cellMatrixHost[i].agent_pointer);
	fclose(fpc);

	status = clEnqueueUnmapMemObject( zone.queue, agentsArrayDevice, agentsArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueUnmapMemObject(status, "--"); return(-1); }
	status = clEnqueueUnmapMemObject( zone.queue, cellMatrixDevice, cellMatrixHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueUnmapMemObject(status, "---"); return(-1); }

	cl_kernel test_kernel = clCreateKernel( zone.program, "test", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "test_kernel"); exit(-1); }

	size_t intArraySizeInBytes = 20 * num_threads * sizeof(cl_int);

	cl_mem intArrayDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, intArraySizeInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "intArrayDevice"); return(-1); }

	status = clSetKernelArg(test_kernel, 0, sizeof(cl_mem), (void *) &intArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of test_kernel"); return(-1); }

	status = clSetKernelArg(test_kernel, 1, sizeof(cl_mem), (void *) &rngSeedsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of test_kernel"); return(-1); }

	status = clSetKernelArg(test_kernel, 2, sizeof(SIM_PARAMS), (void *) &simParams);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of test_kernel"); return(-1); }

	size_t wi = 1;
	status = clEnqueueNDRangeKernel( zone.queue, test_kernel, 1, NULL, &num_threads, &wi, 0, NULL, NULL);
	if (status != CL_SUCCESS) {  PrintErrorEnqueueNDRangeKernel(status, "test_kernel"); }

	cl_int* intArrayHost = (cl_int *) clEnqueueMapBuffer( zone.queue, intArrayDevice, CL_TRUE, CL_MAP_READ, 0, intArraySizeInBytes, 0, NULL, NULL, &status);
	if (status != CL_SUCCESS) { PrintErrorEnqueueMapBuffer(status, "intArrayHost"); return(-1); }

	FILE * fpw = fopen("walks.txt","w");
	for (unsigned int i = 0; i < 20 * num_threads; i++)
		fprintf(fpw, "%d\n", intArrayHost[i]);
	fclose(fpw);
	
	return 0;*/

        // 9. Run the show
	size_t num_work_items = 1;
	cl_uint iter;
	clFinish(zone.queues[0]); // Guarantee all memory transfers are performed

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	for (iter = 1; iter <= params.iters; iter++) {
		// Step 1
		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {
			//printf("iter %d, kernel 1, turn %d\n", iter, turn);
			// Set turn on step1_kernel
			status = clSetKernelArg(krnls.step1, 3, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_goto(status, &err, error, "Arg 3 of step1_kernel");
			
			// Run kernel
			status = clEnqueueNDRangeKernel( zone.queues[0], krnls.step1, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_goto(status, &err, error, "step1_kernel");

			// Barrier
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_goto(status, &err, error, "barrier in sim loop 1");
		}

		// Step 2
		status = clSetKernelArg(krnls.step2, 4, sizeof(cl_uint), (void *) &iter);
		clu_if_error_create_error_goto(status, &err, error, "Arg 4 of step2_kernel");

		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {
			//printf("iter %d, kernel 2, turn %d\n", iter, turn);
			// Set turn on step2_kernel
			status = clSetKernelArg(krnls.step2, 5, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_goto(status, &err, error, "Arg 5 of step2_kernel");
			// Run kernel
			status = clEnqueueNDRangeKernel( zone.queues[0], krnls.step2, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_goto(status, &err, error, "step2_kernel");
			// Barrier
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_goto(status, &err, error, "barrier in sim loop 2");
		}



	}
	clFinish(zone.queues[0]); // Guarantee all kernels have really terminated...

	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  
	
	printf("Time stops now!\n");

	statsArrayHost = (PPStatistics*) clEnqueueMapBuffer( zone.queues[0], statsArrayDevice, CL_TRUE, CL_MAP_READ, 0, statsSizeInBytes, 0, NULL, NULL, &status);
	clu_if_error_create_error_goto(status, &err, error, "Map statsArrayHost");

	// 10. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep, statsArrayHost[i].wolves, statsArrayHost[i].grass );
	fclose(fp1);

	status = clEnqueueUnmapMemObject( zone.queues[0], statsArrayDevice, statsArrayHost, 0, NULL, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Unmap statsArrayHost");

		// TEST STUFF
		/*agentsArrayHost = (AGENT *) clEnqueueMapBuffer( zone.queue, agentsArrayDevice, CL_TRUE, CL_MAP_READ, 0, agentsSizeInBytes, 0, NULL, NULL, &status);
		if (status != CL_SUCCESS) { PrintErrorEnqueueMapBuffer(status, "agentsArrayHost"); return(-1); }
		char * agentsFile = (char*) malloc(40*sizeof(char));
		sprintf(agentsFile, "agents%d.txt", iter);
		FILE * fpa1 = fopen(agentsFile,"w");
		fprintf(fpa1, "Id\tEnergy\tAction\tType\tNext\n");
		for (unsigned int i = 0; i < MAX_AGENTS; i++)
			fprintf(fpa1, "%d\t%d\t%d\t%d\t%d\n", i, agentsArrayHost[i].energy, agentsArrayHost[i].action, agentsArrayHost[i].type, agentsArrayHost[i].next );
		fclose(fpa1);
		cellMatrixHost = (CELL *) clEnqueueMapBuffer( zone.queue, cellMatrixDevice, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, cellMatrixSizeInBytes, 0, NULL, NULL, &status);
		if (status != CL_SUCCESS) { PrintErrorEnqueueMapBuffer(status, "cellMatrixHost"); return(-1); }
		char * matrixFile = (char*) malloc(40*sizeof(char));
		sprintf(matrixFile, "matrix%d.txt", iter);
		FILE * fpc1 = fopen(matrixFile,"w");
		fprintf(fpc1, "Id\tGrass\tAgent\n");
		for (unsigned int i = 0; i < simParams.size_xy; i++)
			fprintf(fpc1, "%d\t%d\t%d\n", i, cellMatrixHost[i].grass, cellMatrixHost[i].agent_pointer);
		fclose(fpc1);
		status = clEnqueueUnmapMemObject( zone.queue, agentsArrayDevice, agentsArrayHost, 0, NULL, NULL);
		if (status != CL_SUCCESS) { PrintErrorEnqueueUnmapMemObject(status, "--"); return(-1); }
		status = clEnqueueUnmapMemObject( zone.queue, cellMatrixDevice, cellMatrixHost, 0, NULL, NULL);
		if (status != CL_SUCCESS) { PrintErrorEnqueueUnmapMemObject(status, "---"); return(-1); }*/



	/* Show profiling info. */
	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	fprintf(stderr, "Error %d: %s\n", err->code, err->message);
	if (zone.build_log) clu_build_log_print(&zone);
	g_error_free(err);

cleanup:
	/* Release OpenCL kernels */
	ppc_kernels_free(&krnls);

	// 12. Free stuff!
	printf("Press enter to free memory...");
	getchar();
	
	//TODO Release events

	/* Free RNG */
	g_rand_free(rng);
		
	// Release memory objects
	if (agentsArrayDevice) clReleaseMemObject(agentsArrayDevice);
	if (cellMatrixDevice) clReleaseMemObject(cellMatrixDevice);
	if (statsArrayDevice) clReleaseMemObject(statsArrayDevice);
	if (rngSeedsDevice) clReleaseMemObject(rngSeedsDevice);

	// Release program, command queues and context
	clu_zone_free(&zone);

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
