// A minimalist OpenCL program.
#include "PredPrey.hpp"
#define MAX_SHEEP 4096
#define INIT_SHEEP 100
#define SHEEP_GAIN_FROM_FOOD 4
#define SHEEP_REPRODUCE_THRESHOLD 2
#define SHEEP_REPRODUCE_PROB 4 //between 0 and 100
#define MAX_WOLVES 4096
#define INIT_WOLVES 25
#define GRASS_RESTART 30
#define GRID_X 512
#define GRID_Y 512
#define MAX_SHEEP_ENERGY 1000
#define ITERS 2500
#define MAX_LWS_GPU 768
#define MIN_LWS_GPU 8

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
	cl_command_queue queue = clCreateCommandQueue( context, device, 0, &status );
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
		for (int j = 0; j < size_x; j++)
		{
			printf("%c", matrix[j + i*size_x] == 0 ? '*' : '-');
			//printf("%d\t", matrix[j + i*size_x]);
		}
		printf("\n");
	}
}

// Main stuff
int main(int argc, char ** argv)
{

	// Status var aux
	cl_int status;	
	
	// Init random number generator
	srandom(1/*(unsigned)(time(0))*/);

	// Timmings
	struct timeval time1, time0;
	double dt = 0;

	// 1. Get the required CL zone.
	CLZONE zone = getClZone("NVIDIA Corporation", CL_DEVICE_TYPE_GPU);


	// 3. Compute work sizes for different kernels.
	size_t sheepsort_gws, sheepmov_gws, grass_gws[2], sheep_gws, sheepcount_gws;
	size_t sheepsort_lws, sheepmov_lws, grass_lws[2], sheep_lws, sheepcount_lws;
	// Preferred number of work groups
	unsigned int num_wgroups = (unsigned int) pow(2, zone.cu / 6);
	// sheep sort worksizes
	sheepsort_gws = MAX_SHEEP / 2;
	sheepsort_lws = fmin(MAX_LWS_GPU, fmax(MIN_LWS_GPU, sheepsort_gws/num_wgroups));
	// sheep movement worksizes
	sheepmov_gws = MAX_SHEEP;
	sheepmov_lws = fmin(MAX_LWS_GPU, fmax(MIN_LWS_GPU, sheepmov_gws/num_wgroups));
	// grass growth worksizes
	grass_gws[0] = GRID_X;  grass_gws[1] = GRID_Y;
	grass_lws[0] = grass_gws[0] / num_wgroups;
	while (grass_gws[0] % grass_lws[0] != 0) grass_lws[0]++;
	grass_lws[1] = grass_gws[1] / num_wgroups;
	while (grass_gws[1] % grass_lws[1] != 0) grass_lws[1]++;
	// sheep action worksizes
	sheep_gws = sheepmov_gws;
	sheep_lws = sheepmov_lws;
	// sheep count worksizes
	sheepcount_gws = sheepmov_gws;
	sheepcount_lws = sheepmov_lws;

	printf("Sizes:\n");
	printf("sheepsort_gws=%d\tsheepsort_lws=%d\n", (int) sheepsort_gws, (int) sheepsort_lws);
	printf("sheepmov_gws=%d\tsheepmov_lws=%d\n", (int) sheepmov_gws, (int) sheepmov_lws);
	printf("grass_gws=[%d,%d]\tgrass_lws=[%d,%d]\n", (int) grass_gws[0], (int) grass_gws[1], (int) grass_lws[0], (int) grass_lws[1]);

	// 5. obtain kernels entry points.
	cl_kernel grass_kernel = clCreateKernel( zone.program, "Grass", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Grass kernel"); exit(-1); }
	cl_kernel agentmov_kernel = clCreateKernel( zone.program, "RandomWalk", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "RandomWalk kernel"); exit(-1); }
	cl_kernel sort_kernel = clCreateKernel( zone.program, "BitonicSort", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "BitonicSort kernel"); exit(-1); }
	cl_kernel sheep_kernel = clCreateKernel( zone.program, "Sheep", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Sheep kernel"); exit(-1); }
	cl_kernel countagents_kernel = clCreateKernel( zone.program, "CountAgents", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "CountAgents kernel"); exit(-1); }

	// 6. Create and initialize host buffers
	// Sheep array
	size_t sheepSizeInBytes = MAX_SHEEP * sizeof(cl_uint4);
	cl_uint4 * sheepArrayHost = (cl_uint4 *) malloc(sheepSizeInBytes);
	for(int i = 0; i < MAX_SHEEP; i++)
	{
		sheepArrayHost[i].x = rand() % GRID_X;
		sheepArrayHost[i].y = rand() % GRID_Y;
		if (i < INIT_SHEEP)
			sheepArrayHost[i].z = 1 + (rand() % (SHEEP_GAIN_FROM_FOOD * 2));
		else
			sheepArrayHost[i].z = 0;
		sheepArrayHost[i].w = 0;
	}
	// Grass matrix
	size_t grassSizeInBytes = GRID_X * GRID_Y * sizeof(cl_uint);
	cl_uint * grassMatrixHost = (cl_uint *) malloc(grassSizeInBytes);
	for(int i = 0; i < GRID_X; i++)
		for (int j = 0; j < GRID_Y; j++)
			grassMatrixHost[i + j*GRID_X] = (rand() % 2) == 0 ? 0 : 1 + (rand() % GRASS_RESTART);
	// Statistics
	size_t statsSize = ITERS * sizeof(cl_uint);
	cl_uint * numSheepsHost = (cl_uint *) malloc(statsSize);
	
	// 7. Creat OpenCL buffers
	cl_mem sheepArrayDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sheepSizeInBytes, sheepArrayHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "sheepArrayDevice"); return(-1); }
	cl_mem grassMatrixDevice = clCreateBuffer(zone.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, grassSizeInBytes, grassMatrixHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "grassMatrixDevice"); return(-1); }
	cl_mem numSheepsDevice = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, statsSize, numSheepsHost, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "numSheepsDevice"); return(-1); }

	// 8. Set fixed kernel arguments
	cl_uint totalsize = MAX_SHEEP;
	cl_uint grid_x = GRID_X;
	cl_uint grid_y = GRID_Y;
	cl_uint sheepGainFromFood = SHEEP_GAIN_FROM_FOOD;
	cl_uint grassRestart = GRASS_RESTART;
	cl_uint sheepReproduceThreshold = SHEEP_REPRODUCE_THRESHOLD;
	cl_uint sheepReproduceProb = SHEEP_REPRODUCE_PROB;
	// Sort kernel
	status = clSetKernelArg(sort_kernel, 0, sizeof(cl_mem), (void *) &sheepArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of sort kernel"); return(-1); }
	// Agent movement kernel
	status = clSetKernelArg(agentmov_kernel, 0, sizeof(cl_mem), (void *) &sheepArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of agentmov kernel"); return(-1); }
	status = clSetKernelArg(agentmov_kernel, 1, sizeof(cl_uint), (void *) &grid_x);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of agentmov kernel"); return(-1); }
	status = clSetKernelArg(agentmov_kernel, 2, sizeof(cl_uint), (void *) &grid_y);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of agentmov kernel"); return(-1); }
	// Grass kernel
	status = clSetKernelArg(grass_kernel, 0, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of grass kernel"); return(-1); }
	status = clSetKernelArg(grass_kernel, 1, sizeof(cl_uint), (void *) &grid_x);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of grass kernel"); return(-1); }
	// Sheep actions kernel
	status = clSetKernelArg(sheep_kernel, 0, sizeof(cl_mem), (void *) &sheepArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 1, sizeof(cl_mem), (void *) &grassMatrixDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 2, sizeof(cl_uint), (void *) &grid_x);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 3, sizeof(cl_uint), (void *) &sheepGainFromFood);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 4, sizeof(cl_uint), (void *) &grassRestart);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 5, sizeof(cl_uint), (void *) &sheepReproduceThreshold);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 5 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 6, sizeof(cl_uint), (void *) &sheepReproduceProb);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 6 of sheep kernel"); return(-1); }
	status = clSetKernelArg(sheep_kernel, 7, sizeof(cl_mem), (void *) &numSheepsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 7 of sheep kernel"); return(-1); }
	// Count agents
	status = clSetKernelArg(countagents_kernel, 0, sizeof(cl_mem), (void *) &sheepArrayDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of countagents kernel"); return(-1); }
	status = clSetKernelArg(countagents_kernel, 1, sizeof(cl_mem), (void *) &numSheepsDevice);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of countagents kernel"); return(-1); }
	

        // 9. Run the show
	cl_uint totalStages = (cl_uint) (logf((float) MAX_SHEEP)/logf(2.0f));
	cl_event sheepsort_event, sheep_move_event, grass_event, sheep_event, sheepcount_event;
	char msg[100];
	gettimeofday(&time0, NULL);
	for (cl_int iter = 0; iter < ITERS; iter++) {
		// Sheep movement
		status = clEnqueueNDRangeKernel( zone.queue, agentmov_kernel, 1, NULL, &sheepmov_gws, &sheepmov_lws, 0, NULL, &sheep_move_event);
		if (status != CL_SUCCESS) { sprintf(msg, "agentmov_kernel, iteration %d ", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		// Grass growth
		status = clEnqueueNDRangeKernel( zone.queue, grass_kernel, 2, NULL, grass_gws, grass_lws, 0, NULL, &grass_event);
		if (status != CL_SUCCESS) { sprintf(msg, "grass_kernel, iteration %d ", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		clEnqueueBarrier(zone.queue);
		// Sort sheep array
		for (int currentStage = 1; currentStage <= totalStages; currentStage++) {
			cl_uint step = currentStage;
			for (int currentStep = step; currentStep > 0; currentStep--) {
				status = clSetKernelArg(sort_kernel, 1, sizeof(cl_uint), (void *) &currentStage);
				if (status != CL_SUCCESS) { sprintf(msg, "argument 1 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clSetKernelArg(sort_kernel, 2, sizeof(cl_uint), (void *) &currentStep);
				if (status != CL_SUCCESS) {  sprintf(msg, "argument 2 of sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorSetKernelArg(status, msg); return(-1); }
				status = clEnqueueNDRangeKernel( zone.queue, sort_kernel, 1, NULL, &sheepsort_gws, &sheepsort_lws, 0, NULL, &sheepsort_event);
				if (status != CL_SUCCESS) {  sprintf(msg, "sort_kernel, iteration %d, stage %d, step %d", iter, currentStage, currentStep); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
				clWaitForEvents(1, &sheepsort_event);
			}
		}
		clEnqueueBarrier(zone.queue);
		// Sheep count
		status = clSetKernelArg(countagents_kernel, 2, sizeof(cl_uint), (void *) &iter);
		if (status != CL_SUCCESS) {  sprintf(msg, "argument 2 of countagents_kernel, iteration %d", iter); PrintErrorSetKernelArg(status, msg); return(-1); }
		status = clEnqueueNDRangeKernel( zone.queue, countagents_kernel, 1, NULL, &sheepcount_gws, &sheepcount_lws, 0, NULL, &sheepcount_event);
		if (status != CL_SUCCESS) { sprintf(msg, "countagents_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); return(-1); }
		clEnqueueBarrier(zone.queue);
		// Sheep actions
		status = clSetKernelArg(sheep_kernel, 8, sizeof(cl_uint), (void *) &iter);
		if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 8 of sheep kernel"); return(-1); }
		status = clEnqueueNDRangeKernel( zone.queue, sheep_kernel, 1, NULL, &sheep_gws, &sheep_lws, 0, NULL, &sheep_event);
		if (status != CL_SUCCESS) { sprintf(msg, "sheep_kernel, iteration %d", iter); PrintErrorEnqueueNDRangeKernel(status, msg); }
		clEnqueueBarrier(zone.queue);
		
	}
	gettimeofday(&time1, NULL);  

	// 10. Get results back
	status = clEnqueueReadBuffer( zone.queue, sheepArrayDevice, CL_TRUE, 0, sheepSizeInBytes, sheepArrayHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback sheepArray"); return(-1); };
	status = clEnqueueReadBuffer( zone.queue, grassMatrixDevice, CL_TRUE, 0, grassSizeInBytes, grassMatrixHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback grassMatrix"); return(-1); };
	status = clEnqueueReadBuffer( zone.queue, numSheepsDevice, CL_TRUE, 0, statsSize, numSheepsHost, 0, NULL, NULL);
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "readback numSheeps"); return(-1); };

	// 11. Check results
	FILE * fp1 = fopen("stats.txt","w");
	for (int i = 0; i < ITERS; i++)
		fprintf(fp1, "%d\n", numSheepsHost[i]);
	fclose(fp1);
	FILE * fp2 = fopen("sheeparray.txt","w");
	for (int i = 0; i < MAX_SHEEP; i++)
		fprintf(fp2, "x=%d\ty=%d\te=%d\tmov=%d\n", sheepArrayHost[i].x, sheepArrayHost[i].y, sheepArrayHost[i].z, sheepArrayHost[i].w);
	fclose(fp2);

	// 12. Print timmings
	dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Time = %f\n", dt);

	// 13. Free stuff!
        free(sheepArrayHost);
	free(grassMatrixHost);
	free(numSheepsHost);
	clReleaseMemObject(sheepArrayDevice);
	clReleaseMemObject(grassMatrixDevice);
	clReleaseMemObject(numSheepsDevice);
	return 0;
}

