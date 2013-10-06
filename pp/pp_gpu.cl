/** 
 * @file
 * @brief OpenCL GPU kernels and data structures for PredPrey simulation.
 * 
 * The kernels in this file expect the following preprocessor defines:
 * 
 * * VW_INT - Vector size used for integers 
 * * VW_CHAR - Vector size used for chars 
 * * REDUCE_GRASS_NUM_WORKGROUPS - Number of work groups in grass reduction step 1 (equivalent to get_num_groups(0)), but to be used in grass reduction step 2.
 * * MAX_LWS - Maximum local work size used in simulation.
 * * CELL_NUM - Number of cells in simulation
 * * MAX_AGENTS - Maximum allowed agents in the simulation
 * 
 * * INIT_SHEEP - Initial number of sheep.
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

#include "pp_common.cl"
#include "libcl/sort.cl"

#define SHEEP_ID 0x1
#define WOLF_ID 0x2

/* Char vector width pre-defines */
#if VW_CHAR == 1
	#define VW_CHAR_SUM(x) (x)
	#define convert_ucharx(x) convert_uchar(x)
	typedef uchar ucharx;
#elif VW_CHAR == 2
	#define VW_CHAR_SUM(x) (x.s0 + x.s1)
	#define convert_ucharx(x) convert_uchar2(x)
	typedef uchar2 ucharx;
#elif VW_CHAR == 4
	#define VW_CHAR_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	#define convert_ucharx(x) convert_uchar4(x)
	typedef uchar4 ucharx;
#elif VW_CHAR == 8
	#define VW_CHAR_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	#define convert_ucharx(x) convert_uchar8(x)
	typedef uchar8 ucharx;
#elif VW_CHAR == 16
	#define VW_CHAR_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 + x.s8 + x.s9 + x.sa + x.sb + x.sc + x.sd + x.se + x.sf)
	#define convert_ucharx(x) convert_uchar16(x)
	typedef uchar16 ucharx;
#endif

/* Integer vector width pre-defines */
#if VW_INT == 1
	#define VW_INT_SUM(x) (x)
	#define convert_uintx(x) convert_uint(x)
	typedef uint uintx;
#elif VW_INT == 2
	#define VW_INT_SUM(x) (x.s0 + x.s1)
	#define convert_uintx(x) convert_uint2(x)
	typedef uint2 uintx;
#elif VW_INT == 4
	#define VW_INT_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	#define convert_uintx(x) convert_uint4(x)
	typedef uint4 uintx;
#elif VW_INT == 8
	#define VW_INT_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	#define convert_uintx(x) convert_uint8(x)
	typedef uint8 uintx;
#elif VW_INT == 16
	#define VW_INT_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 + x.s8 + x.s9 + x.sa + x.sb + x.sc + x.sd + x.se + x.sf)
	#define convert_uintx(x) convert_uint16(x)
	typedef uint16 uintx;
#endif

#define PPG_AG_ENERGY(data) ((data) & 0xFFFF)

#define PPG_AG_TYPE(data) ((data) >> 16)

#define PPG_AG_IS_SHEEP(data) !(((data) & 0xF0000) ^ (SHEEP_ID << 16))
#define PPG_AG_IS_WOLF(data) !(((data) & 0xF0000) ^ (WOLF_ID << 16))

#define PPG_AG_SET(type, energy) (((type) << 16) | ((energy) & 0xFFFF))

#define PPG_AG_REPRODUCE(data) (((data) & 0xF0000) | (((data) & 0xFFFF) / 2))

#define PPG_AG_IS_ALIVE(data) ((data) & 0xFFFF)

#define PPG_AG_DEAD 0

#define PPG_AG_HASH(data, xy) (((PPG_AG_ENERGY(data) == 0) << 31) | ((xy.x) << 15) | (xy.y)) // This assumes 15-bit coordinates at most (32768=x_max=y_max.)
#define PPG_AG_HASH_DEAD -1

#define PPG_CELL_IDX(xy, gid) (xy[gid].y * GRID_X + xy[gid].x)

/**
 * @brief Initialize grid cells. 
 * 
 * @param grass_alive "Is grass alive?" array.
 * @param grass_timer Grass regrowth timer array.
 * @param seeds RNG seeds.
 * */
