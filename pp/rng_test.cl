/** 
 * @file
 * @brief Kernels for testing RNGs
 */

#include "libcl/rng.cl"

__kernel void initRng(
		const ulong main_seed,
		__global ulong *seeds)
{
	
	uint gid = get_global_id(0);
	seeds[gid] = main_seed + gid;
	
}

__kernel void testRng(
		__global rng_state *seeds,
		__global uint *result,
		const uint bits)
{
	
	// Grid position for this work-item
	uint gid = get_global_id(0);

#ifdef RNGT_MAXINT
	result[gid] = randomNextInt(seeds, bits);
#else
	result[gid] = randomNext(seeds) >> (32 - bits);
#endif	
}
