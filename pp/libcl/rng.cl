#include "workitem.cl"

#ifdef PP_RNG_LCG
#include "libcl/rng_lcg.cl"
#elif defined PP_RNG_MWC64X
#include "libcl/rng_mwc64x.cl"
#elif defined PP_RNG_XORSHIFT
#include "libcl/rng_xorshift1.cl"
#elif defined PP_RNG_XORSHIFT128
#include "libcl/rng_xorshift128.cl"
#endif

/**
 * @brief Returns next integer from 0 (including) to n (not including).
 * 
 * @param states Array of RNG states.
 * @param n Returned integer is less than this value.
 * @return Returns next integer from 0 (including) to n (not including).
 */
uint randomNextInt( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint index = GID1();
	
	// Return next random integer from 0 to n.
	return randomNext(states, index) % n;
}

uint2 randomNextInt2( __global rng_state *states, 
			uint n)
{

	// Get state index
	uint2 index = GID2();

	// Return vector of random integers from 0 to n.
	return (uint2) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n);
}

uint4 randomNextInt4( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint4 index = GID4();

	// Return vector of random integers from 0 to n.
	return (uint4) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n,
					randomNext(states, index.s2) % n,
					randomNext(states, index.s3) % n);
}

uint8 randomNextInt8( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint8 index = GID8();

	// Return vector of random integers from 0 to n.
	return (uint8) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n,
					randomNext(states, index.s2) % n,
					randomNext(states, index.s3) % n,
					randomNext(states, index.s4) % n,
					randomNext(states, index.s5) % n,
					randomNext(states, index.s6) % n,
					randomNext(states, index.s7) % n);
}

