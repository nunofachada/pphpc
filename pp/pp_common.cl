/**
 * @file
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

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
 * @param val Minimum value.
 * @param div The return value must be a multiple of the divisor.
 * */
#define PP_NEXT_MULTIPLE(val, div) ((val) + (div) - (val) % (div))

/** Sheep identifier. */
#define SHEEP_ID 0x0

/** Wolf identifier. */
#define WOLF_ID 0x1

/**
 * Simulation statistics for one iteration.
 * */
typedef struct pp_statistics_ocl {

	/** Number of sheep. */
	uint sheep;

	/** Number of wolves. */
	uint wolves;

	/** Grass count. */
	uint grass;

	/** Total sheep energy. */
	uint sheep_en;

	/** Total wolves energy. */
	uint wolves_en;

	/** Total grass countdown value. */
	uint grass_en;

} PPStatisticsOcl;



