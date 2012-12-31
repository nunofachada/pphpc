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
	if ((row < d.rowsA) && (col < d.colsA)) {
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += A[row * d.colsA + i] * A[i * d.colsA + col];
		}
		C[row * d.colsA + col] = sum;
	}
}

/*
 * Matmult kernel coalesced loads
 */
__kernel void matmult1(__global int * A, __global int * C, const MAT_DIMS d, __local int * rowsOfA, __local int * colsOfA)
{
	// Matrix position for this work-item
	uint row = get_global_id(1); 
	uint col = get_global_id(0);
	
	// Multiply!
	if ((row < d.rowsA) && (col < d.colsA)) {
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += A[row * d.colsA + i] * A[i * d.colsA + col];
		}
		C[row * d.colsA + col] = sum;
	}
}
