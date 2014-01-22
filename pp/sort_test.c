/** 
 * @file
 * @brief Test sorting algorithms.
 */

#include "pp_common.h"
#include "pp_gpu_sort.h"

#define SORT_LWS 256
#define SORT_BITS 32

/** A description of the program. */
#define SORT_DESCRIPTION "Test sorting algorithms"

/* Command line arguments and respective default values. */
static char *algorithm = NULL;
static size_t lws = SORT_LWS;
static int dev_idx = -1;
static guint32 rng_seed = PP_DEFAULT_SEED;
static unsigned int bits = SORT_BITS;

/* Valid command line options. */
static GOptionEntry entries[] = {
	{"algorithm",    'a', 0, G_OPTION_ARG_STRING,   &algorithm,     "Sorting algorithm: " PPG_SORT_ALGS,                                           "ALGORITHM"},
	{"localsize",    'l', 0, G_OPTION_ARG_INT,      &lws,           "Maximum local work size (default is " STR(SORT_LWS) ")",                      "SIZE"},
	{"device",       'd', 0, G_OPTION_ARG_INT,      &dev_idx,       "Device index",                                                                "INDEX"},
	{"rng-seed",     's', 0, G_OPTION_ARG_INT,      &rng_seed,      "Seed for random number generator (default is " STR(PP_DEFAULT_SEED) ")",      "SEED"},
	{"bits",         'b', 0, G_OPTION_ARG_INT,      &bits,          "Number of bits in unsigned integers to sort (default " STR(SORT_BITS) ")",    NULL},
	{G_OPTION_REMAINING,  0, 0, G_OPTION_ARG_CALLBACK, pp_args_fail, NULL,                                                                         NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};


/* OpenCL kernel files */
static const char* kernelFiles[] = {"pp/libcl/sort.cl"};

/** Information about the requested random number generation algorithm. */
static PPGSortInfo sort_info = {NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

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
	int status, ocl_status;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
		
	/* Test data structures. */
	gchar* host_data = NULL;
	cl_mem dev_data = NULL;
	gchar* compilerOpts = NULL;
	cl_kernel *krnls = NULL;
	size_t bytes;

	/* Host-based random number generator (mersenne twister) */
	GRand* rng_host = NULL;	

	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* How long will it take? */
	GTimer* timer = NULL;
	
	/* Parse command line options. */
	context = g_option_context_new (" - " SORT_DESCRIPTION);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);	
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	if (algorithm == NULL) algorithm = g_strdup(PPG_DEFAULT_SORT);
	PP_ALG_GET(sort_info, sort_infos, algorithm);
	gef_if_error_create_goto(err, PP_ERROR, !sort_info.tag, status = PP_INVALID_ARGS, error_handler, "Unknown sorting algorithm '%s'.", algorithm);
	gef_if_error_create_goto(err, PP_ERROR, (ones32(bits) != 1) || (bits > 64) || (bits < 4), status = PP_INVALID_ARGS, error_handler, "Number of bits must be 4, 8, 16, 32 or 64.");

	/* Determine size in bytes of each element to sort. */
	bytes = bits / 8;
	
	/* Initialize random number generator. */
	rng_host = g_rand_new_with_seed(rng_seed);
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_ALL, 1, 0, clu_menu_device_selector, (dev_idx != -1 ? &dev_idx : NULL), &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Build program. */
	compilerOpts = g_strconcat(
		PP_KERNEL_INCLUDES, 
		" -D ", sort_info.compiler_const, 
		" -D ", "uagr=uint",
		NULL);
	clu_program_create(zone, kernelFiles, 1, compilerOpts, &err);
	gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);
	
	/* Create sort kernel(s). */
	status = sort_info.kernels_create(&krnls, zone->program, &err);
	gef_if_error_goto(err, GEF_USE_GERROR, status, error_handler);

	/* Print options. */
	printf("\n   =========================== Selected options ============================\n\n");
	printf("     Random number generator seed: %u\n", rng_seed);
	printf("     Maximum local worksize: %d\n", (int) lws);
	printf("     Size in bits (bytes) of elements to sort: %d (%d)\n", bits, (int) bytes);
	printf("     Compiler Options: %s\n", compilerOpts);
	
	/* Create timer. */
	timer = g_timer_new();

	/* Perform test. */
	for (unsigned int N = 1; N < 22; N++) {
		
		unsigned int num_elems = 1 << (N - 1);
		gboolean sorted_ok;
		
		/* Create host buffers */
		host_data = (gchar*) realloc(host_data, bytes * num_elems);
		gef_if_error_create_goto(err, PP_ERROR, host_data == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for host data.");
		
		/* Initialize host buffer. */
		for (unsigned int i = 0;  i < num_elems; i++) {
			/* Get a random 64-bit value by default... */
			gulong value = (gulong) (g_rand_double(rng_host) * G_MAXULONG);
			/* But just use the specified bits. */
			memcpy(host_data + bytes*i, &value, bytes);
		}
		
		/* Create device buffer. */
		dev_data = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, num_elems * bytes, NULL, &ocl_status);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Error creating device buffer: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
		/* Copy data to device. */
		ocl_status = clEnqueueWriteBuffer(
			zone->queues[0], 
			dev_data, 
			CL_FALSE, 
			0, 
			num_elems * bytes, 
			host_data, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Error writing data to device: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
		/* Start timming. */
		g_timer_start(timer);
		
		/* Perform sort. */
		sort_info.sort(
			&(zone->queues[0]), 
			krnls, 
			NULL, 
			lws,
			num_elems,
			0, // This would be the current iteration...
			&err
		);
		gef_if_error_goto(err, PP_LIBRARY_ERROR, status, error_handler);

		/* Stop timming. */
		g_timer_stop(timer);
		
		/* Copy data to host. */
		ocl_status = clEnqueueReadBuffer(
			zone->queues[0], 
			dev_data,
			CL_TRUE, 
			0, 
			num_elems * bytes, 
			host_data, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Error reading data from device: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
		/* Release device buffer. */
		clReleaseMemObject(dev_data);
		
		/* Check if sorting was well performed. */
		sorted_ok = TRUE;
		for (unsigned int i = 0;  i < num_elems - 1; i++) {
			/* Compare value two by two... */
			gulong value1 = 0, value2 = 0;
			/* Get values. */
			memcpy(&value1, host_data + bytes*i, bytes);
			memcpy(&value1, host_data + bytes*(i + 1), bytes);
			/* Compare. */
			if (value1 > value2) {
				sorted_ok = FALSE;
				break;
			}
		}
		
		/* Print info. */
		printf("       - 2^%d %d-bit elements: %fMkeys/s %s\n", N, bits, N / g_timer_elapsed(timer, NULL), sorted_ok ? "" : "(sort did not work)");
		
	}

	/* If we get here, everything went Ok. */
	status = PP_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;
	
error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	g_assert(status != PP_SUCCESS);
	fprintf(stderr, "Error: %s\n", err->message);
	g_error_free(err);	

cleanup:

	/* Free command line options. */
	if (context) g_option_context_free(context);
	if (compilerOpts) g_free(compilerOpts);
	
	/* Free timer. */
	if (timer) g_timer_destroy(timer);
	
	/* Free host-based random number generator. */
	if (rng_host) g_rand_free(rng_host);
	
	/* Release OpenCL kernels */
	if (krnls) sort_info.kernels_free(&krnls);
	
	/* Release OpenCL zone (program, command queue, context) */
	if (zone) clu_zone_free(zone);

	/* Free host resources */
	if (host_data) free(host_data);
	
	/* Bye bye. */
	return status;
	
}

