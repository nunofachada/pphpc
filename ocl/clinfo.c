#include <string.h>
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

char* getDeviceTypeStr(cl_device_type cldt, int full, char* str, int strSize) {

	int occuSpace = 0;
	char temp[30];
	*str = 0;

	if (cldt & CL_DEVICE_TYPE_DEFAULT) {
		strcpy(temp, full ? CL_DEVICE_TYPE_DEFAULT_STR_FULL : CL_DEVICE_TYPE_DEFAULT_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_CPU) {
		strcpy(temp, full ? CL_DEVICE_TYPE_CPU_STR_FULL : CL_DEVICE_TYPE_CPU_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_GPU) {
		strcpy(temp, full ? CL_DEVICE_TYPE_GPU_STR_FULL : CL_DEVICE_TYPE_GPU_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt & CL_DEVICE_TYPE_ACCELERATOR) {
		strcpy(temp, full ? CL_DEVICE_TYPE_ACCELERATOR_STR_FULL : CL_DEVICE_TYPE_ACCELERATOR_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	if (cldt == CL_DEVICE_TYPE_ALL) {
		strcpy(temp, full ? CL_DEVICE_TYPE_ALL_STR_FULL : CL_DEVICE_TYPE_ALL_STR);
		int availSpace = strSize - occuSpace - 2; // 1 for space + 1 for \0 
		if (strlen(temp) <= availSpace) {
			strcat(str, " ");
			strcat(str, temp);
			availSpace -= strlen(temp);
		}
	}
	return str;
}
