/** 
 * @file
 * @brief PredPrey OpenCL GPU simple bitonic sort implementation.
 */

/**
 * @brief A simple bitonic sort kernel.
 * 
 * @param xy
 * @param alive
 * @param energy
 * @param type
 * @param hashes
 * @param stage
 * @param step
 */
__kernel void sbitonicSort(
			__global ushort *xy,
			__global uint *alive,
			__global ushort *energy,
			__global uint *type,
			__global uint *hashes,
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
	uint hash1 = hashes[index1];
	uint hash2 = hashes[index2];
	
	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> stage - 1));
	
	/* Determine it is required to swap the agents. */
	bool swap = (hash1 > hash2) ^ desc;  
	
	/* Perform swap if needed */ 
	if (swap) {
		
		ushort xy_l = xy[index1];
		uchar alive_l = alive[index1]; 
		ushort energy_l = energy[index1]; 
		uchar type_l = type[index1]; 
		
		xy[index1] = xy[index2];
		alive[index1] = alive[index2]; 
		energy[index1] = energy[index2]; 
		type[index1] = type[index2]; 
		hashes[index1] = hash2;

		xy[index2] = xy_l;
		alive[index2] = alive_l; 
		energy[index2] = energy_l; 
		type[index2] = type_l; 
		hashes[index2] = hash1;
	}


}
