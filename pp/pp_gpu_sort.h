/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms header file.
 */

#ifndef PREDPREYGPUSORT_H
#define PREDPREYGPUSORT_H

#include "pp_gpu.h"

/**
 * @brief Available sorting algorithms.
 * */
#define PPG_SORT_ALGS "s-bitonic (default)"

/**
 * @brief Default sorting algorithm.
 * */
#define PPG_DEFAULT_SORT "s-bitonic"

/**
 * @brief Generic sort function definition.
 * 
 * @param queues Available command queues.
 * @param krnls Kernels required for sorting.
 * @param evts Associated events.
 * @param lws_max Maximum local worksize.
 * @param err GError error reporting object.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_sort)(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err);

typedef int (*ppg_sort_kernels_create)(cl_kernel **krnls, cl_program program, GError **err);

typedef int (*ppg_sort_kernelargs_set)(cl_kernel **krnls, PPGBuffersDevice buffersDevice, GError **err);

typedef void (*ppg_sort_kernels_free)(cl_kernel **krnls);

typedef int (*ppg_sort_events_create)(cl_event ***evts, unsigned int iters, GError **err);

typedef void (*ppg_sort_events_free)(cl_event ***evts);

typedef int (*ppg_sort_events_profile)(cl_event **evts, ProfCLProfile *profile, GError **err);

/**
 * @brief Object which represents an agent sorting algorithm.
 * */	
typedef struct ppg_sort_info {
	char* tag;            /**< Tag identifying the algorithm. */
	char* compiler_const; /**< OpenCL compiler constant to include the proper CL file. */
	unsigned int num_queues;  /**< Number of OpenCL command queues required for the algorithm. */
	ppg_sort_sort sort;   /**< The sorting function. */
	ppg_sort_kernels_create kernels_create;
	ppg_sort_kernelargs_set kernelargs_set;
	ppg_sort_kernels_free kernels_free;
	ppg_sort_events_create events_create;
	ppg_sort_events_free events_free;
	ppg_sort_events_profile events_profile;
} PPGSortInfo;

/** @brief Information about the available agent sorting algorithms. */
extern PPGSortInfo sort_infos[];

#endif
