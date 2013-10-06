/** 
 * @file
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

#include "libcl/rng.cl"

/**
 * @brief Performs integer division returning the ceiling instead of
 * the floor if it is not an exact division.
 * 
 * @param a Integer numerator.
 * @param b Integer denominator.
 * */
#define PP_DIV_CEIL(a, b) ((a + b - 1) / b)

/**
 * @brief Determines the next multiple of a given divisor which is equal 
 * or larger than a given value.
 * 
 * Both val and div are assumed to be positive integers.
 * 
 * @param value Minimum value.
 * @param divisor The return value must be a multiple of the divisor.
 * */
#define PP_NEXT_MULTIPLE(val, div) ((val) + (div) - (val) % (div))


typedef struct pp_statistics_ocl {
	uint sheep;
	uint wolves;
	uint grass;
} PPStatisticsOcl;

/** @todo This is only required for pp_cpu, but if we pass sim 
 * as compiler params we can remove this altogheter. */
typedef struct pp_agent_params_ocl {
	uint gain_from_food;
	uint reproduce_threshold;
	uint reproduce_prob; /* between 1 and 100 */
} PPAgentParamsOcl;


