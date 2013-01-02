typedef struct matDims {
	uint rowsA;
	uint colsA;
} MAT_DIMS;

/*
 * Matmult kernel non-optimized
 */
__kernel void matmult1(__global int * A, __global int * C, const MAT_DIMS d)
{
	// Matrix position for this work-item
	uint row = get_global_id(1); 
	uint col = get_global_id(0);
	
	// Multiply!
	if ((row < d.rowsA) && (col < d.rowsA)) {
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += A[row * d.colsA + i] * A[col * d.colsA + i];
		}
		C[row * d.rowsA + col] = sum;
	}
}


/*
 * Matmult kernel coalesced loads
 */
__kernel void matmult2(__global int * A, __global int * C, const MAT_DIMS d, __local int * rowsOfA, __local int * colsOfA)
{
	// Variable used to control reads from global memory into local memory
	uint loops;

	// Global matrix position for this work-item
	uint gRow = get_global_id(1); 
	uint gCol = get_global_id(0);

	// Local matrix position for this work-item
	uint lRow = get_local_id(1); 
	uint lCol = get_local_id(0);
	
	// Load of a tile of A into local memory (coalesced for width of work-group, localCols)
	uint localCols = min(get_local_size(0), d.rowsA);
	loops = (d.colsA % localCols == 0) ? (d.colsA / localCols) : (d.colsA / localCols + 1);
	for (uint i = 0; i < loops; i++) {
		uint tileCol = i * localCols + lCol;
		if (tileCol < d.colsA)
			rowsOfA[lRow * d.colsA + tileCol] = A[gRow * d.colsA + tileCol];
	}

	// Sync. locally
	barrier(CLK_LOCAL_MEM_FENCE);
		
	// Check if this work-item is within bounds of C to perform multiplication
	if ((gRow < d.rowsA) && (gCol < d.rowsA)) {

		// Multiply!
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += rowsOfA[lRow * d.colsA + i] * A[gCol * d.colsA + i];
		}
		C[gRow * d.rowsA + gCol] = sum;
	}
}
