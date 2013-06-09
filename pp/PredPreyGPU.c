/** 
 * @file
 * @brief PredPrey OpenCL GPU implementation.
 * @todo Use radix sort or parallel prefix sum (scan)?
 */

#include "PredPreyGPU.h"

#define MAX_AGENTS 1048576
#define MAX_GWS 1048576
#define REDUCE_GRASS_VECSIZE 4

#ifdef CLPROFILER
	#define QUEUE_PROPERTIES CL_QUEUE_PROFILING_ENABLE
#else
	#define QUEUE_PROPERTIES 0
#endif

//#define LWS_GRASS 256
//#define LWS_REDUCEGRASS1 256

//#define SEED 0

/* OpenCL kernel files */
const char* kernelFiles[] = {"pp/PredPreyCommon_Kernels.cl", "pp/PredPreyGPU_Kernels.cl"};


/**
 *  @brief Main program.
 * */
int main(int argc, char **argv)
{

	/* Program vars. */
	PPGGlobalWorkSizes gws;
	PPGLocalWorkSizes lws;
	PPGKernels krnls = {NULL, NULL, NULL};
	PPGEvents evts = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPGDataSizes dataSizes;
	PPGBuffersHost buffersHost = {NULL, NULL, NULL, NULL, NULL, NULL};
	PPGBuffersDevice buffersDevice = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PPParameters params;

	/* Aux vars. */
	cl_int status;
	int status_pp;
	
	/* Error management. */
	GError *err = NULL;
	
	/* Profiling / Timmings. */
	ProfCLProfile* profile = profcl_profile_new();
	
	/* Create RNG and set seed. */
#ifdef SEED
	GRand* rng = g_rand_new_with_seed(SEED);
#else
	GRand* rng = g_rand_new();
#endif	
	
	/* Get simulation parameters */
	status_pp = pp_load_params(&params, DEFAULT_PARAMS_FILE, &err);
	pp_if_error_goto(status_pp, err, error);

	/* Set simulation parameters in a format more adequate for this program. */
	PPGSimParams simParams = ppg_simparams_init(params);

	/* Get the required CL zone. */
	CLUZone zone;
	status = clu_zone_new(&zone, CL_DEVICE_TYPE_GPU, 2, QUEUE_PROPERTIES, clu_menu_device_selector, NULL, &err);
	clu_if_error_goto(status, err, error);

	/* Compute work sizes for different kernels and print them to screen. */
	status = ppg_worksizes_compute(params, zone.device, &gws, &lws, &err);
	clu_if_error_goto(status, err, error);
	ppg_worksizes_print(gws, lws);

	/* Compiler options. */
	char* compilerOpts = ppg_compiler_opts_build(gws, lws, simParams);
	printf("%s\n", compilerOpts);

	/* Build program. */
	status = clu_program_create(&zone, kernelFiles, 2, compilerOpts, &err);
	clu_if_error_goto(status, err, error);

	/* Create kernels. */
	status = ppg_kernels_create(zone.program, &krnls, &err);
	clu_if_error_goto(status, err, error);
	
	/* Determine size in bytes for host and device data structures. */
	ppg_datasizes_get(params, simParams, &dataSizes, gws, lws);

	/* Initialize host buffers. */
	ppg_hostbuffers_create(&buffersHost, &dataSizes, params, rng);

	/* Create device buffers */
	status = ppg_devicebuffers_create(zone.context, &buffersHost, &buffersDevice, &dataSizes, &err);
	clu_if_error_goto(status, err, error);
	
	/* Create events data structure. */
	ppg_events_create(params, &evts);

	/*  Set fixed kernel arguments. */
	status = ppg_kernelargs_set(&krnls, &buffersDevice, simParams, lws, &dataSizes, &err);
	clu_if_error_goto(status, err, error);
	
	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	/* Simulation!! */
	status = ppg_simulate(params, zone, gws, lws, krnls, &evts, dataSizes, buffersHost, buffersDevice, &err);
	clu_if_error_goto(status, err, error);

	/* Stop basic timing / profiling. */
	profcl_profile_stop(profile);  
	
	/* Output results to file */
	ppg_results_save("stats.txt", buffersHost.stats, params);

	/* Analyze events, show profiling info. */
	status = ppg_profiling_analyze(profile, &evts, params, &err);
	clu_if_error_goto(status, err, error);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	fprintf(stderr, "Error %d: %s\n", err->code, err->message);
	g_error_free(err);
	if (zone.build_log) clu_build_log_print(&zone);

cleanup:
	/* Release OpenCL kernels */
	ppg_kernels_free(&krnls);
	
	/* Release OpenCL memory objects */
	ppg_devicebuffers_free(&buffersDevice);

	/* Release OpenCL zone (program, command queue, context) */
	clu_zone_free(&zone);

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
cl_int ppg_simulate(PPParameters params, CLUZone zone, 
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
	status = clEnqueueWriteBuffer(zone.queues[0], buffersDevice.cells_grass_alive, CL_FALSE, 0, dataSizes.cells_grass_alive, buffersHost.cells_grass_alive, 0, NULL, &evts->write_grass_alive);
	clu_if_error_create_error_return(status, err, "Write device buffer: cells_grass_alive");
	
	status = clEnqueueWriteBuffer(zone.queues[0], buffersDevice.cells_grass_timer, CL_FALSE, 0, dataSizes.cells_grass_timer, buffersHost.cells_grass_timer, 0, NULL, &evts->write_grass_timer);
	clu_if_error_create_error_return(status, err, "Write device buffer: cells_grass_timer");

	status = clEnqueueWriteBuffer(zone.queues[0], buffersDevice.cells_agents_number, CL_FALSE, 0, dataSizes.cells_agents_number, buffersHost.cells_agents_number, 0, NULL, &evts->write_agents_number);
	clu_if_error_create_error_return(status, err, "Write device buffer: cells_agents_number");

	status = clEnqueueWriteBuffer(zone.queues[0], buffersDevice.cells_agents_index, CL_FALSE, 0, dataSizes.cells_agents_index, buffersHost.cells_agents_index, 0, NULL, &evts->write_agents_index);
	clu_if_error_create_error_return(status, err, "Write device buffer: cells_agents_index");

	status = clEnqueueWriteBuffer(zone.queues[0], buffersDevice.rng_seeds, CL_FALSE, 0, dataSizes.rng_seeds, buffersHost.rng_seeds, 0, NULL, &evts->write_rng);
	clu_if_error_create_error_return(status, err, "Write device buffer: rng_seeds");
	
	/* SIMULATION LOOP */
	for (iter = 1; iter <= params.iters; iter++) {
		
		/* Grass kernel: grow grass, set number of prey to zero */
		status = clEnqueueNDRangeKernel(
			zone.queues[0], 
			krnls.grass, 1, 
			NULL, 
			&gws.grass, 
			&lws.grass, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&evts->grass[iter - 1]
#else
			NULL
#endif
		);
		clu_if_error_create_error_return(status, err, "Kernel exec.: grass, iteration %d", iter);

		/* Perform grass reduction. */
		status = clEnqueueNDRangeKernel(
			zone.queues[0], 
			krnls.reduce_grass1, 
			1, 
			NULL, 
			&gws.reduce_grass1, 
			&lws.reduce_grass1, 
			0, 
			NULL,
#ifdef CLPROFILER
			&evts->reduce_grass1[iter - 1]
#else
			NULL
#endif
		);
		clu_if_error_create_error_return(status, err, "Kernel exec.: reduce_grass1, iteration %d", iter);
		
		status = clEnqueueNDRangeKernel(
			zone.queues[0], 
			krnls.reduce_grass2, 
			1, 
			NULL, 
			&gws.reduce_grass2, 
			&lws.reduce_grass2, 
			iter > 1 ? 1 : 0,  
			iter > 1 ? &evts->read_stats[iter - 2] : NULL,
			&evts->reduce_grass2[iter - 1]
		);
		clu_if_error_create_error_return(status, err, "Kernel exec.: reduce_grass2, iteration %d", iter);

		/* Get statistics. */
		status = clEnqueueReadBuffer(
			zone.queues[1], 
			buffersDevice.stats, 
			CL_FALSE, 
			0, 
			sizeof(PPStatistics), 
			&buffersHost.stats[iter], 
			1, 
			&evts->reduce_grass2[iter - 1], 
			&evts->read_stats[iter - 1]
		);
		clu_if_error_create_error_return(status, err, "Read back stats, iteration %d", iter);
		
	}
	
	/* Guarantee all activity has terminated... */
	status = clFinish(zone.queues[0]);
	clu_if_error_create_error_return(status, err, "Finish for queue 0 after simulation");
	status = clFinish(zone.queues[1]);
	clu_if_error_create_error_return(status, err, "Finish for queue 1 after simulation");
	
	/* Successfull simulation! */
	return CL_SUCCESS;
	
}

/* Perform profiling analysis */
cl_int ppg_profiling_analyze(ProfCLProfile* profile, PPGEvents* evts, PPParameters params, GError** err) {

#ifdef CLPROFILER
	
	/* Perfomed detailed analysis onfy if profiling flag is set. */
	cl_int status;
	
	/* One time events. */
	profcl_profile_add(profile, profcl_evinfo_get("Write data", evts->write_grass_alive, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: write_grass_alive");

	profcl_profile_add(profile, profcl_evinfo_get("Write data", evts->write_grass_timer, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: write_grass_timer");

	profcl_profile_add(profile, profcl_evinfo_get("Write data", evts->write_agents_number, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: write_agents_number");

	profcl_profile_add(profile, profcl_evinfo_get("Write data", evts->write_agents_index, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: write_agents_index");

	profcl_profile_add(profile, profcl_evinfo_get("Write data", evts->write_rng, &status));
	clu_if_error_create_error_return(status, err, "Add event to profile: write_rng");

	/* Simulation loop events. */
	for (guint i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("Grass", evts->grass[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: grass[%d]", i);

		profcl_profile_add(profile, profcl_evinfo_get("Read stats", evts->read_stats[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: read_stats[%d]", i);

		profcl_profile_add(profile, profcl_evinfo_get("Reduce grass 1", evts->reduce_grass1[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: reduce_grass1[%d]", i);

		profcl_profile_add(profile, profcl_evinfo_get("Reduce grass 2", evts->reduce_grass2[i], &status));
		clu_if_error_create_error_return(status, err, "Add event to profile: reduce_grass2[%d]", i);

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
	
	/* Check for errors on the OpenCL call */
	clu_if_error_create_error_return(status, err, "Get device info.");

	/* grass growth worksizes */
#ifdef LWS_GRASS
	lws->grass = LWS_GRASS;
#else
	lws->grass = maxWorkGroupSize;
#endif
	gws->grass = lws->grass * ceil(((float) (params.grid_x * params.grid_y)) / lws->grass);
	
	/* grass count worksizes */
#ifdef LWS_REDUCEGRASS1 //TODO This should depend on number of cells, vector width, etc.
	lws->reduce_grass1 = LWS_REDUCEGRASS1; 
#else
	lws->reduce_grass1 = maxWorkGroupSize;
#endif	
	gws->reduce_grass1 = MIN(lws->reduce_grass1 * lws->reduce_grass1, lws->reduce_grass1 * ceil(((float) (params.grid_x * params.grid_y)) / REDUCE_GRASS_VECSIZE / lws->reduce_grass1));
	lws->reduce_grass2 = nlpo2(gws->reduce_grass1 / lws->reduce_grass1);
	gws->reduce_grass2 = lws->reduce_grass2;
	
	return CL_SUCCESS;
}

// Print worksizes
void ppg_worksizes_print(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {
	printf("Kernel work sizes:\n");
	printf("grass_gws=%d\tgrass_lws=%d\n", (int) gws.grass, (int) lws.grass);
	printf("reducegrass1_gws=%d\treducegrass1_lws=%d\n", (int) gws.reduce_grass1, (int) lws.reduce_grass1);
	printf("grasscount2_lws/gws=%d\n", (int) lws.reduce_grass2);
}

// Get kernel entry points
cl_int ppg_kernels_create(cl_program program, PPGKernels* krnls, GError** err) {
	
	cl_int status;
	
	krnls->grass = clCreateKernel(program, "grass", &status);
	clu_if_error_create_error_return(status, err, "Create kernel: grass");
	
	krnls->reduce_grass1 = clCreateKernel(program, "reduceGrass1", &status);
	clu_if_error_create_error_return(status, err, "Create kernel: reduceGrass1");
	
	krnls->reduce_grass2 = clCreateKernel(program, "reduceGrass2", &status);
	clu_if_error_create_error_return(status, err, "Create kernel: reduceGrass2");
	
	return CL_SUCCESS;
}

// Release kernels
void ppg_kernels_free(PPGKernels* krnls) {
	if (krnls->grass) clReleaseKernel(krnls->grass);
	if (krnls->reduce_grass1) clReleaseKernel(krnls->reduce_grass1); 
	if (krnls->reduce_grass2) clReleaseKernel(krnls->reduce_grass2);
}


// Initialize simulation parameters in host, to be sent to GPU
PPGSimParams ppg_simparams_init(PPParameters params) {
	PPGSimParams simParams;
	simParams.size_x = params.grid_x;
	simParams.size_y = params.grid_y;
	simParams.size_xy = params.grid_x * params.grid_y;
	simParams.max_agents = MAX_AGENTS;
	simParams.grass_restart = params.grass_restart;
	return simParams;
}

//  Set fixed kernel arguments.
cl_int ppg_kernelargs_set(PPGKernels* krnls, PPGBuffersDevice* buffersDevice, PPGSimParams simParams, PPGLocalWorkSizes lws, PPGDataSizes* dataSizes, GError** err) {

	/* Aux. variable. */
	cl_int status;
	
	/* Grass kernel */
	status = clSetKernelArg(krnls->grass, 0, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_alive);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 0 of grass");

	status = clSetKernelArg(krnls->grass, 1, sizeof(cl_mem), (void*) &buffersDevice->cells_grass_timer);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 1 of grass");

	status = clSetKernelArg(krnls->grass, 2, sizeof(PPGSimParams), (void*) &simParams);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 2 of grass");
	
	status = clSetKernelArg(krnls->grass, 3, sizeof(cl_mem), (void*) &buffersDevice->rng_seeds);//TODO TEMPORARY, REMOVE!
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 3 of grass");//TODO TEMPORARY, REMOVE!

	/* reduce_grass1 kernel */
	status = clSetKernelArg(krnls->reduce_grass1, 0, sizeof(cl_mem), (void *) &buffersDevice->cells_grass_alive);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 0 of reduce_grass1");

	status = clSetKernelArg(krnls->reduce_grass1, 1, dataSizes->reduce_grass_local, NULL);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 1 of reduce_grass1");

	status = clSetKernelArg(krnls->reduce_grass1, 2, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 2 of reduce_grass1");

	/* reduce_grass2 kernel */
	status = clSetKernelArg(krnls->reduce_grass2, 0, sizeof(cl_mem), (void *) &buffersDevice->reduce_grass_global);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 0 of reduce_grass2");

	status = clSetKernelArg(krnls->reduce_grass2, 1, lws.reduce_grass2 *REDUCE_GRASS_VECSIZE*sizeof(cl_uint), NULL);//TODO Put this size in dataSizes
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 1 of reduce_grass2");

	status = clSetKernelArg(krnls->reduce_grass2, 2, sizeof(cl_mem), (void *) &buffersDevice->stats);
	clu_if_error_create_error_return(status, err, "Set kernel args: arg 2 of reduce_grass2");
	
	/* Success! */
	return CL_SUCCESS;
	
}

// Save results
void ppg_results_save(char* filename, PPStatistics* statsArray, PPParameters params) {
	FILE * fp1 = fopen(filename,"w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArray[i].sheep, statsArray[i].wolves, statsArray[i].grass );
	fclose(fp1);
}

// Determine buffer sizes
void ppg_datasizes_get(PPParameters params, PPGSimParams simParams, PPGDataSizes* dataSizes, PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws) {

	/* Statistics */
	dataSizes->stats = (params.iters + 1) * sizeof(PPStatistics);
	
	/* Environment cells */
	dataSizes->cells_grass_alive = (simParams.size_xy + REDUCE_GRASS_VECSIZE) * sizeof(cl_uchar);
	dataSizes->cells_grass_timer = simParams.size_xy * sizeof(cl_short);
	dataSizes->cells_agents_number = simParams.size_xy * sizeof(cl_short);
	dataSizes->cells_agents_index = simParams.size_xy * sizeof(cl_short);
	
	/* Grass reduction. */
	dataSizes->reduce_grass_local = lws.reduce_grass1 * REDUCE_GRASS_VECSIZE * sizeof(cl_uint); //TODO Verify that GPU supports this local memory requirement
	dataSizes->reduce_grass_global = gws.reduce_grass2 * REDUCE_GRASS_VECSIZE * sizeof(cl_uint);
	
	/* Rng seeds */
	dataSizes->rng_seeds = MAX_GWS * sizeof(cl_ulong);

}

// Initialize host buffers
void ppg_hostbuffers_create(PPGBuffersHost* buffersHost, PPGDataSizes* dataSizes, PPParameters params, GRand* rng) {
	
	/* Statistics */
	buffersHost->stats = (PPStatistics*) malloc(dataSizes->stats);
	buffersHost->stats[0].sheep = params.init_sheep;
	buffersHost->stats[0].wolves = params.init_wolves;
	buffersHost->stats[0].grass = 0;
	
	/* Environment cells */
	buffersHost->cells_grass_alive = (cl_uchar*) malloc(dataSizes->cells_grass_alive);
	buffersHost->cells_grass_timer = (cl_ushort*) malloc(dataSizes->cells_grass_timer);
	buffersHost->cells_agents_number = (cl_ushort*) malloc(dataSizes->cells_agents_number);
	buffersHost->cells_agents_index = (cl_ushort*) malloc(dataSizes->cells_agents_index);
	
	for(guint i = 0; i < params.grid_x; i++) {
		for (guint j = 0; j < params.grid_y; j++) {
			guint gridIndex = i + j * params.grid_x;
			buffersHost->cells_grass_timer[gridIndex] = 
				g_rand_int_range(rng, 0, 2) == 0 
				? 0 
				: g_rand_int_range(rng, 1, params.grass_restart + 1);
			if (buffersHost->cells_grass_timer[gridIndex] == 0) {
				buffersHost->stats[0].grass++;
				buffersHost->cells_grass_alive[gridIndex] = 1;
			} else {
				buffersHost->stats[0].grass++;
				buffersHost->cells_grass_alive[gridIndex] = 1;
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
void ppg_hostbuffers_free(PPGBuffersHost* buffersHost) {
	free(buffersHost->stats);
	free(buffersHost->cells_grass_alive);
	free(buffersHost->cells_grass_timer);
	free(buffersHost->cells_agents_number);
	free(buffersHost->cells_agents_index);
	free(buffersHost->rng_seeds);
}

// Initialize device buffers
cl_int ppg_devicebuffers_create(cl_context context, PPGBuffersHost* buffersHost, PPGBuffersDevice* buffersDevice, PPGDataSizes* dataSizes, GError** err) {
	
	cl_int status;
	
	/* Statistics */
	buffersDevice->stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(PPStatistics), NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: stats");

	/* Cells */
	buffersDevice->cells_grass_alive = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_grass_alive, NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: cells_grass_alive");

	buffersDevice->cells_grass_timer = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_grass_timer, NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: cells_grass_timer");

	buffersDevice->cells_agents_number = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_agents_number, NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: cells_agents_number");

	buffersDevice->cells_agents_index = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->cells_agents_index, NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: cells_agents_index");

	/* Grass reduction (count) */
	buffersDevice->reduce_grass_global = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->reduce_grass_global, NULL, &status);
	clu_if_error_create_error_return(status, err, "Create device buffer: reduce_grass_global");

	/* RNG seeds. */
	buffersDevice->rng_seeds = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSizes->rng_seeds, NULL, &status );
	clu_if_error_create_error_return(status, err, "Create device buffer: rng_seeds");
	
	/* Success! */
	return CL_SUCCESS;

}

// Free device buffers
void ppg_devicebuffers_free(PPGBuffersDevice* buffersDevice) {
	clReleaseMemObject(buffersDevice->stats);
	clReleaseMemObject(buffersDevice->cells_grass_alive);
	clReleaseMemObject(buffersDevice->cells_grass_timer);
	clReleaseMemObject(buffersDevice->cells_agents_number);
	clReleaseMemObject(buffersDevice->cells_agents_index);
	clReleaseMemObject(buffersDevice->reduce_grass_global);
	clReleaseMemObject(buffersDevice->rng_seeds);
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
	if (evts->write_grass_alive) clReleaseEvent(evts->write_grass_alive);
	if (evts->write_grass_timer) clReleaseEvent(evts->write_grass_timer);
	if (evts->write_agents_number) clReleaseEvent(evts->write_agents_number);
	if (evts->write_agents_index) clReleaseEvent(evts->write_agents_index);
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
char* ppg_compiler_opts_build(PPGGlobalWorkSizes gws, PPGLocalWorkSizes lws, PPGSimParams simParams) {
	char* compilerOptsStr;
	GString* compilerOpts = g_string_new("");
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_VECSIZE=%d ", REDUCE_GRASS_VECSIZE);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKITEMS=%d ", (unsigned int) gws.reduce_grass1);
	g_string_append_printf(compilerOpts, "-D REDUCE_GRASS_NUM_WORKGROUPS=%d ", (unsigned int) (gws.reduce_grass1 / lws.reduce_grass1));
	g_string_append_printf(compilerOpts, "-D CELL_NUM=%d", simParams.size_xy);
	compilerOptsStr = compilerOpts->str;
	g_string_free(compilerOpts, FALSE);
	return compilerOptsStr;
}

