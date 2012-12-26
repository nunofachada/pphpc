#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define CL_DEVICE_TYPE_DEFAULT_STR_FULL "CL_DEVICE_TYPE_DEFAULT"
#define CL_DEVICE_TYPE_CPU_STR_FULL "CL_DEVICE_TYPE_CPU"
#define CL_DEVICE_TYPE_GPU_STR_FULL "CL_DEVICE_TYPE_GPU"
#define CL_DEVICE_TYPE_ACCELERATOR_STR_FULL "CL_DEVICE_TYPE_ACCELERATOR"
#define CL_DEVICE_TYPE_ALL_STR_FULL "CL_DEVICE_TYPE_ALL"

#define CL_DEVICE_TYPE_DEFAULT_STR "Default"
#define CL_DEVICE_TYPE_CPU_STR "CPU"
#define CL_DEVICE_TYPE_GPU_STR "GPU"
#define CL_DEVICE_TYPE_ACCELERATOR_STR "Accelerator"
#define CL_DEVICE_TYPE_ALL_STR "All"

typedef struct kernel_work_group_info {
	size_t preferred_work_group_size_multiple;
	size_t compile_work_group_size[3];
	size_t max_work_group_size;
	cl_ulong local_mem_size;
	cl_ulong private_mem_size;
} KERNEL_WORK_GROUP_INFO;

cl_uint getWorkGroupInfo(cl_kernel kernel, cl_device_id device, KERNEL_WORK_GROUP_INFO* kwgi);

void printWorkGroupInfo(KERNEL_WORK_GROUP_INFO kwgi);

char* getDeviceTypeStr(cl_device_type cldt, int full, char* str, int strSize);


