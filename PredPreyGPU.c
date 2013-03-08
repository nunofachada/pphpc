#include "PredPreyGPU.h"

#define MAX_AGENTS 1048576
#define MAX_GWS 1048576
#define REDUCE_GRASS_VECSIZE 4

//#define LWS_GRASS 256
//#define LWS_REDUCEGRASS1 256

#define SEED 0

#ifdef CLPROFILER
	#define DO_PROFILING 1
#else
	#define DO_PROFILING 0
#endif

/**
 *  @detail Main program.
 * */
int main(int argc, char **argv)
{

	/* Program vars */
	GlobalWorkSizes gws;
	LocalWorkSizes lws;
	Kernels krnls;
	Events evts;
	DataSizes dataSizes;
	BuffersHost buffersHost;
	BuffersDevice buffersDevice;

	/* Aux vars. */
	cl_int status;
	char msg[MAX_AUX_BUFF];
	
	/* Create RNG and set seed. */
#ifdef SEED
	GRand* rng = g_rand_new_with_seed(SEED);
#elif
	GRand* rng = g_rand_new();
#endif	
	
	/* Profiling / Timmings */
	ProfCLProfile* profile = profcl_profile_new();

	/* Get the required CL zone. */ //TODO Must accept compiler options
	CLZone zone = getClZone("PredPreyGPU_Kernels.cl", CL_DEVICE_TYPE_GPU, 2, DO_PROFILING);

	/* Get simulation parameters */
	Parameters params = loadParams(CONFIG_FILE);

	/* Set simulation parameters in a format more adequate for this program. */
	SimParams simParams = initSimParams(params);

	/* Compute work sizes for different kernels and print them to screen. */
	computeWorkSizes(params, zone.device, &gws, &lws);
	printWorkSizes();

	/* Create kernels. */
	createKernels(zone.program, &krnls);
	
	/* Determine size in bytes for host and device data structures. */
	getDataSizesInBytes(params, &dataSizes);

	/* Initialize host buffers. */
	createHostBuffers(&buffersHost, &dataSizes, params, rng);

	/* Create device buffers */
	createDeviceBuffers(&buffersHost, &buffersDevice, gws);

	/* Create events data structure. */
	createEventsDataStructure(params, &evts);

	/*  Set fixed kernel arguments. */
	setFixedKernelArgs(&krnls, simParams);
	
	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Simulation!! */
	simulate();

	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  
	
	/* Output results to file */
	saveResults("stats.txt", buffersHost->stats, params);

	/* Analyze events, show profiling info. */
	profilingAnalysis(&evts);

	/* Release OpenCL kernels */
	freeKernels(krnls);
	
	/* Release OpenCL memory objects */
	freeDeviceBuffers(buffersDevice);

	/* Release OpenCL zone (program, command queue, context) */
	destroyClZone(zone);

	/* Free host resources */
	freeHostBuffers(buffersHost);
	
	/* Free events */
	freeEventsDataStructure(&evts); 
	
	/* Free profile data structure */
	profcl_profile_free(profile);

	/* Free RNG */
	g_rand_free(rng);
	
	/* Bye bye. */
	return 0;
	
}

// Perform simulation
void simulate() {
	// Current iteration
	cl_uint iter = 0; 
		

	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer (	zone.queues[0], grassMatrixDevice, CL_FALSE, 0, grassSizeInBytes, grassMatrixHost, 0, NULL, &ev_writeGrass) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "grassMatrixDevice"); return(-1); }
	
	status = clEnqueueWriteBuffer (	zone.queues[0], rngSeedsDevice, CL_FALSE, 0, rngSeedsSizeInBytes, rngSeedsHost, 0, NULL, &ev_writeRng) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "rngSeedsDevice"); return(-1); }


	//////////////////
	//  SIMULATION! //
	//////////////////
	
	// Guarantee all memory transfers are performed
	cl_event writeEvents[2];
	writeEvents[0] = ev_writeGrass;
	writeEvents[1] = ev_writeRng;
	
	status = clWaitForEvents(2, writeEvents);
	if (status != CL_SUCCESS) { PrintErrorWaitForEvents(status, "write events"); return(-1); }
	
