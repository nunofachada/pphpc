/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms host implementation.
 */

#include "pp_gpu_sort.h"

/** Available sorting algorithms and respective properties. */
PPGSortInfo sort_infos[] = {
	{"s-bitonic", "PPG_SORT_SBITONIC", 1, ppg_sort_sbitonic_sort, ppg_sort_sbitonic_kernels_create, ppg_sort_sbitonic_kernels_free, ppg_sort_sbitonic_events_create, ppg_sort_sbitonic_events_free}, 
	{NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL}
};

/** Event index for simple bitonic sort kernel. */
unsigned int sbitonic_evt_idx;

/**
 * @brief A simple bitonic sort kernel.
 * 
 * @param queues
 * @param krnls
 * @param evts
 * @param lws_max
 * @param err
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
int ppg_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event *evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err) {
	
	/* Aux. var. */
	int status;
		
	/* Global worksize. */
	size_t gws = nlpo2(max_agents) / 2;
	
	/* Local worksize. */
	size_t lws = lws_max;
	
	/* Number of bitonic sort stages. */
	cl_uint totalStages = (cl_uint) tzc(gws * 2);

	/* Make sure GWS is a multiple of LWS. */
	/// @todo Check the logic of this
	while (gws % lws != 0)
		lws = lws / 2;
		
	/* Perform agent sorting. */
	for (unsigned int currentStage = 1; currentStage <= totalStages; currentStage++) {
		cl_uint step = currentStage;
		for (int currentStep = step; currentStep > 0; currentStep--) {
			
			status = clSetKernelArg(krnls[0], 6, sizeof(cl_uint), (void *) &currentStage);
			gef_if_error_create_goto(*err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "arg 6 of sort kernel, iter %d  (OpenCL error %d)", iter, status);
			
			status = clSetKernelArg(krnls[0], 7, sizeof(cl_uint), (void *) &currentStep);
			gef_if_error_create_goto(*err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "arg 7 of sort kernel, iter %d  (OpenCL error %d)", iter, status);
			
			status = clEnqueueNDRangeKernel(
				queues[0], 
				krnls[0], 
				1, 
				NULL, 
				&gws, 
				&lws, 
				0, 
				NULL, 
#ifdef CLPROFILER
				evts + sbitonic_evt_idx
#else
				NULL
#endif
			);
			gef_if_error_create_goto(*err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "Executing simple bitonic sort kernel, iter %d  (OpenCL error %d)", iter, status);
			
#ifdef CLPROFILER
			sbitonic_evt_idx++;
#endif
		}
	}
	
	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	/* Set status to error code. */
	status = (*err)->code;
	
finish:
	
	/* Return. */
	return status;	
}

int ppg_sort_sbitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	/* Aux. var. */
	int status;
	
	/* Allocate memory for single kernel required for simple bitonic sort. */
	*krnls = (cl_kernel*) malloc(sizeof(cl_kernel));
	gef_if_error_create_goto(*err, PP_ERROR, *krnls == NULL, PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort kernel.");	
	
	/* Create kernel. */
	*krnls[0] = clCreateKernel(program, "sbitonicSort", &status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Create kernel: sbitonicSort (OpenCL error %d)", status);

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	/* Set status to error code. */
	status = (*err)->code;
	
finish:
	
	/* Return. */
	return status;

	
}

void ppg_sort_sbitonic_kernels_free(cl_kernel **krnls) {
	if (*krnls) free(*krnls);
}

int ppg_sort_sbitonic_events_create(cl_event **evts, unsigned int iters, GError **err) {

	/* Aux. var. */
	int status = PP_SUCCESS;
	
	/* Required number of events, worst case usage scenario. */
	/// @todo Check the logic of this
	int num_evts = iters * sum(tzc(nlpo2(PPG_DEFAULT_MAX_AGENTS))); 
	
	/* Set simple bitonic sort event index to zero (global var) */
	sbitonic_evt_idx = 0;
	
	/* Allocate memory for single kernel required for simple bitonic sort. */
	*evts = (cl_event*) malloc(num_evts * sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, *evts == NULL, PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort events.");	
	
	/* If we got here, everything is OK. */
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	/* Set status to error code. */
	status = (*err)->code;
	
finish:
	
	/* Return. */
	return status;	
}

void ppg_sort_sbitonic_events_free(cl_event **evts) {
	if (*evts) free(*evts);
}

