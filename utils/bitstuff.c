#include "bitstuff.h"

// Returns the next larger power of 2 of the given value
unsigned int nlpo2(register unsigned int x)
{
	/* If value already is power of 2, return it has is. */
	if ((x&(x-1)) == 0) return x;
	/* If not, determine next larger power of 2. */
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return(x+1);
}

// Returns the the number of one bits in the given value.
unsigned int ones32(register unsigned int x)
{
        /* 32-bit recursive reduction using SWAR...
	   but first step is mapping 2-bit values
	   into sum of 2 1-bit values in sneaky way
	*/
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}
// Returns the trailing Zero Count (i.e. the log2 of a base 2 number)
unsigned int tzc(register int x)
{
        return(ones32((x & -x) - 1));
}
