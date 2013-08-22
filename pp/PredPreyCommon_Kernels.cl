/** 
 * @file
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

#include "libcl/rng.cl"

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