#ifdef CLPROFILER
		// Update data transfer profiling info
		updateEventProfile(profiling, "writeGrass", ev_writeGrass);
		updateEventProfile(profiling, "writeRng", ev_writeRng);
#endif
	
	// Release data transfer events	
	status = clReleaseEvent( ev_writeGrass );
	if (status != CL_SUCCESS) { PrintErrorReleaseEvent(status, "write grass"); return(-1); }
	
	status = clReleaseEvent( ev_writeRng );
	if (status != CL_SUCCESS) { PrintErrorReleaseEvent(status, "write rng"); return(-1); }
	
	// SIMULATION LOOP
	for (iter = 1; iter <= params.iters; iter++) {
		
		//printf("------ Start loop iter %d ---------\n", iter);
		
		//printf("Grass kernel iter %d\n", iter);
		// Grass kernel: grow grass, set number of prey to zero
		status = clEnqueueNDRangeKernel( zone.queues[1], grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, &ev_grass));
		if (status != CL_SUCCESS) { sprintf(msg, "grass_kernel, iteration %d, gws=%d lws=%d ", iter, (int) *grass_gws, (int) *grass_lws); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

/*
		///// Gather statistics //////
		size_t grasscount_gws_loop = grasscount_gws;
		size_t grasscount_lws_loop = grasscount_lws;
		for (unsigned int i = 0; i < numGrassCountLoops) {
			status = clEnqueueBarrier(zone.queues[1]);
			if (status != CL_SUCCESS) { sprintf(msg, "in grass count loops, iteration %d", iter); PrintErrorEnqueueBarrier(status, msg); return(-1); }
			status = clEnqueueNDRangeKernel( 
				zone.queues[1], 
				countgrass_kernel, 
				1, 
				NULL,
				&grasscount2_gws_loops, 
				&grasscount2_lws_loops, 
				0, 
				NULL, 
				events->grasscount + i
			);
			if (status != CL_SUCCESS) { sprintf(msg, "countgrass_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
			grasscount_gws_loop = grasscount_gws_loop / grasscount_lws_loop;
			grasscount_lws_loop = grasscount_gws_loop < grasscount_lws_loop ? grasscount_gws_loop : grasscount_lws_loop;
			
		}

		
		//printf("Count grass 1 iter %d\n", iter);
		// Count grass, part 1
		status = clEnqueueNDRangeKernel( zone.queues[1], countgrass1_kernel, 1, NULL, &grasscount1_gws, &grasscount1_lws, 1, &(events->grass), &(events->grasscount1));
		if (status != CL_SUCCESS) { sprintf(msg, "countgrass1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		
		// Count grass, part 2
		for (int i = 0; i < numGrassCount2Loops; i++) {
			//printf("Count grass 1 iter %d, loop %d\n", iter, i);

			status = clSetKernelArg(countgrass2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextGrassToCount[i]);
			if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countgrass2 kernel"); return(-1); }
			status = clEnqueueNDRangeKernel( 
				zone.queues[1], 
				countgrass2_kernel, 
				1, 
				NULL,
				&grasscount2_gws[i], 
				&grasscount2_lws, 
				1, 
				&(events->grasscount1), 
				events->grasscount2 + i
			);
			if (status != CL_SUCCESS) { sprintf(msg, "countgrass2_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

			status = clEnqueueBarrier(zone.queues[1]);
			if (status != CL_SUCCESS) { sprintf(msg, "in grass count loops, iteration %d", iter); PrintErrorEnqueueBarrier(status, msg); return(-1); }

			//events->grasscount2_index++;

		}
		
		// Get statistics 
		//printf("Read back stats iter %d, will read %d iters of stats\n", iter, numItersToRead);
		status = clEnqueueReadBuffer ( zone.queues[0], statsDevice, CL_FALSE, 0, sizeof(STATS), statsArray + iter - 1, 0, NULL, &(events->readStats));
		if (status != CL_SUCCESS) { sprintf(msg, "read stats, iteration %d", iter); PrintErrorEnqueueReadWriteBuffer(status, msg); return(-1); }
		
*/
		// WE CAN OPTIMIZE THIS WITHOUT BARRIER OR CLFINISH, AND START NEW ITERATION EVEN IF STATISTICS ARE NOT YET BACK (if profiling is off)
		// Guarantee all tasks in queue are terminated...
		status = clFinish(zone.queues[0]); 
		status = clFinish(zone.queues[1]); 
		if (status != CL_SUCCESS) { sprintf(msg, "sim loop, iteration %d", iter); PrintErrorFinish(status, msg); return(-1); }

#ifdef CLPROFILER
		// Update simulation profiling info
		updateEventProfile(profiling, "grass", ev_grass);

#endif
		status = clReleaseEvent( ev_grass );
		if (status != CL_SUCCESS) { sprintf(msg, "grass, iteration %d", iter); PrintErrorReleaseEvent(status, msg); return(-1); }
		
		// Release current iteration events
/*		status = clReleaseEvent( events->readStats );
		if (status != CL_SUCCESS) { sprintf(msg, "read stats, iteration %d", iter); PrintErrorReleaseEvent(status, msg); return(-1); }
		status = clReleaseEvent( events->grass );
		if (status != CL_SUCCESS) { sprintf(msg, "countgrass1, iteration %d", iter); PrintErrorReleaseEvent(status, msg); return(-1); }
		
		for (int i = 0; i < numGrassCount2Loops; i++) {
			status = clReleaseEvent( *(events->grasscount2 + i) );
			if (status != CL_SUCCESS) { sprintf(msg, "countgrass2, iteration %d", iter); PrintErrorReleaseEvent(status, msg); return(-1); }
		}*/

	
		
	}

	// Guarantee all activity has terminated...
	clFinish(zone.queues[0]);
	clFinish(zone.queues[1]);
	
}

