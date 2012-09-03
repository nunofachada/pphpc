#include "PredPrey.hpp"

#define MAX_AGENTS 1048576

#define INIT_SHEEP 6400 //25600 //6400
#define SHEEP_GAIN_FROM_FOOD 4
#define SHEEP_REPRODUCE_THRESHOLD 2
#define SHEEP_REPRODUCE_PROB 4 //between 0 and 100
#define SHEEP_ID 0

#define INIT_WOLVES 3200 //12800 //3200
#define WOLVES_GAIN_FROM_FOOD 20
#define WOLVES_REPRODUCE_THRESHOLD 2
#define WOLVES_REPRODUCE_PROB 5 //between 0 and 100
#define WOLF_ID 1

#define GRASS_RESTART 10

#define GRID_X 400 //800 //400
#define GRID_Y 400 //800 //400

#define CELL_SPACE 4
#define CELL_GRASS_OFFSET 0
#define CELL_NUMAGENTS_OFFSET 1
#define CELL_AGINDEX_OFFSET 2

#define ITERS 2000

#define LWS_GPU_MAX 512
#define LWS_GPU_PREF 128
#define LWS_GPU_MIN 8

#define LWS_GPU_PREF_2D_X 16
#define LWS_GPU_PREF_2D_Y 8

#define MAX_GRASS_COUNT_LOOPS 5 //More than enough...

// Get a CL zone with all the stuff required
CLZONE getClZone(const char* vendor, cl_uint deviceType) {
	CLZONE zone;
	cl_int status;
	// Get a platform
	cl_uint numPlatforms;
	cl_platform_id platform = NULL;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	cl_platform_id* platforms = new cl_platform_id[numPlatforms];
	clGetPlatformIDs(numPlatforms, platforms, NULL);
	for(unsigned int i=0; i < numPlatforms; ++i)
	{
		char pbuff[100];
		clGetPlatformInfo( platforms[i], CL_PLATFORM_VENDOR, sizeof(pbuff), pbuff, NULL);
		platform = platforms[i];
		if(!strcmp(pbuff, vendor))
			break;
	}
	zone.platform = platform;
	// Find a device
	cl_device_id device;
	status = clGetDeviceIDs( platform, deviceType, 1, &device, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetDeviceIDs(status, NULL ); exit(-1); }
	zone.device = device;
	// Determine number of compute units for that device
	cl_uint compute_units;
	status = clGetDeviceInfo( device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &compute_units, NULL);
	if( status != CL_SUCCESS ) { PrintErrorGetDeviceInfo( status, NULL ); exit(-1); }
	zone.cu = compute_units;
	// Create a context and command queue on that device.
	cl_context context = clCreateContext( NULL, 1, &device, NULL, NULL, &status);
	if (status != CL_SUCCESS) { PrintErrorCreateContext(status, NULL); exit(-1); }
	zone.context = context;
	cl_command_queue queue = clCreateCommandQueue( context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateCommandQueue(status, NULL); exit(-1); }
	zone.queue = queue;
	// Import kernels
	const char * source = importKernel("PredPrey_Kernels.cl");
	// Create program and perform runtime source compilation
	cl_program program = clCreateProgramWithSource( zone.context, 1, &source, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateProgramWithSource(status, NULL); exit(-1); }
	status = clBuildProgram( program, 1, &device, NULL, NULL, NULL );
	if (status != CL_SUCCESS) {
		PrintErrorBuildProgram( status, NULL );
		//exit(-1);
		char buildLog[15000];
		status = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 15000*sizeof(char), buildLog, NULL);
		if (status == CL_SUCCESS) printf("******************** Start of Build Log *********************:\n\n%s\n******************** End of Build Log *********************\n", buildLog);
		else PrintErrorGetProgramBuildInfo(status, NULL);
		exit(-1);
	}
	zone.program = program;
	// Return zone object
	return zone;
}

// Print array of agents
void printAgentArray(cl_uint4* array, unsigned int size) {
	for (int i = 0; i < size; i++)
	{
		printf("x=%d\ty=%d\te=%d\tmov=%d\n", array[i].x, array[i].y, array[i].z, array[i].w);
	}
}

