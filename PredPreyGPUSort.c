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

#ifdef CLPROFILER
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE
#else
	#define QUEUE_PROPERTIES CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE
#endif


// Global work sizes
size_t agentsort_gws, agent_gws, grass_gws[2], agentcount1_gws, agentcount2_gws, grasscount1_gws, grasscount2_gws[MAX_GRASS_COUNT_LOOPS];

// Local work sizes
size_t agentsort_lws, agent_lws, grass_lws[2], agentcount1_lws, agentcount2_lws, grasscount1_lws, grasscount2_lws;
int effectiveNextGrassToCount[MAX_GRASS_COUNT_LOOPS];
int numGrassCount2Loops;

// Kernels
cl_kernel grass_kernel = NULL, 
	agentmov_kernel = NULL, 
	agentupdate_kernel = NULL, 
	sort_kernel = NULL, 
	agentaction_kernel = NULL, 
	countagents1_kernel = NULL, 
	countagents2_kernel = NULL, 
	countgrass1_kernel = NULL, 
	countgrass2_kernel = NULL;

// Main stuff
int main(int argc, char ** argv)
{
	
	// Status var aux
	cl_int status;	
	
	// Host memory buffers
	PPStatistics * statsArrayHost = NULL;
	cl_uint * numAgentsHost = NULL;
	PPGSAgent * agentArrayHost = NULL;
	cl_uint * grassMatrixHost = NULL;
	cl_ulong * rngSeedsHost = NULL;
	
	// Device memory buffers
	cl_mem statsArrayDevice = NULL,
		agentArrayDevice = NULL,
		grassMatrixDevice = NULL,
		iterDevice = NULL,
		numAgentsDevice = NULL,
		grassCountDevice = NULL,
		agentsCountDevice = NULL,
		agentParamsDevice = NULL,
		rngSeedsDevice = NULL;
		
	// OpenCL events
	cl_event *agentaction_move_event = NULL,
		*grass_event = NULL,
		*agentaction_event = NULL,
		*agentsort_event = NULL,
		*agentcount1_event = NULL,
		*agentcount2_event = NULL,
		*agentupdate_event = NULL,
		*grasscount1_event = NULL,
		*grasscount2_event = NULL,
		*readNumAgents_event = NULL;
	
	
	// Init random number generator
	srandom((unsigned)(time(0)));

	// Profiling / Timmings
	ProfCLProfile* profile = profcl_profile_new();

	// OpenCL kernel files
	const char* kernelFiles[] = {"PredPreyCommon_Kernels.cl", "PredPreyGPUSort_Kernels.cl"};

	// 1. Get the required CL zone.
	CLUZone zone;
	status = clu_zone_new(&zone, kernelFiles, 2, NULL, CL_DEVICE_TYPE_GPU, 1, QUEUE_PROPERTIES);
	clu_if_error_goto(status, "Creating CLUZone", error);

	// 2. Get simulation parameters
	PPParameters params = loadParams(CONFIG_FILE);

	// 3. Compute work sizes for different kernels and print them to screen
	computeWorkSizes(params, zone.device_type, zone.cu);	
	printFixedWorkSizes();

	// 4. obtain kernels entry points.
	status = getKernelEntryPoints(zone.program);
	clu_if_error_goto(status, "Getting kernel entry points", error);

	printf("-------- Simulation start --------\n");		
	

	// 5. Create and initialize host buffers
	// Statistics
	size_t statsSizeInBytes = (params.iters + 1) * sizeof(PPStatistics);
	statsArrayHost = (PPStatistics *) malloc(statsSizeInBytes);
	statsArrayHost[0].sheep = params.init_sheep;
	statsArrayHost[0].wolves = params.init_wolves;
	statsArrayHost[0].grass = 0;
	// Number of agents after each iteration - this will be a mapped with device, so CPU can auto-adjust GPU worksize in each iteration
	numAgentsHost = (cl_uint*) malloc(sizeof(cl_uint));
	(*numAgentsHost) = params.init_sheep + params.init_wolves;
	// Current iteration
	cl_uint iter = 0; 
	// Agent array
	size_t agentsSizeInBytes = MAX_AGENTS * sizeof(PPGSAgent);
	agentArrayHost = (PPGSAgent *) malloc(agentsSizeInBytes);
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
	grassMatrixHost = (cl_uint *) malloc(grassSizeInBytes);
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
	PPAgentParams agent_params[2];
	agent_params[SHEEP_ID].gain_from_food = params.sheep_gain_from_food;
	agent_params[SHEEP_ID].reproduce_threshold = params.sheep_reproduce_threshold;
	agent_params[SHEEP_ID].reproduce_prob = params.sheep_reproduce_prob;
	agent_params[WOLF_ID].gain_from_food = params.wolves_gain_from_food;
	agent_params[WOLF_ID].reproduce_threshold = params.wolves_reproduce_threshold;
	agent_params[WOLF_ID].reproduce_prob = params.wolves_reproduce_prob;
	// Sim parameters
	PPGSSimParams sim_params;
	sim_params.size_x = params.grid_x;
	sim_params.size_y = params.grid_y;
	sim_params.size_xy = params.grid_x * params.grid_y;
	sim_params.max_agents = MAX_AGENTS;
	sim_params.grass_restart = params.grass_restart;
	sim_params.grid_cell_space = CELL_SPACE;
	// RNG seeds
	size_t rngSeedsSizeInBytes = MAX_AGENTS * sizeof(cl_ulong);
	rngSeedsHost = (cl_ulong*) malloc(rngSeedsSizeInBytes);
	for (int i = 0; i < MAX_AGENTS; i++) {
		rngSeedsHost[i] = rand();
	}

	// 6. Create OpenCL buffers
	statsArrayDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, statsSizeInBytes, statsArrayHost, &status );
	clu_if_error_goto(status, "Creating statsArrayDevice", error);

	agentArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, agentsSizeInBytes, agentArrayHost, &status );
	clu_if_error_goto(status, "Creating agentsArrayDevice", error);

	grassMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes, grassMatrixHost, &status );
	clu_if_error_goto(status, "Creating grassMatrixDevice", error);

	iterDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint), &iter, &status );
	clu_if_error_goto(status, "Creating iterDevice", error);

	// Stuff to get number of agents out in each iteration
	numAgentsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_uint), numAgentsHost, &status );
	clu_if_error_goto(status, "Creating numAgentsDevice", error);

	grassCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, grasscount2_gws[0]*sizeof(cl_uint), NULL, &status );
	clu_if_error_goto(status, "Creating grassCountDevice", error);

	agentsCountDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, (MAX_AGENTS / agentcount2_lws)*sizeof(cl_uint2), NULL, &status ); // This size is the maximum you'll ever need for the given maximum number of agents
	clu_if_error_goto(status, "Creating agentsCountDevice", error);

	agentParamsDevice = clCreateBuffer(zone.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2*sizeof(PPAgentParams), agent_params, &status ); // Two types of agent, thus two packs of agent parameters
	clu_if_error_goto(status, "Creating agentParamsDevice", error);

	rngSeedsDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, rngSeedsSizeInBytes, rngSeedsHost, &status );
	clu_if_error_goto(status, "Creating rngSeedsDevice", error);

	// 7. Set fixed kernel arguments

	// Sort kernel
	status = clSetKernelArg(sort_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	clu_if_error_goto(status, "Set arg 0 of sort kernel", error);

	// Agent movement kernel
	status = clSetKernelArg(agentmov_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	clu_if_error_goto(status, "Set arg 0 of agentmov kernel", error);

	status = clSetKernelArg(agentmov_kernel, 1, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_goto(status, "Set arg 1 of agentmov kernel", error);

	status = clSetKernelArg(agentmov_kernel, 2, sizeof(PPGSSimParams), (void *) &sim_params);
	clu_if_error_goto(status, "Set arg 2 of agentmov kernel", error);

	status = clSetKernelArg(agentmov_kernel, 3, sizeof(cl_mem), (void *) &iterDevice);
	clu_if_error_goto(status, "Set arg 3 of agentmov kernel", error);

	// Agent grid update kernel
	status = clSetKernelArg(agentupdate_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	clu_if_error_goto(status, "Set arg 0 of agentupdate kernel", error);

	status = clSetKernelArg(agentupdate_kernel, 1, sizeof(cl_mem), (void *) &grassMatrixDevice);
	clu_if_error_goto(status, "Set arg 1 of agentupdate kernel", error);

	status = clSetKernelArg(agentupdate_kernel, 2, sizeof(PPGSSimParams), (void *) &sim_params);
	clu_if_error_goto(status, "Set arg 2 of agentupdate kernel", error);

	// Grass kernel
	status = clSetKernelArg(grass_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	clu_if_error_goto(status, "Set arg 0 of grass kernel", error);

	status = clSetKernelArg(grass_kernel, 1, sizeof(PPGSSimParams), (void *) &sim_params);
	clu_if_error_goto(status, "Set arg 1 of grass kernel", error);

	// Agent actions kernel
	status = clSetKernelArg(agentaction_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	clu_if_error_goto(status, "Set arg 0 of agentaction kernel", error);

	status = clSetKernelArg(agentaction_kernel, 1, sizeof(cl_mem), (void *) &grassMatrixDevice);
	clu_if_error_goto(status, "Set arg 1 of agentaction kernel", error);

	status = clSetKernelArg(agentaction_kernel, 2, sizeof(PPGSSimParams), (void *) &sim_params);
	clu_if_error_goto(status, "Set arg 2 of agentaction kernel", error);

	status = clSetKernelArg(agentaction_kernel, 3, sizeof(cl_mem), (void *) &agentParamsDevice);
	clu_if_error_goto(status, "Set arg 3 of agentaction kernel", error);

	status = clSetKernelArg(agentaction_kernel, 4, sizeof(cl_mem), (void *) &rngSeedsDevice);
	clu_if_error_goto(status, "Set arg 4 of agentaction kernel", error);

	status = clSetKernelArg(agentaction_kernel, 5, sizeof(cl_mem), (void *) &numAgentsDevice);
	clu_if_error_goto(status, "Set arg 5 of agentaction kernel", error);

	// Count agents
	status = clSetKernelArg(countagents1_kernel, 0, sizeof(cl_mem), (void *) &agentArrayDevice);
	clu_if_error_goto(status, "Set arg 0 of countagents1 kernel", error);

	status = clSetKernelArg(countagents1_kernel, 1, sizeof(cl_mem), (void *) &agentsCountDevice);
	clu_if_error_goto(status, "Set arg 1 of countagents1 kernel", error);

	status = clSetKernelArg(countagents1_kernel, 2, agentcount1_lws*sizeof(cl_uint2), NULL);
	clu_if_error_goto(status, "Set arg 2 of countagents1 kernel", error);

	status = clSetKernelArg(countagents2_kernel, 0, sizeof(cl_mem), (void *) &agentsCountDevice);
	clu_if_error_goto(status, "Set arg 0 of countagents2 kernel", error);

	status = clSetKernelArg(countagents2_kernel, 1, agentcount2_lws*sizeof(cl_uint2), NULL);
	clu_if_error_goto(status, "Set arg 1 of countagents2 kernel", error);

	status = clSetKernelArg(countagents2_kernel, 3, sizeof(cl_mem), (void *) &numAgentsDevice);
	clu_if_error_goto(status, "Set arg 3 of countagents2 kernel", error);

	status = clSetKernelArg(countagents2_kernel, 4, sizeof(cl_mem), (void *) &statsArrayDevice);
	clu_if_error_goto(status, "Set arg 4 of countagents2 kernel", error);

	status = clSetKernelArg(countagents2_kernel, 5, sizeof(cl_mem), (void *) &iterDevice);
	clu_if_error_goto(status, "Set arg 5 of countagents2 kernel", error);

	// Count grass
	status = clSetKernelArg(countgrass1_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	clu_if_error_goto(status, "Set arg 0 of countgrass1 kernel", error);

	status = clSetKernelArg(countgrass1_kernel, 1, sizeof(cl_mem), (void *) &grassCountDevice);
	clu_if_error_goto(status, "Set arg 1 of countgrass1 kernel", error);

	status = clSetKernelArg(countgrass1_kernel, 2, grasscount1_lws*sizeof(cl_uint), NULL);
	clu_if_error_goto(status, "Set arg 2 of countgrass1 kernel", error);

	status = clSetKernelArg(countgrass1_kernel, 3, sizeof(PPGSSimParams), (void *) &sim_params);
	clu_if_error_goto(status, "Set arg 3 of countgrass1 kernel", error);

	status = clSetKernelArg(countgrass2_kernel, 0, sizeof(cl_mem), (void *) &grassCountDevice);
	clu_if_error_goto(status, "Set arg 0 of countgrass2 kernel", error);

	status = clSetKernelArg(countgrass2_kernel, 1, grasscount2_gws[0]*sizeof(cl_uint), NULL);
	clu_if_error_goto(status, "Set arg 1 of countgrass2 kernel", error);

	status = clSetKernelArg(countgrass2_kernel, 3, sizeof(cl_mem), (void *) &statsArrayDevice);
	clu_if_error_goto(status, "Set arg 3 of countgrass2 kernel", error);

	status = clSetKernelArg(countgrass2_kernel, 4, sizeof(cl_mem), (void *) &iterDevice);
	clu_if_error_goto(status, "Set arg 4 of countgrass2 kernel", error);

	//printf("sort: %d\t agc2: %d\t gc2: %d\n", params.iters * sum(tzc(nlpo2(MAX_AGENTS))), params.iters * (MAX_AGENTS/4) / agentcount2_lws, params.iters * numGrassCount2Loops);

	// 8. Run the show
	agentaction_move_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	grass_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	agentaction_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	agentsort_event = (cl_event*) calloc(params.iters * sum(tzc(nlpo2(MAX_AGENTS))), sizeof(cl_event)); //Worse case usage scenario
	agentcount1_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	agentcount2_event = (cl_event*) calloc((MAX_AGENTS/4) / agentcount2_lws * params.iters, sizeof(cl_event)); // Optimistic usage to save memory => may break
	agentupdate_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	grasscount1_event = (cl_event*) calloc(params.iters, sizeof(cl_event)); 
	grasscount2_event = (cl_event*) calloc(params.iters * numGrassCount2Loops, sizeof(cl_event)); // Exact usage scenario
	readNumAgents_event = (cl_event*) calloc(params.iters, sizeof(cl_event));
	cl_uint agentsort_event_index = 0, agentcount2_event_index = 0, grasscount2_event_index = 0;
	clFinish(zone.queues[0]); // Guarantee all memory transfers are performed
	profcl_profile_start(profile);
	for (iter = 1; iter <= params.iters; iter++) {
		//printf("iter %d\n", iter);

		// Determine agent kernels size for this iteration
		cl_uint maxOccupiedSpace = (*numAgentsHost) * 2; // Worst case array agent (dead or alive) occupation
		agent_gws = LWS_GPU_PREF * ceil(((float) maxOccupiedSpace) / LWS_GPU_PREF);
		agentcount1_gws = LWS_GPU_MAX * ceil(((float) maxOccupiedSpace) / LWS_GPU_MAX);
		cl_uint effectiveNextAgentsToCount = agentcount1_gws / agentcount1_lws;
		cl_uint iterbase = iter - 1;

		// Agent movement
		status = clEnqueueNDRangeKernel( zone.queues[0], agentmov_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, agentaction_move_event + iterbase);
		clu_if_error_goto(status, "agentmov kernel", error);

		// Grass growth and agent number reset
		status = clEnqueueNDRangeKernel( zone.queues[0], grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, grass_event + iterbase);
		clu_if_error_goto(status, "grass kernel", error);

		// Sort agent array
		agentsort_gws = nlpo2(maxOccupiedSpace) / 2;
		agentsort_lws = LWS_GPU_PREF;
		while (agentsort_gws % agentsort_lws != 0)
			agentsort_lws = agentsort_lws / 2;
		cl_uint totalStages = (cl_uint) tzc(agentsort_gws * 2);
		status = clEnqueueWaitForEvents(zone.queues[0], 1, agentaction_move_event + iterbase);
		clu_if_error_goto(status, "wait for events after agentmov", error);
		for (unsigned int currentStage = 1; currentStage <= totalStages; currentStage++) {
			cl_uint step = currentStage;
			for (int currentStep = step; currentStep > 0; currentStep--) {
				status = clSetKernelArg(sort_kernel, 1, sizeof(cl_uint), (void *) &currentStage);
				clu_if_error_goto(status, "arg 1 of sort kernel", error);
				status = clSetKernelArg(sort_kernel, 2, sizeof(cl_uint), (void *) &currentStep);
				clu_if_error_goto(status, "arg 2 of sort kernel", error);
				status = clEnqueueNDRangeKernel( zone.queues[0], sort_kernel, 1, NULL, &agentsort_gws, &agentsort_lws, 0, NULL, agentsort_event + agentsort_event_index);
				clu_if_error_goto(status, "sort kernel", error);
				status = clEnqueueBarrier(zone.queues[0]);
				clu_if_error_goto(status, "in sort agents loop", error);

				agentsort_event_index++;
			}
		}

		// Update agent number in grid
		status = clEnqueueNDRangeKernel( zone.queues[0], agentupdate_kernel, 1, NULL, &agent_gws, &agent_lws, 0, NULL, agentupdate_event + iterbase);
		clu_if_error_goto(status, "agentupdate_kernel", error);

		// agent actions
		status = clEnqueueNDRangeKernel( zone.queues[0], agentaction_kernel, 1, NULL, &agent_gws, &agent_lws, 1, agentupdate_event + iterbase, agentaction_event + iterbase);
		clu_if_error_goto(status, "agentaction kernel", error);

		// Gather statistics
		// Count agents, part 1
		status = clEnqueueNDRangeKernel( zone.queues[0], countagents1_kernel, 1, NULL, &agentcount1_gws, &agentcount1_lws, 1, agentaction_event + iterbase, agentcount1_event + iterbase);
		clu_if_error_goto(status, "countagents1 kernel", error);
		// Count grass, part 1
		status = clEnqueueNDRangeKernel( zone.queues[0], countgrass1_kernel, 1, NULL, &grasscount1_gws, &grasscount1_lws, 1, agentaction_event + iterbase, grasscount1_event + iterbase);
		clu_if_error_goto(status, "countgrass1 kernel", error);
		// Count agents, part 2
		do {
			agentcount2_gws = LWS_GPU_MAX * ceil(((float) effectiveNextAgentsToCount) / LWS_GPU_MAX);

			status = clSetKernelArg(countagents2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextAgentsToCount);
			clu_if_error_goto(status, "Arg 2 of countagents2 kernel", error);

			status = clEnqueueNDRangeKernel( zone.queues[0], countagents2_kernel, 1, NULL, &agentcount2_gws, &agentcount2_lws, 1, agentcount1_event + iterbase, agentcount2_event + agentcount2_event_index);
			clu_if_error_goto(status, "countagents2 kernel", error);

			effectiveNextAgentsToCount = agentcount2_gws / agentcount2_lws;

			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_goto(status, "in agent count loops", error);

			agentcount2_event_index++;

		} while (effectiveNextAgentsToCount > 1);

		// Get total number of agents
		status = clEnqueueReadBuffer(zone.queues[0], numAgentsDevice, CL_FALSE, 0, sizeof(cl_uint), numAgentsHost, 0, NULL, readNumAgents_event + iterbase);
		clu_if_error_goto(status, "numAgents read", error);

		// Count grass, part 2
		for (int i = 0; i < numGrassCount2Loops; i++) {

			status = clSetKernelArg(countgrass2_kernel, 2, sizeof(cl_uint), (void *) &effectiveNextGrassToCount[i]);
			clu_if_error_goto(status, "Arg 2 of countgrass2 kernel", error);

			status = clEnqueueNDRangeKernel( zone.queues[0], countgrass2_kernel, 1, NULL, &grasscount2_gws[i], &grasscount2_lws, 1, grasscount1_event + iterbase, grasscount2_event + grasscount2_event_index);
			clu_if_error_goto(status, "countgrass2_kernel", error);

			status = clEnqueueBarrier(zone.queues[0]);
			clu_if_error_goto(status, "in grass count loops", error);

			grasscount2_event_index++;

		}

		// Confirm that number of agents have been read
		status = clWaitForEvents(1, readNumAgents_event + iterbase); // Maybe put this in device queue instead of being in CPU time
		clu_if_error_goto(status, "wait for numAgents read", error);

	}

	// Guarantee all kernels have really terminated...
	clFinish(zone.queues[0]);

	// Finish profiling
	profcl_profile_stop(profile);  

	// Get statistics
	status = clEnqueueReadBuffer(zone.queues[0], statsArrayDevice, CL_TRUE, 0, statsSizeInBytes, statsArrayHost, 0, NULL, NULL);
	clu_if_error_goto(status, "statsArray read", error);
	
	// 9. Output results to file
	FILE * fp1 = fopen("stats.txt","w");
	for (unsigned int i = 0; i <= params.iters; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statsArrayHost[i].sheep, statsArrayHost[i].wolves, statsArrayHost[i].grass );
	fclose(fp1);

	/* Analyze events */
#ifdef CLPROFILER 
	for (unsigned int i = 0; i < params.iters; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("agentaction_move", agentaction_move_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("grass", grass_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("agentupdate", agentupdate_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("agentaction", agentaction_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("agentcount1", agentcount1_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("grasscount1", grasscount1_event[i], &status));
		profcl_profile_add(profile, profcl_evinfo_get("readNumAgents", readNumAgents_event[i], &status));
	}
	for (unsigned int i = 0; i < agentsort_event_index; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("agentsort", agentsort_event[i], &status));
	}
	for (unsigned int i = 0; i < grasscount2_event_index; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("grasscount2", grasscount2_event[i], &status));
	}
	for (unsigned int i = 0; i < agentcount2_event_index; i++) {
		profcl_profile_add(profile, profcl_evinfo_get("agentcount2", agentcount2_event[i], &status));
	}
	profcl_profile_aggregate(profile);
	profcl_profile_overmat(profile);	
#endif

	/* Print profiling info */ 
	profcl_print_info(profile);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	if (zone.build_log)
		printf(
			"\n******************************* Build Log *******************************\n\
			 \n%s\
			 \n*************************************************************************\n\n", 
			 zone.build_log);

cleanup:
	
	/* 11. Free stuff! */
	if (profile)
		profcl_profile_free(profile);
		
		
	/* Release events */
	if (agentaction_move_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentaction_move_event[i])
				clReleaseEvent(agentaction_move_event[i]);	
		free(agentaction_move_event);
	}
	
	if (grass_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (grass_event[i])
				clReleaseEvent(grass_event[i]);	
		free(grass_event);
	}
	if (agentupdate_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentupdate_event[i])
				clReleaseEvent(agentupdate_event[i]);	
		free(agentupdate_event);
	}
	if (agentaction_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentaction_event[i])
				clReleaseEvent(agentaction_event[i]);	
		free(agentaction_event);
	}
	if (agentcount1_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (agentcount1_event[i])
				clReleaseEvent(agentcount1_event[i]);	
		free(agentcount1_event);
	}
	if (grasscount1_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (grasscount1_event[i])
				clReleaseEvent(grasscount1_event[i]);	
		free(grasscount1_event);
	}
	if (readNumAgents_event) {
		for (unsigned int i = 0; i < params.iters; i++)
			if (readNumAgents_event[i])
				clReleaseEvent(readNumAgents_event[i]);	
		free(readNumAgents_event);
	}
	if (agentsort_event) {
		for (unsigned int i = 0; i < params.iters * sum(tzc(nlpo2(MAX_AGENTS))); i++) {
			clReleaseEvent(agentsort_event[i]);
		}
		free(agentsort_event);
	}
	if (grasscount2_event) {
		for (unsigned int i = 0; i < params.iters * numGrassCount2Loops; i++) {
			clReleaseEvent(grasscount2_event[i]);
		}
		free(grasscount2_event);
	}
	if (agentcount2_event) {
		for (unsigned int i = 0; i < (MAX_AGENTS/4) / agentcount2_lws * params.iters; i++) {
			clReleaseEvent(agentcount2_event[i]);
		}
		free(agentcount2_event);
	}


	// Free OpenCL kernels
	if (grass_kernel) clReleaseKernel(grass_kernel);
	if (agentmov_kernel) clReleaseKernel(agentmov_kernel);
	if (agentupdate_kernel) clReleaseKernel(agentupdate_kernel);
	if (sort_kernel) clReleaseKernel(sort_kernel);
	if (agentaction_kernel) clReleaseKernel(agentaction_kernel);
	if (countagents1_kernel) clReleaseKernel(countagents1_kernel);
	if (countagents2_kernel) clReleaseKernel(countagents2_kernel);
	if (countgrass1_kernel) clReleaseKernel(countgrass1_kernel); 
	if (countgrass2_kernel) clReleaseKernel(countgrass2_kernel);

	// Free OpenCL memory objects
	if (statsArrayDevice) clReleaseMemObject(statsArrayDevice);
	if (agentArrayDevice) clReleaseMemObject(agentArrayDevice);
	if (grassMatrixDevice) clReleaseMemObject(grassMatrixDevice);
	if (iterDevice) clReleaseMemObject(iterDevice);
	if (numAgentsDevice) clReleaseMemObject(numAgentsDevice);
	if (grassCountDevice) clReleaseMemObject(grassCountDevice);
	if (agentsCountDevice) clReleaseMemObject(agentsCountDevice );
	if (agentParamsDevice) clReleaseMemObject(agentParamsDevice);
	if (rngSeedsDevice) clReleaseMemObject(rngSeedsDevice);

	// Free OpenCL zone
	clu_zone_free(&zone);

	// Free host resources
	if (statsArrayHost) free(statsArrayHost);
	if (numAgentsHost) free(numAgentsHost);
	if (agentArrayHost) free(agentArrayHost);
	if (grassMatrixHost) free(grassMatrixHost);
	if (rngSeedsHost) free(rngSeedsHost);

	return 0;
	
}


// Compute worksizes depending on the device type and number of available compute units
void computeWorkSizes(PPParameters params, cl_uint device_type, cl_uint cu) {
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
cl_int getKernelEntryPoints(cl_program program) {
	cl_int status;
	grass_kernel = clCreateKernel( program, "Grass", &status );
	clu_if_error_return(status);
	agentmov_kernel = clCreateKernel( program, "RandomWalk", &status );
	clu_if_error_return(status);
	agentupdate_kernel = clCreateKernel( program, "AgentsUpdateGrid", &status );
	clu_if_error_return(status);
	sort_kernel = clCreateKernel( program, "BitonicSort", &status );
	clu_if_error_return(status);
	agentaction_kernel = clCreateKernel( program, "AgentAction", &status );
	clu_if_error_return(status);
	countagents1_kernel = clCreateKernel( program, "CountAgents1", &status );
	clu_if_error_return(status);
	countagents2_kernel = clCreateKernel( program, "CountAgents2", &status );
	clu_if_error_return(status);
	countgrass1_kernel = clCreateKernel( program, "CountGrass1", &status );
	clu_if_error_return(status);
	countgrass2_kernel = clCreateKernel( program, "CountGrass2", &status );
	clu_if_error_return(status);
	return CL_SUCCESS;
}