__kernel void initCell(
			__global uint *grass, 
			__global rng_state *seeds)
{
	
	/* Grid position for this work-item */
	uint gid = get_global_id(0);

	/* Check if this workitem will initialize a cell. This might include
	 * padding cells if vectors are used. */
	if (gid < PP_NEXT_MULTIPLE(CELL_NUM, VW_INT)) {
		/* Cells within bounds may be dead or alive with 50% chance.
		 * Padding cells (gid >= CELL_NUM) are always dead. */
		uint is_alive = select((uint) 0, (uint) randomNextInt(seeds, 2), gid < CELL_NUM);
		/* If cell is alive, value will be zero. Otherwise, randomly
		 * determine a counter value. */
		grass[gid] = select((uint) (randomNextInt(seeds, GRASS_RESTART) + 1), (uint) 0, is_alive);
	}
	
}

/**
 * @brief Initialize agents.
 * 
 * @param xy
 * @param alive
 * @param energy
 * @param type
 * @param hashes
 * @param seeds
 * @param max_agents
 * */
__kernel void initAgent(
			__global ushort2 *xy,
			__global uint *data,
			__global uint *hashes,
			__global rng_state *seeds,
			uint max_agents
) 
{
	/* Agent to be handled by this workitem. */
	uint gid = get_global_id(0);
	
	/* Determine what this workitem will do. */
	if (gid < INIT_SHEEP + INIT_WOLVES) {
		/* This workitem will initialize an alive agent. */
		ushort2 xy_l = (ushort2) (randomNextInt(seeds, GRID_X), randomNextInt(seeds, GRID_Y));
		xy[gid] = xy_l;
		hashes[gid] = PPG_AG_HASH(1, xy_l);
		/* The remaining parameters depend on the type of agent. */
		if (gid < INIT_SHEEP) { 
			/* A sheep agent. */
			data[gid] = PPG_AG_SET(SHEEP_ID, randomNextInt(seeds, SHEEP_GAIN_FROM_FOOD * 2) + 1);
		} else {
			/* A wolf agent. */
			data[gid] = PPG_AG_SET(WOLF_ID, randomNextInt(seeds, WOLVES_GAIN_FROM_FOOD * 2) + 1);
		}
	} else if (gid < max_agents) {
		/* This workitem will initialize a dead agent with no type. */
		data[gid] = PPG_AG_DEAD;
		hashes[gid] = PPG_AG_HASH_DEAD;
	}
	
	/* In commit 00ea5434a83d7aa134a7ecba413e2f2341086630 there is a
	 * streamlined, theoretically faster version of this kernel, with
	 * less divergence and so on, but it is actually slower. */
}


/**
 * @brief Grass kernel.
 * 
 * @param grass_alive
 * @param grass_timer
 * */
__kernel void grass(
			__global uint *grass,
			__global uint2 *agents_index)
{
	/* Grid position for this workitem */
	uint gid = get_global_id(0);

	/* Check if this workitem will do anything */
	if (gid < CELL_NUM) {
		
		uint grass_l = grass[gid];
		/* Decrement counter if grass is dead */
		grass[gid] = select((uint) 0, grass_l - 1, grass_l);
		
		/* Reset cell start and finish. */
		agents_index[gid] = (uint2) (MAX_AGENTS, MAX_AGENTS);
	}
}

/**
 * @brief Grass reduction kernel, part 1.
 * 
 * @param grass_alive
 * @param partial_sums
 * @param reduce_grass_global
 * */
__kernel void reduceGrass1(
			__global uintx *grass,
			__local uintx *partial_sums,
			__global uintx *reduce_grass_global) {
				
	/* Global and local work-item IDs */
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	uint global_size = get_global_size(0);
	
	/* Serial sum */
	uintx sum = 0;
	
	/* Serial count */
	uint cellVectorCount = PP_DIV_CEIL(CELL_NUM, VW_INT);
	uint serialCount = PP_DIV_CEIL(cellVectorCount, global_size);
	for (uint i = 0; i < serialCount; i++) {
		uint index = i * global_size + gid;
		if (index < cellVectorCount) {
			sum += 0x1 & convert_uintx(!grass[index]);
		}
	}
	
	/* Put serial sum in local memory */
	partial_sums[lid] = sum; 
	
	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		reduce_grass_global[get_group_id(0)] = partial_sums[0];
	}
		
}

