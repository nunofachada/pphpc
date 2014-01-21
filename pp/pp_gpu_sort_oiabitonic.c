/** 
 * @file
 * @brief PredPrey OpenCL GPU OIA bitonic sort host implementation.
 */

#include "pp_gpu_sort_oiabitonic.h"

#define PPG_SORT_OIA_NUMKRNLS 5

#define PPG_SORT_OIA_KINIT 0
#define PPG_SORT_OIA_KSTAGE0 1
#define PPG_SORT_OIA_KSTAGEN 2
#define PPG_SORT_OIA_KMERGE 3
#define PPG_SORT_OIA_KMERGELAST 4

#ifdef CLPROFILER
/** Event index for OIA bitonic sort kernel. */
static unsigned int evt_idx[PPG_SORT_OIA_NUMKRNLS];
#endif

/**
 * @brief Sort agents using the OIA bitonic sort.
 * 
 * @see ppg_sort_sort()
 */
int ppg_sort_oiabitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err) {
	
#ifndef CLPROFILER
	evts = evts;
#endif

	/* Aux. var. */
	int status, ocl_status;
		
	/* Number of stages. */
	int num_stages;
	
	/* Determine global and local worksizes. */
	size_t lws = lws_max;
	size_t gws = max_agents / 8;
	
	/* Adjust LWS so that its equal or smaller than GWS, making sure that
	 * GWS is still a multiple of it. */
	while (gws % lws != 0)
		lws = lws / 2;
		
	/* Enqueue initial sorting kernel. */
	ocl_status = clEnqueueNDRangeKernel(
		queues[0], 
		krnls[PPG_SORT_OIA_KINIT], 
		1, 
		NULL, 
		&gws, 
		&lws, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts[PPG_SORT_OIA_KINIT][evt_idx[PPG_SORT_OIA_KINIT]]
#else
		NULL
#endif
	);
	gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing bsort_init kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));
#ifdef CLPROFILER
	evt_idx[PPG_SORT_OIA_KINIT]++;
#endif

	/* Execute further stages. */
	num_stages = gws / lws;
	
	for(int high_stage = 2; high_stage < num_stages; high_stage <<= 1) {

		ocl_status = clSetKernelArg(krnls[PPG_SORT_OIA_KSTAGE0], 2, sizeof(int), &high_stage);		
		gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Set arg 2 of bsort_stage0 kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg(krnls[PPG_SORT_OIA_KSTAGEN], 3, sizeof(int), &high_stage);
		gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Set arg 3 of bsort_stagen kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));

		for(int stage = high_stage; stage > 1; stage >>= 1) {

			ocl_status = clSetKernelArg(krnls[PPG_SORT_OIA_KSTAGEN], 2, sizeof(int), &stage);
			gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Set arg 2 of bsort_stagen kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));

			ocl_status = clEnqueueNDRangeKernel(
				queues[0], 
				krnls[PPG_SORT_OIA_KSTAGEN], 
				1, 
				NULL, 
				&gws, 
				&lws, 
				0, 
				NULL, 
#ifdef CLPROFILER
				&evts[PPG_SORT_OIA_KSTAGEN][evt_idx[PPG_SORT_OIA_KSTAGEN]]
#else
				NULL
#endif
			);
			gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing bsort_stagen kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));
#ifdef CLPROFILER
			evt_idx[PPG_SORT_OIA_KSTAGEN]++;
#endif
			
		}

		ocl_status = clEnqueueNDRangeKernel(
			queues[0], 
			krnls[PPG_SORT_OIA_KSTAGE0], 
			1, 
			NULL, 
			&gws, 
			&lws, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&evts[PPG_SORT_OIA_KSTAGEN][evt_idx[PPG_SORT_OIA_KSTAGE0]]
#else
			NULL
#endif
		); 
		gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing bsort_stage0 kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));
#ifdef CLPROFILER
		evt_idx[PPG_SORT_OIA_KSTAGE0]++;
#endif
	}

	/* Perform the bitonic merge */
	for(int stage = num_stages; stage > 1; stage >>= 1) {

		ocl_status = clSetKernelArg(krnls[PPG_SORT_OIA_KMERGE], 2, sizeof(int), &stage);
		gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Set arg 2 of bsort_merge kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));

		ocl_status = clEnqueueNDRangeKernel(
			queues[0], 
			krnls[PPG_SORT_OIA_KMERGE], 
			1, 
			NULL, 
			&gws, 
			&lws, 
			0, 
			NULL, 
#ifdef CLPROFILER
			&evts[PPG_SORT_OIA_KMERGE][evt_idx[PPG_SORT_OIA_KMERGE]]
#else
			NULL
#endif
		); 
		gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing bsort_merge kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));