// Perform profiling analysis
void profilingAnalysis(Events* evts, Parameters params) {
#ifdef CLPROFILER
	/* Perform detailed analysis. */
	for (guint i = 0; i < params.iters; i++) {
		//TODO Add events to profiling info
	}
	profcl_profile_aggregate(profile);
	profcl_profile_overmat(profile);	
#endif
	/* Show profiling info. */
	profcl_print_info(profile);
}

// Compute worksizes depending on the device type and number of available compute units
void computeWorkSizes(Parameters params, cl_device device, GlobalWorkSizes *gws, LocalWorkSizes *lws) {
	
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
	
	/* Check for errors on the OpenCL call */
	if (status != CL_SUCCESS) {PrintErrorGetDeviceInfo( status, "Get maximum workgroup size." ); exit(EXIT_FAILURE); }

	/* grass growth worksizes */
#ifdef LWS_GRASS
	lws->grass = LWS_GRASS;
#else
	lws->grass = maxWorkGroupSize;
#endif
	gws->grass = lws->grass * ceil(((float) (params.grid_x * params.grix_y)) / lws->grass);
	
	/* grass count worksizes */
#ifdef LWS_REDUCEGRASS1 //TODO This should depend on number of cells, vector width, etc.
	lws->reducegrass1 = LWS_REDUCEGRASS1; 
#else
	lws->reducegrass1 = maxWorkGroupSize;
#endif	
	gws->reducegrass1 = lws->reducegrass1 * lws->reducegrass1;
	lws->reducegrass2 = lws->reducegrass1;
	gws->reducegrass2 = lws->reducegrass1;	
}

// Print worksizes
void printWorkSizes(GlobalWorkSizes *gws, LocalWorkSizes *lws) {
	printf("Kernel work sizes:\n");
	printf("grass_gws=%d\tgrass_lws=%d\n", (int) gws->grass, (int) lws->grass);
	printf("reducegrass1_gws=%d\treducegrass1_lws=%d\n", (int) gws->reducegrass1, (int) lws->reducegrass1);
	printf("grasscount2_lws/gws=%d\n", (int) lws->reducegrass2);
}

