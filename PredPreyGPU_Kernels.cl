/* 
 * Expected preprocessor defines:
 * 
 * REDUCE_GRASS_VECSIZE - Vector size used in reductions 
 * REDUCE_GRASS_NUM_WORKITEMS - Number of work items for grass reduction (equivalent to get_global_size(0))
 * CELL_NUM - Number of cells in simulation 
 *  */

/* Grass reduction pre defines */
#if REDUCE_GRASS_VECSIZE == 1
	#define REDUCE_GRASS_ZERO 0
	#define REDUCE_GRASS_FINAL_SUM(x) (x)
	typedef uint uintx;
	typedef uchar ucharx;
#elif REDUCE_GRASS_VECSIZE == 2
	#define REDUCE_GRASS_ZERO (uint2) (0, 0)
	#define REDUCE_GRASS_FINAL_SUM(x) (x.s0 + x.s1)
	typedef uint2 uintx;
	typedef uchar2 ucharx;
#elif REDUCE_GRASS_VECSIZE == 4
	#define REDUCE_GRASS_ZERO (uint4) (0, 0, 0, 0)
	#define REDUCE_GRASS_FINAL_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	typedef uint4 uintx;
	typedef uchar4 ucharx;
#elif REDUCE_GRASS_VECSIZE == 8
	#define REDUCE_GRASS_ZERO (uint8) (0, 0, 0, 0, 0, 0, 0, 0)
	#define REDUCE_GRASS_FINAL_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	typedef uint8 uintx;
	typedef uchar8 ucharx;
#elif REDUCE_GRASS_VECSIZE == 16
	#define REDUCE_GRASS_ZERO (uint16) (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
	#define REDUCE_GRASS_FINAL_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 + x.s8 + x.s9 + x.s10 + x.s11 + x.s12 + x.s13 + x.s14 + x.s15)
	typedef uint16 uintx;
	typedef uchar16 ucharx;
#endif

#define CELL_VECTOR_NUM = CELL_NUM / REDUCE_GRASS_VECSIZE;
#define REDUCE_GRASS_SERIAL_COUNT = CELL_VECTOR_NUM / REDUCE_GRASS_NUM_WORKITEMS;

typedef struct cell {
	uchar* grass_alive;
	ushort* grass_timer;
	ushort* agents_number;
	ushort* agents_index;
} CELL;

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
	uint gid = get_global_id(0);

	// Check if this thread will do anything
	if (gid < CELL_NUM) {
		
		// Decrement counter if grass is dead
		uint index = x + sim_params.size_x * y;
		if (!matrix->grass_alive[index]) {
			ushort timer = --matrix->grass_timer[index];
			if (timer == 0) {
				matrix->grass_alive[index] = 1;
			}
		} else if (randomNextInt(seeds, 10) < 1) {
			// REMOVE THIS
			matrix->grass_alive[index] = 0;
			matrix->grass_timer[index] = randomNextInt(seeds, 30);
		}

	}
}

__kernel void reduceGrass1(
			__global ucharx * grass_alive,
			__local uintx * partial_sums,
			__global uintx * output) {
				
	// Global and local work-item IDs
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	// Serial sum
	uintx sum = REDUCE_GRASS_ZERO;
	
	// Serial count
	for (uint i = 0; i < REDUCE_GRASS_SERIAL_COUNT; i++) {
		uint index = i * REDUCE_GRASS_NUM_WORKITEMS + gid;
		if (index < CELL_VECTOR_NUM) {
			sum += grass_alive[index];
		}
	}
	
	// Put serial sum in local memory
	partial_sums[lid] = sum; 
	
	// Wait for all work items to perform previous operation
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Reduce
	for (uint i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	// Put in global memory
	if (lid == 0) {
		output[get_group_id(0)] = partial_sums[0];
	}
		
}

__kernel void reduceGrass2(
			__global ucharx * grass_alive,
			__local uintx * partial_sums,
			__global uint * total_grass) {
				
	// Global and local work-item IDs
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	// Load partial sum in local memory
	partial_sums[lid] = grass_alive[lid]; 
	
	// Wait for all work items to perform previous operation
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Reduce
	for (uint i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	// Put in global memory
	if (lid == 0) {
		total_grass[0] = REDUCE_GRASS_FINAL_SUM(partial_sums[0]);
	}
		
}





