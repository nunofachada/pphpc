#ifndef CLUTILS_H
#define CLUTILS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define CLU_DEVICE_TYPE_DEFAULT_STR_FULL "CL_DEVICE_TYPE_DEFAULT"
#define CLU_DEVICE_TYPE_CPU_STR_FULL "CL_DEVICE_TYPE_CPU"
#define CLU_DEVICE_TYPE_GPU_STR_FULL "CL_DEVICE_TYPE_GPU"
#define CLU_DEVICE_TYPE_ACCELERATOR_STR_FULL "CL_DEVICE_TYPE_ACCELERATOR"
#define CLU_DEVICE_TYPE_ALL_STR_FULL "CL_DEVICE_TYPE_ALL"

#define CLU_DEVICE_TYPE_DEFAULT_STR "Default"
#define CLU_DEVICE_TYPE_CPU_STR "CPU"
#define CLU_DEVICE_TYPE_GPU_STR "GPU"
#define CLU_DEVICE_TYPE_ACCELERATOR_STR "Accelerator"
#define CLU_DEVICE_TYPE_ALL_STR "All"

#define CLU_MAX_AUX_BUFF 500

#define CLU_MAX_PLATFORMS 10
#define CLU_MAX_DEVICES_PER_PLATFORM 10
#define CLU_MAX_DEVICES_TOTAL 20

#define CLU_NUM_INFO_MSGS 2

#define clu_if_error_exit(err, msg) if (err != CL_SUCCESS) { printf("Error %d!\n", err); perror(msg); exit(EXIT_FAILURE); }
#define clu_if_error_return(err) if (err != CL_SUCCESS) return err
#define clu_if_error_goto(err, msg, dest) if (err != 0) { printf("Error %d!\n", err); perror(msg); goto dest; }

typedef struct clu_kernel_work_group_info {
	size_t preferred_work_group_size_multiple;
	size_t compile_work_group_size[3];
	size_t max_work_group_size;
	cl_ulong local_mem_size;
	cl_ulong private_mem_size;
} CLUKernelWorkgroupInfo;

typedef struct clu_zone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint device_type;
	cl_uint cu;
	cl_context context;
	cl_command_queue* queues;
	cl_program program;
	cl_uint numQueues;
	char* device_name;
	char* platform_name;
	char* build_log;
} CLUZone;

typedef struct clu_device_info {
	cl_device_id id;
	char name[CLU_MAX_AUX_BUFF];
	cl_platform_id platformId;
	char platformName[CLU_MAX_AUX_BUFF];
} CLUDeviceInfo;

cl_uint clu_get_workgroup_info(cl_kernel kernel, cl_device_id device, CLUKernelWorkgroupInfo* kwgi);

void clu_print_workgroup_info(CLUKernelWorkgroupInfo kwgi);

char* clu_get_device_type_str(cl_device_type cldt, int full, char* str, int strSize);

cl_int clu_zone_new(CLUZone* zone, const char** kernelFiles, cl_uint numKernelFiles, const char* compilerOpts, cl_uint deviceType, cl_uint numQueues, cl_int queueProperties);

void clu_zone_free(CLUZone* zone);

char* clu_load_source( const char* );

void clu_free_source( char* );

#endif
