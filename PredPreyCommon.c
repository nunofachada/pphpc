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
CLZONE getClZone(const char* kernels_file, cl_uint deviceType, cl_uint numQueues, char profile) {
	// Helper variables
	cl_int status;
	// Setup queue
	cl_int queue_properties;
	if (profile)
		queue_properties = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
	else
		queue_properties = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
	// Get number of platforms
	cl_uint numPlatforms;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) { PrintErrorGetPlatformIDs(status, "I was getting the number of platforms!\n"); exit(EXIT_FAILURE); }
	
	// Get existing platforms
	cl_platform_id platfIds[numPlatforms];
	status = clGetPlatformIDs(numPlatforms, platfIds, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetPlatformIDs(status, "I was getting the platform IDs...\n"); exit(EXIT_FAILURE); }
	// Cycle through platforms, get specified devices in existing platforms
	DEVICE_INFO devInfos[MAX_DEVICES_TOTAL];
	unsigned int totalNumDevices = 0;
	for(unsigned int i = 0; i < numPlatforms; i++)
	{
		// Get specified devices for current platform
		cl_uint numDevices;
		cl_device_id devIds[MAX_DEVICES_PER_PLATFORM];
		status = clGetDeviceIDs( platfIds[i], deviceType, MAX_DEVICES_PER_PLATFORM, devIds, &numDevices );
		if (status != CL_DEVICE_NOT_FOUND) {
			// At least one device found, lets take note
			if (status != CL_SUCCESS) { PrintErrorGetDeviceIDs(status, NULL ); exit(-1); }
			for (unsigned int j = 0; j < numDevices; j++) {
				devInfos[totalNumDevices].id = devIds[j];
				devInfos[totalNumDevices].platformId = platfIds[i];
				status = clGetDeviceInfo(devIds[j], CL_DEVICE_NAME, sizeof(devInfos[totalNumDevices].name), devInfos[totalNumDevices].name, NULL);
				if (status != CL_SUCCESS) { PrintErrorGetDeviceInfo(status, NULL ); exit(-1); }
				status = clGetPlatformInfo( platfIds[i], CL_PLATFORM_VENDOR, sizeof(devInfos[totalNumDevices].platformName), devInfos[totalNumDevices].platformName, NULL);
				if (status != CL_SUCCESS) { PrintErrorGetPlatformInfo(status, NULL ); exit(-1); }
				totalNumDevices++;
			}
		}
	}
	// Check whether any devices of the specified type were found
	unsigned int deviceInfoIndex;
	if (totalNumDevices == 0) {
		// No devices of the specified type where found, exit
		printf("No devices of the specified type were found. Exiting...\n");
		exit(EXIT_FAILURE);
	} else if (totalNumDevices == 1) {
		// Only one device of the specified type was found, use it
		printf("Using device '%s' from platform '%s'\n", devInfos[0].name, devInfos[0].platformName);
		deviceInfoIndex = 0;
	} else {
		// Several compatible devices found, ask user to chose one
		printf("Several devices of the specified type found. Please chose one:\n\n");
		int result;
		do {
			// Print available devices
			for (unsigned int i = 0; i < totalNumDevices; i++)
				printf("\t(%d) %s, %s\n", i, devInfos[i].name, devInfos[i].platformName);			
			printf("\n>> ");
			result = scanf ("%u", &deviceInfoIndex);
			// Clean keyboard buffer
			int c;
			do { c = getchar(); } while (c != '\n' && c != EOF);
			// Check if result is Ok and break the loop if so
			if (1 == result) {
				if ((deviceInfoIndex >= 0) && (deviceInfoIndex < totalNumDevices))
					break;
			}
			// Result not Ok, print error message
			printf("\n Invalid choice, please insert a value between 0 and %u.\n\n", totalNumDevices - 1);
		} while (1);
	}
	
	// Define the CL zone
	CLZONE zone;
	zone.device_type = deviceType;
	zone.device = devInfos[deviceInfoIndex].id;
	zone.platform = devInfos[deviceInfoIndex].platformId;

	// Determine number of compute units for that device
	cl_uint compute_units;
	status = clGetDeviceInfo( zone.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &compute_units, NULL);
	if( status != CL_SUCCESS ) { PrintErrorGetDeviceInfo( status, NULL ); exit(-1); }
	zone.cu = compute_units;
	
	// Create a context on that device.
	cl_context_properties cps[3] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties) devInfos[deviceInfoIndex].platformId, 0};
	cl_context context = clCreateContext( cps, 1, &zone.device, NULL, NULL, &status);
	if (status != CL_SUCCESS) { PrintErrorCreateContext(status, NULL); exit(-1); }
	zone.context = context;
	
	// Create the specified command queues on that device
	zone.queues = (cl_command_queue*) malloc(numQueues * sizeof(cl_command_queue));
	for (unsigned int i = 0; i < numQueues; i++) {
		cl_command_queue queue = clCreateCommandQueue( context, zone.device, queue_properties, &status );
		if (status != CL_SUCCESS) { PrintErrorCreateCommandQueue(status, NULL); exit(-1); }
		zone.queues[i] = queue;
	}
	
	// Import kernels
	const char * source1 = importKernel("PredPreyCommon_Kernels.cl");
	const char * source2 = importKernel(kernels_file);
	const char ** source = (const char**) malloc(2 * sizeof(const char*));
	source[0] = source1;
	source[1] = source2;
	
	// Create program and perform runtime source compilation
	cl_program program = clCreateProgramWithSource( zone.context, 2, source, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateProgramWithSource(status, NULL); exit(-1); }
	status = clBuildProgram( program, 1, &zone.device, NULL, NULL, NULL );
	if (status != CL_SUCCESS) {
		PrintErrorBuildProgram( status, NULL );
		//exit(-1);
		char buildLog[15000];
		status = clGetProgramBuildInfo(program, devInfos[deviceInfoIndex].id, CL_PROGRAM_BUILD_LOG, 15000*sizeof(char), buildLog, NULL);
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
    for (unsigned int i = 0; i < zone.numQueues; i++)
		if (zone.queues[i]) clReleaseCommandQueue(zone.queues[i]);
	free(zone.queues);
    if (zone.context) clReleaseContext(zone.context);
}

