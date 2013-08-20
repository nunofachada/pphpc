/** 
 * @file
 * @brief Test RNGs.
 */

#include "PredPreyCommon.h"

#define RNGT_OUTPUT "out"
#define RNGT_RNG "lcg" 
#define RNGT_GWS 262144
#define RNGT_LWS 256
#define RNGT_RUNS 50
#define RNGT_BITS 32

/** A description of the program. */
#define PPTRNG_DESCRIPTION "Test RNGs"

/* Command line arguments and respective default values. */
static char *rng = NULL;
static char *output = NULL;
static size_t gws = RNGT_GWS;
static size_t lws = RNGT_LWS;
static unsigned int runs = RNGT_RUNS;
static int dev_idx = -1;
static guint32 rng_seed = PP_DEFAULT_SEED;
static gboolean gid_seed = FALSE;
static unsigned char bits = RNGT_BITS;

/* Valid command line options. */
static GOptionEntry entries[] = {
	{"rng",          'r', 0, G_OPTION_ARG_STRING,   &rng,           "lcg (default), xorshift or mcw64x",                                           "RNG"},
	{"output",       'o', 0, G_OPTION_ARG_FILENAME, &output,        "Output file w/o extension (default is " RNGT_OUTPUT ")",                      "FILENAME"},
	{"globalsize",   'g', 0, G_OPTION_ARG_INT,      &gws,           "Global work size (default is " STR(RNGT_GWS) ")",                             "SIZE"},
	{"localsize",    'l', 0, G_OPTION_ARG_INT,      &lws,           "Local work size (default is " STR(RNGT_LWS) ")",                              "SIZE"},
	{"runs",         'n', 0, G_OPTION_ARG_INT,      &runs,          "Number of random numbers per workitem (default is " STR(RNGT_RUNS) ")",       "SIZE"},
	{"device",       'd', 0, G_OPTION_ARG_INT,      &dev_idx,       "Device index",                                                                "INDEX"},
	{"rng-seed",     's', 0, G_OPTION_ARG_INT,      &rng_seed,      "Seed for random number generator (default is " STR(RNGT_SEED) ")",            "SEED"},
	{"use-gid-seed", 'u', 0, G_OPTION_ARG_NONE,     &gid_seed,      "Use GID-based workitem seeds instead of MT derived seeds from host.",         NULL},
	{"bits",         'b', 0, G_OPTION_ARG_INT,      &bits,          "Number of bits in unsigned integers to produce (default " STR(RNGT_BITS) ")", NULL},
	{G_OPTION_REMAINING,  0, 0, G_OPTION_ARG_CALLBACK, pp_args_fail, NULL,                                                                         NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/* Acceptable RNGs */
static const char* knownRngs[] = {"lcg", "xorshift", "mcw64x"};
static int numberOfKnownRngs = 3;

/* OpenCL kernel files */
static const char* kernelFiles[] = {"pp/rng_test.cl"};

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
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
		
	/* Test data structures. */
	cl_kernel init_rng = NULL, test_rng = NULL;
	cl_uint **result_host = NULL;
	cl_mem seeds = NULL, result_dev = NULL;
	FILE *fp_dh, *fp_ml;
	gchar *outputMatlab = NULL, *outputDieharder = NULL;
	gchar* compilerOpts = NULL;
	gchar* rngUpper = NULL;
	
	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* How long will it take? */
	GTimer* timer = NULL;
	
	/* Parse command line options. */
	context = g_option_context_new (" - " PPTRNG_DESCRIPTION);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);	
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	if (output == NULL) output = g_strdup(RNGT_OUTPUT);
	if (rng == NULL) rng = g_strdup(RNGT_RNG);
	/* Check selected RNG. */
	status = PP_INVALID_ARGS;
	for (int i = 0; i < numberOfKnownRngs; i++) {
		if (pp_in_array(rng, (void**) knownRngs, numberOfKnownRngs, (cmpfunc) g_strcmp0)) {
			status = PP_SUCCESS;
			break;
		}
	}
	gef_if_error_create_goto(err, PP_ERROR, PP_SUCCESS != status, PP_INVALID_ARGS, error_handler, "Unknown RNG '%s'", rng);
	/* Check bits. */
	gef_if_error_create_goto(err, PP_ERROR, (bits > 32) || (bits < 1), PP_INVALID_ARGS, error_handler, "Number of bits must be between 1 and 32.");
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_ALL, 1, 0, clu_menu_device_selector, (dev_idx != -1 ? &dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Build program. */
	rngUpper = g_ascii_strup(rng, -1);
	compilerOpts = g_strconcat(PP_KERNEL_INCLUDES, "-D PP_RNG_", rngUpper, NULL);
	status = clu_program_create(zone, kernelFiles, 1, compilerOpts, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	

	/* Create kernels. */
	init_rng = clCreateKernel(zone->program, "initRng", &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel init_rng (OpenCL error %d)", status);
	test_rng = clCreateKernel(zone->program, "testRng", &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel test_rng (OpenCL error %d)", status);
	
	/* Create host buffer */
	result_host = (cl_uint**) malloc(sizeof(cl_uint*) * runs);
	for (unsigned int i = 0; i < runs; i++) {
		result_host[i] = (cl_uint*) malloc(sizeof(cl_uint) * gws);
	}
	
	/* Create device buffer */
	seeds = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, gws * sizeof(cl_ulong), NULL, &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer 1 (OpenCL error %d)", status);
	
	result_dev = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, gws * sizeof(cl_int), NULL, &status);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create device buffer 2 (OpenCL error %d)", status);

	/*  Set fixed kernel arguments. */
	status = clSetKernelArg(init_rng, 0, sizeof(cl_ulong), (void*) &rng_seed);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 init (OpenCL error %d)", status);

	status = clSetKernelArg(init_rng, 1, sizeof(cl_mem), (void*) &seeds);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 init (OpenCL error %d)", status);

	status = clSetKernelArg(test_rng, 0, sizeof(cl_mem), (void*) &seeds);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 test (OpenCL error %d)", status);
	
	status = clSetKernelArg(test_rng, 1, sizeof(cl_mem), (void*) &result_dev);
	gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 test (OpenCL error %d)", status);

	/* Print options. */
	printf("\n   =========================== Selected options ============================\n\n");
	printf("     Random number generator (seed): %s (%u)\n", rng, rng_seed);
	printf("     Seeds in workitems: %s\n", gid_seed ? "GID-based" : "Host-based (using Mersenne Twister)");
	printf("     Global/local worksizes: %d/%d\n", (int) gws, (int) lws);
	printf("     Number of runs: %d\n", runs);
	printf("     Number of bits / Maximum integer: %d / %u\n", bits, (1 << (bits - 1)) - 1);
	printf("     Compiler Options: %s\n", compilerOpts);
	
	/* Inform about execution. */
	printf("\n   =========================== Execution status ============================\n\n");
	printf("     Producing random numbers...\n");
	
	/* Start timming. */
	timer = g_timer_new();

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
	for (unsigned int i = 0; i < runs; i++) {
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
			gws * sizeof(cl_uint), 
			result_host[i], 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Read back results, iteration %d (OpenCL error %d)", i, status);
	}

	/* Stop timming. */
	g_timer_stop(timer);
	
	/* Print timming. */
	printf("     Finished, ellapsed time: %lfs\n", g_timer_elapsed(timer, NULL));
	printf("     Saving to files...\n");

	/* Output results to files to be tested with dieharder and matlab*/
	outputMatlab = g_strconcat(output, "_ml_", rng, ".tsv", NULL);
	outputDieharder = g_strconcat(output, "_dh_", rng, ".txt", NULL);
	fp_dh = fopen(outputDieharder, "w");
	fp_ml = fopen(outputMatlab, "w");
	fprintf(fp_dh, "type: d\n");
	fprintf(fp_dh, "count: %d\n", (int) (gws * runs));
	fprintf(fp_dh, "numbit: 16\n");
	for (unsigned int i = 0; i < runs; i++) {
		for (unsigned int j = 0; j < gws; j++) {
			fprintf(fp_dh, "%u\n", result_host[i][j]);
			fprintf(fp_ml, "%u\t", result_host[i][j]);
		}
		fprintf(fp_ml, "\n");
	}
	fclose(fp_dh);
	fclose(fp_ml);

	printf("     Done...\n");

	/* If we get here, no need for error checking, jump to cleanup. */
	goto cleanup;
	
error_handler:
	/* Handle error. */
	pp_error_handle(err, status);

cleanup:

	/* Free command line options. */
	if (context) g_option_context_free(context);
	if (rng) g_free(rng);	
	if (output) g_free(output);
	if (compilerOpts) g_free(compilerOpts);
	if (rngUpper) g_free(rngUpper);
	
	/* Free timer. */
	if (timer) g_timer_destroy(timer);
	
	/* Free filename strings. */
	if (outputMatlab) g_free(outputMatlab);
	if (outputDieharder) g_free(outputDieharder);
	
	/* Release OpenCL kernels */
	if (init_rng) clReleaseKernel(init_rng);
	if (test_rng) clReleaseKernel(test_rng);
	
	/* Release OpenCL memory objects */
	if (seeds) clReleaseMemObject(seeds);
	if (result_dev) clReleaseMemObject(result_dev);

	/* Release OpenCL zone (program, command queue, context) */
	if (zone) clu_zone_free(zone);

	/* Free host resources */
	if (result_host) {
		for (unsigned int i = 0; i < runs; i++) {
			if (result_host[i]) free(result_host[i]);
		}
		free(result_host);
	}
	
	/* Bye bye. */
	return 0;
	
}

