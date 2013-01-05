

#include "../PredPreyCommon.h"
#include <omp.h>

#define WS_X 1024
#define WS_Y 1024

#define LWS_GPU_PREF_2D_X 32
#define LWS_GPU_PREF_2D_Y 32

// Main stuff
int main(int argc, char *argv[])
{

	// Aux vars
	cl_int status;
	char msg[MAX_AUX_BUFF];
	
	// Get the required CL zone.
	CLZONE zone = getClZone("BankConflictTest_kernels.cl", CL_DEVICE_TYPE_GPU, 1, 1);
	
	// Global work sizes
	size_t gws_bankconf[2];

	// Local work sizes
	size_t lws_bankconf[2];

	// Events
	cl_event events[2];
	
	// Kernel
	cl_kernel kernel_bankconf = clCreateKernel( zone.program, "bankconf", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "bankconf kernel"); exit(EXIT_FAILURE); }

	////////////////////////////////////////	
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	// Data in host
	size_t sizeDataInBytes = WS_X * WS_Y * sizeof(cl_int);
	cl_int *data_host = (cl_int*) malloc(sizeDataInBytes);
	for (unsigned int i = 0; i < WS_Y * WS_X; i++)
			data_host[i] = i;

	///////////////////////////
	// Create device buffers //
	///////////////////////////

	// Data in device
	cl_mem data_device = clCreateBuffer(zone.context, CL_MEM_READ_WRITE, sizeDataInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "data device"); return(-1); }
	
	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer (	zone.queues[0], data_device, CL_TRUE, 0, sizeDataInBytes, data_host, 0, NULL, &(events[0]) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "data device"); return(-1); }

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
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of bankconf kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_bankconf, 1, localMemSizeInBytes, NULL);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of bankconf kernel"); exit(EXIT_FAILURE); }
	

	//////////////////
	//  Run kernel! //
	//////////////////
	
	status = clEnqueueNDRangeKernel( zone.queues[0], kernel_bankconf, 2, NULL, gws_bankconf, lws_bankconf, 1, events, &(events[1]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueNDRangeKernel(status, "bankconf kernel"); exit(EXIT_FAILURE); }

	//////////////////////////
	//  Show profiling info //
	//////////////////////////
	
	status = clWaitForEvents(1, &(events[1]));
	if (status != CL_SUCCESS) { PrintErrorWaitForEvents(status, "bankconf kernel"); exit(EXIT_FAILURE); }
	
	cl_ulong startEv, finishEv, durationEvs[2], totalEv = 0;
	
	for (unsigned int i = 0; i < 2; i++) {
		status = clGetEventProfilingInfo (events[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startEv, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "event profiling start");  return(-1); }
		status = clGetEventProfilingInfo (events[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &finishEv, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "event profiling finish");  return(-1); }
		durationEvs[i] = finishEv - startEv;
		totalEv += durationEvs[i];
	}
	
	
	// Event 0 - transfer matrix A to device
	printf("Transfer matrix A to device   : %fms (%3.4f%%)\n", durationEvs[0] * 1e-6, 100*((double) durationEvs[0])/((double) totalEv));
	// Event 1 - transfer matrix B to device
	printf("Kernel execution (bankconf)   : %fms (%3.4f%%)\n", durationEvs[1] * 1e-6, 100*((double) durationEvs[1])/((double) totalEv));
	// Total GPU time
	printf("Total GPU time                : %fms\n\n", (double) totalEv * 1e-6);
	
	
	/////////////////
	// Free stuff! //
	/////////////////
	
	// Release events
	for (unsigned int i = 0; i < 2; i++) {
		status = clReleaseEvent( events[i] );
		if (status != CL_SUCCESS) { sprintf(msg, "release event %d", i); PrintErrorReleaseEvent(status, msg); return(-1); }
	}

	// Release OpenCL kernels
	clReleaseKernel(kernel_bankconf);
	
	// Release OpenCL memory objects
	clReleaseMemObject(data_device);

	// Release OpenCL zone (program, command queue, context)
	destroyClZone(zone);

	// Free host resources
	free(data_host);
	
	return 0;

}

