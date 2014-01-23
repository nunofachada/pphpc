/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms kernels.
 */

#ifndef SORT_ELEM_TYPE
	#define SORT_ELEM_TYPE uint
#endif

#ifdef PPG_SORT_SBITONIC
#include "libcl/sort_sbitonic.cl"
#elif defined PPG_SORT_OIABITONIC
#include "libcl/sort_oiabitonic.cl"
#endif

