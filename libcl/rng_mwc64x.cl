/** 
 * @file
 * @brief GPU implementation of a random number generator based on a 
 * Multiply-With-Carry (MWC) generator, developed by David B. Thomas
 * from Imperial College London. More information at 
 * http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html.
 */

#include "workitem.cl"

 typedef uint2 rng_state;
 
/** 
 * @brief RNG utility function, not to be called directly from kernels.
 * 
 * @param states Array of RNG states.
 * @return The next pseudorandom value from this random number 
 * generator's sequence.
 */
int randomNext(__global rng_state *states) {

    enum { A=4294883355U};

	// Get state index
	uint index = getWorkitemIndex()
	
	// Unpack the state	
	uint x = states[index].x, c=states[index].y;
	// Calculate the result
	uint res = x^c;       
	// Step the RNG              
	uint hi = mul_hi(x,A);
	x=x*A+c;
	c=hi+(x<c);
	// Pack the state back up
	states[index]=(rng_state) (x, c);
	// Return the next result
	return res;
}

/**
 * @brief Returns next integer from 0 (including) to n (not including).
 * 
 * @param states Array of RNG states.
 * @param n Returned integer is less than this value.
 * @return Returns next integer from 0 (including) to n (not including).
 */
int randomNextInt( __global rng_state *states, 
			int n)
{
	return randomNext(states) % n;
}
