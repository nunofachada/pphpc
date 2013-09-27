/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms host implementation.
 */

#include "pp_gpu_sort.h"

/** Available sorting algorithms and respective properties. */
PPGSortInfo sort_infos[] = {
	{"s-bitonic", "PPG_SORT_SBITONIC", 1, ppg_sort_sbitonic_sort, ppg_sort_sbitonic_kernels_create, ppg_sort_sbitonic_kernelargs_set, ppg_sort_sbitonic_kernels_free, ppg_sort_sbitonic_events_create, ppg_sort_sbitonic_events_free}, 
	{NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL}
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
int ppg_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err) {
	
#ifndef CLPROFILER
	evts = evts;
#endif

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
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		cl_uint step = currentStage;
		for (cl_uint currentStep = step; currentStep > 0; currentStep--) {
			
			status = clSetKernelArg(krnls[0], 5, sizeof(cl_uint), (void *) &currentStage);
			gef_if_error_create_goto(*err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "arg 5 of sort kernel, iter %d  (OpenCL error %d)", iter, status);
			
			status = clSetKernelArg(krnls[0], 6, sizeof(cl_uint), (void *) &currentStep);
			gef_if_error_create_goto(*err, PP_ERROR, status != CL_SUCCESS, PP_LIBRARY_ERROR, error_handler, "arg 6 of sort kernel, iter %d  (OpenCL error %d)", iter, status);
			
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
				evts[0] + sbitonic_evt_idx
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
	*krnls = (cl_kernel*) calloc(1, sizeof(cl_kernel));
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

int ppg_sort_sbitonic_kernelargs_set(cl_kernel **krnls, PPGBuffersDevice buffersDevice, GError **err) {
	
	/* Aux. var. */
	int status;
	
	/* Set kernel arguments. */
	status = clSetKernelArg(*krnls[0], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_xy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of sbitonic_sort (OpenCL error %d)", status);

	status = clSetKernelArg(*krnls[0], 1, sizeof(cl_mem), (void*) &buffersDevice.agents_alive);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of sbitonic_sort(OpenCL error %d)", status);

	status = clSetKernelArg(*krnls[0], 2, sizeof(cl_mem), (void*) &buffersDevice.agents_energy);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 2 of sbitonic_sort (OpenCL error %d)", status);

	status = clSetKernelArg(*krnls[0], 3, sizeof(cl_mem), (void*) &buffersDevice.agents_type);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 3 of sbitonic_sort (OpenCL error %d)", status);

	status = clSetKernelArg(*krnls[0], 4, sizeof(cl_mem), (void*) &buffersDevice.agents_hash);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != status, PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 4 of sbitonic_sort (OpenCL error %d)", status);

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
	if (*krnls) {
		clReleaseKernel(*krnls[0]);
		free(*krnls);
	}
}

int ppg_sort_sbitonic_events_create(cl_event ***evts, unsigned int iters, GError **err) {

	/* Aux. var. */
	int status = PP_SUCCESS;
	
	/* Required number of events, worst case usage scenario. */
	/// @todo Check the logic of this
	int num_evts = iters * sum(tzc(nlpo2(PPG_DEFAULT_MAX_AGENTS))); 
	
	/* Set simple bitonic sort event index to zero (global var) */
	sbitonic_evt_idx = 0;
	
	/* Only one type of event required for the simple bitonic sort kernel. */
	*evts = (cl_event**) calloc(1, sizeof(cl_event*));
	gef_if_error_create_goto(*err, PP_ERROR, *evts == NULL, PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort events (1).");	
	
	/* Allocate memory for all occurrences of the event (i.e. executions of the simple bitonic sort kernel). */
	*evts[0] = (cl_event*) calloc(num_evts, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, *evts[0] == NULL, PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for simple bitonic sort events (2).");	
	
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

void ppg_sort_sbitonic_events_free(cl_event ***evts) {
	if (*evts) {
		if (*evts[0]) {
			for (unsigned int i = 0; i < sbitonic_evt_idx; i++) {
				if (*evts[0][i]) {
					clReleaseEvent(*evts[0][i]);
				}
			}
			free(*evts[0]);
		}
		free(*evts);
	}
}

