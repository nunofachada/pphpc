#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <limits.h>

/*
 * RNG utility function, not to be called directly from kernels.
 */
cl_int randomNext(cl_ulong * seed, cl_int bits) {

	// Update seed
	(*seed) = ((*seed) * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	// Return value
	return (cl_int) ((*seed) >> (48 - bits));
}

/*
 * Random number generator. Returns next integer from 0 (including) to n (not including).
 */
cl_int randomNextInt( cl_ulong * seed, cl_int n)
{
	if ((n & -n) == n)  // i.e., n is a power of 2
		return (cl_int) ((n * ((cl_ulong) randomNext(seed, 31))) >> 31);
	cl_int bits, val;
	do {
		bits = randomNext(seed, 31);
		val = bits % n;
	} while (bits - val + (n-1) < 0);
	return val;
}

int main(int argc, char ** argv)
{
	cl_ulong seed = 123;
	for (int i=0; i<500; i++) {
		cl_int x = randomNextInt(&seed, 10221);
		printf("%d (seed: %d) ", x, seed);
	}
	printf("teste ok\n");
	return 0;
}
