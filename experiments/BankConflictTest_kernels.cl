// If STRIDE=16 there will be lots of bank conflicts and kernel will be slower
#define STRIDE 2

/*
 * Testing bank conflicts
 */
__kernel void bankconf(__global int * globalData, __local int * localData)
{
	// Data position for this work-item
	uint gRow = get_global_id(1); 
	uint gCol = get_global_id(0);
	uint lRow = get_local_id(1); 
	uint lCol = get_local_id(0);
	uint gTotCols = get_global_size(0);
	uint lTotCols = get_local_size(0);
	uint lTotElems = get_local_size(0) * get_local_size(1);
	uint gIndex = gRow * gTotCols + gCol;
	uint lIndex = lRow * lTotCols + lCol;
	
	
	// Copy to local memory
	localData[lIndex] = globalData[gIndex];
	
	// Do some conflicts!
	int sum = 0;
	for (uint i = 0; i < 10; i++) {
		sum += localData[lCol * STRIDE + i];
	}

	// Copy to global memory
	globalData[gIndex] = sum;
}
