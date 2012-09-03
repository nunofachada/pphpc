#include "PredPreyCommon.h"

// Load parameters
PARAMS loadParams(const char* paramsFile) {
	PARAMS parameters;
	FILE * fp = fopen(paramsFile, "r");
	if(fp == NULL) {
		printf("Error: File 'config.txt' not found!\n");
		exit(-1);
	}	
	char param[100];
	unsigned int value;
	unsigned int check = 0;
	while (fscanf(fp, "%s = %d", param, &value) != EOF) {
		if (strcmp(param, "INIT_SHEEP") == 0) {
			if ((1 & check) == 0) {
				parameters.init_sheep = value;
				check = check | 1;
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 1)) == 0) {
				parameters.sheep_gain_from_food = value;
				check = check | (1 << 1);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 2)) == 0) {
				parameters.sheep_reproduce_threshold = value;
				check = check | (1 << 2);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 3)) == 0) {
				parameters.sheep_reproduce_prob = value;
				check = check | (1 << 3);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "INIT_WOLVES") == 0) {
			if ((1 & (check >> 4)) == 0) {
				parameters.init_wolves = value;
				check = check | (1 << 4);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 5)) == 0) {
				parameters.wolves_gain_from_food = value;
				check = check | (1 << 5);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 6)) == 0) {
				parameters.wolves_reproduce_threshold = value;
				check = check | (1 << 6);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 7)) == 0) {
				parameters.wolves_reproduce_prob = value;
				check = check | (1 << 7);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRASS_RESTART") == 0) {
			if ((1 & (check >> 8)) == 0) {
				parameters.grass_restart = value;
				check = check | (1 << 8);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRID_X") == 0) {
			if ((1 & (check >> 9)) == 0) {
				parameters.grid_x = value;
				check = check | (1 << 9);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRID_Y") == 0) {
			if ((1 & (check >> 10)) == 0) {
				parameters.grid_y = value;
				check = check | (1 << 10);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "ITERS") == 0) {
			if ((1 & (check >> 11)) == 0) {
				parameters.iters = value;
				check = check | (1 << 11);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else {
			printf("Error: Invalid parameter '%s' in config file!\n", param);
			exit(-1);
		}
	}
	if (check != 0x0fff) {
			printf("Error: Insufficient parameters in config file (check=%d)!\n", check);
			exit(-1);
	}
	return parameters;
} 

// Get a CL zone with all the stuff required
CLZONE getClZone(const char* vendor, const char* kernels_file, cl_uint deviceType) {
	char pbuff[100];
	CLZONE zone;
	cl_int status;
#ifdef CLPROFILER
	cl_int queue_properties = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
#else
	cl_int queue_properties = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
#endif
	// Set device type
	zone.device_type = deviceType;
	// Get a platform
	cl_uint numPlatforms;
	cl_platform_id platform = NULL;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	cl_platform_id platforms[numPlatforms];
	clGetPlatformIDs(numPlatforms, platforms, NULL);
	for(unsigned int i=0; i < numPlatforms; ++i)
	{
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
	cl_context_properties cps[3] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties) platform, 0};
	cl_context context = clCreateContext( cps, 1, &device, NULL, NULL, &status);
	if (status != CL_SUCCESS) { PrintErrorCreateContext(status, NULL); exit(-1); }
	zone.context = context;
	cl_command_queue queue = clCreateCommandQueue( context, device, queue_properties, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateCommandQueue(status, NULL); exit(-1); }
	zone.queue = queue;
	// Import kernels
	const char * source1 = importKernel("PredPreyCommon_Kernels.cl");
	const char * source2 = importKernel(kernels_file);
	const char ** source = (const char**) malloc(2 * sizeof(const char*));
	source[0] = source1;
	source[1] = source2;
	
	// Create program and perform runtime source compilation
	cl_program program = clCreateProgramWithSource( zone.context, 2, source, NULL, &status );
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

// Destroy a CL zone
void destroyClZone(CLZONE zone) {
    if (zone.program) clReleaseProgram(zone.program);
    if (zone.queue) clReleaseCommandQueue(zone.queue);
    if (zone.context) clReleaseContext(zone.context);
}

