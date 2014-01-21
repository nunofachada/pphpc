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
#define PPG_SORT_ALGS "s-bitonic (default), oia-bitonic"

/**
 * @brief Default sorting algorithm.
 * */
#define PPG_DEFAULT_SORT "s-bitonic"

/**
 * @brief Sort agents.
 * 
 * @param queues Available command queues.
 * @param krnls Sort kernels.
 * @param evts Associated events.
 * @param lws_max Maximum local worksize.
 * @param err GError error reporting object.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_sort)(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err);

/**
 * @brief Create sort kernels.
 * 
 * @param krnls Sort kernels.
 * @param program OpenCL program from where kernels are created.
 * @param err GError error reporting object.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_kernels_create)(cl_kernel **krnls, cl_program program, GError **err);

/**
 * @brief Set sort kernels arguments.
 * 
 * @param krnls Sort kernels.
 * @param buffersDevice Device buffers (agents, cells, etc.).
 * @param err GError error reporting object.
 * @param lws Local work size.
 * @param agent_len Agent length, 4 or 8 bytes.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_kernelargs_set)(cl_kernel **krnls, PPGBuffersDevice buffersDevice, size_t lws, size_t agent_len, GError **err);

/**
 * @brief Free sort kernels.
 * 
 * @param krnls Sort kernels.
 */
typedef void (*ppg_sort_kernels_free)(cl_kernel **krnls);

/**
 * @brief Create sort events.
 * 
 * @param evts Sort events.
 * @param iters Number of iterations.
 * @param err GError error reporting object.
 * @param max_agents Maximum number of agents for this simulation.
 * @param lws_max Maximum local worksize for sort kernels.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_events_create)(cl_event ***evts, unsigned int iters, size_t max_agents, size_t lws_max, GError **err);

/**
 * @brief Free sort events.
 * 
 * @param evts Sort events.
 */
typedef void (*ppg_sort_events_free)(cl_event ***evts);

/**
 * @brief Add sort events to profiler object.
 * 
 * @param evts Sort events.
 * @param profile Profiler object.
 * @param err GError error reporting object.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*ppg_sort_events_profile)(cl_event **evts, ProfCLProfile *profile, GError **err);

/**
 * @brief Object which represents an agent sorting algorithm.
 * */	
typedef struct ppg_sort_info {
	char* tag;            /**< Tag identifying the algorithm. */
	char* compiler_const; /**< OpenCL compiler constant to include the proper CL file. */
	unsigned int num_queues;  /**< Number of OpenCL command queues required for the algorithm. */
	ppg_sort_sort sort;   /**< The sorting function. */
	ppg_sort_kernels_create kernels_create; /**< Create sort kernels function. */
	ppg_sort_kernelargs_set kernelargs_set; /**< Set sort kernels arguments function. */
	ppg_sort_kernels_free kernels_free;     /**< Free sort kernels function. */
	ppg_sort_events_create events_create;   /**< Create sort events function.*/
	ppg_sort_events_free events_free;       /**< Free sort events function. */
	ppg_sort_events_profile events_profile; /**< Add sort events to profiler object function. */
} PPGSortInfo;

/** @brief Information about the available agent sorting algorithms. */
extern PPGSortInfo sort_infos[];

#endif
