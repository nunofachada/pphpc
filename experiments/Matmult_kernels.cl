/*
 * Matmult kernel
 */
__kernel void matmult1(__global int * A, __global int * B, __global int * C, const uint4 mdims)
{
	// Matrix position for this work-item
	uint row = get_global_size(1); //get_group_id(1) * get_local_size(1) + get_local_size(1);
	uint col = get_global_size(0); //get_group_id(0) * get_local_size(0) + get_local_size(0);
	
	// Multiply!
	if (row * col < mdims.x * mdims.w) {
		int sum = 0;
		for (uint i = 0; i < mdims.y; i++) {
			sum = A[row * mdims.y + i] * B[i * mdims.w + col];
		}
		C[row * mdims.w + col] = sum;
	}
}

