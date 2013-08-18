/** 
 * @file
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

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