// Get kernel entry points
void createKernels(cl_program program, Kernels* krnls) {
	cl_int status;
	krnls->grass = clCreateKernel( program, "grass", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "grass kernel"); exit(EXIT_FAILURE); }
	krnls->reduce_grass1 = clCreateKernel( program, "reduceGrass1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "reduceGrass1 kernel"); exit(EXIT_FAILURE); }
	krnls->reduce_grass2 = clCreateKernel( program, "reduceGrass2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "reduceGrass2 kernel"); exit(EXIT_FAILURE); }
}

// Release kernels
void freeKernels(Kernels* krnls) {
	clReleaseKernel(krnls->grass);
	clReleaseKernel(krnls->reduce_grass1); 
	clReleaseKernel(krnls->reduce_grass2);
}


// Initialize simulation parameters in host, to be sent to GPU
SimParams initSimParams(Parameters params) {
	SimParams simParams;
	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = MAX_AGENTS;
	simParams.grass_restart = params.grass_restart;
	return simParams;
}

//  Set fixed kernel arguments.
void setFixedKernelArgs(Kernels* krnls, BuffersDevice* buffersDevice, SimParams simParams, LocalWorkSizes lws) {

	cl_int status;
	
	/* Grass kernel */
	status = clSetKernelArg(krnls->grass, 0, sizeof(cl_mem), (void *) &buffersDevice->cells_grass_alive);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of grass kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(krnls->grass, 1, sizeof(cl_mem), (void *) &buffersDevice->cells_grass_timer);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of grass kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(grass_kernel, 2, sizeof(SIM_PARAMS), (void *) &simParams);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of grass kernel"); exit(EXIT_FAILURE); }

	
	status = clSetKernelArg(grass_kernel, 3, sizeof(cl_mem), (void *) &bufferDevice->rng_seeds);//TODO TEMPORARY, REMOVE!
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of grass kernel"); exit(EXIT_FAILURE); }//TODO TEMPORARY, REMOVE!

	/* reduce_grass1 kernel */
	status = clSetKernelArg(krnls->reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice->cells_grass_alive);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of reduce_grass1 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(krnls->reduce_grass1, 1, lws->reduce_grass1 * sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of reduce_grass1 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(krnls->reduce_grass1, 2, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of reduce_grass1 kernel"); exit(EXIT_FAILURE); }

	/* reduce_grass2 kernel */
	status = clSetKernelArg(krnls->reduce_grass2, 0, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of reduce_grass2 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(krnls->reduce_grass2, 1, lws->reduce_grass2 * sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of reduce_grass2 kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(krnls->reduce_grass2, 2, sizeof(cl_mem), (void *) &buffersDevice->stats);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of reduce_grass2 kernel"); exit(EXIT_FAILURE); }
	
}

// Save results
void saveResults(char* filename, Statistics* statsArray, Parameters params) {
	FILE * fp1 = fopen(filename,"w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArray[i].sheep, statsArray[i].wolves, statsArray[i].grass );
	fclose(fp1);
}

// Determine buffer sizes
void getDataSizesInBytes(Parameters params, DataSizes* dataSizes, GlobalWorkSizes gws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(STATS);
	
	/* Environment cells */
	dataSizes->cells_grass_alive = (params.grid_xy + REDUCE_GRASS_VECSIZE) * sizeof(cl_uchar);
	dataSizes->cells_grass_timer = params.grid_xy * sizeof(cl_short);
	dataSizes->cells_agents_number = params.grid_xy * sizeof(cl_short);
	dataSizes->cells_agents_index = params.grid_xy * sizeof(cl_short);
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local = lws->reduce_grass1 * sizeof(cl_uint);
	dataSizes->reduce_grass_global = gws->reduce_grass2 * sizeof(cl_uint);
	
	/* Rng seeds */
	dataSizes->rng_seeds = MAX_GWS * sizeof(cl_ulong);

}

// Initialize host buffers
void createHostBuffers(BuffersHost* buffersHost, DataSizes* dataSizes, Parameters params, GRand* rng) {
	
	/* Statistics */
	buffersHost->stats = (Statistics*) malloc(dataSizes->stats);
	buffersHost->stats[0].sheep = params.init_sheep;
	buffersHost->stats[0].wolves = params.init_wolves;
	buffersHost->stats[0].grass = 0;
	
	/* Environment cells */
	buffersHost->cell_grass_alive = (cl_uchar*) malloc(dataSizes->cells_grass_alive);
	buffersHost->cell_grass_timer = (cl_ushort*) malloc(dataSizes->cells_grass_timer);
	buffersHost->cell_agents_number = (cl_ushort*) malloc(dataSizes->cells_agents_number);
	buffersHost->cell_agents_index = (cl_ushort*) malloc(dataSizes->cells_agents_index);
	
	for(guint i = 0; i < params.grid_x; i++) {
		for (guint j = 0; j < params.grid_y; j++) {
			guint gridIndex = i + j * params.grid_x;
			buffersHost->cell_grass_timer[gridIndex] = 
				g_rand_int_range(rng, 0, 2) == 0 
				? 0 
				: g_rand_int_range(rng, 1, params.grass_restart + 1);
			if (buffersHost->cell_grass_timer[gridIndex] == 0) {
				buffersHost->stats[0].grass++;
				buffersHost->cell_grass_alive[gridIndex] = 1;
			} else {
				buffersHost->cell_grass_alive[gridIndex] = 0;
			}
		}
	}
	
	/* RNG seeds */
	buffersHost->rng_seeds = (cl_ulong*) malloc(dataSizes->rng_seeds);
	for (int i = 0; i < MAX_GWS; i++) {
		buffersHost->rng_seeds[i] = (cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
	}	
	
}

// Free host buffers
void freeHostBuffers(BuffersHost* buffersHost) {
	free(buffersHost->stats);
	free(buffersHost->cell_grass_alive);
	free(buffersHost->cell_grass_timer);
	free(buffersHost->cell_agents_number);
	free(buffersHost->cell_agents_index);
	free(buffersHost->rng_seeds);
}

// Initialize device buffers
void createDeviceBuffers(cl_context context, BuffersHost* buffersHost, BuffersDevice* buffersDevice, DataSizes* dataSizes) {
	
	cl_int status;
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(Statistics), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->stats"); exit(EXIT_FAILURE); }

	/* Cells */
	buffersDevice->cells_grass_alive = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(dataSizes->cells_grass_alive), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->cells_grass_alive"); exit(EXIT_FAILURE); }
	buffersDevice->cells_grass_timer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(dataSizes->cells_grass_timer), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->cells_grass_timer"); exit(EXIT_FAILURE); }
	buffersDevice->cells_agents_number = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(dataSizes->cells_agents_number), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->cells_agents_number"); exit(EXIT_FAILURE); }
	buffersDevice->cells_agents_index = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(dataSizes->cells_agents_index), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->cells_agents_index"); exit(EXIT_FAILURE); }

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, dataSizes->reduce_grass_global, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->reduce_grass_global"); exit(EXIT_FAILURE); }

	/* RNG seeds. */
	buffersDevice->rng_seeds = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, dataSizes->rng_seeds, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "buffersDevice->rng_seeds"); exit(EXIT_FAILURE); }
}

// Free device buffers
void freeDeviceBuffers(BuffersDevice* buffersDevice) {
	clReleaseMemObject(buffersDevice->stats);
	clReleaseMemObject(buffersDevice->cells_grass_alive);
	clReleaseMemObject(buffersDevice->cells_grass_timer);
	clReleaseMemObject(buffersDevice->cells_agents_number);
	clReleaseMemObject(buffersDevice->cells_agents_index);
	clReleaseMemObject(buffersDevice->reduce_grass);
	clReleaseMemObject(buffersDevice->rng_seeds);
}

// Create event data structure
void createEventsDataStructure(Parameters params, Events* evts) {

	evts->grass = (cl_event*) malloc(params.iters * sizeof(cl_event));
	evts->read_stats = (cl_event*) malloc(params.iters * sizeof(cl_event));
	evts->reduce_grass1 = (cl_event*) malloc(params.iters * sizeof(cl_event));
	evts->reduce_grass2 = (cl_event*) malloc(params.iters * sizeof(cl_event));
}

// Free event data structure
void freeEventsDataStructure(Parameters params, Events* evts) {
	clReleaseEvent(evts->write_grass);
	clReleaseEvent(evts->write_rng);
	for (guint i = 0; i < params.iters; i++) {
		clReleaseEvent(evts->grass[i]);
		clReleaseEvent(evts->read_stats[i]);
		clReleaseEvent(evts->reduce_grass1[i]);
		clReleaseEvent(evts->reduce_grass2[i]);
	}
	free(evts->grass);
	free(evts->read_stats);
	free(evts->reduce_grass1);
	free(evts->reduce_grass2);
}
