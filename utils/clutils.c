#include "clutils.h"

// Get a CL zone with all the stuff required
CLZONE getClZone(const char* vendor, const char* kernels_file, cl_uint deviceType) {
	CLZONE zone;
	cl_int status;
	// Set device type
	zone.device_type = deviceType;
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
	const char * source = importKernel(kernels_file);
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
