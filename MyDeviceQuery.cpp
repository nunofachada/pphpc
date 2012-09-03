// A minimalist OpenCL program.
#include <CL/cl.h>
#include <stdio.h>

#define MAX_DEVICES_QUERY 10
#define MAX_INFO_STRING 250

int main(int argc, char ** argv)
{

	// Get platforms
	cl_uint numPlatforms;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	printf("Number of platforms: %d\n", numPlatforms);
	cl_platform_id* platforms = new cl_platform_id[numPlatforms];
	clGetPlatformIDs(numPlatforms, platforms, NULL);

	// Cycle through platforms
	for(unsigned int i = 0; i < numPlatforms; i++)
	{
		// Print platform info
		char pbuff[MAX_INFO_STRING];
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
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, sizeof(pbuff), pbuff, NULL);
			printf("\t           %s\n", pbuff);
		}
	}
	return 0;
}