#ifdef CLPROFILER
		evt_idx[PPG_SORT_OIA_KMERGE]++;
#endif
	}
	
	ocl_status = clEnqueueNDRangeKernel(
		queues[0], 
		krnls[PPG_SORT_OIA_KMERGELAST], 
		1, 
		NULL, 
		&gws, 
		&lws, 
		0, 
		NULL, 
#ifdef CLPROFILER
		&evts[PPG_SORT_OIA_KMERGELAST][evt_idx[PPG_SORT_OIA_KMERGELAST]]
#else
		NULL
#endif
	); 
	gef_if_error_create_goto(*err, PP_ERROR, ocl_status != CL_SUCCESS, status = PP_LIBRARY_ERROR, error_handler, "Executing bsort_merge_last kernel (from OIA bitonic sort), iter %d. OpenCL error %d: %s", iter, ocl_status, clerror_get(ocl_status));
#ifdef CLPROFILER
	evt_idx[PPG_SORT_OIA_KMERGELAST]++;
#endif
	
	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;	
}

/** 
 * @brief Create kernels for the OIA bitonic sort. 
 * 
 * @see ppg_sort_kernels_create()
 * */
int ppg_sort_oiabitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {


	/* Aux. var. */
	int status, ocl_status;
	
	/* Allocate memory for single kernel required for OIA bitonic sort. */
	*krnls = (cl_kernel*) calloc(PPG_SORT_OIA_NUMKRNLS, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, PP_ERROR, *krnls == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for OIA bitonic sort kernel.");	
	
	/* Create kernels. */
	(*krnls)[PPG_SORT_OIA_KINIT] = clCreateKernel(program, "bsort_init", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: bsort_init. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	(*krnls)[PPG_SORT_OIA_KSTAGE0] = clCreateKernel(program, "bsort_stage_0", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: bsort_stage_0. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	(*krnls)[PPG_SORT_OIA_KSTAGEN] = clCreateKernel(program, "bsort_stage_n", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: bsort_stage_n. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	(*krnls)[PPG_SORT_OIA_KMERGE] = clCreateKernel(program, "bsort_merge", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: bsort_merge. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	(*krnls)[PPG_SORT_OIA_KMERGELAST] = clCreateKernel(program, "bsort_merge_last", &ocl_status);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Create kernel: bsort_merge_last. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;

}

/** 
 * @brief Set kernels arguments for the OIA bitonic sort. 
 * 
 * @see ppg_sort_kernelargs_set()
 * */
int ppg_sort_oiabitonic_kernelargs_set(cl_kernel **krnls, PPGBuffersDevice buffersDevice, size_t lws, size_t agent_len, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Set bsort_init arguments. */
	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KINIT], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of bsort_init. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KINIT], 1, 8 * lws * agent_len, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of bsort_init. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* Set bsort_stage_0 arguments. */
	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KSTAGE0], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of bsort_stage_0. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KSTAGE0], 1, 8 * lws * agent_len, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of bsort_stage_0. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* Set bsort_stage_n arguments. */
	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KSTAGEN], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of bsort_stage_n. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KSTAGEN], 1, 8 * lws * agent_len, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of bsort_stage_n. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* Set bsort_merge arguments. */
	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KMERGE], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of bsort_merge. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KMERGE], 1, 8 * lws * agent_len, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of bsort_merge. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* Set bsort_merge_last arguments. */
	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KMERGELAST], 0, sizeof(cl_mem), (void*) &buffersDevice.agents_data);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 0 of bsort_merge_last. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[PPG_SORT_OIA_KMERGELAST], 1, 8 * lws * agent_len, NULL);
	gef_if_error_create_goto(*err, PP_ERROR, CL_SUCCESS != ocl_status, status = PP_LIBRARY_ERROR, error_handler, "Set kernel args: arg 1 of bsort_merge_last. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* If we got here, everything is OK. */
	status = PP_SUCCESS;
	g_assert(*err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
	
finish:
	
	/* Return. */
	return status;
	
}

/** 
 * @brief Free the OIA bitonic sort kernels. 
 * 
 * @see ppg_sort_kernels_free()
 * */
void ppg_sort_oiabitonic_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		for (int i = 0; i < PPG_SORT_OIA_NUMKRNLS; i++) {
			if ((*krnls)[i]) clReleaseKernel((*krnls)[i]);
		}
		free(*krnls);
	}
}

/** 
 * @brief Create events for the OIA bitonic sort kernels. 
 * 
 * @see ppg_sort_events_create()
 * */
