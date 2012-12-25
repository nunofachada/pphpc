#include "../PredPreyCommon.h"

#define A_ROWS 50
#define A_COLS 10
#define B_ROWS 10
#define B_COLS 100

#define LWS_GPU_PREF_2D_X 8
#define LWS_GPU_PREF_2D_Y 8

// Global work sizes
size_t gws_matmult[2];

// Local work sizes
size_t lws_matmult[2];

// Kernels
;

// Main stuff
int main(int argc, char ** argv)
{

#ifdef CLPROFILER
	printf("Profiling is ON!\n");
#else
	printf("Profiling is OFF!\n");
#endif

	// Aux vars
	cl_int status;
	char msg[MAX_AUX_BUFF];
	
	// Timmings
	struct timeval time1, time0;
	
	// Get the required CL zone.
	CLZONE zone = getClZone("Matmult_kernels.cl", CL_DEVICE_TYPE_GPU, 1, 1);
	
	// Set RNG seed
	srand((unsigned)time(NULL));  
	
	// Events
	cl_event events[4];
	
	// Kernel
	cl_kernel kernel_matmult = clCreateKernel( zone.program, "matmult1", &status );
	if (status != CL_SUCCESS) { PrintErrorCreateKernel(status, "Matmult kernel"); exit(EXIT_FAILURE); }

	// Show kernel info - this should then influence the stuff above
	KERNEL_WORK_GROUP_INFO kwgi;
	getWorkGroupInfo(kernel_matmult, zone.device, &kwgi);
	printWorkGroupInfo(kwgi);
	
	////////////////////////////////////////
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	// Matrix A
	size_t sizeMatrixAInBytes = A_ROWS * A_COLS * sizeof(cl_int);
	cl_int *matrixA_host = (cl_int*) malloc(sizeMatrixAInBytes);
	for (unsigned int i = 0; i < A_ROWS * A_COLS; i++)
		matrixA_host[i] = (rand() % 200) - 100;
	
	// Matrix B
	size_t sizeMatrixBInBytes = B_ROWS * B_COLS * sizeof(cl_int);
	cl_int *matrixB_host = (cl_int*) malloc(sizeMatrixBInBytes);
	for (unsigned int i = 0; i < B_ROWS * B_COLS; i++)
		matrixB_host[i] = (rand() % 200) - 100;
	
	// Matrix C (result)
	size_t sizeMatrixCInBytes = A_ROWS * B_COLS * sizeof(cl_int);
	cl_int *matrixC_host = (cl_int*) malloc(sizeMatrixCInBytes);

	///////////////////////////
	// Create device buffers //
	///////////////////////////

	// Matrix A
	cl_mem matrixA_device = clCreateBuffer(zone.context, CL_MEM_READ_ONLY, sizeMatrixAInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "matrix A device"); return(-1); }
	
	// Matrix B
	cl_mem matrixB_device = clCreateBuffer(zone.context, CL_MEM_READ_ONLY, sizeMatrixBInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "matrix B device"); return(-1); }

	// Matrix C
	cl_mem matrixC_device = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY, sizeMatrixCInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "matrix C device"); return(-1); }
	
	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer (	zone.queues[0], matrixA_device, CL_TRUE, 0, sizeMatrixAInBytes, matrixA_host, 0, NULL, &(events[0]) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "matrix A device"); return(-1); }

	status = clEnqueueWriteBuffer (	zone.queues[0], matrixB_device, CL_TRUE, 0, sizeMatrixBInBytes, matrixB_host, 0, NULL, &(events[1]) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "matrix A device"); return(-1); }

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	status = clSetKernelArg(kernel_matmult, 0, sizeof(cl_mem), (void *) &matrixA_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of matmult kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_matmult, 1, sizeof(cl_mem), (void *) &matrixB_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of matmult kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_matmult, 2, sizeof(cl_mem), (void *) &matrixC_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of matmult kernel"); exit(EXIT_FAILURE); }
	
	cl_uint4 dims = { A_ROWS, A_COLS, B_ROWS, B_COLS };
	status = clSetKernelArg(kernel_matmult, 3, sizeof(cl_uint4), (void *) &dims);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of matmult kernel"); exit(EXIT_FAILURE); }
	
	//////////////////////////
	//  Determine worksizes //
	//////////////////////////
	
	lws_matmult[0] = LWS_GPU_PREF_2D_X;
	lws_matmult[1] = LWS_GPU_PREF_2D_Y;
	gws_matmult[0] = LWS_GPU_PREF_2D_X * ceil(((float) B_COLS) / LWS_GPU_PREF_2D_X);
	gws_matmult[1] = LWS_GPU_PREF_2D_Y * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_Y);
	
	printf("\n------------------------------------------------\n");
	printf("Local work size  : (%d, %d)\n", lws_matmult[0], lws_matmult[1]);
	printf("Global work size : (%d, %d)\n", gws_matmult[0], gws_matmult[1]);
	printf("------------------------------------------------\n\n");
	

	//////////////////
	//  Run kernel! //
	//////////////////
	
	status = clEnqueueNDRangeKernel( zone.queues[0], kernel_matmult, 2, NULL, gws_matmult, lws_matmult, 0, NULL, &(events[2]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueNDRangeKernel(status, "Matmult kernel"); return(-1); }

	status = clEnqueueReadBuffer ( zone.queues[0], matrixC_device, CL_TRUE, 0, sizeMatrixCInBytes, matrixC_host, 1, &(events[2]), &(events[3]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "Read matrix C"); return(-1); }

	status = clFinish(zone.queues[0]); 

	//////////////////////////
	//  Show profiling info //
	//////////////////////////
	cl_ulong startEv, finishEv, durationEvs[4], totalEv = 0;
	
	for (unsigned int i = 0; i < 4; i++) {
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
	printf("Transfer matrix B to device   : %fms (%3.4f%%)\n", durationEvs[1] * 1e-6, 100*((double) durationEvs[1])/((double) totalEv));
	// Event 2 - Perform multiplication
	printf("Kernel execution (matmult)    : %fms (%3.4f%%)\n", durationEvs[2] * 1e-6, 100*((double) durationEvs[2])/((double) totalEv));
	// Event 3 - transfer matrix C from device
	printf("Transfer matrix C from device : %fms (%3.4f%%)\n\n", durationEvs[3] * 1e-6, 100*((double) durationEvs[3])/((double) totalEv));
	
	
	// Release events
	for (unsigned int i = 0; i < 4; i++) {
		status = clReleaseEvent( events[i] );
		if (status != CL_SUCCESS) { sprintf(msg, "release event %d", i); PrintErrorReleaseEvent(status, msg); return(-1); }
	}

	////////////////////////
	// Do the same on CPU //
	////////////////////////

	// Start timer
	gettimeofday(&time0, NULL);

	// Multiply!
	cl_int matrixC_test[B_COLS][A_ROWS];
	for (unsigned int i = 0; i < B_COLS; i++) {
		for (unsigned j = 0; j < A_ROWS; j++) {
			matrixC_test[i][j] = 0;
			for (unsigned int k = 0; k < A_COLS; k++) {
				matrixC_test[i][j] += matrixA_host[]
			}
		}
	}

	// Get finishing time	
	gettimeofday(&time1, NULL);  

	/////////////////
	// Free stuff! //
	/////////////////
	
	// Release OpenCL kernels
	clReleaseKernel(kernel_matmult);
	
	// Release OpenCL memory objects
	clReleaseMemObject(matrixA_device);
	clReleaseMemObject(matrixB_device);
	clReleaseMemObject(matrixC_device);

	// Release OpenCL zone (program, command queue, context)
	destroyClZone(zone);

	// Free host resources
	free(matrixA_host);
	free(matrixB_host);
	free(matrixC_host);
	
	return 0;

}
