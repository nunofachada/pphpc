/** 
 * @file
 * @brief Kernels for testing RNGs
 */

#ifdef PP_RNG_LCG
#include "libcl/rng_lcg.cl"
#elif defined PP_RNG_MWC64X
#include "libcl/rng_mwc64x.cl"
#elif defined PP_RNG_XORSHIFT
#include "libcl/rng_xorshift.cl"
#endif

__kernel void initRng(
		const ulong main_seed,
		__global ulong *seeds)
{
	
	uint gid = get_global_id(0);
	seeds[gid] = main_seed + gid;
	
}

__kernel void testRng(
		__global rng_state *seeds,
		__global uint *result)
{
	
	// Grid position for this work-item
	uint gid = get_global_id(0);

	result[gid] = randomNextInt(seeds, USHRT_MAX);
	
}
