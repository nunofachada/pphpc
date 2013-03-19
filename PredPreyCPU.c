#include "PredPreyCPU.h"

#define MAX_AGENTS 16777216
#define NULL_AGENT_POINTER UINT_MAX

#define SHEEP_ID 0
#define WOLF_ID 1

// Kernels
cl_kernel step1_kernel, step2_kernel;
// Number of threads
size_t num_threads_sugested, num_threads_max, num_threads;

// OpenCL kernel files
const char* kernelFiles[] = {"PredPreyCommon_Kernels.cl", "PredPreyCPU_Kernels.cl"};

// Main stuff
int main(int argc, char ** argv)
{

	// Status var aux
	cl_int status;	
	
	// Error management
	GError *err = NULL;
	
	// Init random number generator
	srand((unsigned)(time(0)));

	// Timmings
	struct timeval time1, time0;
	double dt = 0;
	
	// Host buffers
	PPStatistics* statsArrayHost = NULL;
	PPCCell* cellMatrixHost = NULL;
	PPCAgent* agentsArrayHost = NULL;
	cl_ulong* rngSeedsHost = NULL;
	
	// Device buffers	
	cl_mem statsArrayDevice = NULL,
		cellMatrixDevice = NULL,
		agentsArrayDevice = NULL,
		rngSeedsDevice = NULL,
		agentParamsDevice = NULL;
	
	// 1. Get the required CL zone.
	CLUZone zone;
	status = clu_zone_new(&zone, CL_DEVICE_TYPE_CPU, 1, QUEUE_PROPERTIES, &err);
	clu_if_error_goto(status, err, error);

	/* Build program. */
	status = clu_program_create(&zone, kernelFiles, 2, NULL, &err);
	clu_if_error_goto(status, err, error);

	// 2. Get simulation parameters
	PPParameters params = pp_load_params(CONFIG_FILE);

	// 3. Determine number of sugested threads to use
	if (argc == 1) {
		num_threads_sugested = zone.cu;
	} else if (argc == 2) {
		num_threads_sugested = atoi(argv[1]);
	} else {
		printf("Usage: %s [num_threads]\n", argv[0]);
	}
	// Determine maximum number of threads which can be used for current problem (each pair threads must process lines which are separated by two lines not being processed)
	num_threads_max = params.grid_y / 3;
	// Determine effective number of threads to use
	num_threads = (num_threads_sugested < num_threads_max) ? num_threads_sugested : num_threads_max;
	// ...and lines to be computed per thread
	cl_uint lines_per_thread = params.grid_y / num_threads + (params.grid_y % num_threads > 0);

	// 4. Print simulation info to screen
	printf("-------- Compute Parameters --------\n");	
	printf("Compute units: %d\n", zone.cu);
	printf("Suggested number of threads: %d\tMaximum number of threads for this problem: %d\n", (int) num_threads_sugested, (int) num_threads_max);
	printf("Effective number of threads: %d\n", (int) num_threads);
	printf("Lines per thread: %d\n", lines_per_thread);

	// 5. obtain kernels entry points.
	step1_kernel = clCreateKernel( zone.program, "step1", &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating step1 kernel");
	step2_kernel = clCreateKernel( zone.program, "step2", &status );
	clu_if_error_create_error_goto(status, &err, error, "Creating step2 kernel");
	
	printf("-------- Simulation start --------\n");	

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
			cl_uint grassState = (rand() % 2) == 0 ? 0 : 1 + (rand() % params.grass_restart);
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
			cl_uint x = rand() % params.grid_x;
			cl_uint y = rand() % params.grid_y;
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
				agentsArrayHost[i].energy = 1 + (rand() % (params.sheep_gain_from_food * 2));
				agentsArrayHost[i].type = 0; // Sheep
			}
			else
			{
				/* Initialize wolf specific stuff. */
				agentsArrayHost[i].energy = 1 + (rand() % (params.wolves_gain_from_food * 2));
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
		rngSeedsHost[i] = rand();
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
	status = clSetKernelArg(step1_kernel, 0, sizeof(cl_mem), (void *) &agentsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of step1_kernel");

	status = clSetKernelArg(step1_kernel, 1, sizeof(cl_mem), (void *) &cellMatrixDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of step1_kernel");

	status = clSetKernelArg(step1_kernel, 2, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 2 of step1_kernel");

	status = clSetKernelArg(step1_kernel, 4, sizeof(PPCSimParams), (void *) &simParams);
	clu_if_error_create_error_goto(status, &err, error, "Arg 4 of step1_kernel");

	// AgentActionsGetStats (step2) kernel
	status = clSetKernelArg(step2_kernel, 0, sizeof(cl_mem), (void *) &agentsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of step2_kernel");

	status = clSetKernelArg(step2_kernel, 1, sizeof(cl_mem), (void *) &cellMatrixDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of step2_kernel");

	status = clSetKernelArg(step2_kernel, 2, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 2 of step2_kernel");

	status = clSetKernelArg(step2_kernel, 3, sizeof(cl_mem), (void *) &statsArrayDevice);
	clu_if_error_create_error_goto(status, &err, error, "Arg 3 of step2_kernel");

	status = clSetKernelArg(step2_kernel, 6, sizeof(PPCSimParams), (void *) &simParams);
	clu_if_error_create_error_goto(status, &err, error, "Arg 6 of step2_kernel");

	status = clSetKernelArg(step2_kernel, 7, sizeof(cl_mem), (void *) &agentParamsDevice);
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
	gettimeofday(&time0, NULL);
	for (iter = 1; iter <= params.iters; iter++) {
		// Step 1
		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {
			//printf("iter %d, kernel 1, turn %d\n", iter, turn);
			// Set turn on step1_kernel
			status = clSetKernelArg(step1_kernel, 3, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_goto(status, &err, error, "Arg 3 of step1_kernel");
			
			// Run kernel
			status = clEnqueueNDRangeKernel( zone.queues[0], step1_kernel, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_goto(status, &err, error, "step1_kernel");

			// Barrier
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_goto(status, &err, error, "barrier in sim loop 1");
		}

		// Step 2
		status = clSetKernelArg(step2_kernel, 4, sizeof(cl_uint), (void *) &iter);
		clu_if_error_create_error_goto(status, &err, error, "Arg 4 of step2_kernel");

		for (cl_uint turn = 0; turn < lines_per_thread; turn++ ) {
			//printf("iter %d, kernel 2, turn %d\n", iter, turn);
			// Set turn on step2_kernel
			status = clSetKernelArg(step2_kernel, 5, sizeof(cl_uint), (void *) &turn);
			clu_if_error_create_error_goto(status, &err, error, "Arg 5 of step2_kernel");
			// Run kernel
			status = clEnqueueNDRangeKernel( zone.queues[0], step2_kernel, 1, NULL, &num_threads, &num_work_items, 0, NULL, NULL);
			clu_if_error_create_error_goto(status, &err, error, "step2_kernel");
			// Barrier
			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_create_error_goto(status, &err, error, "barrier in sim loop 2");
		}



	}
	clFinish(zone.queues[0]); // Guarantee all kernels have really terminated...
	gettimeofday(&time1, NULL);
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





	// 11. Print timmings
	dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Time = %f\n", dt);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	if (zone.build_log) clu_build_log_print(&zone);
	g_error_free(err);

cleanup:

	// 12. Free stuff!
	printf("Press enter to free memory...");
	getchar();
	
	//TODO Release events
	
	// Release memory objects
	if (agentsArrayDevice) clReleaseMemObject(agentsArrayDevice);
	if (cellMatrixDevice) clReleaseMemObject(cellMatrixDevice);
	if (statsArrayDevice) clReleaseMemObject(statsArrayDevice);
	if (rngSeedsDevice) clReleaseMemObject(rngSeedsDevice);
	// Release kernels
	if (step1_kernel) clReleaseKernel(step1_kernel);  
	if (step2_kernel) clReleaseKernel(step2_kernel);
	// Release program, command queues and context
	clu_zone_free(&zone);

	printf("Press enter to bail out...");
	getchar();
	

	return 0;
}

