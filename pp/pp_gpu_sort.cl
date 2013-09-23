/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms kernels.
 */

/**
 * @brief A simple bitonic sort kernel.
 * 
 * @param agents
 * @param stage
 * @param step
 */
__kernel void BitonicSort(
				__global PPGSAgent * agents,
				const uint stage,
				const uint step)
{
	/* Global id for this work-item. */
	uint gid = get_global_id(0);

	/* Determine what to compare and possibly swap. */
	uint pair_stride = (uint) 1 << (step - 1);
	uint index1 = gid + (gid / pair_stride) * pair_stride;
	uint index2 = index1 + pair_stride;
	
	/* Get values from global memory. */
	PPGSAgent agent1 = agents[index1];
	PPGSAgent agent2 = agents[index2];
	
	// Determine if ascending or descending
	bool desc = (bool) (0x1 & (gid >> stage - 1));
	// Determine if agent1 > agent2 according to our criteria
	bool gt = 0;

	if (agent2.alive)
	{
		if (!agent1.alive) 
		{
			// If agent is dead, we want to put it in end of list, so we set "greater than" = true
			gt = 1;
		}
		else if (agent1.x > agent2.x)
		{
			// x is the main dimension to check if value1 is "greater than" value2
			gt = 1;
		}
		else if (agent1.x == agent2.x)
		{
			// Break tie by checking y position
			gt = agent1.y > agent2.y;
		}
	}

	/* Perform swap if needed */ 
	if ( (desc && !gt) || (!desc && gt))
	{
		//swap = 1;
		agents[index1] = agent2;
		agents[index2] = agent1;
	}


}
