/** 
 * @file
 * @brief GPU implementation of a linear congruential pseudorandom 
 * number generator (LCG), as defined by D. H. Lehmer and described by 
 * Donald E. Knuth in The Art of Computer Programming, Volume 3: 
 * Seminumerical Algorithms, section 3.2.1. It is a similar 
 * implementation to Java Random class.
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef ulong rng_state;
 
/**
 * @brief Returns the next pseudorandom value using a LCG random number 
 * generator.
 * 
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a LCG random number 
 * generator.
 */
uint randomNext( __global rng_state *states, uint index) {

	// Assume 32 bits
	uint bits = 32;
	// Get current state
	rng_state state = states[index];
	// Update state
	state = (state * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	// Keep state
	states[index] = state;
	// Return value
	return (uint) (state >> (48 - bits));
}

#endif
