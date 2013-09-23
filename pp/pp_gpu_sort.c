/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms host implementation.
 */

#include "pp_gpu_sort.h"

/** Available sorting algorithms and respective properties. */
PPGSortInfo sort_infos[] = {
	{"s-bitonic", "SBIT"}, 
	{NULL, NULL}
};
