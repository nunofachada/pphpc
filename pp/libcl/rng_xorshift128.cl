/** 
 * @file
 * @brief GPU implementation of XorShift random number generator with
 * 128 bit state.
 * 
 * Based on code available [here](http://en.wikipedia.org/wiki/Xorshift).
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef uint4 rng_state;

/**
 * @brief Returns the next pseudorandom value using a xorshift random
 * number generator with 128 bit state.
 * 
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a xorshift random number 
 * generator with 128 bit state.
 */
uint randomNext( __global rng_state *states, uint index) {

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
