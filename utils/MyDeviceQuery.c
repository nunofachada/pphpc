// A minimalist OpenCL program.
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include "clutils.h"

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
		cl_ulong ulongaux, ulongaux2;
		cl_device_type dtypeaux;
		cl_device_local_mem_type dlmt;
		cl_bool boolaux;
		cl_command_queue_properties cqpaux;
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
			printf("\t           Type: %s\n", clu_get_device_type_str(dtypeaux, 0, pbuff, MAX_INFO_STRING));
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, sizeof(pbuff), pbuff, NULL);
			printf("\t           %s\n", pbuff);
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(uintaux), &uintaux, NULL);
			printf("\t           Max. Compute units: %d\n", uintaux);
			clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(ulongaux), &ulongaux, NULL);
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(ulongaux2), &ulongaux2, NULL);
			clGetDeviceInfo(devices[j], CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(boolaux), &boolaux, NULL);
			printf("\t           Global mem. size: %ld Mb %s (max. alloc. %ld Mb)\n", (unsigned long int) ulongaux / 1024l / 1024l, boolaux ? "shared with host" : "dedicated", (unsigned long int) ulongaux2 / 1024l / 1024l);
			clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_TYPE, sizeof(dlmt), &dlmt, NULL);
			clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(ulongaux), &ulongaux, NULL);
			printf("\t           Local mem. size (type): %ld Kb (%s)\n", (unsigned long int) ulongaux / 1024l, (dlmt == CL_LOCAL ? "local" : "global"));
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(sizetaux), &sizetaux, NULL);
			printf("\t           Max. work-group size: %d\n", (int) sizetaux);
			// Print extra info if any arg is given
			if (argc > 1) {

				clGetDeviceInfo(devices[j], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(ulongaux), &ulongaux, NULL);
				printf("\t           Max. constant buffer size: %ld Kb\n", ulongaux / 1024l);

				clGetDeviceInfo(devices[j], CL_DEVICE_ENDIAN_LITTLE, sizeof(boolaux), &boolaux, NULL);
				printf("\t           Endianness: %s\n", boolaux ? "Little" : "Big");
				
				printf("\t           Pref. vec. width:");
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(uintaux), &uintaux, NULL);
				printf(" Char=%d,", uintaux);
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(uintaux), &uintaux, NULL);
				printf(" Short=%d,", uintaux);
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(uintaux), &uintaux, NULL);
				printf(" Long=%d,", uintaux);
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(uintaux), &uintaux, NULL);
				printf(" Float=%d,", uintaux);
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(uintaux), &uintaux, NULL);
				printf(" Double=%d,", uintaux);
				clGetDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(uintaux), &uintaux, NULL);
				printf(" Half=%d.\n", uintaux);

				clGetDeviceInfo(devices[j], CL_DEVICE_QUEUE_PROPERTIES, sizeof(cqpaux), &cqpaux, NULL);
				printf("\t           Command queue properties:");
				if (cqpaux & CL_QUEUE_PROFILING_ENABLE) printf(" Prof. OK,"); else printf("Prof. KO,");
				if (cqpaux & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) printf(" Out-of-order OK\n"); else printf(" Out-of-order KO\n");

			}

		}
	}
	return 0;
}

