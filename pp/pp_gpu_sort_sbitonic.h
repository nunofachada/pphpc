/** 
 * @file
 * @brief PredPrey OpenCL GPU simple bitonic sort header file.
 */

#ifndef PREDPREYGPUSORTSBITONIC_H
#define PREDPREYGPUSORTSBITONIC_H

#include "pp_gpu_sort.h"

/** @brief A simple bitonic sort kernel. */
int ppg_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int max_agents, unsigned int iter, GError **err);

int ppg_sort_sbitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

int ppg_sort_sbitonic_kernelargs_set(cl_kernel **krnls, PPGBuffersDevice buffersDevice, GError **err);

void ppg_sort_sbitonic_kernels_free(cl_kernel **krnls);

int ppg_sort_sbitonic_events_create(cl_event ***evts, unsigned int iters, GError **err);

void ppg_sort_sbitonic_events_free(cl_event ***evts);

int ppg_sort_sbitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err);

#endif