// Print grass matrix
void printGrassMatrix(cl_uint* matrix, unsigned int size_x,  unsigned int size_y) {
	for (int i = 0; i < size_y; i++)
	{
		printf("NUM: ");
		for (int j = 0; j < size_x; j++)
		{
			unsigned int index = CELL_SPACE*(j + i*size_x);
			//printf("%c", matrix[index + CELL_GRASS_OFFSET] == 0 ? '*' : '-');
			//printf("%d\t", matrix[index + CELL_GRASS_OFFSET]);
			printf("%d\t", matrix[index + CELL_NUMAGENTS_OFFSET]);
		}
		printf("\n");
		printf("IDX: ");
		for (int j = 0; j < size_x; j++)
		{
			unsigned int index = CELL_SPACE*(j + i*size_x);
			//printf("%c", matrix[index + CELL_GRASS_OFFSET] == 0 ? '*' : '-');
			//printf("%d\t", matrix[index + CELL_GRASS_OFFSET]);
			printf("%d\t", matrix[index + CELL_AGINDEX_OFFSET]);
		}
		printf("\n\n");
	}
}

// Returns the next larger power of 2 of the given value
unsigned int nlpo2(register unsigned int x)
{
	/* If value already is power of 2, return it has is. */
	if ((x&(x-1)) == 0) return x;
	/* If not, determine next larger power of 2. */
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return(x+1);
}

