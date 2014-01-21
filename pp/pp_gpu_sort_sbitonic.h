/** 
 * @file
 * @brief PredPrey OpenCL GPU simple bitonic sort header file.
 */

#ifndef PREDPREYGPUSORTSBITONIC_H
#define PREDPREYGPUSORTSBITONIC_H

#include "pp_gpu_sort.h"

/** @brief Sort agents using the simple bitonic sort. */
int ppg_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err);

/** @brief Create kernels for the simple bitonic sort. */
int ppg_sort_sbitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

/** @brief Set kernels arguments for the simple bitonic sort. */
int ppg_sort_sbitonic_kernelargs_set(cl_kernel **krnls, PPGBuffersDevice buffersDevice, size_t lws, size_t agent_len, GError **err);

/** @brief Free the simple bitonic sort kernels. */
void ppg_sort_sbitonic_kernels_free(cl_kernel **krnls);

/** @brief Create events for the simple bitonic sort kernels. */
int ppg_sort_sbitonic_events_create(cl_event ***evts, unsigned int iters, size_t max_agents, size_t lws_max, GError **err);

/** @brief Free the simple bitonic sort events. */
void ppg_sort_sbitonic_events_free(cl_event ***evts);

/** @brief Add bitonic sort events to the profiler object. */
int ppg_sort_sbitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err);

#endif
