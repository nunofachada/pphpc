#include "../PredPreyCommon.h"
#include <omp.h>

#define A_ROWS 11137
#define A_COLS 180
#define B_ROWS A_COLS
#define B_COLS 11543

#define LWS_GPU_PREF_2D_X 32
#define LWS_GPU_PREF_2D_Y 32

#define RANGE_MATRIX 100

#define KERNEL_ID 3

#define DEBUG 0

typedef struct matDims {
	uint rowsA;
	uint colsA;
	uint rowsB;
	uint colsB;
} MAT_DIMS;


// Main stuff
int main(int argc, char *argv[])
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
	
	// Global work sizes
	size_t gws_matmult[2];

	// Local work sizes
	size_t lws_matmult[2];

	// Events
	cl_event events[4];
	
	// Kernel
	char * kernelName = (char*) malloc(9 * sizeof(char));
	strcpy(kernelName, "matmult?");
	kernelName[7] = KERNEL_ID + '0';
	cl_kernel kernel_matmult = clCreateKernel( zone.program, kernelName, &status );
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
		matrixA_host[i] = (rand() % RANGE_MATRIX) - RANGE_MATRIX / 2;
	
	// Matrix B
	size_t sizeMatrixBInBytes = B_ROWS * B_COLS * sizeof(cl_int);
	cl_int *matrixB_host = (cl_int*) malloc(sizeMatrixBInBytes);
	for (unsigned int i = 0; i < B_ROWS * B_COLS; i++)
		matrixB_host[i] = (rand() % RANGE_MATRIX) - RANGE_MATRIX / 2;
	
	// Matrix C (result)
	size_t sizeMatrixCInBytes = B_COLS * A_ROWS * sizeof(cl_int);
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

	//////////////////////////
	//  Determine worksizes //
	//////////////////////////
	
	lws_matmult[0] = LWS_GPU_PREF_2D_X;
	lws_matmult[1] = LWS_GPU_PREF_2D_Y;
	gws_matmult[0] = LWS_GPU_PREF_2D_X * ceil(((float) B_COLS) / LWS_GPU_PREF_2D_X);
	gws_matmult[1] = LWS_GPU_PREF_2D_Y * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_Y);
	
	printf("\n------------------------------------------------\n");
	printf("Local work size  : (%zu, %zu)\n", lws_matmult[0], lws_matmult[1]);
	printf("Global work size : (%zu, %zu)\n", gws_matmult[0], gws_matmult[1]);
	printf("------------------------------------------------\n\n");
	
	/////////////////////////////////////////
	// Determine and print required memory //
	/////////////////////////////////////////

	size_t globalMemSizeInBytes = sizeof(cl_int) * (A_ROWS * A_COLS + B_ROWS * B_COLS + A_ROWS * B_COLS);
	printf("\nGlobal memory required        : %zu bytes (%zu Kb = %zu Mb)", globalMemSizeInBytes, globalMemSizeInBytes / 1024, globalMemSizeInBytes / 1024 / 1024);
	size_t localMemSizeAInBytes = 0;
	size_t localMemSizeBInBytes = 0;
	
	if (KERNEL_ID >= 2)
			localMemSizeAInBytes = A_COLS * lws_matmult[1] * sizeof(cl_int);
	if (KERNEL_ID >= 3)
			localMemSizeBInBytes = lws_matmult[0] * B_ROWS * sizeof(cl_int);
	
	printf("\nLocal memory required         : %zu bytes (%zu Kb)\n\n", localMemSizeAInBytes + localMemSizeBInBytes, (localMemSizeAInBytes + localMemSizeBInBytes) / 1024);

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	status = clSetKernelArg(kernel_matmult, 0, sizeof(cl_mem), (void *) &matrixA_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of matmult kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_matmult, 1, sizeof(cl_mem), (void *) &matrixB_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of matmult kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_matmult, 2, sizeof(cl_mem), (void *) &matrixC_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of matmult kernel"); exit(EXIT_FAILURE); }
	
	MAT_DIMS dims = { .rowsA = A_ROWS, .colsA = A_COLS, .rowsB = B_ROWS, .colsB = B_COLS };
	status = clSetKernelArg(kernel_matmult, 3, sizeof(MAT_DIMS), (void *) &dims);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of matmult kernel"); exit(EXIT_FAILURE); }
	
	if (KERNEL_ID >= 2) {
		status = clSetKernelArg(kernel_matmult, 4, localMemSizeAInBytes, NULL);
		if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of matmult kernel"); exit(EXIT_FAILURE); }
	}
	if (KERNEL_ID >= 3) {
		status = clSetKernelArg(kernel_matmult, 5, localMemSizeBInBytes, NULL);
		if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 5 of matmult kernel"); exit(EXIT_FAILURE); }
	}
	

	//////////////////
	//  Run kernel! //
	//////////////////
	
	status = clEnqueueNDRangeKernel( zone.queues[0], kernel_matmult, 2, NULL, gws_matmult, lws_matmult, 0, NULL, &(events[2]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueNDRangeKernel(status, "Matmult kernel"); exit(EXIT_FAILURE); }

	/////////////////////////////
	//  Get result from device //
	/////////////////////////////

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
	// Total GPU time
	printf("Total GPU time                : %fms\n\n", (double) totalEv * 1e-6);
	
	
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
	int *matrixC_test = (int*) malloc(B_COLS * A_ROWS * sizeof(int));
	#pragma omp parallel for
	for (unsigned int col = 0; col < B_COLS; col++) {
		for (unsigned row = 0; row < A_ROWS; row++) {
			matrixC_test[row * B_COLS + col] = 0;
			for (unsigned int i = 0; i < A_COLS; i++) {
				matrixC_test[row * B_COLS + col] += matrixA_host[row * A_COLS + i] * matrixB_host[i * B_COLS + col];
			}
		}
	}
	
	// Get finishing time	
	gettimeofday(&time1, NULL);  

	// Print CPU timmings
	double dt = time1.tv_sec - time0.tv_sec;
	if (time1.tv_usec >= time0.tv_usec)
		dt = dt + (time1.tv_usec - time0.tv_usec) * 1e-6;
	else
		dt = (dt-1) + (1e6 + time1.tv_usec - time0.tv_usec) * 1e-6;
	printf("Total CPU Time                : %fms\n\n", dt * 1000);
	printf("SpeedUp                       : %fx\n\n", (dt * 1000) / (totalEv * 1e-6));
	
	// Check for correctness
	int error = 0;
	unsigned int sizeC = B_COLS * A_ROWS;
	for (unsigned int index = 0; index < sizeC; index++) {
		error += matrixC_host[index] - matrixC_test[index];
	}
	printf("Error (GPU-CPU)               : %d\n\n", error);


	if (DEBUG) {
		printf("\nMatrix A:\n");
		for (unsigned int i = 0; i < A_ROWS; i++) {
			printf("|\t");
			for (unsigned j = 0; j < A_COLS; j++) {
				printf("%d\t", matrixA_host[A_COLS * i + j]);
			}
			printf("|\n");
		}

		printf("\nMatrix B:\n");
		for (unsigned int i = 0; i < B_ROWS; i++) {
			printf("|\t");
			for (unsigned j = 0; j < B_COLS; j++) {
				printf("%d\t", matrixB_host[B_COLS * i + j]);
			}
			printf("|\n");
		}

		printf("\nGPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			printf("|\t");
			for (unsigned int col = 0; col < B_COLS; col++) {
				printf("%d\t", matrixC_host[B_COLS * row + col]);
			}
			printf("|\n");
		}

		printf("\nCPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			printf("|\t");
			for (unsigned int col = 0; col < B_COLS; col++) {
				printf("%d\t", matrixC_test[B_COLS * row + col]);
			}
			printf("|\n");
		}
	}

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
	free(matrixC_test);
	
	return 0;

}
