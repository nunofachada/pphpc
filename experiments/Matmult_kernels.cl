typedef struct matDims {
	uint rowsA;
	uint colsA;
	uint rowsB;
	uint colsB;
} MAT_DIMS;

/*
 * Matmult kernel non-optimized
 */
__kernel void matmult1(__global int * A, __global int * B, __global int * C, const MAT_DIMS d)
{
	// Matrix position for this work-item
	uint row = get_global_id(1); 
	uint col = get_global_id(0);
	
	// Multiply!
	if ((row < d.rowsA) && (col < d.colsB)) {
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += A[row * d.colsA + i] * B[i * d.colsB + col];
		}
		C[row * d.colsB + col] = sum;
	}
}

/*
 * Matmult kernel optimized for local memory (matrix A reads)
 */
__kernel void matmult2(__global int * A, __global int * B, __global int * C, const MAT_DIMS d, __local int * tileOfA)
{
	// Global matrix position for this work-item
	uint gRow = get_global_id(1); 
	uint gCol = get_global_id(0);

	// Local matrix position for this work-item
	uint lRow = get_local_id(1); 
	uint lCol = get_local_id(0);
		
	// Load of a tile of A into local memory (coalesced for width of work-group, localCols)
	uint localCols = min(get_local_size(0), d.colsB);
	uint loops = (d.colsA % localCols == 0) ? (d.colsA / localCols) : (d.colsA / localCols + 1);
	for (uint i = 0; i < loops; i++) {
		uint tileCol = i * localCols + lCol;
		if (tileCol < d.colsA)
			tileOfA[lRow * d.colsA + tileCol] = A[gRow * d.colsA + tileCol];
	}
		
	// Sync. locally
	barrier(CLK_LOCAL_MEM_FENCE);
		
	// Check if this work-item is within bounds of C to perform multiplication
	if ((gRow < d.rowsA) && (gCol < d.colsB)) {

		// Multiply!
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += tileOfA[lRow * d.colsA + i] * B[i * d.colsB + gCol];
		}
		C[gRow * d.colsB + gCol] = sum;
	}
}

/*
 * Matmult kernel optimized for local memory (matrix A reads, matrix B reads)
 */
__kernel void matmult3(__global int * A, __global int * B, __global int * C, const MAT_DIMS d, __local int * tileOfA, __local int * tileOfB)
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
	uint localCols = min(get_local_size(0), d.colsB);
	loops = (d.colsA % localCols == 0) ? (d.colsA / localCols) : (d.colsA / localCols + 1);
	for (uint i = 0; i < loops; i++) {
		uint tileCol = i * localCols + lCol;
		if (tileCol < d.colsA)
			tileOfA[lRow * d.colsA + tileCol] = A[gRow * d.colsA + tileCol];
	}
		
	// Load of a tile of B into local memory (coalesced for width of work-group, localCols)
	uint localRows = min(get_local_size(1), d.rowsB);
	loops = (d.rowsB % localRows == 0) ? (d.rowsB / localRows) : (d.rowsB / localRows + 1);
	for (uint i = 0; i < loops; i++) {
		uint localPos = i * localCols * localRows + lRow * localCols + lCol;
		uint globalRow = i * localRows + lRow;
		uint globalCol = get_group_id(0) * localCols + lCol;
		if ((globalRow < d.rowsB) && (globalCol < d.colsB))
			tileOfB[localPos] = B[globalRow * d.colsB + globalCol];
	}

	// Sync. locally
	barrier(CLK_LOCAL_MEM_FENCE);
		
	// Check if this work-item is within bounds of C to perform multiplication
	if ((gRow < d.rowsA) && (gCol < d.colsB)) {

		// Multiply!
		int sum = 0;
		for (uint i = 0; i < d.colsA; i++) {
			sum += tileOfA[lRow * d.colsA + i] * tileOfB[i * localCols + lCol];
		}
		C[gRow * d.colsB + gCol] = sum;
	}
}
