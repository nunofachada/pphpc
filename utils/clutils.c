#include "clutils.h"

cl_uint clu_get_workgroup_info(cl_kernel kernel, cl_device_id device, CLUKernelWorkgroupInfo* kwgi) {

	cl_uint status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &(kwgi->preferred_work_group_size_multiple), NULL);
	clu_if_error_return(status);

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 3 * sizeof(size_t), kwgi->compile_work_group_size, NULL);
	clu_if_error_return(status);

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &(kwgi->max_work_group_size), NULL);
	clu_if_error_return(status);

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(cl_ulong), &(kwgi->local_mem_size), NULL);
	clu_if_error_return(status);

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &(kwgi->private_mem_size), NULL);
	clu_if_error_return(status);

	return CL_SUCCESS;
}

void clu_print_workgroup_info(CLUKernelWorkgroupInfo kwgi) {

	printf("Preferred multiple of workgroup size: %d\n", (int) kwgi.preferred_work_group_size_multiple);
	printf("WG size in __attribute__((reqd_work_gr oup_size(X, Y, Z))) qualifier: (%d, %d, %d)\n", (int) kwgi.compile_work_group_size[0], (int) kwgi.compile_work_group_size[1], (int) kwgi.compile_work_group_size[2]);
	printf("Max. workgroup size: %d\n", (int) kwgi.max_work_group_size);
	printf("Local memory used by kernel: %ld bytes\n", (long) kwgi.local_mem_size);
	printf("Min. private memory used by each workitem: %ld bytes\n", (long) kwgi.private_mem_size);

}

