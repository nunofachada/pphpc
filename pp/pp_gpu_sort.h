/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms header file.
 */

#ifndef PREDPREYGPUSORT_H
#define PREDPREYGPUSORT_H

#include <stdlib.h>

/**
 * @brief Available sorting algorithms.
 * */
#define PPG_SORT_ALGS "s-bitonic (default)"

/**
 * @brief Default sorting algorithm.
 * */
#define PPG_DEFAULT_SORT "s-bitonic"

/**
 * @brief Information about an agent sorting algorithm.
 * */	
typedef struct ppg_sort_info {
	char* tag;            /**< Tag identifying the RNG. */
	char* compiler_const; /**< RNG OpenCL compiler constant. */
} PPGSortInfo;

/** @brief Information about the available agent sorting algorithms. */
extern PPGSortInfo sort_infos[];

#endif
