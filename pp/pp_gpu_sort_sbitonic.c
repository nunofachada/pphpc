/** 
 * @file
 * @brief PredPrey OpenCL GPU simple bitonic sort host implementation.
 */

#include "pp_gpu_sort_sbitonic.h"

/** Event index for simple bitonic sort kernel. */
static unsigned int sbitonic_evt_idx;

/**
 * @brief Sort agents using the simple bitonic sort.
 * 
 * @see ppg_sort_sort()
 */
int ppg_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int numel, gboolean profile, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
		
	/* Global worksize. */
	size_t gws = nlpo2(numel) / 2;
	
	/* Local worksize. */
	size_t lws = lws_max;
	
	/* Number of bitonic sort stages. */
	cl_uint totalStages = (cl_uint) tzc(gws * 2);
	
	/* Event pointer, in case profiling is on. */
	cl_event* evt;

	/* Adjust LWS so that its equal or smaller than GWS, making sure that
	 * GWS is still a multiple of it. */
	while (gws % lws != 0)
		lws = lws / 2;
		
	/* Perform agent sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		cl_uint step = currentStage;
		for (cl_uint currentStep = step; currentStep > 0; currentStep--) {
			
			ocl_status = clSetKernelArg(krnls[0], 1, sizeof(cl_uint), (void *) &currentStage);
			gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "arg 1 of sort kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			
			ocl_status = clSetKernelArg(krnls[0], 2, sizeof(cl_uint), (void *) &currentStep);
			gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "arg 2 of sort kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			
			evt = profile ? &evts[0][sbitonic_evt_idx] : NULL;
			
			ocl_status = clEnqueueNDRangeKernel(
				queues[0], 
				krnls[0], 
				1, 
				NULL, 
				&gws, 
				&lws, 
				0, 
				NULL,
				evt
			);
			gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing simple bitonic sort kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			sbitonic_evt_idx++;
		}
	}
	
	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;	
}

/** 
 * @brief Create kernels for the simple bitonic sort. 
 * 
 * @see ppg_sort_kernels_create()
 * */
int ppg_sort_sbitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Allocate memory for single kernel required for simple bitonic sort. */
	*krnls = (cl_kernel*) calloc(1, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, PP_ERROR, *krnls == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort kernel.");	
	
	/* Create kernel. */
	(*krnls)[0] = clCreateKernel(program, "sbitonicSort", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create sbitonicSort kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;

}

/** 
 * @brief Set kernels arguments for the simple bitonic sort. 
 * 
 * @see ppg_sort_kernelargs_set()
 * */
int ppg_sort_sbitonic_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t agent_len, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* LWS and agent_len not used here. */
	lws = lws;
	agent_len = agent_len;
	
	/* Set kernel arguments. */
	ocl_status = clSetKernelArg(*krnls[0], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set arg 0 of sbitonic_sort kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;
	
}

/** 
 * @brief Free the simple bitonic sort kernels. 
 * 
 * @see ppg_sort_kernels_free()
 * */
void ppg_sort_sbitonic_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		if ((*krnls)[0]) clReleaseKernel((*krnls)[0]);
		free(*krnls);
	}
}

/** 
 * @brief Create events for the simple bitonic sort kernels. 
 * 
 * @see ppg_sort_events_create()
 * */
int ppg_sort_sbitonic_events_create(cl_event ***evts, unsigned int iters, size_t numel, size_t lws_max, GError **err) {

	/* Aux. var. */
	int status;
	
	/* LWS not used for simple bitonic sort. */
	lws_max = lws_max;
	
	/* Required number of events, worst case usage scenario. The instruction 
	 * below sums all numbers from 0 to x, where is is the log2 (tzc) of
	 * the next larger power of 2 of the maximum possible agents. */
	int num_evts = iters * sum(tzc(nlpo2(numel))); 
	
	/* Set simple bitonic sort event index to zero (global var) */
	sbitonic_evt_idx = 0;
	
	/* Only one type of event required for the simple bitonic sort kernel. */
	*evts = (cl_event**) calloc(1, sizeof(cl_event*));
	gef_if_error_create_goto(*err, PP_ERROR, *evts == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort events (1).");	
	
	/* Allocate memory for all occurrences of the event (i.e. executions of the simple bitonic sort kernel). */
	(*evts)[0] = (cl_event*) calloc(num_evts, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[0] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort events (2).");	
	
	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:
	
	/* Return. */
	return status;	
}

/** 
 * @brief Free the simple bitonic sort events. 
 * 
 * @see ppg_sort_events_free()
 * */
void ppg_sort_sbitonic_events_free(cl_event ***evts) {
	if (evts) {
		if (*evts) {
			if ((*evts)[0]) {
				for (unsigned int i = 0; i < sbitonic_evt_idx; i++) {
					if ((*evts)[0][i]) {
						clReleaseEvent((*evts)[0][i]);
					}
				}
				free((*evts)[0]);
			}
			free(*evts);
		}
	}
}

/** 
 * @brief Add bitonic sort events to the profiler object. 
 * 
 * @see ppg_sort_events_profile()
 * */
int ppg_sort_sbitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err) {
	
	int status;

	for (unsigned int i = 0; i < sbitonic_evt_idx; i++) {
		profcl_profile_add(profile, "SBitonic Sort", evts[0][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = PP_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;	
	
}
