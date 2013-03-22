#include <math.h>
#include <glib.h>
#include <omp.h>
#include "../utils/Profiler.h"
#include "../utils/clutils.h"

#define A_ROWS 5134
#define A_COLS 97

#define LWS_GPU_PREF_2D_X 16
#define LWS_GPU_PREF_2D_Y 16

#define RANGE_MATRIX 4

#define KERNEL_ID 1

#define DEBUG 0

typedef struct matDims {
	cl_uint rowsA;
	cl_uint colsA;
} MAT_DIMS;


// Main stuff
int main(int argc, char *argv[])
{

	/* Aux vars */
	cl_int status;
	
	/* Error management. */
	GError *err = NULL;
	
	/* Global work sizes */
	size_t gws_matmult[2];

	/* Local work sizes */
	size_t lws_matmult[2];

	/* Events */
	cl_event events[3] = {NULL, NULL, NULL};

	/* Profiling / Timmings */
	ProfCLProfile* profile = profcl_profile_new();
	GTimer* cpuTimer = NULL;
	
	/* Kernel file */
	const char* kernelFiles[] = {"MatmultT_kernels.cl"};	
		
	/* Get the required CL zone. */
	CLUZone zone;
	status = clu_zone_new(&zone, CL_DEVICE_TYPE_GPU, 1, CL_QUEUE_PROFILING_ENABLE, &err);
	clu_if_error_goto(status, err, error);
	
	/* Build program. */
	status = clu_program_create(&zone, kernelFiles, 1, NULL, &err);
	clu_if_error_goto(status, err, error);
	
	/* Set RNG seed */
	srand((unsigned)time(NULL));  

	/* Kernel */
	char * kernelName = (char*) malloc(9 * sizeof(char));
	strcpy(kernelName, "matmult?");
	kernelName[7] = KERNEL_ID + '0';
	cl_kernel kernel_matmult = clCreateKernel( zone.program, kernelName, &status );
	clu_if_error_create_error_goto(status, &err, error, "MatmultT kernel: create");

	////////////////////////////////////////	
	// Create and initialize host buffers //
	////////////////////////////////////////
	
	/* Matrix A */
	size_t sizeMatrixAInBytes = A_ROWS * A_COLS * sizeof(cl_int);
	cl_int *matrixA_host = (cl_int*) malloc(sizeMatrixAInBytes);
	for (unsigned int i = 0; i < A_ROWS * A_COLS; i++)
		matrixA_host[i] = (rand() % RANGE_MATRIX) - RANGE_MATRIX / 2;
	
	/* Matrix C (result) */
	size_t sizeMatrixCInBytes = A_ROWS * A_ROWS * sizeof(cl_int);
	cl_int *matrixC_host = (cl_int*) malloc(sizeMatrixCInBytes);

	///////////////////////////
	// Create device buffers //
	///////////////////////////

	/* Matrix A */
	cl_mem matrixA_device = clCreateBuffer(zone.context, CL_MEM_READ_ONLY, sizeMatrixAInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Matrix A device: create");
	
	/* Matrix C */
	cl_mem matrixC_device = clCreateBuffer(zone.context, CL_MEM_WRITE_ONLY, sizeMatrixCInBytes, NULL, &status );
	clu_if_error_create_error_goto(status, &err, error, "Matrix C device: create");
	
	///////////////////////////////
	// Initialize device buffers //
	///////////////////////////////
	
	status = clEnqueueWriteBuffer (	zone.queues[0], matrixA_device, CL_TRUE, 0, sizeMatrixAInBytes, matrixA_host, 0, NULL, &(events[0]) );
	clu_if_error_create_error_goto(status, &err, error, "Matrix A device: write");

	//////////////////////////
	//  Determine worksizes //
	//////////////////////////
	
	lws_matmult[0] = LWS_GPU_PREF_2D_X;
	lws_matmult[1] = LWS_GPU_PREF_2D_Y;
	gws_matmult[0] = LWS_GPU_PREF_2D_X * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_X);
	gws_matmult[1] = LWS_GPU_PREF_2D_Y * ceil(((float) A_ROWS) / LWS_GPU_PREF_2D_Y);
	
	printf("\n------------------------------------------------\n");
	printf("Local work size  : (%lu, %lu)\n", (unsigned long) lws_matmult[0], (unsigned long) lws_matmult[1]);
	printf("Global work size : (%lu, %lu)\n", (unsigned long) gws_matmult[0], (unsigned long) gws_matmult[1]);
	printf("------------------------------------------------\n\n");
	
	/////////////////////////////////////////
	// Determine and print required memory //
	/////////////////////////////////////////

	size_t globalMemSizeInBytes = sizeMatrixAInBytes + sizeMatrixCInBytes;
	printf("\nGlobal memory required        : %lu bytes (%lu Kb = %lu Mb)", 
		(unsigned long) globalMemSizeInBytes, 
		(unsigned long) (globalMemSizeInBytes / 1024), 
		(unsigned long) (globalMemSizeInBytes / 1024 / 1024));
	size_t localMemSizeAInBytes = 0, localMemSizeATInBytes = 0;
	
	if (KERNEL_ID == 2) {
		localMemSizeAInBytes = lws_matmult[1] * A_COLS * sizeof(cl_int);
		localMemSizeATInBytes = lws_matmult[0] * A_COLS * sizeof(cl_int);
	}
	
	printf("\nLocal memory required         : %lu bytes (%lu Kb)\n\n", 
		(unsigned long) (localMemSizeAInBytes + localMemSizeATInBytes), 
		(unsigned long) ((localMemSizeAInBytes + localMemSizeATInBytes) / 1024));

	/////////////////////////////////
	//  Set fixed kernel arguments //
	/////////////////////////////////

	status = clSetKernelArg(kernel_matmult, 0, sizeof(cl_mem), (void *) &matrixA_device);
	clu_if_error_create_error_goto(status, &err, error, "Arg 0 of matmult kernel");

	status = clSetKernelArg(kernel_matmult, 1, sizeof(cl_mem), (void *) &matrixC_device);
	clu_if_error_create_error_goto(status, &err, error, "Arg 1 of matmult kernel");
	
	MAT_DIMS dims = { .rowsA = A_ROWS, .colsA = A_COLS };
	status = clSetKernelArg(kernel_matmult, 2, sizeof(MAT_DIMS), (void *) &dims);
	clu_if_error_create_error_goto(status, &err, error, "Arg 2 of matmult kernel");
	
	if (KERNEL_ID == 2) {
		status = clSetKernelArg(kernel_matmult, 3, localMemSizeAInBytes, NULL);
		clu_if_error_create_error_goto(status, &err, error, "Arg 3 of matmult kernel");

		status = clSetKernelArg(kernel_matmult, 4, localMemSizeATInBytes, NULL);
		clu_if_error_create_error_goto(status, &err, error, "Arg 4 of matmult kernel");
	}
	

	//////////////////
	//  Run kernel! //
	//////////////////
	
	/* Start basic timming / profiling. */
	profcl_profile_start(profile);
	
	/* Run kernel */ 
	status = clEnqueueNDRangeKernel(zone.queues[0], kernel_matmult, 2, NULL, gws_matmult, lws_matmult, 0, NULL, &(events[1]));
	clu_if_error_create_error_goto(status, &err, error, "Matmult kernel: execute");

	/////////////////////////////
	//  Get result from device //
	/////////////////////////////

	status = clEnqueueReadBuffer (zone.queues[0], matrixC_device, CL_TRUE, 0, sizeMatrixCInBytes, matrixC_host, 1, &(events[1]), &(events[2]));
	clu_if_error_create_error_goto(status, &err, error, "Matrix C device: read");

	status = clFinish(zone.queues[0]); 
	clu_if_error_create_error_goto(status, &err, error, "Waiting for queue to finish");

	//////////////////////////
	//  Show profiling info //
	//////////////////////////
	profcl_profile_stop(profile); 

	profcl_profile_add(profile, profcl_evinfo_get("Transfer matrix A to device", events[0], &status));
	clu_if_error_create_error_return(status, &err, "Add event to profile: Transfer matrix A to device");

	profcl_profile_add(profile, profcl_evinfo_get("Kernel execution (Matmult)", events[1], &status));
	clu_if_error_create_error_return(status, &err, "Add event to profile: Kernel execution (Matmult)");

	profcl_profile_add(profile, profcl_evinfo_get("Transfer matrix C from device", events[2], &status));
	clu_if_error_create_error_return(status, &err, "Add event to profile: Transfer matrix C from device");

	profcl_profile_aggregate(profile);

	profcl_print_info(profile, PROFCL_AGGEVDATA_SORT_TIME);
	
	////////////////////////
	// Do the same on CPU //
	////////////////////////

	/*  Start timer */
	cpuTimer = g_timer_new();
	
	/* Multiply! */
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
	g_timer_stop(cpuTimer); 

	// Print CPU timmings
	printf("Total CPU Time                : %fs\n\n", g_timer_elapsed(cpuTimer, NULL));
	printf("SpeedUp                       : %fx\n\n", g_timer_elapsed(cpuTimer, NULL) / g_timer_elapsed(profile->timer, NULL));
	
	// Check for correctness
	int error = 0;
	unsigned int sizeC = A_ROWS * A_ROWS;
	for (unsigned int index = 0; index < sizeC; index++) {
		error += matrixC_host[index] - matrixC_test[index];
	}
	printf("Error (GPU-CPU)               : %d\n\n", error);


	if (DEBUG) {

		FILE *fpA, *fpC_GPU, *fpC_CPU;
		if (DEBUG == 2) {
			fpA = fopen("A.tsv", "w");
			fpC_GPU = fopen("CGPU.tsv", "w");
			fpC_CPU = fopen("CCPU.tsv", "w");
		}

		if (DEBUG == 1) printf("\nMatrix A:\n");
		for (unsigned int i = 0; i < A_ROWS; i++) {
			if (DEBUG == 1) printf("|\t");
			for (unsigned j = 0; j < A_COLS; j++) {
				DEBUG == 1 ? printf("%d\t", matrixA_host[A_COLS * i + j]) : fprintf(fpA, "%d\t", matrixA_host[A_COLS * i + j]);
			}
			DEBUG == 1 ? printf("|\n") : fprintf(fpA, "\n");
		}

		if (DEBUG == 1) printf("\nGPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			if (DEBUG == 1) printf("|\t");
			for (unsigned int col = 0; col < A_ROWS; col++) {
				DEBUG == 1 ? printf("%d\t", matrixC_host[A_ROWS * row + col]) : fprintf(fpC_GPU, "%d\t", matrixC_host[A_ROWS * row + col]);
			}
			DEBUG == 1 ? printf("|\n") : fprintf(fpC_GPU, "\n");
		}

		if (DEBUG == 1) printf("\nCPU matrix C:\n");
		for (unsigned row = 0; row < A_ROWS; row++) {
			if (DEBUG == 1) printf("|\t");
			for (unsigned int col = 0; col < A_ROWS; col++) {
				DEBUG == 1 ? printf("%d\t", matrixC_test[A_ROWS * row + col]) : fprintf(fpC_CPU, "%d\t", matrixC_test[A_ROWS * row + col]);
			}
			DEBUG == 1 ? printf("|\n") : fprintf(fpC_CPU, "\n");;
		}
		if (DEBUG == 2) {
			fclose(fpA);
			fclose(fpC_GPU);
			fclose(fpC_CPU);
		}
	}
	
	/* If we get here, no need for error treatment, jump to cleanup. */
	goto cleanup;
	
error:
	fprintf(stderr, "Error %d: %s\n", err->code, err->message);
	g_error_free(err);
	if (zone.build_log) clu_build_log_print(&zone);

cleanup:
	/////////////////
	// Free stuff! //
	/////////////////
	
	// Free profile and cpu timer
	if (profile) profcl_profile_free(profile);
	if (cpuTimer) g_timer_destroy(cpuTimer);
	
	// Release events
	for (unsigned int i = 0; i < 3; i++) {
		if (events[i]) status = clReleaseEvent(events[i]);
	}

	// Release OpenCL kernels
	if (kernel_matmult) clReleaseKernel(kernel_matmult);
	
	// Release OpenCL memory objects
	if (matrixA_device) clReleaseMemObject(matrixA_device);
	if (matrixC_device) clReleaseMemObject(matrixC_device);

	/* Free OpenCL zone */
	clu_zone_free(&zone);

	// Free host resources
	free(matrixA_host);
	free(matrixC_host);
	free(matrixC_test);
	
	return 0;

}
