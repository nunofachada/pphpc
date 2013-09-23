/** 
 * @file
 * @brief PredPrey OpenCL GPU sorting algorithms kernels.
 */

/**
 * @brief A simple bitonic sort kernel.
 * 
 * @param x
 * @param y
 * @param alive
 * @param energy
 * @param type
 * @param hashes
 * @param stage
 * @param step
 */
__kernel void BitonicSort(
			__global ushort *x,
			__global ushort *y,
			__global uchar *alive,
			__global ushort *energy,
			__global uchar *type,
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
		
		ushort x_l = x[index1];
		ushort y_l = y[index1];
		uchar alive_l = alive[index1]; 
		ushort energy_l = energy[index1]; 
		uchar type_l = type[index1]; 
		
		x[index1] = x[index2];
		y[index1] = y[index2];
		alive[index1] = alive[index2]; 
		energy[index1] = energy[index2]; 
		type[index1] = type[index2]; 
		hashes[index1] = hash2;

		x[index2] = x_l;
		y[index2] = y_l;
		alive[index2] = alive_l; 
		energy[index2] = energy_l; 
		type[index2] = type_l; 
		hashes[index2] = hash1;
	}


}
