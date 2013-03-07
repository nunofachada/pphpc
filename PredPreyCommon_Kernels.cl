typedef struct statistics {
	uint sheep;
	uint wolves;
	uint grass;
} Statistics;

typedef struct agent_params {
	uint gain_from_food;
	uint reproduce_threshold;
	uint reproduce_prob; /* between 1 and 100 */
} AgentParams;

/*
 * RNG utility function, not to be called directly from kernels.
 */
int randomNext( __global ulong * seeds, 
			int bits) {

	// Get seed index
	uint index;
	uint dims = get_work_dim();
	if (dims == 1)
		index = get_global_id(0);
	else if (dims == 2)
		index = get_global_id(0) * get_global_size(0) + get_global_id(1);
	else
		index = get_global_id(0) * get_global_size(0) + get_global_id(1) * get_global_size(1) + get_global_id(2);
	// Get current seed
	ulong seed = seeds[index];
	// Update seed
	seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	// Keep seed
	seeds[index] = seed;
	// Return value
	return (int) (seed >> (48 - bits));
}

/*
 * Random number generator. Returns next integer from 0 (including) to n (not including).
 */
int randomNextInt( __global ulong * seeds, 
			int n)
{
	if ((n & -n) == n)  // i.e., n is a power of 2
		return (int) ((n * ((ulong) randomNext(seeds, 31))) >> 31);
	int bits, val;
	do {
		bits = randomNext(seeds, 31);
		val = bits % n;
	} while (bits - val + (n-1) < 0);
	return val;
}


