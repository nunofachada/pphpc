/** 
 * @file
 * @brief GPU implementation of a random number generator based on a 
 * Multiply-With-Carry (MWC) generator, developed by David B. Thomas
 * from Imperial College London. More information at 
 * http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html.
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG

#include "workitem.cl"

typedef uint2 rng_state;
 
/**
 * @brief Returns the next pseudorandom value using a MWC random number 
 * generator.
 * 
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a MWC random number 
 * generator.
 */
uint randomNext(__global rng_state *states, uint index) {

    enum { A=4294883355U};

	// Unpack the state	
	uint x = states[index].x, c = states[index].y;
	// Calculate the result
	uint res = x^c;       
	// Step the RNG              
	uint hi = mul_hi(x,A);
	x= x * A + c;
	c= hi + (x < c);
	// Pack the state back up
	states[index] = (rng_state) (x, c);
	// Return the next result
	return res;
}


#endif
