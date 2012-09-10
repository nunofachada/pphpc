// A minimalist OpenCL program.
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include "clinfo.h"

#define MAX_DEVICES_QUERY 10
#define MAX_INFO_STRING 250

int main(int argc, char ** argv)
{

	// Get platforms
	cl_uint numPlatforms;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	printf("Number of platforms: %d\n", numPlatforms);
	cl_platform_id* platforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, platforms, NULL);

	// Cycle through platforms
	for(unsigned int i = 0; i < numPlatforms; i++)
	{
		// Print platform info
		char pbuff[MAX_INFO_STRING];
		size_t sizetaux;
		cl_uint uintaux;
		cl_ulong ulongaux;
		cl_device_type dtypeaux;
		cl_device_local_mem_type dlmt;
		cl_bool boolaux;
		clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(pbuff), pbuff, NULL);
		printf("Platform #%d: %s\n", i, pbuff);
		// Get devices in platform
		cl_device_id devices[MAX_DEVICES_QUERY];
		cl_uint numDevices;
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, MAX_DEVICES_QUERY, devices, &numDevices);
		// Cycle through devices
		for (unsigned int j = 0; j < numDevices; j++)
		{
			// Print device info
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(pbuff), pbuff, NULL);
			printf("\tDevice #%d: %s\n", j, pbuff);
			clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(cl_device_type), &dtypeaux, NULL);
			printf("\t           Type: %s\n", getDeviceTypeStr(dtypeaux, 0, pbuff, MAX_INFO_STRING));
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, sizeof(pbuff), pbuff, NULL);
			printf("\t           %s\n", pbuff);
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(uintaux), &uintaux, NULL);
			printf("\t           Max. Compute units: %d\n", uintaux);
			clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(ulongaux), &ulongaux, NULL);
			clGetDeviceInfo(devices[j], CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(boolaux), &boolaux, NULL);
			printf("\t           Global mem. size: %ld Kb = %ld Mb (%s)\n", ulongaux / 1024, ulongaux / 1024 / 1024, boolaux ? "shared with host" : "dedicated");
			clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_TYPE, sizeof(dlmt), &dlmt, NULL);
			clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(ulongaux), &ulongaux, NULL);
			printf("\t           Local mem. type / size: %s / %ld Kb\n", (dlmt == CL_LOCAL ? "local" : "global"), ulongaux / 1024);
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(sizetaux), &sizetaux, NULL);
			printf("\t           Max. work-group size: %d\n", (int) sizetaux);

		}
	}
	return 0;
}

