#include "PredPreyGPU.h"

#define MAX_AGENTS 1048576

#define LWS_GPU_MAX 256 //512
#define LWS_GPU_PREF 64 //128
#define LWS_GPU_MIN 8

#define LWS_GPU_PREF_2D_X 16
#define LWS_GPU_PREF_2D_Y 8

#define MAX_GRASS_COUNT_LOOPS 5 //More than enough...

// Global work sizes
size_t grass_gws[2];
size_t grasscount1_gws;
size_t grasscount2_gws[MAX_GRASS_COUNT_LOOPS];

// Local work sizes
size_t grass_lws[2];
size_t grasscount1_lws;
size_t grasscount2_lws;

int effectiveNextGrassToCount[MAX_GRASS_COUNT_LOOPS];
int numGrassCount2Loops;

// Kernels
cl_kernel grass_kernel;
cl_kernel countgrass1_kernel;
cl_kernel countgrass2_kernel;

// Events
cl_event* grass_event; 
cl_event *grasscount1_event; 
cl_event *grasscount2_event;

// Main stuff
int main(int argc, char ** argv)
{

#ifdef CLPROFILER
	printf("Profiling is ON!\n");
#else
	printf("Profiling is OFF!\n");
#endif

	// Aux vars
	cl_int status;
	char msg[500];
	
	// Timmings
	struct timeval time1, time0;

	// Get the required CL zone.
	CLZONE zone = getClZone("NVIDIA Corporation", "PredPreyGPU_Kernels.cl", CL_DEVICE_TYPE_GPU);

	// Get simulation parameters
	PARAMS params = loadParams(CONFIG_FILE);

	// Compute work sizes for different kernels and print them to screen
	computeWorkSizes(params, zone.device_type, zone.cu);	
	printFixedWorkSizes();

	// obtain kernels entry points.
	getKernelEntryPoints(zone.program);
	
	// Show kernel info - this should then influence the stuff above
	showKernelInfo(zone);
	
	////////////////////////////////////////
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	// Statistics
	size_t statsSizeInBytes = (params.iters + 1) * sizeof(STATS);
	STATS * statsArrayHost = initStatsArrayHost(params, statsSizeInBytes);

	// Grass matrix 
	size_t grassSizeInBytes = params.grid_x * params.grid_y * sizeof(CELL);
	CELL * grassMatrixHost = initGrassMatrixHost(params, grassSizeInBytes, statsArrayHost);
	
	// RNG seeds
	size_t rngSeedsSizeInBytes = MAX_AGENTS * sizeof(cl_ulong);
	cl_ulong * rngSeedsHost = initRngSeedsHost(rngSeedsSizeInBytes) ;

	// Sim parameters
	SIM_PARAMS sim_params = initSimParams(params);
	
	// Current iteration
	cl_uint iter = 0; 
	
	//////////////////////////////////////////
	// Create and initialize device buffers //
	//////////////////////////////////////////

	// Statistics
	cl_mem statsArrayDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, statsSizeInBytes, statsArrayHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "statsArrayDevice"); return(-1); }

	// Grass matrix
	cl_mem grassMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes, grassMatrixHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassMatrixDevice"); return(-1); }

	// Iterations
	cl_mem iterDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint), &iter, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "iterDevice"); return(-1); }

	// Grass count
	cl_mem grassCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, grasscount2_gws[0] * sizeof(cl_uint), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassCountDevice"); return(-1); }

	// RNG seeds
	cl_mem rngSeedsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes, rngSeedsHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "rngSeedsDevice"); return(-1); }

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	// Grass kernel
	setGrassKernelArgs(grassMatrixDevice, sim_params);

	// Count grass
	setCountGrassKernelArgs(grassMatrixDevice, grassCountDevice, statsArrayDevice, iterDevice, sim_params);
	
	//////////////////////////
	//  Setup OpenCL events //
	//////////////////////////
	grass_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	grasscount1_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	grasscount2_event = (cl_event*) malloc(sizeof(cl_event) * params.iters * numGrassCount2Loops); // Exact usage scenario
	cl_uint grasscount2_event_index = 0;
	
	// Guarantee all memory transfers are performed
	clFinish(zone.queue); 
	
	// Start timer
	gettimeofday(&time0, NULL);
	
	
	//////////////////
	//  SIMULATION! //
	//////////////////
	for (iter = 1; iter <= params.iters; iter++) {
		
		///// Simulation steps ////////
		cl_uint iterbase = iter - 1;

		// Grass kernel: grow grass, set number of prey to zero
		status = clEnqueueNDRangeKernel( zone.queue, grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, grass_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "grass_kernel, iteration %d, gws=%d lws=%d ", iter, (int) *grass_gws, (int) *grass_lws); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

		///// Gather statistics //////
		
		// Count grass, part 1
		status = clEnqueueNDRangeKernel( zone.queue, countgrass1_kernel, 1, NULL, &grasscount1_gws, &grasscount1_lws, 1, NULL, grasscount1_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "countgrass1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		
		// Count grass, part 2
		for (int i = 0; i < numGrassCount2Loops; i++) {

			status = clSetKernelArg(countgrass2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextGrassToCount[i]);
			if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countgrass2 kernel"); return(-1); }

			status = clEnqueueNDRangeKernel( zone.queue, countgrass2_kernel, 1, NULL, &grasscount2_gws[i], &grasscount2_lws, 1, grasscount1_event + iterbase, grasscount2_event + grasscount2_event_index);
			if (status != CL_SUCCESS) { sprintf(msg, "countgrass2_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

			status = clEnqueueBarrier(zone.queue);
			if (status != CL_SUCCESS) {  sprintf(msg, "in grass count loops"); PrintErrorEnqueueBarrier(status, msg); return(-1); }

			grasscount2_event_index++;

		}

	}

	// Guarantee all kernels have really terminated...
	clFinish(zone.queue);

	// Get finishing time	
	gettimeofday(&time1, NULL);  

	// Get statistics
	status = clEnqueueReadBuffer(zone.queue, statsArrayDevice, CL_TRUE, 0, statsSizeInBytes, statsArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) {  PrintErrorEnqueueReadWriteBuffer(status, "statsArray"); return(-1); }
	
	// Output results to file
	saveResults("stats.txt", statsArrayHost, params.iters);


	// Print timmings
#ifdef CLPROFILER
	// Calculate and show profiling info
	double totalTime = printTimmings(time0, time1);
	showProfilingInfo(totalTime, params.iters, grasscount2_event_index);
#else
	printTimmings(time0, time1);
#endif

	/////////////////
	// Free stuff! //
	/////////////////
	
	// Release OpenCL kernels
	releaseKernels();
	
	// Release OpenCL program and command queue
    clReleaseProgram(zone.program);
    clReleaseCommandQueue(zone.queue);
    
	// Release OpenCL memory objects
	clReleaseMemObject(statsArrayDevice);
	clReleaseMemObject(grassMatrixDevice);
	clReleaseMemObject(iterDevice);
	clReleaseMemObject(grassCountDevice);
	clReleaseMemObject(rngSeedsDevice);

	// Release OpenCL context
	clReleaseContext(zone.context);
	
	// Free host resources
	free(statsArrayHost);
	free(grassMatrixHost);
	free(rngSeedsHost);
	free(grass_event); 
	free(grasscount1_event); 
	free(grasscount2_event);

	return 0;
	
}

// Compute worksizes depending on the device type and number of available compute units
void computeWorkSizes(PARAMS params, cl_uint device_type, cl_uint cu) {
	// grass growth worksizes
	grass_lws[0] = LWS_GPU_PREF_2D_X;
	grass_gws[0] = LWS_GPU_PREF_2D_X * ceil(((float) params.grid_x) / LWS_GPU_PREF_2D_X);
	grass_lws[1] = LWS_GPU_PREF_2D_Y;
	grass_gws[1] = LWS_GPU_PREF_2D_Y * ceil(((float) params.grid_y) / LWS_GPU_PREF_2D_Y);
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
	printf("grasscount1_gws=%d\tgrasscount1_lws=%d\n", (int) grasscount1_gws, (int) grasscount1_lws);
	printf("grasscount2_lws=%d\n", (int) grasscount2_lws);
	for (int i = 0; i < numGrassCount2Loops; i++) {
		printf("grasscount2_gws[%d]=%d (effective grass to count: %d)\n", i, (int) grasscount2_gws[i], effectiveNextGrassToCount[i]);
	}
	printf("Total of %d grass count loops.\n", numGrassCount2Loops);

}

// Get kernel entry points
void getKernelEntryPoints(cl_program program) {
	cl_int status;
	grass_kernel = clCreateKernel( program, "Grass", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Grass kernel"); exit(EXIT_FAILURE); }
	countgrass1_kernel = clCreateKernel( program, "CountGrass1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass1 kernel"); exit(EXIT_FAILURE); }
	countgrass2_kernel = clCreateKernel( program, "CountGrass2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass2 kernel"); exit(EXIT_FAILURE); }
}

// Show kernel info
void showKernelInfo(CLZONE zone) {
	KERNEL_WORK_GROUP_INFO kwgi;
	printf("\n-------- grass_kernel information --------\n");	
	getWorkGroupInfo(grass_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countgrass1_kernel information --------\n");	
	getWorkGroupInfo(countgrass1_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countgrass2_kernel information --------\n");	
	getWorkGroupInfo(countgrass2_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
}

// Initialize statistics array in host
STATS* initStatsArrayHost(PARAMS params, size_t statsSizeInBytes) 
{
	STATS* statsArrayHost = (STATS*) malloc(statsSizeInBytes);
	statsArrayHost[0].sheep = params.init_sheep;
	statsArrayHost[0].wolves = params.init_wolves;
	statsArrayHost[0].grass = 0;
	return statsArrayHost;
}

// Initialize grass matrix in host
CELL* initGrassMatrixHost(PARAMS params, size_t grassSizeInBytes, STATS* statsArrayHost) 
{

	CELL * grassMatrixHost = (CELL *) malloc(grassSizeInBytes);
	for(unsigned int i = 0; i < params.grid_x; i++)
	{
		for (unsigned int j = 0; j < params.grid_y; j++)
		{
			unsigned int gridIndex = i + j*params.grid_x;
			grassMatrixHost[gridIndex].grass = (rand() % 2) == 0 ? 0 : 1 + (rand() % params.grass_restart);
			if (grassMatrixHost[gridIndex].grass == 0)
				statsArrayHost[0].grass++;
		}
	}
	return grassMatrixHost;
}

// Initialize random seeds array in host
cl_ulong* initRngSeedsHost(size_t rngSeedsSizeInBytes) {
	cl_ulong * rngSeedsHost = (cl_ulong*) malloc(rngSeedsSizeInBytes);
	for (int i = 0; i < MAX_AGENTS; i++) {
		rngSeedsHost[i] = rand();
	}
	return rngSeedsHost;
}

// Initialize simulation parameters in host, to be sent to GPU
SIM_PARAMS initSimParams(PARAMS params) {
	SIM_PARAMS sim_params;
	sim_params.size_x = params.grid_x;
	sim_params.size_y = params.grid_y;
	sim_params.size_xy = params.grid_x * params.grid_y;
	sim_params.max_agents = MAX_AGENTS;
	sim_params.grass_restart = params.grass_restart;
	return sim_params;
}

// Set grass kernel parameters
void setGrassKernelArgs(cl_mem grassMatrixDevice, SIM_PARAMS sim_params) {

	cl_int status;

	status = clSetKernelArg(grass_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of grass kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(grass_kernel, 1, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of grass kernel"); exit(EXIT_FAILURE); }
}

// Set grass count kernels parameters
void setCountGrassKernelArgs(cl_mem grassMatrixDevice, cl_mem grassCountDevice, cl_mem statsArrayDevice, cl_mem iterDevice, SIM_PARAMS sim_params) {
	
	cl_int status;

	// Grass count kernel 1
	status = clSetKernelArg(countgrass1_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countgrass1 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass1_kernel, 1, sizeof(cl_mem), (void *) &grassCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countgrass1 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass1_kernel, 2, grasscount1_lws*sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countgrass1 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass1_kernel, 3, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countgrass1 kernel"); exit(EXIT_FAILURE); }

	// Grass count kernel 2
	status = clSetKernelArg(countgrass2_kernel, 0, sizeof(cl_mem), (void *) &grassCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countgrass2 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass2_kernel, 1, grasscount2_gws[0]*sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countgrass2 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass2_kernel, 3, sizeof(cl_mem), (void *) &statsArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countgrass2 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(countgrass2_kernel, 4, sizeof(cl_mem), (void *) &iterDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of countgrass2 kernel"); exit(EXIT_FAILURE); }
}

// Release kernels
void releaseKernels() {
	clReleaseKernel(grass_kernel);
	clReleaseKernel(countgrass1_kernel); 
	clReleaseKernel(countgrass2_kernel);
}

// Save results
void saveResults(char* filename, STATS* statsArrayHost, unsigned int iters) {
	FILE * fp1 = fopen(filename,"w");
	for (unsigned int i = 0; i <= iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep, statsArrayHost[i].wolves, statsArrayHost[i].grass );
	fclose(fp1);
}

// Print timmings
double printTimmings(struct timeval time0, struct timeval time1) {
	double dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Total Simulation Time = %f", dt);
	return dt;
}

// Show profiling info
void showProfilingInfo(double dt, unsigned int iters, cl_uint grasscount2_event_index) {
	cl_uint status;
	char msg[500];
	cl_ulong grass_profile = 0, grasscount1_profile = 0, grasscount2_profile = 0;
	cl_ulong grass_profile_start, grasscount1_profile_start, grasscount2_profile_start;
	cl_ulong grass_profile_end, grasscount1_profile_end, grasscount2_profile_end;

	for (unsigned int i = 0; i < iters; i++) {
		// Grass kernel profiling
		status = clGetEventProfilingInfo (grass_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grass_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grass start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  exit(EXIT_FAILURE); }
		status = clGetEventProfilingInfo (grass_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grass_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grass end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  exit(EXIT_FAILURE); }
		grass_profile += grass_profile_end - grass_profile_start;
		// Count grass 1 kernel profiling
		status = clGetEventProfilingInfo (grasscount1_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount1_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount1 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); exit(EXIT_FAILURE);  }
		status = clGetEventProfilingInfo (grasscount1_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount1_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount1 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  exit(EXIT_FAILURE); }
		grasscount1_profile += grasscount1_profile_end - grasscount1_profile_start;
	}

	for (unsigned int i = 0; i < grasscount2_event_index; i++) {
		// Grass count 2 kernel profiling
		status = clGetEventProfilingInfo (grasscount2_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount2_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount2 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  exit(EXIT_FAILURE); }
		status = clGetEventProfilingInfo (grasscount2_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount2_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount2 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  exit(EXIT_FAILURE); }
		grasscount2_profile += grasscount2_profile_end - grasscount2_profile_start;
	}

	cl_ulong gpu_profile_total = grass_profile + grasscount1_profile + grasscount2_profile;
	double gpu_exclusive = gpu_profile_total * 1e-9;
	double cpu_exclusive = dt - gpu_exclusive;
	printf(", of which %f (%f%%) is CPU and %f (%f%%) is GPU.\n", cpu_exclusive, 100*cpu_exclusive/dt, gpu_exclusive, 100*gpu_exclusive/dt);
	printf("grass: %fms (%f%%)\n", grass_profile*1e-6, 100*((double) grass_profile)/((double) gpu_profile_total));
	printf("grasscount1: %fms (%f%%)\n", grasscount1_profile*1e-6, 100*((double) grasscount1_profile)/((double) gpu_profile_total));
	printf("grasscount2: %fms (%f%%)\n", grasscount2_profile*1e-6, 100*((double) grasscount2_profile)/((double) gpu_profile_total));
	printf("\n");

	
}
