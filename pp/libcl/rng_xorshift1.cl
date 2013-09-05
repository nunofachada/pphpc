/** 
 * @file
 * @brief GPU implementation of simple XorShift random number generator.
 * 
 * Based on code available [here](http://www.javamex.com/tutorials/random_numbers/xorshift.shtml).
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef ulong rng_state;

/**
 * @brief Returns the next pseudorandom value using a xorshift random
 * number generator with 64 bit state.
 * 
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a xorshift random number 
 * generator with 64 bit state.
 */
uint randomNext( __global rng_state *states, uint index) {

	// Get current state
	rng_state state = states[index];
	
	// Update state
	state ^= (state << 21);
	state ^= (state >> 35);
	state ^= (state << 4);
	
	// Keep state
	states[index] = state;

	// Return value
	return convert_uint(state);
	
	// The instruction above should work because of what the OpenCL
	// spec says: "Out-of-Range Behavior: When converting between 
	// integer types, the resulting value for out-of-range inputs will 
	// be equal to the set of least significant bits in the source 
	// operand element that fit in the corresponding destination 
	// element."
	
}

#endif
