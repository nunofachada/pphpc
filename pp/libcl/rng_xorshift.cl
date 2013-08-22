/** 
 * @file
 * @brief GPU implementation of simple XorShift random number generator.
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"
 
typedef ulong rng_state;

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
	state ^= (state << 13);
	state ^= (state >> 17);
	state ^= (state << 5);
	
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
