#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

typedef struct kernel_work_group_info {
	size_t preferred_work_group_size_multiple;
	size_t compile_work_group_size[3];
	size_t max_work_group_size;
	cl_ulong local_mem_size;
	cl_ulong private_mem_size;
} KERNEL_WORK_GROUP_INFO;

cl_uint getWorkGroupInfo(cl_kernel kernel, cl_device_id device, KERNEL_WORK_GROUP_INFO* kwgi);

void printWorkGroupInfo(KERNEL_WORK_GROUP_INFO kwgi);