/**
 * @brief Grass reduction kernel, part 2.
 * 
 * @param reduce_grass_global
 * @param partial_sums
 * @param stats
 * */
 __kernel void reduceGrass2(
			__global uintx *reduce_grass_global,
			__local uintx *partial_sums,
			__global PPStatisticsOcl *stats) {
				
	/* Global and local work-item IDs */
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	/* Load partial sum in local memory */
	if (lid < REDUCE_GRASS_NUM_WORKGROUPS)
		partial_sums[lid] = reduce_grass_global[lid];
	else
		partial_sums[lid] = 0;
	
	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	/* Put in global memory */
	if (lid == 0) {
		stats[0].grass = VW_INT_SUM(partial_sums[0]);
	}
		
}

/**
 * @brief Agent reduction kernel, part 1.
 * 
 * @param alive
 * @param type 
 * @param partial_sums
 * @param reduce_agent_global
 * @param max_agents = (stats[0].sheep + stats[0].wolves) * 2 //set in host
 * */
__kernel void reduceAgent1(
			__global uintx *data,
			__local uintx *partial_sums,
			__global uintx *reduce_agent_global,
			uint max_agents) {
				
	/* Global and local work-item IDs */
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	uint global_size = get_global_size(0);
	uint group_id = get_group_id(0);
	
	/* Serial sum */
	uintx sumSheep = 0;
	uintx sumWolves = 0;
	
	/* Serial count */
	uint agentVectorCount = PP_DIV_CEIL(max_agents, VW_INT);
	uint serialCount = PP_DIV_CEIL(agentVectorCount, global_size);
	
	for (uint i = 0; i < serialCount; i++) {
		uint index = i * global_size + gid;
		if (index < agentVectorCount) {
			uintx data_l = data[index];
			uintx is_alive = 0x1 & convert_uintx(((data_l) & 0xFFFF) > 0);
			sumSheep += is_alive & convert_uintx(PPG_AG_IS_SHEEP(data_l));
			sumWolves += is_alive & convert_uintx(PPG_AG_IS_WOLF(data_l));
		}
	}

	
	/* Put serial sum in local memory */
	partial_sums[lid] = sumSheep;
	partial_sums[group_size + lid] = sumWolves;
	
	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
			partial_sums[group_size + lid] += partial_sums[group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		reduce_agent_global[group_id] = partial_sums[0];
		reduce_agent_global[MAX_LWS + group_id] = partial_sums[group_size];
	}
		
}

