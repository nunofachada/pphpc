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
	return randomNext(states) % n;
}
