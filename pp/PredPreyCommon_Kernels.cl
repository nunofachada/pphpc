/** 
 * @file
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

#ifdef PP_RNG_LCG
#include "libcl/rng_lcg.cl"
#elif defined PP_RNG_MWC64X
#include "libcl/rng_mwc64x.cl"
#elif defined PP_RNG_XORSHIFT
#include "libcl/rng_xorshift.cl"
#endif

#define SHEEP_ID 0
#define WOLF_ID 1
#define GRASS_ID 2

typedef struct pp_statistics_ocl {
	uint sheep;
	uint wolves;
	uint grass;
} PPStatisticsOcl;

typedef struct pp_agent_params_ocl {
	uint gain_from_food;
	uint reproduce_threshold;
	uint reproduce_prob; /* between 1 and 100 */
} PPAgentParamsOcl;


