

typedef struct cell {
	cl_uint grass;
	cl_ushort numpreys_start;
	cl_ushort numpreys_end;
} CELL __attribute__ ((aligned (8)));

/*
 * Grass kernel
 */
__kernel void Grass(__global CELL * matrix, 
			const SIM_PARAMS sim_params)
{
	// Grid position for this work-item
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	// Check if this thread will do anything
	if ((x < sim_params.size_x) && (y < sim_params.size_y)) {
		// Decrement counter if grass is dead
		uint index = x + sim_params.size_x * y;
		if (matrix[index].grass > 0)
			matrix[index].grass--;
		// Set number of prey to zero
		matrix[index].numpreys_start = 0;
		matrix[index].numpreys_end = 0;
	}
}
