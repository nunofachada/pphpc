/*
 * Matmult kernel non-optimized
 */
__kernel void matmult1(__global int * A, __global int * B, __global int * C, const uint4 mdims)
{
	// Matrix position for this work-item
	uint row = get_global_id(1); 
	uint col = get_global_id(0);
	
	// Multiply!
	if ((row < mdims.x) && (col < mdims.w)) {
		int sum = 0;
		for (uint i = 0; i < mdims.y; i++) {
			sum += A[row * mdims.y + i] * B[i * mdims.w + col];
		}
		C[row * mdims.w + col] = sum;
	}
}

/*
 * Matmult kernel optimized for local memory
 */
__kernel void matmult2(__global int * A, __global int * B, __global int * C, const uint4 mdims, __local int * tileOfA)
{
	// Global matrix position for this work-item
	uint gRow = get_global_id(1); 
	uint gCol = get_global_id(0);

		// Global matrix position for this work-item
		uint lRow = get_local_id(1); 
		uint lCol = get_local_id(0);
		
		// Tile dimension (one of), assumes get_local_size(0) == get_local_size(1) //DOES IT REALLY?
		//uint tileDim = get_local_size(0); // What if local_size(0) > mdims.y (A_COLS)
		uint tileDim = mdims.y; 
		
		// Coalesced load of a tile of A into local memory
		tileOfA[lRow * tileDim + lCol] = A[gRow * mdims.y + lCol]; // What if local_size(0) < mdims.y (A_COLS) ?? lCol will never reach number of columns in A!
		
		// Sync. locally
		barrier(CLK_LOCAL_MEM_FENCE);
		barrier(CLK_GLOBAL_MEM_FENCE);

	// Check if this work-item is within bounds
	if ((gRow < mdims.x) && (gCol < mdims.w)) {


		// Multiply!
		int sum = 0;
		for (uint i = 0; i < tileDim; i++) {
			sum += tileOfA[lRow * tileDim + i] * B[i * mdims.w + gCol];
		}
		C[gRow * mdims.w + gCol] = sum;
	}
}
