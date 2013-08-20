/** 
 * @file
 * @brief GPU implementation of simple XorShift random number generator.
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef uint2 rng_state;

/**
 * @brief RNG utility function, not to be called directly from kernels.
 * 
 * @param states Array of RNG states.
 * @param bits Random bits.
 * @return The next pseudorandom value from this random number 
 * generator's sequence.
 */
uint randomNext( __global rng_state *states) {

	// Get state index
	uint index = getWorkitemIndex();
	// Get current state
	rng_state state = states[index];
	
	// Update state
	uint x = state.x * 17 + state.y * 13123;
	state.x = (x<<13) ^ x;
	state.y ^= (x<<7);
	
	// Keep state
	states[index] = state;

	// Return value
	return x * (x * x * 15731 + 74323) + 871483;	
	
}

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
#endif
