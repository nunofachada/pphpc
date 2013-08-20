/** 
 * @file
 * @brief Test RNGs.
 */

#include "PredPreyCommon.h"

#define GWS 262144
#define LWS 256
#define SEED 0x5DEECE66DUL
#define RUNS 30

/* OpenCL kernel files */
const char* kernelFiles[] = {"pp/rng_test.cl"};

/**
 * @brief Main program.
 * 
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program 
 * terminates successfully, or another value of #pp_error_codes if an
 * error occurs.
 * */
int main(int argc, char **argv)
{

	/* Status var aux */
	int status = PP_SUCCESS;
	argc = argc;
	
	/* Test data structures. */
	size_t gws = GWS;
	size_t lws = LWS;
	cl_kernel init_rng = NULL, test_rng = NULL;
	cl_uint **result_host = NULL;
	cl_mem seeds = NULL, result_dev = NULL;
	cl_ulong mainSeed = SEED;
	FILE *fp;
	
	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_GPU, 1, 0, clu_menu_device_selector, NULL, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Build program. */
	status = clu_program_create(zone, kernelFiles, 1, argv[1], &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);

	/* Create kernels. */
	init_rng = clCreateKernel(zone->program, "initRng", &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel init_rng (OpenCL error %d)", status);
	test_rng = clCreateKernel(zone->program, "testRng", &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel test_rng (OpenCL error %d)", status);
	
	/* Create host buffer */
	result_host = (cl_uint**) malloc(sizeof(cl_uint*) * RUNS);
	for (int i = 0; i < RUNS; i++) {
		result_host[i] = (cl_uint*) malloc(sizeof(cl_uint) * GWS);
	}
	
	/* Create device buffer */
	seeds = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, GWS * sizeof(cl_ulong), NULL, &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer 1 (OpenCL error %d)", status);
	
	result_dev = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, GWS * sizeof(cl_int), NULL, &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer 2 (OpenCL error %d)", status);

	/*  Set fixed kernel arguments. */
	status = clSetKernelArg(init_rng, 0, sizeof(cl_ulong), (void*) &mainSeed);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 init (OpenCL error %d)", status);

	status = clSetKernelArg(init_rng, 1, sizeof(cl_mem), (void*) &seeds);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 init (OpenCL error %d)", status);

	status = clSetKernelArg(test_rng, 0, sizeof(cl_mem), (void*) &seeds);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 test (OpenCL error %d)", status);
	
	status = clSetKernelArg(test_rng, 1, sizeof(cl_mem), (void*) &result_dev);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 test (OpenCL error %d)", status);

	/* Init RNG. */
	status = clEnqueueNDRangeKernel(
		zone->queues[0], 
		init_rng, 
		1, 
		NULL, 
		&gws, 
		&lws, 
		0, 
		NULL, 
		NULL
	);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel init (OpenCL error %d)", status);

	/* Test! */
	for (int i = 0; i < RUNS; i++) {
		/* Run kernel. */
		status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			test_rng, 
			1, 
			NULL, 
			&gws, 
			&lws, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Kernel exec iter %d (OpenCL error %d)", i, status);
		/* Read data. */
		status = clEnqueueReadBuffer(
			zone->queues[0], 
			result_dev, 
			CL_TRUE, 
			0, 
			GWS * sizeof(cl_uint), 
			result_host[i], 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Read back results, iteration %d (OpenCL error %d)", i, status);
	}


	/* Output results to file */
	fp = fopen("rng_test.tsv", "w");
	for (int i = 0; i < RUNS; i++) {
		for (int j = 0; j < GWS; j++) {
			fprintf(fp, "%d\t", result_host[i][j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error_handler:
	/* Handle error. */
	pp_error_handle(err, status);

cleanup:

	/* Release OpenCL kernels */
	if (init_rng) clReleaseKernel(init_rng);
	if (test_rng) clReleaseKernel(test_rng);
	
	/* Release OpenCL memory objects */
	if (seeds) clReleaseMemObject(seeds);
	if (result_dev) clReleaseMemObject(result_dev);

	/* Release OpenCL zone (program, command queue, context) */
	clu_zone_free(zone);

	/* Free host resources */
	if (result_host) {
		for (int i = 0; i < RUNS; i++) {
			if (result_host[i]) free(result_host[i]);
		}
		free(result_host);
	}
	
	/* Bye bye. */
	return 0;
	
}