char* clu_get_device_type_str(cl_device_type cldt, int full, char* str, int strSize) {

	int occuSpace = 0;
	char temp[30];
	*str = 0;

	if (cldt & CL_DEVICE_TYPE_DEFAULT) {
		strcpy(temp, full ? CLU_DEVICE_TYPE_DEFAULT_STR_FULL : CLU_DEVICE_TYPE_DEFAULT_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_CPU) {
		strcpy(temp, full ? CLU_DEVICE_TYPE_CPU_STR_FULL : CLU_DEVICE_TYPE_CPU_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_GPU) {
		strcpy(temp, full ? CLU_DEVICE_TYPE_GPU_STR_FULL : CLU_DEVICE_TYPE_GPU_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_ACCELERATOR) {
		strcpy(temp, full ? CLU_DEVICE_TYPE_ACCELERATOR_STR_FULL : CLU_DEVICE_TYPE_ACCELERATOR_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt == CL_DEVICE_TYPE_ALL) {
		strcpy(temp, full ? CLU_DEVICE_TYPE_ALL_STR_FULL : CLU_DEVICE_TYPE_ALL_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	return str;
}


// Get a CL zone with all the stuff required
cl_int clu_zone_new(CLUZone* zone, const char** kernelFiles, cl_uint numKernelFiles, const char* compilerOpts, cl_uint deviceType, cl_uint numQueues, cl_int queueProperties) {
	
	// Helper variables
	cl_int status;
	
	// Initialize zone
	zone->platform = NULL;
	zone->device = NULL;
	zone->context = NULL;
	zone->queues = NULL;
	zone->program = NULL;
	zone->device_name = NULL;
	zone->platform_name = NULL;
	zone->build_log = NULL;	
		
	// Get number of platforms
	cl_uint numPlatforms;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	clu_if_error_return(status);
	
	// Get existing platforms
	cl_platform_id platfIds[numPlatforms];
	status = clGetPlatformIDs(numPlatforms, platfIds, NULL);
	clu_if_error_return(status);

	// Cycle through platforms, get specified devices in existing platforms
	CLUDeviceInfo devInfos[CLU_MAX_DEVICES_TOTAL];
	unsigned int totalNumDevices = 0;
	for(unsigned int i = 0; i < numPlatforms; i++)
	{
		// Get specified devices for current platform
		cl_uint numDevices;
		cl_device_id devIds[CLU_MAX_DEVICES_PER_PLATFORM];
		status = clGetDeviceIDs( platfIds[i], deviceType, CLU_MAX_DEVICES_PER_PLATFORM, devIds, &numDevices );
		if (status != CL_DEVICE_NOT_FOUND) {
			// At least one device found, lets take note
			clu_if_error_return(status);
			for (unsigned int j = 0; j < numDevices; j++) {
				devInfos[totalNumDevices].id = devIds[j];
				devInfos[totalNumDevices].platformId = platfIds[i];
				status = clGetDeviceInfo(devIds[j], CL_DEVICE_NAME, sizeof(devInfos[totalNumDevices].name), devInfos[totalNumDevices].name, NULL);
				clu_if_error_return(status);
				status = clGetPlatformInfo( platfIds[i], CL_PLATFORM_VENDOR, sizeof(devInfos[totalNumDevices].platformName), devInfos[totalNumDevices].platformName, NULL);
				clu_if_error_return(status);
				totalNumDevices++;
			}
		}
	}
	
	// Check whether any devices of the specified type were found
	unsigned int deviceInfoIndex = 0;
	if (totalNumDevices == 0) {
		// No devices of the specified type where found, return
		return CL_DEVICE_NOT_FOUND;
	} else if (totalNumDevices > 1) {
		// Several compatible devices found, ask user to chose one
		// TODO Outsource this question to another function given by the caller function
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

	// Store info about the device and platform being used
	zone->device_type = deviceType;
	zone->device = devInfos[deviceInfoIndex].id;
	zone->platform = devInfos[deviceInfoIndex].platformId;
	zone->device_name = (char*) malloc((strlen(devInfos[deviceInfoIndex].name) + 1) * sizeof(char));
	strcpy(zone->device_name, devInfos[deviceInfoIndex].name);
	zone->platform_name = (char*) malloc((strlen(devInfos[deviceInfoIndex].platformName) + 1) * sizeof(char));
	strcpy(zone->platform_name, devInfos[deviceInfoIndex].platformName);

	// Determine number of compute units for that device
	cl_uint compute_units;
	status = clGetDeviceInfo( zone->device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &compute_units, NULL);
	clu_if_error_return(status);
	zone->cu = compute_units;
	
	// Create a context on that device.
	cl_context_properties cps[3] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties) devInfos[deviceInfoIndex].platformId, 0};
	cl_context context = clCreateContext( cps, 1, &zone->device, NULL, NULL, &status);
	clu_if_error_return(status);
	zone->context = context;
	
	// Create the specified command queues on that device
	zone->numQueues = numQueues;
	zone->queues = (cl_command_queue*) malloc(numQueues * sizeof(cl_command_queue));
	for (unsigned int i = 0; i < numQueues; i++) {
		cl_command_queue queue = clCreateCommandQueue( context, zone->device, queueProperties, &status );
		clu_if_error_return(status);
		zone->queues[i] = queue;
	}
	
	// Import kernels
	char** source = (char**) malloc(numKernelFiles * sizeof(char*));
	for (unsigned int i = 0; i < numKernelFiles; i++) {
		char * partialSource = clu_load_source(kernelFiles[i]);
		source[i] = partialSource;
	}
	
	// Load kernels sources and create program 
	cl_program program = clCreateProgramWithSource( zone->context, numKernelFiles, (const char**) source, NULL, &status );
	for (unsigned int i = 0; i < numKernelFiles; i++) {
		clu_free_source(source[i]);
	}
	free(source);
	clu_if_error_return(status);
	
	// Perform runtime source compilation of program
	cl_int bpStatus = clBuildProgram( program, 1, &zone->device, compilerOpts, NULL, NULL );
	if (bpStatus != CL_SUCCESS) {
		size_t logsize;
		status = clGetProgramBuildInfo(program, devInfos[deviceInfoIndex].id, CL_PROGRAM_BUILD_LOG, 0, NULL, &logsize);
		clu_if_error_return(status);
		char * buildLog = (char*) malloc(logsize + 1);
		buildLog[logsize] = '\0';
		status = clGetProgramBuildInfo(program, devInfos[deviceInfoIndex].id, CL_PROGRAM_BUILD_LOG, logsize + 1, buildLog, NULL);
		if (status == CL_SUCCESS) 
			zone->build_log = buildLog;
		else 
			clu_if_error_return(status);
		return bpStatus;
	}
	zone->program = program;

	// Success!
	return CL_SUCCESS;
}

// Destroy a CL zone
void clu_zone_free(CLUZone* zone) {
	for (unsigned int i = 0; i < zone->numQueues; i++)
		if (zone->queues[i]) clReleaseCommandQueue(zone->queues[i]);
	free(zone->queues);
	if (zone->program) clReleaseProgram(zone->program);
	if (zone->context) clReleaseContext(zone->context);
	if (zone->device_name) free(zone->device_name);
	if (zone->platform_name) free(zone->platform_name);
	if (zone->build_log) free(zone->build_log);

}

char* clu_load_source( const char * filename ) 
{
	FILE * fp = fopen(filename, "r");
	char * sourcetmp;

	if (!fp) {
		sourcetmp = (char*) malloc(sizeof(char));
		sourcetmp[0] = '\0';
		return sourcetmp;
	}
	
	fseek(fp, 0L, SEEK_END);
	int prog_size = (int) ftell(fp);
	sourcetmp = (char*) malloc((prog_size + 1)*sizeof(char));
	rewind(fp);
	fread(sourcetmp, sizeof(char), prog_size, fp);
	sourcetmp[prog_size] = '\0';
	fclose(fp);
	return sourcetmp;
}

void clu_free_source( char* source) {
	free(source);
}

