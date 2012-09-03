#include "PredPreyGPU.hpp"

#define MAX_AGENTS 1048576

#define SHEEP_ID 0
#define WOLF_ID 1

#define CELL_SPACE 4 // 0-Grass state, 1-number of agents in cell, 2-pointer to first agent in cell, 3-unused
#define CELL_GRASS_OFFSET 0
#define CELL_NUMAGENTS_OFFSET 1
#define CELL_AGINDEX_OFFSET 2

#define LWS_GPU_MAX 512
#define LWS_GPU_PREF 128
#define LWS_GPU_MIN 8

#define LWS_GPU_PREF_2D_X 16
#define LWS_GPU_PREF_2D_Y 8

#define MAX_GRASS_COUNT_LOOPS 5 //More than enough...

// Global work sizes
size_t agentsort_gws, agent_gws, grass_gws[2], agentcount1_gws, agentcount2_gws, grasscount1_gws, grasscount2_gws[MAX_GRASS_COUNT_LOOPS];
// Local work sizes
size_t agentsort_lws, agent_lws, grass_lws[2], agentcount1_lws, agentcount2_lws, grasscount1_lws, grasscount2_lws;
int effectiveNextGrassToCount[MAX_GRASS_COUNT_LOOPS];
int numGrassCount2Loops;
// Kernels
cl_kernel grass_kernel, agentmov_kernel, agentupdate_kernel, sort_kernel, agentaction_kernel, countagents1_kernel, countagents2_kernel, countgrass1_kernel, countgrass2_kernel;

