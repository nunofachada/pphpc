
#include "../PredPreyCommon.h"
#include <omp.h>

#define A_ROWS 2400
#define A_COLS 170

#define LWS_GPU_PREF_2D_X 32
#define LWS_GPU_PREF_2D_Y 32

#define RANGE_MATRIX 4

#define KERNEL_ID 2

#define DEBUG 1

typedef struct matDims {
	cl_uint rowsA;
	cl_uint colsA;
} MAT_DIMS;


// Main stuff
int main(int argc, char *argv[])
{

	// Aux vars
	cl_int status;
	char msg[MAX_AUX_BUFF];
	
	// Timmings
	struct timeval time1, time0;
	
	// Get the required CL zone.
	CLZONE zone = getClZone("MatmultT_kernels.cl", CL_DEVICE_TYPE_GPU, 1, 1);
	
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

	////////////////////////////////////////	
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	// Matrix A
	size_t sizeMatrixAInBytes = A_ROWS * A_COLS * sizeof(cl_int);
	cl_int *matrixA_host = (cl_int*) malloc(sizeMatrixAInBytes);
	for (unsigned int i = 0; i < A_ROWS * A_COLS; i++)
		matrixA_host[i] = (rand() % RANGE_MATRIX) - RANGE_MATRIX / 2;
	
	// Matrix C (result)
	size_t sizeMatrixCInBytes = A_ROWS * A_ROWS * sizeof(cl_int);
	cl_int *matrixC_host = (cl_int*) malloc(sizeMatrixCInBytes);

	///////////////////////////
	// Create device buffers //
	///////////////////////////

	// Matrix A
	cl_mem matrixA_device = clCreateBuffer(zone.context, CL_MEM_READ_ONLY, sizeMatrixAInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "matrix A device"); return(-1); }
	
	// Matrix C
	cl_mem matrixC_device = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY, sizeMatrixCInBytes, NULL, &status );
	if (status != CL_SUCCESS) { PrintErrorCreateBuffer(status, "matrix C device"); return(-1); }
	
	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer (	zone.queues[0], matrixA_device, CL_TRUE, 0, sizeMatrixAInBytes, matrixA_host, 0, NULL, &(events[0]) );
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "matrix A device"); return(-1); }

	//////////////////////////
	//  Determine worksizes //
	//////////////////////////
	
	lws_matmult[0] = LWS_GPU_PREF_2D_X;
	lws_matmult[1] = LWS_GPU_PREF_2D_Y;
	gws_matmult[0] = LWS_GPU_PREF_2D_X * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_X);
	gws_matmult[1] = LWS_GPU_PREF_2D_Y * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_Y);
	
	printf("\n------------------------------------------------\n");
	printf("Local work size  : (%zu, %zu)\n", lws_matmult[0], lws_matmult[1]);
	printf("Global work size : (%zu, %zu)\n", gws_matmult[0], gws_matmult[1]);
	printf("------------------------------------------------\n\n");
	
	/////////////////////////////////////////
	// Determine and print required memory //
	/////////////////////////////////////////

	size_t globalMemSizeInBytes = sizeMatrixAInBytes + sizeMatrixCInBytes;
	printf("\nGlobal memory required        : %zu bytes (%zu Kb = %zu Mb)", globalMemSizeInBytes, globalMemSizeInBytes / 1024, globalMemSizeInBytes / 1024 / 1024);
	size_t localMemSizeAInBytes = 0, localMemSizeATInBytes = 0;
	
	if (KERNEL_ID == 2) {
		localMemSizeAInBytes = lws_matmult[1] * A_COLS * sizeof(cl_int);
		localMemSizeATInBytes = lws_matmult[0] * A_COLS * sizeof(cl_int);
	}
	
	printf("\nLocal memory required         : %zu bytes (%zu Kb)\n\n", localMemSizeAInBytes + localMemSizeATInBytes, (localMemSizeAInBytes + localMemSizeATInBytes) / 1024);

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	status = clSetKernelArg(kernel_matmult, 0, sizeof(cl_mem), (void *) &matrixA_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 0 of matmult kernel"); exit(EXIT_FAILURE); }

	status = clSetKernelArg(kernel_matmult, 1, sizeof(cl_mem), (void *) &matrixC_device);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 1 of matmult kernel"); exit(EXIT_FAILURE); }
	
	MAT_DIMS dims = { .rowsA = A_ROWS, .colsA = A_COLS };
	status = clSetKernelArg(kernel_matmult, 2, sizeof(MAT_DIMS), (void *) &dims);
	if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 2 of matmult kernel"); exit(EXIT_FAILURE); }
	
	if (KERNEL_ID == 2) {
		status = clSetKernelArg(kernel_matmult, 3, localMemSizeAInBytes, NULL);
		if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 3 of matmult kernel"); exit(EXIT_FAILURE); }

		status = clSetKernelArg(kernel_matmult, 4, localMemSizeATInBytes, NULL);
		if (status != CL_SUCCESS) { PrintErrorSetKernelArg(status, "Arg 4 of matmult kernel"); exit(EXIT_FAILURE); }
	}
	

	//////////////////
	//  Run kernel! //
	//////////////////
	
	status = clEnqueueNDRangeKernel( zone.queues[0], kernel_matmult, 2, NULL, gws_matmult, lws_matmult, 0, NULL, &(events[1]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueNDRangeKernel(status, "Matmult kernel"); exit(EXIT_FAILURE); }

	/////////////////////////////
	//  Get result from device //
	/////////////////////////////

	status = clEnqueueReadBuffer ( zone.queues[0], matrixC_device, CL_TRUE, 0, sizeMatrixCInBytes, matrixC_host, 1, &(events[1]), &(events[2]));
	if (status != CL_SUCCESS) { PrintErrorEnqueueReadWriteBuffer(status, "Read matrix C"); return(-1); }

	status = clFinish(zone.queues[0]); 

	//////////////////////////
	//  Show profiling info //
	//////////////////////////
	cl_ulong startEv, finishEv, durationEvs[3], totalEv = 0;
	
	for (unsigned int i = 0; i < 3; i++) {
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
	printf("Kernel execution (matmult)    : %fms (%3.4f%%)\n", durationEvs[1] * 1e-6, 100*((double) durationEvs[1])/((double) totalEv));
	// Event 3 - transfer matrix C from device
	printf("Transfer matrix C from device : %fms (%3.4f%%)\n\n", durationEvs[2] * 1e-6, 100*((double) durationEvs[2])/((double) totalEv));
	// Total GPU time
	printf("Total GPU time                : %fms\n\n", (double) totalEv * 1e-6);
	
	
	// Release events
	for (unsigned int i = 0; i < 3; i++) {
		status = clReleaseEvent( events[i] );
		if (status != CL_SUCCESS) { sprintf(msg, "release event %d", i); PrintErrorReleaseEvent(status, msg); return(-1); }
	}

	////////////////////////
	// Do the same on CPU //
	////////////////////////

	// Start timer
	gettimeofday(&time0, NULL);

	// Multiply!
	int *matrixC_test = (int*) malloc(A_ROWS * A_ROWS * sizeof(int));
	#pragma omp parallel for
	for (unsigned int row = 0; row < A_ROWS; row++) {
		for (unsigned int col = 0; col < A_ROWS; col++) {
			matrixC_test[row * A_ROWS + col] = 0;
			for (unsigned int i = 0; i < A_COLS; i++) {
				matrixC_test[row * A_ROWS + col] += matrixA_host[row * A_COLS + i] * matrixA_host[col * A_COLS + i];
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
	unsigned int sizeC = A_ROWS * A_ROWS;
	for (unsigned int index = 0; index < sizeC; index++) {
		error += matrixC_host[index] - matrixC_test[index];
	}
	printf("Error (GPU-CPU)               : %d\n\n", error);


	if (DEBUG) {
		FILE *fpA = fopen("A.tsv", "w");
		FILE *fpC_GPU = fopen("CGPU.tsv", "w");
		FILE *fpC_CPU = fopen("CCPU.tsv", "w");
		//printf("\nMatrix A:\n");
		for (unsigned int i = 0; i < A_ROWS; i++) {
			//printf("|\t");
			for (unsigned j = 0; j < A_COLS; j++) {
				//printf("%d\t", matrixA_host[A_COLS * i + j]);
				fprintf(fpA, "%d\t", matrixA_host[A_COLS * i + j]);
			}
			//printf("|\n");
			fprintf(fpA, "\n");
		}

		//printf("\nGPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			//printf("|\t");
			for (unsigned int col = 0; col < A_ROWS; col++) {
				//printf("%d\t", matrixC_host[A_ROWS * row + col]);
				fprintf(fpC_GPU, "%d\t", matrixC_host[A_ROWS * row + col]);
			}
			//printf("|\n");
			fprintf(fpC_GPU, "\n");
		}

		//printf("\nCPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			//printf("|\t");
			for (unsigned int col = 0; col < A_ROWS; col++) {
				//printf("%d\t", matrixC_test[A_ROWS * row + col]);
				fprintf(fpC_CPU, "%d\t", matrixC_test[A_ROWS * row + col]);
			}
			fprintf(fpC_CPU, "\n");
			//printf("|\n");
		}
		fclose(fpA);
		fclose(fpC_GPU);
		fclose(fpC_CPU);
	}

	/////////////////
	// Free stuff! //
	/////////////////
	
	// Release OpenCL kernels
	clReleaseKernel(kernel_matmult);
	
	// Release OpenCL memory objects
	clReleaseMemObject(matrixA_device);
	clReleaseMemObject(matrixC_device);

	// Release OpenCL zone (program, command queue, context)
	destroyClZone(zone);

	// Free host resources
	free(matrixA_host);
	free(matrixC_host);
	free(matrixC_test);
	
	return 0;

}