int ppg_sort_oiabitonic_events_create(cl_event ***evts, unsigned int iters, size_t max_agents, size_t lws_max, GError **err) {

	/* Aux. var. */
	int status;
	
#ifdef CLPROFILER

	size_t max_loop = tzc(max_agents / 8 * lws_max);
	
	/* Events required for the five kernel. */
	*evts = (cl_event**) calloc(PPG_SORT_OIA_NUMKRNLS, sizeof(cl_event*));
	gef_if_error_create_goto(*err, PP_ERROR, *evts == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for OIA bitonic sort kernel events.");

	/* Required number of events for each kernel, worst case usage scenario. */
	(*evts)[PPG_SORT_OIA_KINIT] = (cl_event*) calloc(iters, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[PPG_SORT_OIA_KINIT] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for bsort_init events.");	
	
	(*evts)[PPG_SORT_OIA_KSTAGE0] = (cl_event*) calloc(iters * max_loop, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[PPG_SORT_OIA_KSTAGE0] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for bsort_stage0 events.");

	(*evts)[PPG_SORT_OIA_KSTAGEN] = (cl_event*) calloc(iters * max_loop * max_loop, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[PPG_SORT_OIA_KSTAGEN] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for bsort_stagen events.");	

	(*evts)[PPG_SORT_OIA_KMERGE] = (cl_event*) calloc(iters * max_loop, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[PPG_SORT_OIA_KMERGE] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for bsort_merge events.");

	(*evts)[PPG_SORT_OIA_KMERGELAST] = (cl_event*) calloc(iters, sizeof(cl_event));
	gef_if_error_create_goto(*err, PP_ERROR, (*evts)[PPG_SORT_OIA_KMERGELAST] == NULL, status = PP_ALLOC_MEM_FAIL, error_handler, "Unable to allocate memory for bsort_merge_last events.");

	/* Set OIA bitonic sort event indexes to zero (global var) */
	for (int i = 0; i < PPG_SORT_OIA_NUMKRNLS; i++)
		evt_idx[i] = 0;

#else
	/* Avoid compiler warnings. */
	evts = evts;
	iters = iters;
	max_agents = max_agents;
	lws_max = lws_max;
#endif
	
	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;

#ifdef CLPROFILER	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
#endif	
finish:

	/* Return. */
	return status;	
}

/** 
 * @brief Free the OIA bitonic sort events. 
 * 
 * @see ppg_sort_events_free()
 * */
void ppg_sort_oiabitonic_events_free(cl_event ***evts) {
#ifdef CLPROFILER
	if (evts) {
		if (*evts) {
			for (int k = 0; k < PPG_SORT_OIA_NUMKRNLS; k++) {
				if ((*evts)[k]) {
					for (unsigned int i = 0; i < evt_idx[k]; i++) {
						if ((*evts)[k][i]) {
							clReleaseEvent((*evts)[k][i]);
						}
					}
					free((*evts)[k]);
				}
				free(*evts);
			}
		}
	}
#else
	/* Avoid compiler warnings. */
	evts = evts;
#endif
}

/** 
 * @brief Add bitonic sort events to the profiler object. 
 * 
 * @see ppg_sort_events_profile()
 * */
int ppg_sort_oiabitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err) {
	
	int status;

#ifdef CLPROFILER

	unsigned int i;

	for (i = 0; i < evt_idx[PPG_SORT_OIA_KINIT]; i++) {
		profcl_profile_add(profile, "bsort_init", evts[PPG_SORT_OIA_KINIT][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}

	for (i = 0; i < evt_idx[PPG_SORT_OIA_KSTAGE0]; i++) {
		profcl_profile_add(profile, "bsort_stage0", evts[PPG_SORT_OIA_KSTAGE0][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}

	for (i = 0; i < evt_idx[PPG_SORT_OIA_KSTAGEN]; i++) {
		profcl_profile_add(profile, "bsort_stagen", evts[PPG_SORT_OIA_KSTAGEN][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}

	for (i = 0; i < evt_idx[PPG_SORT_OIA_KMERGE]; i++) {
		profcl_profile_add(profile, "bsort_merge", evts[PPG_SORT_OIA_KMERGE][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}

	for (i = 0; i < evt_idx[PPG_SORT_OIA_KMERGELAST]; i++) {
		profcl_profile_add(profile, "bsort_merge_last", evts[PPG_SORT_OIA_KMERGELAST][i], err);
		gef_if_error_goto(*err, PP_LIBRARY_ERROR, status, error_handler);
	}
#else
	/* Avoid compiler warnings. */
	evts = evts;
	profile = profile;
#endif
	/* If we got here, everything is OK. */
	g_assert(*err == NULL);
	status = PP_SUCCESS;
	goto finish;

#ifdef CLPROFILER	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(*err != NULL);
#endif
	
finish:
	
	/* Return. */
	return status;	
	
}
