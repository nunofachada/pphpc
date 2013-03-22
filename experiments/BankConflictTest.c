#include <math.h>
#include <glib.h>
#include "../utils/Profiler.h"
#include "../utils/clutils.h"

#define WS_X 4096
#define WS_Y 4096

#define LWS_GPU_PREF_2D_X 32
#define LWS_GPU_PREF_2D_Y 16

// Main stuff
int main(int argc, char *argv[])
{

	/* Aux vars */
	cl_int status;

	/* Error management. */
	GError *err = NULL;
		
	/* Profiling / Timmings */
	ProfCLProfile* profile = profcl_profile_new();

	/* Global work sizes */
	size_t gws_bankconf[2];

	/* Local work sizes */
	size_t lws_bankconf[2];

	/* Events */
	cl_event events[2] = {NULL, NULL};
	
	/* Kernel file */
	const char* kernelFiles[] = {"BankConflictTest_kernels.cl"};
	
	/* Get the required CL zone. */
	CLUZone zone;
	status = clu_zone_new(&zone, CL_DEVICE_TYPE_GPU, 1, CL_QUEUE_PROFILING_ENABLE, &err);
	clu_if_error_goto(status, err, error);
	
	/* Build program. */
	status = clu_program_create(&zone, kernelFiles, 1, NULL, &err);
	clu_if_error_goto(status, err, error);

	/* Kernel */
	cl_kernel kernel_bankconf = NULL;
	kernel_bankconf = clCreateKernel(zone.program, "bankconf", &status);
	clu_if_error_create_error_goto(status, &err, error, "bankconf kernel: create");

	/* Start basic timming / profiling. */
	profcl_profile_start(profile);

	////////////////////////////////////////	
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	/* Data in host */
	size_t sizeDataInBytes = WS_X * WS_Y * sizeof(cl_int);
	cl_int *data_host = (cl_int*) malloc(sizeDataInBytes);
	for (unsigned int i = 0; i < WS_Y * WS_X; i++)
			data_host[i] = i;

	///////////////////////////
	// Create device buffers //
	///////////////////////////

	/* Data in device */
	cl_mem data_device = NULL;
	data_device = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, sizeDataInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "data device: create");
	
	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer(zone.queues[0], data_device, CL_TRUE, 0, sizeDataInBytes, data_host, 0, NULL, &(events[0]));
	clu_if_error_create_error_goto(status, &err, error, "data device : write");

	//////////////////////////
	//  Determine worksizes //
	//////////////////////////
	
	lws_bankconf[0] = LWS_GPU_PREF_2D_X;
	lws_bankconf[1] = LWS_GPU_PREF_2D_Y;
	gws_bankconf[0] = LWS_GPU_PREF_2D_X * ceil(((float) WS_Y) / LWS_GPU_PREF_2D_X);
	gws_bankconf[1] = LWS_GPU_PREF_2D_Y * ceil(((float) WS_Y) / LWS_GPU_PREF_2D_Y);
	
	printf("\n------------------------------------------------\n");
	printf("Local work size  : (%zu, %zu)\n", lws_bankconf[0], lws_bankconf[1]);
	printf("Global work size : (%zu, %zu)\n", gws_bankconf[0], gws_bankconf[1]);
	printf("------------------------------------------------\n\n");
	
	/////////////////////////////////////////
	// Determine and print required memory //
	/////////////////////////////////////////

	size_t globalMemSizeInBytes = sizeDataInBytes;
	printf("\nGlobal memory required        : %zu bytes (%zu Kb = %zu Mb)", globalMemSizeInBytes, globalMemSizeInBytes / 1024, globalMemSizeInBytes / 1024 / 1024);
	
	size_t localMemSizeInBytes = lws_bankconf[1] * lws_bankconf[0] * sizeof(cl_int);
	printf("\nLocal memory required         : %zu bytes (%zu Kb)\n\n", localMemSizeInBytes, localMemSizeInBytes / 1024);

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	status = clSetKernelArg(kernel_bankconf, 0, sizeof(cl_mem), (void *) &data_device);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of bankconf kernel");

	status = clSetKernelArg(kernel_bankconf, 1, localMemSizeInBytes, NULL);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of bankconf kernel");
	
	//////////////////
	//  Run kernel! //
	//////////////////
	
	status = clEnqueueNDRangeKernel( zone.queues[0], kernel_bankconf, 2, NULL, gws_bankconf, lws_bankconf, 1, events, &(events[1]));
	clu_if_error_create_error_goto(status, &err, error, "bankconf kernel: execute");

	status = clWaitForEvents(1, &(events[1]));
	clu_if_error_create_error_goto(status, &err, error, "bankconf kernel: wait for events");

	//////////////////////////
	//  Show profiling info //
	//////////////////////////
	
	profcl_profile_stop(profile); 

	profcl_profile_add(profile, profcl_evinfo_get("Transfer matrix A to device", events[0], &status));
	clu_if_error_create_error_return(status, &err, "Add event to profile: Transfer matrix A to device");

	profcl_profile_add(profile, profcl_evinfo_get("Kernel execution (bankconf)", events[1], &status));
	clu_if_error_create_error_return(status, &err, "Add event to profile: Kernel execution (bankconf)");

	profcl_profile_aggregate(profile);

	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME);
	
	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error:
	fprintf(stderr, "Error %d: %s\n", err->code, err->message);
	g_error_free(err);
	if (zone.build_log) clu_build_log_print(&zone);

cleanup:
		
	/////////////////
	// Free stuff! //
	/////////////////
	
	// Free profile
	if (profile) profcl_profile_free(profile);
	
	// Release events
	if (events[0]) clReleaseEvent(events[0]);
	if (events[1]) clReleaseEvent(events[1]);
	
	// Release OpenCL kernels
	if (kernel_bankconf) clReleaseKernel(kernel_bankconf);
	
	// Release OpenCL memory objects
	if (data_device) clReleaseMemObject(data_device);

	// Free OpenCL zone
	clu_zone_free(&zone);

	// Free host resources
	if (data_host) free(data_host);
	
	return 0;

}

