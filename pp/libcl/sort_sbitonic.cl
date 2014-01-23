/** 
 * @file
 * @brief PredPrey OpenCL GPU simple bitonic sort implementation.
 */

/**
 * @brief A simple bitonic sort kernel.
 * 
 * @param data Array of agent data.
 * @param stage Current bitonic sort step.
 * @param step Current bitonic sort stage.
 */
__kernel void sbitonicSort(
			__global SORT_ELEM_TYPE *data,
			const uint stage,
			const uint step)
{
	/* Global id for this work-item. */
	uint gid = get_global_id(0);

	/* Determine what to compare and possibly swap. */
	uint pair_stride = (uint) (1 << (step - 1));
	uint index1 = gid + (gid / pair_stride) * pair_stride;
	uint index2 = index1 + pair_stride;
	
	/* Get hashes from global memory. */
	SORT_ELEM_TYPE data1 = data[index1];
	SORT_ELEM_TYPE data2 = data[index2];
	
	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> stage - 1));
	
	/* Determine it is required to swap the agents. */
	bool swap = (data1 > data2) ^ desc;  
	
	/* Perform swap if needed */ 
	if (swap) {
		data[index1] = data2; 
		data[index2] = data1; 
	}


}
