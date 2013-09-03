/** 
 * @file
 * @brief OpenCL GPU kernels and data structures for PredPrey simulation.
 * 
 * The kernels in this file expect the following preprocessor defines:
 * 
 * * VW_INT - Vector size used for integers 
 * * REDUCE_GRASS_NUM_WORKITEMS - Number of work items for grass reduction step 1 (equivalent to get_global_size(0))
 * * REDUCE_GRASS_NUM_WORKGROUPS - Number of work groups for grass reduction step 1 (equivalent to get_num_groups(0))
 * * CELL_NUM - Number of cells in simulation
 * 
 * * INIT_SHEEP . Initial number of sheep.
 * * SHEEP_GAIN_FROM_FOOD - Sheep energy gain when eating grass.
 * * SHEEP_REPRODUCE_THRESHOLD - Energy required for sheep to reproduce.
 * * SHEEP_REPRODUCE_PROB - Probability (between 1 and 100) of sheep reproduction.
 * * INIT_WOLVES - Initial number of wolves.
 * * WOLVES_GAIN_FROM_FOOD - Wolves energy gain when eating sheep.
 * * WOLVES_REPRODUCE_THRESHOLD - Energy required for wolves to reproduce.
 * * WOLVES_REPRODUCE_PROB - Probability (between 1 and 100) of wolves reproduction.
 * * GRASS_RESTART - Number of iterations that the grass takes to regrow after being eaten by a sheep.
 * * GRID_X - Number of grid columns (horizontal size, width). 
 * * GRID_Y - Number of grid rows (vertical size, height). 
 * * ITERS - Number of iterations. 
 * */

#include "PredPreyCommon_Kernels.cl"


/* Grass reduction pre defines */
#if VW_INT == 1
	#define VW_INT_ZERO 0
	#define VW_REDUCE_FINAL_SUM(x) (x)
	#define convert_uintx(x) convert_uint(x)
	typedef uint uintx;
	typedef uchar ucharx;
#elif VW_INT == 2
	#define VW_INT_ZERO (uint2) (0, 0)
	#define VW_REDUCE_FINAL_SUM(x) (x.s0 + x.s1)
	#define convert_uintx(x) convert_uint2(x)
	typedef uint2 uintx;
	typedef uchar2 ucharx;
#elif VW_INT == 4
	#define VW_INT_ZERO (uint4) (0, 0, 0, 0)
	#define VW_REDUCE_FINAL_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	#define convert_uintx(x) convert_uint4(x)
	typedef uint4 uintx;
	typedef uchar4 ucharx;
#elif VW_INT == 8
	#define VW_INT_ZERO (uint8) (0, 0, 0, 0, 0, 0, 0, 0)
	#define VW_REDUCE_FINAL_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	#define convert_uintx(x) convert_uint8(x)
	typedef uint8 uintx;
	typedef uchar8 ucharx;
#endif

#define CELL_VECTOR_NUM() CELL_NUM/VW_INT
#define REDUCE_GRASS_SERIAL_COUNT() ceil(CELL_VECTOR_NUM()/(float) REDUCE_GRASS_NUM_WORKITEMS)

__kernel void initCell(
			__global uchar* grass_alive, 
			__global ushort* grass_timer, 
			__global rng_state * seeds)
{
	
	// Grid position for this work-item
	uint gid = get_global_id(0);

	// Check if this thread will do anything
	if (gid < CELL_NUM) {
		if (randomNextInt(seeds, 2)) {
			// Grass is alive
			grass_alive[gid] = 1;
			grass_timer[gid] = 0;
		} else {
			// Grass is dead
			grass_alive[gid] = 0;
			grass_timer[gid] = randomNextInt(seeds, GRASS_RESTART) + 1;
		}
	}
	
}

/*
 * Grass kernel
 */
__kernel void grass(
			__global uchar* grass_alive, 
			__global ushort* grass_timer)
{
	// Grid position for this work-item
	uint gid = get_global_id(0);

	// Check if this thread will do anything
	if (gid < CELL_NUM) {
		
		// Decrement counter if grass is dead
		if (!grass_alive[gid]) {
			ushort timer = --grass_timer[gid];
			if (timer == 0) {
				grass_alive[gid] = 1;
			}
		}
	}
}

__kernel void reduceGrass1(
			__global ucharx * grass_alive,
			__local uintx * partial_sums,
			__global uintx * reduce_grass_global) {
				
	// Global and local work-item IDs
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	// Serial sum
	uintx sum = VW_INT_ZERO;
	
	// Serial count
	for (uint i = 0; i < REDUCE_GRASS_SERIAL_COUNT(); i++) {
		uint index = i * REDUCE_GRASS_NUM_WORKITEMS + gid;
		if (index < CELL_VECTOR_NUM()) {
			sum += convert_uintx(grass_alive[index]);
		}
	}
	
	// Put serial sum in local memory
	partial_sums[lid] = sum; 
	//partial_sums[lid] = (1, 1, 1, 1);
	
	// Wait for all work items to perform previous operation
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Reduce
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	// Put in global memory
	if (lid == 0) {
		reduce_grass_global[get_group_id(0)] = partial_sums[0];
	}
		
}

__kernel void reduceGrass2(
			__global uintx * reduce_grass_global,
			__local uintx * partial_sums,
			__global PPStatisticsOcl * stats) {
				
	// Global and local work-item IDs
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	// Load partial sum in local memory
	if (lid < REDUCE_GRASS_NUM_WORKGROUPS)
		partial_sums[lid] = reduce_grass_global[lid];
	else
		partial_sums[lid] = VW_INT_ZERO;
	
	// Wait for all work items to perform previous operation
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Reduce
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	// Put in global memory
	if (lid == 0) {
		stats[0].grass = VW_REDUCE_FINAL_SUM(partial_sums[0]);
		stats[0].sheep = 2;
		stats[0].wolves = 5;
	}
		
}





