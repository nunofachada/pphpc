/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms host implementation.
 */

#include "pp_gpu_sort.h"
#include "pp_gpu_sort_sbitonic.h"
//~ #include "pp_gpu_sort_oiabitonic.h"

/** Available sorting algorithms and respective properties. */
PPGSortInfo sort_infos[] = {
	{"s-bitonic", "PPG_SORT_SBITONIC", 1, 
		ppg_sort_sbitonic_sort, 
		ppg_sort_sbitonic_kernels_create, ppg_sort_sbitonic_kernelargs_set, ppg_sort_sbitonic_kernels_free, 
		ppg_sort_sbitonic_events_create, ppg_sort_sbitonic_events_free, ppg_sort_sbitonic_events_profile}, 
	//~ {"oia-bitonic", "PPG_SORT_OIABITONIC", 1, 
		//~ ppg_sort_oiabitonic_sort, 
		//~ ppg_sort_oiabitonic_kernels_create, ppg_sort_oiabitonic_kernelargs_set, ppg_sort_oiabitonic_kernels_free, 
		//~ ppg_sort_oiabitonic_events_create, ppg_sort_oiabitonic_events_free, ppg_sort_oiabitonic_events_profile}, 
	{NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

