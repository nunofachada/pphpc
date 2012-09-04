#include "clinfo.h"

cl_uint getWorkGroupInfo(cl_kernel kernel, cl_device_id device, KERNEL_WORK_GROUP_INFO* kwgi) {

	cl_uint status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &(kwgi->preferred_work_group_size_multiple), NULL);
	if (status != CL_SUCCESS) return status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 3 * sizeof(size_t), kwgi->compile_work_group_size, NULL);
	if (status != CL_SUCCESS) return status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &(kwgi->max_work_group_size), NULL);
	if (status != CL_SUCCESS) return status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(cl_ulong), &(kwgi->local_mem_size), NULL);
	if (status != CL_SUCCESS) return status;

	status = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &(kwgi->private_mem_size), NULL);
	if (status != CL_SUCCESS) return status;

	return CL_SUCCESS;
}

void printWorkGroupInfo(KERNEL_WORK_GROUP_INFO kwgi) {

	printf("Preferred multiple of workgroup size: %d\n", (int) kwgi.preferred_work_group_size_multiple);
	printf("WG size in __attribute__((reqd_work_gr oup_size(X, Y, Z))) qualifier: (%d, %d, %d)\n", (int) kwgi.compile_work_group_size[0], (int) kwgi.compile_work_group_size[1], (int) kwgi.compile_work_group_size[2]);
	printf("Max. workgroup size: %d\n", (int) kwgi.max_work_group_size);
	printf("Local memory used by kernel: %ld bytes\n", (long) kwgi.local_mem_size);
	printf("Min. private memory used by each workitem: %ld bytes\n", (long) kwgi.private_mem_size);

}
