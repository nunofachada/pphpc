/*
 * Matmult kernel
 */
__kernel void matmult1(__global int * A, __global int * B, __global int * C, const uint4 mdims)
{
	// Matrix position for this work-item
	uint row = get_global_id(1); 
	uint col = get_global_id(0);
	
	// Multiply!
	if ((row < mdims.z) && (col < mdims.y)) {
		int sum = 0;
		for (uint i = 0; i < mdims.x; i++) {
			sum += A[mdims.y * i + col] * B[mdims.w * row + i];
		}
		C[row * mdims.y + col] = sum;
	}
}

