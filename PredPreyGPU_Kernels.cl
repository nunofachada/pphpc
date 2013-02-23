typedef struct cell {
	uint grass;
	ushort numpreys_start;
	ushort numpreys_end;
} CELL __attribute__ ((aligned (8)));

typedef struct sim_params {
	uint size_x;
	uint size_y;
	uint size_xy;
	uint max_agents;
	uint grass_restart;
} SIM_PARAMS;


/*
 * Grass kernel
 */
__kernel void grass(__global CELL * matrix, 
			const SIM_PARAMS sim_params,
			__global ulong * seeds)
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
			
		// REMOVE THIS
		if (randomNextInt(seeds, 10) < 1)
			matrix[index].grass = randomNextInt(seeds, 30);
			
			
		// Set number of prey to zero
		matrix[index].numpreys_start = 0;
		matrix[index].numpreys_end = 0;
	}
}

__kernel void countGrass(__global CELL * grass,
			__global uint * gcounter,
			__local uint * lcounter,
			const SIM_PARAMS sim_params)



// OLD REDUCTION CODE, REMOVE
/*
 * Reduction code for grass count kernels
 */
void reduceGrass(__local uint * lcounter, uint lid) 
{
	barrier(CLK_LOCAL_MEM_FENCE);
	/* Determine number of stages/loops. */
	uint lsbits = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint stages = ((1 << lsbits) == get_local_size(0)) ? lsbits : lsbits + 1;
	/* Perform loops/stages. */
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) 1 << i; 
		uint divisible = (uint) stride * 2;
		if ((lid % divisible) == 0) {
			if (lid + stride < get_local_size(0)) {
				lcounter[lid] += lcounter[lid + stride];
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
}

/*
 * Count grass part 1.
 */
__kernel void CountGrass1(__global CELL * grass,
			__global uint * gcounter,
			__local uint * lcounter,
			const SIM_PARAMS sim_params)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	if (gid < sim_params.size_xy) {
		lcounter[lid] = (grass[gid].grass == 0 ? 1 : 0);
	} else {
		lcounter[lid] = 0;
	}
	reduceGrass(lcounter, lid);
	if (lid == 0)
		gcounter[get_group_id(0)] = lcounter[lid];
}

/*
 * Count grass part 2.
 */
__kernel void CountGrass2(__global uint * gcounter,
			__local uint * lcounter,
			const uint maxgid,
			__global STATS * stats)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint wgid = get_group_id(0);

	if (gid < maxgid)
		lcounter[lid] = gcounter[wgid * get_local_size(0) + lid];
	else
		lcounter[lid] = 0;
	reduceGrass(lcounter, lid);
	if (lid == 0) {
		gcounter[wgid] = lcounter[lid];
		if ((gid == 0) && (get_num_groups(0) == 1)) {
			stats[0].grass = lcounter[0];
			
			// REMOVE THIS
			stats[0].sheep = 0;
			stats[0].wolves = 0;
			
		}
	}


}