/**
 * @brief Agent reduction kernel, part 2.
 * 
 * @param reduce_agent_global
 * @param partial_sums
 * @param stats
 * @param num_slots Number of workgroups in step 1.
 * */
 __kernel void reduceAgent2(
			__global uintx *reduce_agent_global,
			__local uintx *partial_sums,
			__global PPStatisticsOcl *stats,
			uint num_slots) {
				
	/* Global and local work-item IDs */
	uint lid = get_local_id(0);
	uint group_size = get_local_size(0);
	
	/* Load partial sum in local memory */
	if (lid < num_slots) {
		partial_sums[lid] = reduce_agent_global[lid];
		partial_sums[group_size + lid] = reduce_agent_global[MAX_LWS + lid];
	} else {
		partial_sums[lid] = 0;
		partial_sums[group_size + lid] = 0;
	}
	
	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
			partial_sums[group_size + lid] += partial_sums[group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	/* Put in global memory */
	if (lid == 0) {
		stats[0].sheep = VW_INT_SUM(partial_sums[0]);
		stats[0].wolves = VW_INT_SUM(partial_sums[group_size]);
	}
		
}

/**
 * @brief Agent movement kernel.
 * 
 * @param xy_g
 * @param alive_g
 * @param energy_g
 * @param hashes
 * @param seeds
 */
__kernel void moveAgent(
			__global ushort2 *xy,
			__global uint *data,
			__global uint *hashes,
			__global rng_state *seeds)
{
	
	ushort2 xy_op[5] = {
		(ushort2) (0, 0), 
		(ushort2) (1, 0),
		(ushort2) (-1, 0),
		(ushort2) (0, 1),
		(ushort2) (0, -1)
	};
	
	/* Global id for this work-item */
	uint gid = get_global_id(0);

	/* Load agent state locally. */
	uint data_l = data[gid];

	/* Only perform if agent is alive. */
	if (PPG_AG_IS_ALIVE(data_l)) {
		ushort2 xy_l = xy[gid];
		
		uint direction = randomNextInt(seeds, 5);
		
		/* Perform the actual walk */
		xy_l = (xy_l + xy_op[direction]) % ((ushort2) (GRID_X, GRID_Y));

		/* Lose energy */
		data_l--;
		if (!PPG_AG_IS_ALIVE(data_l))
			data_l = PPG_AG_DEAD;
		
		/* Update global mem */
		xy[gid] = xy_l;
		data[gid] = data_l;
	
		/* Determine and set agent hash (for sorting). */
		hashes[gid] = PPG_AG_HASH(data_l, xy_l);
	}
	
}

/**
 * @brief Find cell start and finish.
 * 
 * @param xy
 * @param alive
 * */
__kernel void findCellIdx(
			__global ushort2 *xy,
			__global uint *data,
			__global uint *hashes,
			__global uint2 *cell_agents_idx) 
{
	/* Agent to be handled by this workitem. */
	uint gid = get_global_id(0);
	
	/* Only perform this if agent is alive. */
	if (PPG_AG_IS_ALIVE(data[gid])) {
		
		/* Find cell where this agent lurks... */
		uint cell_idx = PPG_CELL_IDX(xy, gid);
		
		/* Check if this agent is the start of a cell index. */
		if ((gid == 0) || (hashes[gid] - hashes[max((int) (gid - 1), (int) 0)])) {
			cell_agents_idx[cell_idx].s0 = gid;
		}
		/* Check if this agent is the end of a cell index. */
		if (hashes[gid] - hashes[gid + 1]) {
			cell_agents_idx[cell_idx].s1 = gid;
		}
	}
	
}

/**
 * @brief Agents action kernel
 * 
 * @param matrix
 */
__kernel void actionAgent(
			__global uint *grass, 
			__global uint2 *cell_agents_idx,
			__global ushort2 *xy,
			__global uint *data,
			__global rng_state *seeds)
{
	
	/* Global id for this workitem */
	uint gid = get_global_id(0);
	
	/* Get agent for this workitem */
	ushort2 xy_l = xy[gid];
	uint data_l = data[gid];
	
	/* If agent is alive, do stuff */
	if (PPG_AG_IS_ALIVE(data_l)) {
		
		/* Get cell index where agent is */
		uint cell_idx = PPG_CELL_IDX(xy, gid);
		
		/* Reproduction threshold and probability (used further ahead) */
		uchar reproduce_threshold, reproduce_prob;
				
		/* Perform specific agent actions */
		if (PPG_AG_IS_SHEEP(data_l)) { /* Agent is sheep, perform sheep actions. */
		
			/* Set reproduction threshold and probability */
			reproduce_threshold = SHEEP_REPRODUCE_THRESHOLD;
			reproduce_prob = SHEEP_REPRODUCE_PROB;

			/* If there is grass, eat it (and I can be the only one to do so)! */
			if (atomic_cmpxchg(&grass[cell_idx], (uint) 0, GRASS_RESTART) == 0) {
				/* If grass is alive, sheep eats it and gains energy */
				data_l += SHEEP_GAIN_FROM_FOOD;
			}
			
		} else if (PPG_AG_IS_WOLF(data_l)) { /* Agent is wolf, perform wolf actions. */
			
			/* Set reproduction threshold and probability */
			reproduce_threshold = WOLVES_REPRODUCE_THRESHOLD;
			reproduce_prob = WOLVES_REPRODUCE_PROB;

			/* Cycle through agents in this cell */
			uint2 cai = cell_agents_idx[cell_idx];
			for (uint i = cai.s0; i < cai.s1; i++) {
				if (PPG_AG_IS_SHEEP(data[i])) {
					/* If it is a sheep, try to eat it! */
					if (atomic_min(&(data[i]), PPG_AG_DEAD)) {
						/* If wolf catches sheep he's satisfied for now, so let's get out of this loop */
						data_l += WOLVES_GAIN_FROM_FOOD;
						break;
					}
				}
			}

		}
		
		/* Try reproducing this agent if energy > reproduce_threshold */
		if (PPG_AG_ENERGY(data_l) > reproduce_threshold) {
			
			/* Throw dice to see if agent reproduces */
			if (randomNextInt(seeds, 100) < reproduce_prob ) {
				
				/* Agent will reproduce! */
				uint pos_new = get_global_size(0) + gid;
				uint data_new = PPG_AG_REPRODUCE(data_l);
				data[pos_new] = data_new;
				xy[pos_new] = xy_l;
				
				/* Current agent's energy will be halved also */
				data_l =  data_l - PPG_AG_ENERGY(data_new);
				
			}
		}
		
		
		/* My actions only affect my data (energy), so I will only put back data (energy)... */
		data[gid] = data_l;
		
	}
}
