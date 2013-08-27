/** 
 * @file
 * @brief GPU implementation of simple XorShift random number generator.
 * 
 * Based on code available [here](http://www.javamex.com/tutorials/random_numbers/xorshift.shtml).
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef uint4 rng_state;

/**
 * @brief RNG utility function, not to be called directly from kernels.
 * 
 * @param states Array of RNG states.
 * @return The next pseudorandom value from this random number 
 * generator's sequence.
 */
uint randomNext( __global rng_state *states) {

	// Get state index
	uint index = getWorkitemIndex();
	// Get current state
	rng_state state = states[index];
	
	// Update state
	uint t = state.x ^ (state.x << 11);
	state.x = state.y;
	state.y = state.z;
	state.z = state.w;
	state.w = state.w ^ (state.w >> 19) ^ (t ^ (t >> 8));
	
	// Keep state
	states[index] = state;

	// Return value
	return state.w;

}

#endif
