#include "PredPreyGPUSort.h"

#define MAX_AGENTS 1048576

#define SHEEP_ID 0
#define WOLF_ID 1

#define CELL_SPACE 4 // 0-Grass state, 1-number of agents in cell, 2-pointer to first agent in cell, 3-unused
#define CELL_GRASS_OFFSET 0
#define CELL_NUMAGENTS_OFFSET 1
#define CELL_AGINDEX_OFFSET 2

#define LWS_GPU_MAX 256 //512
#define LWS_GPU_PREF 64 //128
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

#ifdef CLPROFILER
	printf("Profiling is ON!\n");
#else
	printf("Profiling is OFF!\n");
#endif


	// Status var aux
	cl_int status;	
	
	// Init random number generator
	srandom((unsigned)(time(0)));

	// Timmings
	struct timeval time1, time0;
	double dt = 0;

	// 1. Get the required CL zone.
	CLZONE zone = getClZone("NVIDIA Corporation", "PredPreyGPUSort_Kernels.cl", CL_DEVICE_TYPE_GPU);
	//CLZONE zone = getClZone("Advanced Micro Devices, Inc.", "PredPreyGPUSort_Kernels.cl", CL_DEVICE_TYPE_GPU);

	// 2. Get simulation parameters
	PARAMS params = loadParams(CONFIG_FILE);

	// 3. Compute work sizes for different kernels and print them to screen
	computeWorkSizes(params, zone.device_type, zone.cu);	
	printFixedWorkSizes();

	// 4. obtain kernels entry points.
	getKernelEntryPoints(zone.program);
	
	// Show kernel info - this should then influence the stuff above
	KERNEL_WORK_GROUP_INFO kwgi;
	printf("\n-------- grass_kernel information --------\n");	
	getWorkGroupInfo(grass_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- agentmov_kernel information --------\n");	
	getWorkGroupInfo(agentmov_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- agentupdate_kernel information --------\n");	
	getWorkGroupInfo(agentupdate_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- sort_kernel information --------\n");	
	getWorkGroupInfo(sort_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- agentaction_kernel information --------\n");	
	getWorkGroupInfo(agentaction_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countagents1_kernel information --------\n");	
	getWorkGroupInfo(countagents1_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countagents2_kernel information --------\n");	
	getWorkGroupInfo(countagents2_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countgrass1_kernel information --------\n");	
	getWorkGroupInfo(countgrass1_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	printf("\n-------- countgrass2_kernel information --------\n");	
	getWorkGroupInfo(countgrass2_kernel, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);

	printf("-------- Simulation start --------\n");		
	

	// 5. Create and initialize host buffers
	// Statistics
	size_t statsSizeInBytes = (params.iters + 1) * sizeof(STATS);
	STATS * statsArrayHost = (STATS *) malloc(statsSizeInBytes);
	statsArrayHost[0].sheep = params.init_sheep;
	statsArrayHost[0].wolves = params.init_wolves;
	statsArrayHost[0].grass = 0;
	// Number of agents after each iteration - this will be a mapped with device, so CPU can auto-adjust GPU worksize in each iteration
	cl_uint * numAgentsHost = (cl_uint*) malloc(sizeof(cl_uint));
	(*numAgentsHost) = params.init_sheep + params.init_wolves;
	// Current iteration
	cl_uint iter = 0; 
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
				statsArrayHost[0].grass++;
		}
	}
	// Agent parameters
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

	// 6. Create OpenCL buffers
	cl_mem statsArrayDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, statsSizeInBytes, statsArrayHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "statsArrayDevice"); return(-1); }

	cl_mem agentArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, agentsSizeInBytes, agentArrayHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentArrayDevice"); return(-1); }

	cl_mem grassMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes, grassMatrixHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassMatrixDevice"); return(-1); }

	cl_mem iterDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint), &iter, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "iterDevice"); return(-1); }

	// Stuff to get number of agents out in each iteration
	cl_mem numAgentsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_uint), numAgentsHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "numAgentsDevice"); return(-1); }

	cl_mem grassCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, grasscount2_gws[0]*sizeof(cl_uint), NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassCountDevice"); return(-1); }

	cl_mem agentsCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, (MAX_AGENTS / agentcount2_lws)*sizeof(cl_uint2), NULL, &status ); // This size is the maximum you'll ever need for the given maximum number of agents
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentsCountDevice"); return(-1); }

	cl_mem agentParamsDevice = clCreateBuffer(zone.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2*sizeof(AGENT_PARAMS), agent_params, &status ); // Two types of agent, thus two packs of agent parameters
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "agentParamsDevice"); return(-1); }

	cl_mem rngSeedsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes, rngSeedsHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "rngSeedsDevice"); return(-1); }

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

	status = clSetKernelArg(agentmov_kernel, 3, sizeof(cl_mem), (void *) &iterDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of agentmov kernel"); return(-1); }

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

	status = clSetKernelArg(agentaction_kernel, 5, sizeof(cl_mem), (void *) &numAgentsDevice);
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

	status = clSetKernelArg(countagents2_kernel, 3, sizeof(cl_mem), (void *) &numAgentsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countagents2 kernel"); return(-1); }

	status = clSetKernelArg(countagents2_kernel, 4, sizeof(cl_mem), (void *) &statsArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of countagents2 kernel"); return(-1); }

	status = clSetKernelArg(countagents2_kernel, 5, sizeof(cl_mem), (void *) &iterDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 5 of countagents2 kernel"); return(-1); }

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

	status = clSetKernelArg(countgrass2_kernel, 3, sizeof(cl_mem), (void *) &statsArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of countgrass2 kernel"); return(-1); }

	status = clSetKernelArg(countgrass2_kernel, 4, sizeof(cl_mem), (void *) &iterDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of countgrass2 kernel"); return(-1); }

	//printf("sort: %d\t agc2: %d\t gc2: %d\n", params.iters * sum(tzc(nlpo2(MAX_AGENTS))), params.iters * (MAX_AGENTS/4) / agentcount2_lws, params.iters * numGrassCount2Loops);

        // 8. Run the show
	cl_event *agentaction_move_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *grass_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *agentaction_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *agentsort_event = (cl_event*) malloc(sizeof(cl_event) * params.iters * sum(tzc(nlpo2(MAX_AGENTS)))); //Worse case usage scenario
	cl_event *agentcount1_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *agentcount2_event = (cl_event*) malloc(sizeof(cl_event) * params.iters * (MAX_AGENTS/4) / agentcount2_lws); // Optimistic usage to save memory => may break
	cl_event *agentupdate_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *grasscount1_event = (cl_event*) malloc(sizeof(cl_event) * params.iters); 
	cl_event *grasscount2_event = (cl_event*) malloc(sizeof(cl_event) * params.iters * numGrassCount2Loops); // Exact usage scenario
	cl_event *readNumAgents_event = (cl_event*) malloc(sizeof(cl_event) * params.iters);
	cl_uint agentsort_event_index = 0, agentcount2_event_index = 0, grasscount2_event_index = 0;
	char msg[500];
	clFinish(zone.queue); // Guarantee all memory transfers are performed
	gettimeofday(&time0, NULL);
	for (iter = 1; iter <= params.iters; iter++) {
		//printf("iter %d\n", iter);

		// Determine agent kernels size for this iteration
		cl_uint maxOccupiedSpace = (*numAgentsHost) * 2; // Worst case array agent (dead or alive) occupation
		agent_gws = LWS_GPU_PREF * ceil(((float) maxOccupiedSpace) / LWS_GPU_PREF);
		agentcount1_gws = LWS_GPU_MAX * ceil(((float) maxOccupiedSpace) / LWS_GPU_MAX);
		cl_uint effectiveNextAgentsToCount = agentcount1_gws / agentcount1_lws;
		cl_uint iterbase = iter - 1;

		// Agent movement
		status = clEnqueueNDRangeKernel( zone.queue, agentmov_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, agentaction_move_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "agentmov_kernel, iteration %d ", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

		// Grass growth and agent number reset
		status = clEnqueueNDRangeKernel( zone.queue, grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, grass_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "grass_kernel, iteration %d, gws=%d lws=%d ", iter, (int) *grass_gws, (int) *grass_lws); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

		// Sort agent array
		agentsort_gws = nlpo2(maxOccupiedSpace) / 2;
		agentsort_lws = LWS_GPU_PREF;
		while (agentsort_gws % agentsort_lws != 0)
			agentsort_lws = agentsort_lws / 2;
		cl_uint totalStages = (cl_uint) tzc(agentsort_gws * 2);
		status = clEnqueueWaitForEvents(zone.queue, 1, agentaction_move_event + iterbase);
		if (status != CL_SUCCESS) {  sprintf(msg, "clEnqueueWaitForEvents after agent mov, iteration %d", iter); PrintErrorEnqueueWaitForEvents(status, msg); return(-1); }
		for (unsigned int currentStage = 1; currentStage <= totalStages; currentStage++) {
			cl_uint step = currentStage;
			for (int currentStep = step; currentStep > 0; currentStep--) {
				status = clSetKernelArg(sort_kernel, 1, sizeof(cl_uint), (void *) &currentStage);
				if (status != CL_SUCCESS) { sprintf(msg, "argument 1 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clSetKernelArg(sort_kernel, 2, sizeof(cl_uint), (void *) &currentStep);
				if (status != CL_SUCCESS) {  sprintf(msg, "argument 2 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clEnqueueNDRangeKernel( zone.queue, sort_kernel, 1, NULL, &agentsort_gws, &agentsort_lws, 0, NULL, agentsort_event + agentsort_event_index);
				if (status != CL_SUCCESS) {  sprintf(msg, "sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
				status = clEnqueueBarrier(zone.queue);
				if (status != CL_SUCCESS) {  sprintf(msg, "in sort agents loop, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorEnqueueBarrier(status, msg); return(-1); }

				agentsort_event_index++;
			}
		}

		// Update agent number in grid
		status = clEnqueueNDRangeKernel( zone.queue, agentupdate_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, agentupdate_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "agentupdate_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); }

		// agent actions
		status = clEnqueueNDRangeKernel( zone.queue, agentaction_kernel, 1, NULL, &agent_gws, &agent_lws, 1, agentupdate_event + iterbase, agentaction_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "agentaction_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); }

		// Gather statistics
		// Count agents, part 1
		status = clEnqueueNDRangeKernel( zone.queue, countagents1_kernel, 1, NULL, &agentcount1_gws, &agentcount1_lws, 1, agentaction_event + iterbase, agentcount1_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "countagents1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		// Count grass, part 1
		status = clEnqueueNDRangeKernel( zone.queue, countgrass1_kernel, 1, NULL, &grasscount1_gws, &grasscount1_lws, 1, agentaction_event + iterbase, grasscount1_event + iterbase);
		if (status != CL_SUCCESS) { sprintf(msg, "countgrass1_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		// Count agents, part 2
		do {
			agentcount2_gws = LWS_GPU_MAX * ceil(((float) effectiveNextAgentsToCount) / LWS_GPU_MAX);

			status = clSetKernelArg(countagents2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextAgentsToCount);
			if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of countagents2 kernel"); return(-1); }

			status = clEnqueueNDRangeKernel( zone.queue, countagents2_kernel, 1, NULL, &agentcount2_gws, &agentcount2_lws, 1, agentcount1_event + iterbase, agentcount2_event + agentcount2_event_index);
			if (status != CL_SUCCESS) { sprintf(msg, "countagents2_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }

			effectiveNextAgentsToCount = agentcount2_gws / agentcount2_lws;

			status = clEnqueueBarrier(zone.queue);
			if (status != CL_SUCCESS) {  sprintf(msg, "in agent count loops"); PrintErrorEnqueueBarrier(status, msg); return(-1); }

			agentcount2_event_index++;

		} while (effectiveNextAgentsToCount > 1);

		// Get total number of agents
		status = clEnqueueReadBuffer(zone.queue, numAgentsDevice, CL_FALSE, 0, sizeof(cl_uint), numAgentsHost, 0, NULL, readNumAgents_event + iterbase);
		if (status != CL_SUCCESS) {  PrintErrorEnqueueReadWriteBuffer(status, "numAgents"); return(-1); }

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

		// Confirm that number of agents have been read
		status = clWaitForEvents(1, readNumAgents_event + iterbase); // Maybe put this in device queue instead of being in CPU time
		if (status != CL_SUCCESS) {  PrintErrorWaitForEvents(status, "numAgents"); return(-1); }

	}

	// Guarantee all kernels have really terminated...
	clFinish(zone.queue);

	// Get finishing time	
	gettimeofday(&time1, NULL);  

	// Get statistics
	status = clEnqueueReadBuffer(zone.queue, statsArrayDevice, CL_TRUE, 0, statsSizeInBytes, statsArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) {  PrintErrorEnqueueReadWriteBuffer(status, "statsArray"); return(-1); }
	
	// 9. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep, statsArrayHost[i].wolves, statsArrayHost[i].grass );
	fclose(fp1);

	// 10. Print timmings
	dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Total Simulation Time = %f", dt);

#ifdef CLPROFILER
	// Calculate and show profiling info

	cl_ulong agentaction_move_profile = 0, grass_profile = 0, agentaction_profile = 0, agentsort_profile = 0, agentcount1_profile = 0, agentcount2_profile = 0, agentupdate_profile = 0, grasscount1_profile = 0, grasscount2_profile = 0, readNumAgents_profile = 0;
	cl_ulong agentaction_move_profile_start, grass_profile_start, agentaction_profile_start, agentsort_profile_start, agentcount1_profile_start, agentcount2_profile_start, agentupdate_profile_start, grasscount1_profile_start, grasscount2_profile_start, readNumAgents_profile_start;
	cl_ulong agentaction_move_profile_end, grass_profile_end, agentaction_profile_end, agentsort_profile_end, agentcount1_profile_end, agentcount2_profile_end, agentupdate_profile_end, grasscount1_profile_end, grasscount2_profile_end, readNumAgents_profile_end;

	for (unsigned int i = 0; i < params.iters; i++) {
		// Agent movement kernel profiling
		status = clGetEventProfilingInfo (agentaction_move_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentaction_move_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentaction_move start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		status = clGetEventProfilingInfo (agentaction_move_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentaction_move_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentaction_move end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		agentaction_move_profile += agentaction_move_profile_end - agentaction_move_profile_start;
		// Grass kernel profiling
		status = clGetEventProfilingInfo (grass_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grass_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grass start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		status = clGetEventProfilingInfo (grass_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grass_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grass end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		grass_profile += grass_profile_end - grass_profile_start;
		// Update agent number kernel profiling
		status = clGetEventProfilingInfo (agentupdate_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentupdate_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentupdate start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1); }
		status = clGetEventProfilingInfo (agentupdate_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentupdate_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentupdate end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1); }
		agentupdate_profile += agentupdate_profile_end - agentupdate_profile_start;
		// Agent actions kernel profiling
		status = clGetEventProfilingInfo (agentaction_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentaction_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentaction start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1); }
		status = clGetEventProfilingInfo (agentaction_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentaction_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentaction end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		agentaction_profile += agentaction_profile_end - agentaction_profile_start;
		// Count agents 1 kernel profiling
		status = clGetEventProfilingInfo (agentcount1_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentcount1_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentcount1 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1);  }
		status = clGetEventProfilingInfo (agentcount1_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentcount1_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentcount1 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		agentcount1_profile += agentcount1_profile_end - agentcount1_profile_start;
		// Count grass 1 kernel profiling
		status = clGetEventProfilingInfo (grasscount1_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount1_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount1 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1);  }
		status = clGetEventProfilingInfo (grasscount1_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount1_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount1 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		grasscount1_profile += grasscount1_profile_end - grasscount1_profile_start;
		// Get total number of agents profiling
		status = clGetEventProfilingInfo (readNumAgents_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &readNumAgents_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling readNumAgents start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1);  }
		status = clGetEventProfilingInfo (readNumAgents_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &readNumAgents_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling readNumAgents end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg); return(-1);  }
		readNumAgents_profile += readNumAgents_profile_end - readNumAgents_profile_start;
	}

	for (unsigned int i = 0; i < agentsort_event_index; i++) {
		// Agent sort kernel profiling
		status = clGetEventProfilingInfo (agentsort_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentsort_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentsort start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		status = clGetEventProfilingInfo (agentsort_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentsort_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentsort end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		agentsort_profile += agentsort_profile_end - agentsort_profile_start;
	}

	for (unsigned int i = 0; i < grasscount2_event_index; i++) {
		// Grass count 2 kernel profiling
		status = clGetEventProfilingInfo (grasscount2_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount2_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount2 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		status = clGetEventProfilingInfo (grasscount2_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount2_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling grasscount2 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		grasscount2_profile += grasscount2_profile_end - grasscount2_profile_start;
	}

	for (unsigned int i = 0; i < agentcount2_event_index; i++) {
		// Agent count 2 kernel profiling
		status = clGetEventProfilingInfo (agentcount2_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &agentcount2_profile_start, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentcount2 start, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		status = clGetEventProfilingInfo (agentcount2_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &agentcount2_profile_end, NULL);
		if (status != CL_SUCCESS) { sprintf(msg, "profiling agentcount2 end, iteration %d", i); PrintErrorGetEventProfilingInfo(status, msg);  return(-1); }
		agentcount2_profile += agentcount2_profile_end - agentcount2_profile_start;
	}

	cl_ulong gpu_profile_total = agentaction_move_profile + grass_profile + agentaction_profile + agentsort_profile + agentcount1_profile + agentcount2_profile + agentupdate_profile + grasscount1_profile + grasscount2_profile + readNumAgents_profile;
	double gpu_exclusive = gpu_profile_total * 1e-9;
	double cpu_exclusive = dt - gpu_exclusive;
	printf(", of which %f (%f%%) is CPU and %f (%f%%) is GPU.\n", cpu_exclusive, 100*cpu_exclusive/dt, gpu_exclusive, 100*gpu_exclusive/dt);
	printf("agentaction_move: %fms (%f%%)\n", agentaction_move_profile*1e-6, 100*((double) agentaction_move_profile)/((double) gpu_profile_total));
	printf("grass: %fms (%f%%)\n", grass_profile*1e-6, 100*((double) grass_profile)/((double) gpu_profile_total));
	printf("agentaction: %fms (%f%%)\n", agentaction_profile*1e-6, 100*((double) agentaction_profile)/((double) gpu_profile_total));
	printf("agentsort: %fms (%f%%)\n", agentsort_profile*1e-6, 100*((double) agentsort_profile)/((double) gpu_profile_total));
	printf("agentcount1: %fms (%f%%)\n", agentcount1_profile*1e-6, 100*((double) agentcount1_profile)/((double) gpu_profile_total));
	printf("agentcount2: %fms (%f%%)\n", agentcount2_profile*1e-6, 100*((double) agentcount2_profile)/((double) gpu_profile_total));
	printf("agentupdate: %fms (%f%%)\n", agentupdate_profile*1e-6, 100*((double) agentupdate_profile)/((double) gpu_profile_total));
	printf("grasscount1: %fms (%f%%)\n", grasscount1_profile*1e-6, 100*((double) grasscount1_profile)/((double) gpu_profile_total));
	printf("grasscount2: %fms (%f%%)\n", grasscount2_profile*1e-6, 100*((double) grasscount2_profile)/((double) gpu_profile_total));
	printf("readNumAgents: %fms (%f%%)\n", readNumAgents_profile*1e-6, 100*((double) readNumAgents_profile)/((double) gpu_profile_total));
#endif

	printf("\n");

	// 11. Free stuff!
	
	// Free OpenCL kernels
	clReleaseKernel(grass_kernel);
	clReleaseKernel(agentmov_kernel);
	clReleaseKernel(agentupdate_kernel);
	clReleaseKernel(sort_kernel);
	clReleaseKernel(agentaction_kernel);
	clReleaseKernel(countagents1_kernel);
	clReleaseKernel(countagents2_kernel);
	clReleaseKernel(countgrass1_kernel); 
	clReleaseKernel(countgrass2_kernel);
	// Free OpenCL program and command queue
    clReleaseProgram(zone.program);
    clReleaseCommandQueue(zone.queue);
	// Free OpenCL memory objects
	clReleaseMemObject(statsArrayDevice);
	clReleaseMemObject(agentArrayDevice);
	clReleaseMemObject(grassMatrixDevice);
	clReleaseMemObject(iterDevice);
	clReleaseMemObject(numAgentsDevice);
	clReleaseMemObject(grassCountDevice);
	clReleaseMemObject(agentsCountDevice );
	clReleaseMemObject(agentParamsDevice);
	clReleaseMemObject(rngSeedsDevice);
	// Free OpenCL context
	clReleaseContext(zone.context);
	// Free host resources
	free(statsArrayHost);
	free(numAgentsHost);
	free(agentArrayHost);
	free(grassMatrixHost);
	free(rngSeedsHost);
	free(agentaction_move_event); 
	free(grass_event); 
	free(agentaction_event); 
	free(agentsort_event);
	free(agentcount1_event); 
	free(agentcount2_event);
	free(agentupdate_event); 
	free(grasscount1_event); 
	free(grasscount2_event);
	free(readNumAgents_event);

	return 0;
	
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