// Main stuff
int main(int argc, char ** argv)
{

	// Status var aux
	cl_int status;	
	
	// Init random number generator
	srandom((unsigned)(time(0)));

	// Timmings
	struct timeval time1, time0;
	double dt = 0;

	// 1. Get the required CL zone.
	CLZONE zone = getClZone("NVIDIA Corporation", "PredPreyGPU_Kernels.cl", CL_DEVICE_TYPE_GPU);

	// 2. Get simulation parameters
	PARAMS params = loadParams(CONFIG_FILE);

	// 3. Compute work sizes for different kernels and print them to screen
	computeWorkSizes(params, zone.device_type, zone.cu);	
	printFixedWorkSizes();

	// 4. obtain kernels entry points.
	getKernelEntryPoints(zone.program);

	// 5. Create and initialize host buffers
	// Statistics
	STATS statistics;
	size_t statsSize = (params.iters + 1) * sizeof(unsigned int);
	statistics.sheep = (unsigned int *) malloc(statsSize);
	statistics.wolves = (unsigned int *) malloc(statsSize);
	statistics.grass = (unsigned int *) malloc(statsSize);
	statistics.sheep[0] = params.init_sheep;
	statistics.wolves[0] = params.init_wolves;
	statistics.grass[0] = 0;
	// Agent array
	size_t agentsSizeInBytes = MAX_AGENTS * sizeof(AGENT);
	AGENT * agentArrayHost = (AGENT *) malloc(agentsSizeInBytes);
	for(unsigned int i = 0; i < MAX_AGENTS; i++)
	{
		agentArrayHost[i].x = rand() % params.grid_x;
		agentArrayHost[i].y = rand() % params.grid_y;
		if (i < params.init_sheep)
		{
			agentArrayHost[i].energy = 1 + (rand() % (params.sheep_gain_from_food * 2));
			agentArrayHost[i].type = 0; // Sheep
			agentArrayHost[i].alive = 1;
		}
		else if (i < params.init_sheep + params.init_wolves)
		{
			agentArrayHost[i].energy = 1 + (rand() % (params.wolves_gain_from_food * 2));
			agentArrayHost[i].type = 1; // Wolves
			agentArrayHost[i].alive = 1;
		}
		else {
			agentArrayHost[i].energy = 0;
			agentArrayHost[i].type = 0;
			agentArrayHost[i].alive = 0;
		}
	}
	// Grass matrix
	size_t grassSizeInBytes = CELL_SPACE * params.grid_x * params.grid_y * sizeof(cl_uint);
	cl_uint * grassMatrixHost = (cl_uint *) malloc(grassSizeInBytes);
	for(unsigned int i = 0; i < params.grid_x; i++)
	{
		for (unsigned int j = 0; j < params.grid_y; j++)
		{
			unsigned int gridIndex = (i + j*params.grid_x) * CELL_SPACE;
			grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] = (rand() % 2) == 0 ? 0 : 1 + (rand() % params.grass_restart);
			if (grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] == 0)
				statistics.grass[0]++;
		}
	}
	// Agent parameters
	cl_uint grassIndex = 2;
	AGENT_PARAMS agent_params[2];
	agent_params[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	agent_params[SHEEP_ID].reproduce_threshold = params.sheep_reproduce_threshold;
	agent_params[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	agent_params[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	agent_params[WOLF_ID].reproduce_threshold = params.wolves_reproduce_threshold;
	agent_params[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;
	// Sim parameters
	SIM_PARAMS sim_params;
	sim_params.size_x = params.grid_x;
	sim_params.size_y = params.grid_y;
	sim_params.size_xy = params.grid_x * params.grid_y;
	sim_params.max_agents = MAX_AGENTS;
	sim_params.grass_restart = params.grass_restart;
	sim_params.grid_cell_space = CELL_SPACE;
	// RNG seeds
	size_t rngSeedsSizeInBytes = MAX_AGENTS * sizeof(cl_ulong);
	cl_ulong * rngSeedsHost = (cl_ulong*) malloc(rngSeedsSizeInBytes);
	for (int i = 0; i < MAX_AGENTS; i++) {
		rngSeedsHost[i] = rand();
	}
	// Debugz
	//size_t debugSizeInBytes = MAX_AGENTS * sizeof(cl_uint8);
	//cl_uint8 * dbgHost = (cl_uint8 *) malloc(debugSizeInBytes);

	
	// 6. Create OpenCL buffers
	cl_uint tmpStats[4] = {statistics.sheep[0], statistics.wolves[0], statistics.grass[0], statistics.sheep[0] + statistics.wolves[0]};

	cl_mem agentArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, agentsSizeInBytes, agentArrayHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentArrayDevice"); return(-1); }

	cl_mem grassMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes, grassMatrixHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassMatrixDevice"); return(-1); }

	cl_mem statsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 4*sizeof(cl_uint), tmpStats, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "statsDevice"); return(-1); }

	cl_mem grassCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, grasscount2_gws[0]*sizeof(cl_uint), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassCountDevice"); return(-1); }

	cl_mem agentsCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, (MAX_AGENTS / agentcount2_lws)*sizeof(cl_uint2), NULL, &status ); // This size is the maximum you'll ever need for the given maximum number of agents
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentsCountDevice"); return(-1); }

	cl_mem agentParamsDevice = clCreateBuffer(zone.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2*sizeof(AGENT_PARAMS), agent_params, &status ); // Two types of agent, thus two packs of agent parameters
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentParamsDevice"); return(-1); }

	cl_mem rngSeedsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes, rngSeedsHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "rngSeedsDevice"); return(-1); }

	// Debugz
	//cl_mem dbgDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY, debugSizeInBytes, NULL, &status );
	//if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "debugzDevice"); return(-1); }
	

	// 7. Set fixed kernel arguments

	// Sort kernel
	status = clSetKernelArg(sort_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of sort kernel"); return(-1); }

	// Agent movement kernel
	status = clSetKernelArg(agentmov_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of agentmov kernel"); return(-1); }

	status = clSetKernelArg(agentmov_kernel, 1, sizeof(cl_mem), (void *) &rngSeedsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of agentmov kernel"); return(-1); }

	status = clSetKernelArg(agentmov_kernel, 2, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of agentmov kernel"); return(-1); }

	// Agent grid update kernel
	status = clSetKernelArg(agentupdate_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of agentupdate kernel"); return(-1); }

	status = clSetKernelArg(agentupdate_kernel, 1, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of agentupdate kernel"); return(-1); }

	status = clSetKernelArg(agentupdate_kernel, 2, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of agentupdate kernel"); return(-1); }

	// Grass kernel
	status = clSetKernelArg(grass_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of grass kernel"); return(-1); }

	status = clSetKernelArg(grass_kernel, 1, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of grass kernel"); return(-1); }

	// Agent actions kernel
	status = clSetKernelArg(agentaction_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of agentaction kernel"); return(-1); }

	status = clSetKernelArg(agentaction_kernel, 1, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of agentaction kernel"); return(-1); }

	status = clSetKernelArg(agentaction_kernel, 2, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of agentaction kernel"); return(-1); }

	status = clSetKernelArg(agentaction_kernel, 3, sizeof(cl_mem), (void *) &agentParamsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of agentaction kernel"); return(-1); }

	status = clSetKernelArg(agentaction_kernel, 4, sizeof(cl_mem), (void *) &rngSeedsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of agentaction kernel"); return(-1); }

	status = clSetKernelArg(agentaction_kernel, 5, sizeof(cl_mem), (void *) &statsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 5 of agentaction kernel"); return(-1); }

	// Count agents
	status = clSetKernelArg(countagents1_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countagents1 kernel"); return(-1); }

	status = clSetKernelArg(countagents1_kernel, 1, sizeof(cl_mem), (void *) &agentsCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countagents1 kernel"); return(-1); }

	status = clSetKernelArg(countagents1_kernel, 2, agentcount1_lws*sizeof(cl_uint2), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countagents1 kernel"); return(-1); }

	status = clSetKernelArg(countagents2_kernel, 0, sizeof(cl_mem), (void *) &agentsCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countagents2 kernel"); return(-1); }

	status = clSetKernelArg(countagents2_kernel, 1, agentcount2_lws*sizeof(cl_uint2), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countagents2 kernel"); return(-1); }

	status = clSetKernelArg(countagents2_kernel, 3, sizeof(cl_mem), (void *) &statsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countagents2 kernel"); return(-1); }

	// Count grass
	status = clSetKernelArg(countgrass1_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countgrass1 kernel"); return(-1); }

	status = clSetKernelArg(countgrass1_kernel, 1, sizeof(cl_mem), (void *) &grassCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countgrass1 kernel"); return(-1); }

	status = clSetKernelArg(countgrass1_kernel, 2, grasscount1_lws*sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countgrass1 kernel"); return(-1); }

	status = clSetKernelArg(countgrass1_kernel, 3, sizeof(SIM_PARAMS), (void *) &sim_params);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countgrass1 kernel"); return(-1); }

	status = clSetKernelArg(countgrass2_kernel, 0, sizeof(cl_mem), (void *) &grassCountDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countgrass2 kernel"); return(-1); }

	status = clSetKernelArg(countgrass2_kernel, 1, grasscount2_gws[0]*sizeof(cl_uint), NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countgrass2 kernel"); return(-1); }

	status = clSetKernelArg(countgrass2_kernel, 3, sizeof(cl_mem), (void *) &statsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countgrass2 kernel"); return(-1); }
	
        // 8. Run the show
	cl_event agentaction_move_event, grass_event, agentaction_event, agentcount1_event, agentupdate_event, grasscount1_event;
	char msg[500];
	clFinish(zone.queue); // Guarantee all memory transfers are performed
	gettimeofday(&time0, NULL);
	for (unsigned int iter = 1; iter <= params.iters; iter++) {
		//printf("iter %d\n", iter);

		// Determine agent kernels size for this iteration
		unsigned int maxOccupiedSpace = tmpStats[3] * 2; // Worst case array agent (dead or alive) occupation
		agent_gws = LWS_GPU_PREF * ceil(((float) maxOccupiedSpace) / LWS_GPU_PREF);
		agentcount1_gws = LWS_GPU_MAX * ceil(((float) maxOccupiedSpace) / LWS_GPU_MAX);
		cl_uint effectiveNextAgentsToCount = agentcount1_gws / agentcount1_lws;

		// Agent movement
		status = clEnqueueNDRangeKernel( zone.queue, agentmov_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, &agentaction_move_event);
		if (status != CL_SUCCESS) { sprintf(msg, "agentmov_kernel, iteration %d ", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

		// Grass growth and agent number reset
		status = clEnqueueNDRangeKernel( zone.queue, grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, &grass_event);
		if (status != CL_SUCCESS) { sprintf(msg, "grass_kernel, iteration %d ", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

		// Sort agent array
		agentsort_gws = nlpo2(maxOccupiedSpace) / 2;
		agentsort_lws = LWS_GPU_PREF;
		while (agentsort_gws % agentsort_lws != 0)
			agentsort_lws = agentsort_lws / 2;
		cl_uint totalStages = (cl_uint) tzc(agentsort_gws * 2);
		status = clEnqueueWaitForEvents(zone.queue, 1, &agentaction_move_event);
		if (status != CL_SUCCESS) {  sprintf(msg, "clEnqueueWaitForEvents after agent mov, iteration %d", iter); PrintErrorEnqueueWaitForEvents(status, msg); return(-1); }
		for (unsigned int currentStage = 1; currentStage <= totalStages; currentStage++) {
			cl_uint step = currentStage;
			for (int currentStep = step; currentStep > 0; currentStep--) {
				status = clSetKernelArg(sort_kernel, 1, sizeof(cl_uint), (void *) &currentStage);
				if (status != CL_SUCCESS) { sprintf(msg, "argument 1 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clSetKernelArg(sort_kernel, 2, sizeof(cl_uint), (void *) &currentStep);
				if (status != CL_SUCCESS) {  sprintf(msg, "argument 2 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clEnqueueNDRangeKernel( zone.queue, sort_kernel, 1, NULL, &agentsort_gws, &agentsort_lws, 0, NULL, NULL);
				if (status != CL_SUCCESS) {  sprintf(msg, "sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
				status = clEnqueueBarrier(zone.queue);
				if (status != CL_SUCCESS) {  sprintf(msg, "in sort agents loop, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorEnqueueBarrier(status, msg); return(-1); }
			}
		}

		// Update agent number in grid
		status = clEnqueueNDRangeKernel( zone.queue, agentupdate_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, &agentupdate_event);
		if (status != CL_SUCCESS) { sprintf(msg, "agentupdate_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); }

		// agent actions
		status = clEnqueueNDRangeKernel( zone.queue, agentaction_kernel, 1, NULL, &agent_gws, &agent_lws, 1, &agentupdate_event, &agentaction_event);
		if (status != CL_SUCCESS) { sprintf(msg, "agentaction_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); }

		// Gather statistics
		// Count agents, part 1
		status = clEnqueueNDRangeKernel( zone.queue, countagents1_kernel, 1, NULL, &agentcount1_gws, &agentcount1_lws, 1, &agentaction_event, &agentcount1_event);
		if (status != CL_SUCCESS) { sprintf(msg, "countagents1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		// Count grass, part 1
		status = clEnqueueNDRangeKernel( zone.queue, countgrass1_kernel, 1, NULL, &grasscount1_gws, &grasscount1_lws, 1, &agentaction_event, &grasscount1_event);
		if (status != CL_SUCCESS) { sprintf(msg, "countgrass1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		// Count agents, part 2
		do {
			agentcount2_gws = LWS_GPU_MAX * ceil(((float) effectiveNextAgentsToCount) / LWS_GPU_MAX);

			status = clSetKernelArg(countagents2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextAgentsToCount);
			if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countagents2 kernel"); return(-1); }

			status = clEnqueueNDRangeKernel( zone.queue, countagents2_kernel, 1, NULL, &agentcount2_gws, &agentcount2_lws, 1, &agentcount1_event, NULL);
			if (status != CL_SUCCESS) { sprintf(msg, "countagents2_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

			effectiveNextAgentsToCount = agentcount2_gws / agentcount2_lws;

			status = clEnqueueBarrier(zone.queue);
			if (status != CL_SUCCESS) {  sprintf(msg, "in agent count loops"); PrintErrorEnqueueBarrier(status, msg); return(-1); }


		} while (effectiveNextAgentsToCount > 1);
		// Count grass, part 2
		for (int i = 0; i < numGrassCount2Loops; i++) {

			status = clSetKernelArg(countgrass2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextGrassToCount[i]);
			if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countgrass2 kernel"); return(-1); }

			status = clEnqueueNDRangeKernel( zone.queue, countgrass2_kernel, 1, NULL, &grasscount2_gws[i], &grasscount2_lws, 1, &grasscount1_event, NULL);
			if (status != CL_SUCCESS) { sprintf(msg, "countgrass2_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

			status = clEnqueueBarrier(zone.queue);
			if (status != CL_SUCCESS) {  sprintf(msg, "in grass count loops"); PrintErrorEnqueueBarrier(status, msg); return(-1); }

		}

		// Get stats
		status = clEnqueueReadBuffer( zone.queue, statsDevice, CL_TRUE, 0, 4*sizeof(cl_uint), tmpStats, 0, NULL, NULL );
		if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback stats"); return(-1); };

		statistics.sheep[iter] = tmpStats[SHEEP_ID];
		statistics.wolves[iter] = tmpStats[WOLF_ID];
		statistics.grass[iter] = tmpStats[grassIndex];

	}
	clFinish(zone.queue); // Guarantee all kernels have really terminated...
	gettimeofday(&time1, NULL);  
	
	// 9. Debug (optional)

	/*status = clEnqueueReadBuffer( zone.queue, agentArrayDevice, CL_TRUE, 0, agentsSizeInBytes, agentArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback agentArray"); return(-1); };

	status = clEnqueueReadBuffer( zone.queue, grassMatrixDevice, CL_TRUE, 0, grassSizeInBytes, grassMatrixHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback grassMatrix"); return(-1); };

	status = clEnqueueReadBuffer( zone.queue, dbgDevice, CL_TRUE, 0, debugSizeInBytes, dbgHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback dbg"); return(-1); };

	FILE * fp2 = fopen("agentArray.txt","w");
	for (int i = 0; i < MAX_AGENTS; i++)
		fprintf(fp2, "x=%d\ty=%d\te=%d\ttype=%d\talive=%d\n", agentArrayHost[i].x, agentArrayHost[i].y, agentArrayHost[i].energy, agentArrayHost[i].type, agentArrayHost[i].alive);
	fclose(fp2);
	FILE * fp3 = fopen("debug.txt","w");
	for (int i = 0; i < (MAX_AGENTS); i++) {
		if ((dbgHost[i].s0 == 1) && (dbgHost[i].s1 == 1) && (dbgHost[i].s5 < MAX_AGENTS))
			fprintf(fp3, "w_gid=%d\tw_x=%d\tw_y=%d\ts_gid=%d\ts_x=%d\ts_y=%d\n", dbgHost[i].s2, dbgHost[i].s3, dbgHost[i].s4, dbgHost[i].s5, dbgHost[i].s6, dbgHost[i].s7);
	}
	fclose(fp3);
	printGrassMatrix(grassMatrixHost, GRID_X, GRID_Y);*/


	// 10. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statistics.sheep[i], statistics.wolves[i], statistics.grass[i] );
	fclose(fp1);


	// 11. Print timmings
	dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Time = %f\n", dt);

	// 12. Free stuff!
        free(agentArrayHost);
	free(grassMatrixHost);
	free(statistics.sheep);
	free(statistics.wolves);
	free(statistics.grass);
	free(rngSeedsHost);
	//clReleaseMemObject(agentArrayDevice);
	//clReleaseMemObject(grassMatrixDevice);
	//clReleaseMemObject(statsDevice);
	return 0;
}


// Print array of agents
void printAgentArray(cl_uint4* array, unsigned int size) {
	for (unsigned int i = 0; i < size; i++)
	{
		printf("x=%d\ty=%d\te=%d\tmov=%d\n", array[i].x, array[i].y, array[i].z, array[i].w);
	}
}

// Print grass matrix
void printGrassMatrix(cl_uint* matrix, unsigned int size_x,  unsigned int size_y) {
	for (unsigned int i = 0; i < size_y; i++)
	{
		printf("NUM: ");
		for (unsigned int j = 0; j < size_x; j++)
		{
			unsigned int index = CELL_SPACE*(j + i*size_x);
			//printf("%c", matrix[index + CELL_GRASS_OFFSET] == 0 ? '*' : '-');
			//printf("%d\t", matrix[index + CELL_GRASS_OFFSET]);
			printf("%d\t", matrix[index + CELL_NUMAGENTS_OFFSET]);
		}
		printf("\n");
		printf("IDX: ");
		for (unsigned int j = 0; j < size_x; j++)
		{
			unsigned int index = CELL_SPACE*(j + i*size_x);
			//printf("%c", matrix[index + CELL_GRASS_OFFSET] == 0 ? '*' : '-');
			//printf("%d\t", matrix[index + CELL_GRASS_OFFSET]);
			printf("%d\t", matrix[index + CELL_AGINDEX_OFFSET]);
		}
		printf("\n\n");
	}
}

// Compute worksizes depending on the device type and number of available compute units
void computeWorkSizes(PARAMS params, cl_uint device_type, cl_uint cu) {
	// grass growth worksizes
	grass_lws[0] = LWS_GPU_PREF_2D_X;
	grass_gws[0] = LWS_GPU_PREF_2D_X * ceil(((float) params.grid_x) / LWS_GPU_PREF_2D_X);
	grass_lws[1] = LWS_GPU_PREF_2D_Y;
	grass_gws[1] = LWS_GPU_PREF_2D_Y * ceil(((float) params.grid_y) / LWS_GPU_PREF_2D_Y);
	// fixed local agent kernel worksizes
	agent_lws = LWS_GPU_PREF;
	agentcount1_lws = LWS_GPU_MAX;
	agentcount2_lws = LWS_GPU_MAX;
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
	printf("agent_lws=%d\n", (int) agent_lws);
	printf("agentcount1_lws=%d\n", (int) agentcount1_lws);
	printf("agentcount2_lws=%d\n", (int) agentcount2_lws);
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
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Grass kernel"); exit(-1); }
	agentmov_kernel = clCreateKernel( program, "RandomWalk", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "RandomWalk kernel"); exit(-1); }
	agentupdate_kernel = clCreateKernel( program, "AgentsUpdateGrid", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "AgentsUpdateGrid kernel"); exit(-1); }
	sort_kernel = clCreateKernel( program, "BitonicSort", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "BitonicSort kernel"); exit(-1); }
	agentaction_kernel = clCreateKernel( program, "AgentAction", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Agent kernel"); exit(-1); }
	countagents1_kernel = clCreateKernel( program, "CountAgents1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountAgents kernel"); exit(-1); }
	countagents2_kernel = clCreateKernel( program, "CountAgents2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountAgents kernel"); exit(-1); }
	countgrass1_kernel = clCreateKernel( program, "CountGrass1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass1 kernel"); exit(-1); }
	countgrass2_kernel = clCreateKernel( program, "CountGrass2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass2 kernel"); exit(-1); }
}