// Returns the the number of one bits in the given value.
unsigned int ones32(register unsigned int x)
{
        /* 32-bit recursive reduction using SWAR...
	   but first step is mapping 2-bit values
	   into sum of 2 1-bit values in sneaky way
	*/
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}
// Returns the trailing Zero Count (i.e. the log2 of a base 2 number)
unsigned int tzc(register int x)
{
        return(ones32((x & -x) - 1));
}

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
	CLZONE zone = getClZone("NVIDIA Corporation", CL_DEVICE_TYPE_GPU);


	// 3. Compute work sizes for different kernels.
	size_t agentsort_gws, agent_gws, grass_gws[2], agentcount1_gws, agentcount2_gws, grasscount1_gws, grasscount2_gws[MAX_GRASS_COUNT_LOOPS];
	size_t agentsort_lws, agent_lws, grass_lws[2], agentcount1_lws, agentcount2_lws, grasscount1_lws, grasscount2_lws;
	int effectiveNextGrassToCount[MAX_GRASS_COUNT_LOOPS];


	// grass growth worksizes
	grass_lws[0] = LWS_GPU_PREF_2D_X;
	grass_gws[0] = LWS_GPU_PREF_2D_X * ceil(((float) GRID_X) / LWS_GPU_PREF_2D_X);
	grass_lws[1] = LWS_GPU_PREF_2D_Y;
	grass_gws[1] = LWS_GPU_PREF_2D_Y * ceil(((float) GRID_Y) / LWS_GPU_PREF_2D_Y);
	// fixed local agent kernel worksizes
	agent_lws = LWS_GPU_PREF;
	agentcount1_lws = LWS_GPU_MAX;
	agentcount2_lws = LWS_GPU_MAX;

	// grass count worksizes
	grasscount1_lws = LWS_GPU_MAX;
	grasscount1_gws = LWS_GPU_MAX * ceil((((float) GRID_X * GRID_Y)) / LWS_GPU_MAX);
	grasscount2_lws = LWS_GPU_MAX;
	effectiveNextGrassToCount[0] = grasscount1_gws / grasscount1_lws;
	grasscount2_gws[0] = LWS_GPU_MAX * ceil(((float) effectiveNextGrassToCount[0]) / LWS_GPU_MAX);
	
	// Determine number of loops of secondary count required to perform complete reduction (count)
	int numGrassCount2Loops = 1;
	while ( grasscount2_gws[numGrassCount2Loops - 1] > grasscount2_lws) {
		effectiveNextGrassToCount[numGrassCount2Loops] = grasscount2_gws[numGrassCount2Loops - 1] / grasscount2_lws;
		grasscount2_gws[numGrassCount2Loops] = LWS_GPU_MAX * ceil(((float) effectiveNextGrassToCount[numGrassCount2Loops]) / LWS_GPU_MAX);
		numGrassCount2Loops++;
	}
	
	//grasscount2_gws[0] = grasscount1_gws / grasscount1_lws;
	//grasscount2_lws = grasscount1_gws / grasscount1_lws;

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

	// 5. obtain kernels entry points.
	cl_kernel grass_kernel = clCreateKernel( zone.program, "Grass", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Grass kernel"); exit(-1); }
	cl_kernel agentmov_kernel = clCreateKernel( zone.program, "RandomWalk", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "RandomWalk kernel"); exit(-1); }
	cl_kernel agentupdate_kernel = clCreateKernel( zone.program, "AgentsUpdateGrid", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "AgentsUpdateGrid kernel"); exit(-1); }
	cl_kernel sort_kernel = clCreateKernel( zone.program, "BitonicSort", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "BitonicSort kernel"); exit(-1); }
	cl_kernel agentaction_kernel = clCreateKernel( zone.program, "AgentAction", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Agent kernel"); exit(-1); }
	cl_kernel countagents1_kernel = clCreateKernel( zone.program, "CountAgents1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountAgents kernel"); exit(-1); }
	cl_kernel countagents2_kernel = clCreateKernel( zone.program, "CountAgents2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountAgents kernel"); exit(-1); }
	cl_kernel countgrass1_kernel = clCreateKernel( zone.program, "CountGrass1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass1 kernel"); exit(-1); }
	cl_kernel countgrass2_kernel = clCreateKernel( zone.program, "CountGrass2", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountGrass2 kernel"); exit(-1); }

	// 6. Create and initialize host buffers
	// Statistics
	STATS statistics;
	size_t statsSize = (ITERS + 1) * sizeof(unsigned int);
	statistics.sheep = (unsigned int *) malloc(statsSize);
	statistics.wolves = (unsigned int *) malloc(statsSize);
	statistics.grass = (unsigned int *) malloc(statsSize);
	statistics.sheep[0] = INIT_SHEEP;
	statistics.wolves[0] = INIT_WOLVES;
	statistics.grass[0] = 0;
	// Agent array
	size_t agentsSizeInBytes = MAX_AGENTS * sizeof(AGENT);
	AGENT * agentArrayHost = (AGENT *) malloc(agentsSizeInBytes);
	for(int i = 0; i < MAX_AGENTS; i++)
	{
		agentArrayHost[i].x = rand() % GRID_X;
		agentArrayHost[i].y = rand() % GRID_Y;
		if (i < INIT_SHEEP)
		{
			agentArrayHost[i].energy = 1 + (rand() % (SHEEP_GAIN_FROM_FOOD * 2));
			agentArrayHost[i].type = 0; // Sheep
			agentArrayHost[i].alive = 1;
		}
		else if (i < INIT_SHEEP + INIT_WOLVES)
		{
			agentArrayHost[i].energy = 1 + (rand() % (WOLVES_GAIN_FROM_FOOD * 2));
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
	size_t grassSizeInBytes = CELL_SPACE * GRID_X * GRID_Y * sizeof(cl_uint);
	cl_uint * grassMatrixHost = (cl_uint *) malloc(grassSizeInBytes);
	for(int i = 0; i < GRID_X; i++)
	{
		for (int j = 0; j < GRID_Y; j++)
		{
			unsigned int gridIndex = (i + j*GRID_X) * CELL_SPACE;
			grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] = (rand() % 2) == 0 ? 0 : 1 + (rand() % GRASS_RESTART);
			if (grassMatrixHost[gridIndex + CELL_GRASS_OFFSET] == 0)
				statistics.grass[0]++;
		}
	}
	// Agent parameters
	cl_uint grassIndex = 2;
	AGENT_PARAMS agent_params[2];
	agent_params[SHEEP_ID].gain_from_food = SHEEP_GAIN_FROM_FOOD;
	agent_params[SHEEP_ID].reproduce_threshold = SHEEP_REPRODUCE_THRESHOLD;
	agent_params[SHEEP_ID].reproduce_prob = SHEEP_REPRODUCE_PROB;
	agent_params[WOLF_ID].gain_from_food = WOLVES_GAIN_FROM_FOOD;
	agent_params[WOLF_ID].reproduce_threshold = WOLVES_REPRODUCE_THRESHOLD;
	agent_params[WOLF_ID].reproduce_prob = WOLVES_REPRODUCE_PROB;
	// Sim parameters
	SIM_PARAMS sim_params;
	sim_params.size_x = GRID_X;
	sim_params.size_y = GRID_Y;
	sim_params.size_xy = GRID_X * GRID_Y;
	sim_params.max_agents = MAX_AGENTS;
	sim_params.grass_restart = GRASS_RESTART;
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
	
	// 7. Creat OpenCL buffers

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
	

	// 8. Set fixed kernel arguments

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
	
        // 9. Run the show
	cl_event agentsort_event, agentaction_move_event, grass_event, agentaction_event, agentcount1_event, agentupdate_event, grasscount1_event;
	char msg[500];
	gettimeofday(&time0, NULL);
	for (cl_int iter = 1; iter <= ITERS; iter++) {

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
		for (int currentStage = 1; currentStage <= totalStages; currentStage++) {
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
	gettimeofday(&time1, NULL);  
	
	// 10. Get results back
	status = clEnqueueReadBuffer( zone.queue, agentArrayDevice, CL_TRUE, 0, agentsSizeInBytes, agentArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback agentArray"); return(-1); };

	status = clEnqueueReadBuffer( zone.queue, grassMatrixDevice, CL_TRUE, 0, grassSizeInBytes, grassMatrixHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback grassMatrix"); return(-1); };

	//status = clEnqueueReadBuffer( zone.queue, dbgDevice, CL_TRUE, 0, debugSizeInBytes, dbgHost, 0, NULL, NULL);
	//if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback dbg"); return(-1); };

	// 11. Output results
	FILE * fp1 = fopen("stats.txt","w");
	for (int i = 0; i <= ITERS; i++)
		fprintf(fp1, "%d\t%d\t%d\n", statistics.sheep[i], statistics.wolves[i], statistics.grass[i] );
	fclose(fp1);

	FILE * fp2 = fopen("agentArray.txt","w");
	for (int i = 0; i < MAX_AGENTS; i++)
		fprintf(fp2, "x=%d\ty=%d\te=%d\ttype=%d\talive=%d\n", agentArrayHost[i].x, agentArrayHost[i].y, agentArrayHost[i].energy, agentArrayHost[i].type, agentArrayHost[i].alive);
	fclose(fp2);
	/*FILE * fp3 = fopen("debug.txt","w");
	for (int i = 0; i < (MAX_AGENTS); i++) {
		if ((dbgHost[i].s0 == 1) && (dbgHost[i].s1 == 1) && (dbgHost[i].s5 < MAX_AGENTS))
			fprintf(fp3, "w_gid=%d\tw_x=%d\tw_y=%d\ts_gid=%d\ts_x=%d\ts_y=%d\n", dbgHost[i].s2, dbgHost[i].s3, dbgHost[i].s4, dbgHost[i].s5, dbgHost[i].s6, dbgHost[i].s7);
	}
	fclose(fp3);
	printGrassMatrix(grassMatrixHost, GRID_X, GRID_Y);*/

	// 12. Print timmings
	dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Time = %f\n", dt);

	// 13. Free stuff!
        free(agentArrayHost);
	free(grassMatrixHost);
	free(statistics.sheep);
	free(statistics.wolves);
	free(statistics.grass);
	//clReleaseMemObject(agentArrayDevice);
	//clReleaseMemObject(grassMatrixDevice);
	//clReleaseMemObject(statsDevice);
	return 0;
}

