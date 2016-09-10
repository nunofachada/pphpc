/*
 * PPHPC-OCL, an OpenCL implementation of the PPHPC agent-based model
 * Copyright (C) 2016 Nuno Fachada
 *
 * This file is part of PPHPC-OCL.
 *
 * PPHPC-OCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PPHPC-OCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PPHPC-OCL. If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * @brief OpenCL GPU kernels and data structures for PredPrey simulation.
 *
 * The kernels in this file expect the following preprocessor defines:
 *
 * * VW_GRASS - Vector size used in grass kernel (vector of uints).
 * * VW_REDUCEGRASS - Vector size used in reduce grass kernels (vector of uints)
 * * VW_REDUCEAGENTS - Vector size used in reduce agents kernels (vector of
 *   ulongs or uints)
 * * REDUCE_GRASS_NUM_WORKGROUPS - Number of work groups in grass reduction
 *   step 1 (equivalent to get_num_groups(0)), but to be used in grass reduction
 *   step 2.
 * * MAX_LWS - Maximum local work size used in simulation.
 * * CELL_NUM - Number of cells in simulation.
 * * MAX_AGENTS - Maximum allowed agents in the simulation.
 *
 * * PPG_AG_xx - Specifies the size in memory of each agent (32 or 64 bits).
 * * PPG_RNG_xxx - Specifies the random number generation algorithm to use.
 * * PPG_SORT_xxx - Specifies the sorting algorithm to use.
 *
 * * INIT_SHEEP - Initial number of sheep.
 * * SHEEP_GAIN_FROM_FOOD - Sheep energy gain when eating grass.
 * * SHEEP_REPRODUCE_THRESHOLD - Energy required for sheep to reproduce.
 * * SHEEP_REPRODUCE_PROB - Probability (between 1 and 100) of sheep
 *   reproduction.
 * * INIT_WOLVES - Initial number of wolves.
 * * WOLVES_GAIN_FROM_FOOD - Wolves energy gain when eating sheep.
 * * WOLVES_REPRODUCE_THRESHOLD - Energy required for wolves to reproduce.
 * * WOLVES_REPRODUCE_PROB - Probability (between 1 and 100) of wolves
 *   reproduction.
 * * GRASS_RESTART - Number of iterations that the grass takes to regrow after
 *   being eaten by a sheep.
 * * GRID_X - Number of grid columns (horizontal size, width).
 * * GRID_Y - Number of grid rows (vertical size, height).
 * * ITERS - Number of iterations.
 * */

/* Constants which depend on device endianess. When the agent structure
 * is accessed in half, these will point to the correct location in
 * global memory. */
#ifdef __ENDIAN_LITTLE__

	#define PPG_AG_HI_IDX 1 /**< Logical "high" position (MSBs). */
	#define PPG_AG_LO_IDX 0 /**< Logical "low" position (LSBs). */

#else

	#define PPG_AG_HI_IDX 0 /**< Logical "high" position (MSBs). */
	#define PPG_AG_LO_IDX 1 /**< Logical "low" position (LSBs). */

#endif

/* Macros and type definitions for grass kernel which depend on chosen vector
 * width. */
#if VW_GRASS == 1

	#define VW_GRASS_SUM(x) (x)
	typedef uint grass_uintx;
	typedef ulong grass_ulongx;

#elif VW_GRASS == 2

	#define VW_GRASS_SUM(x) (x.s0 + x.s1)
	typedef uint2 grass_uintx;
	typedef ulong2 grass_ulongx;

#elif VW_GRASS == 4

	#define VW_GRASS_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	typedef uint4 grass_uintx;
	typedef ulong4 grass_ulongx;

#elif VW_GRASS == 8

	#define VW_GRASS_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	typedef uint8 grass_uintx;
	typedef ulong8 grass_ulongx;

#elif VW_GRASS == 16

	#define VW_GRASS_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 \
		+ x.s8 + x.s9 + x.sa + x.sb + x.sc + x.sd + x.se + x.sf)
	typedef uint16 grass_uintx;
	typedef ulong16 grass_ulongx;

#endif

/* Macros and type definitions for grass reduction kernels which depend on
 * chosen vector width. */
#if VW_GRASSREDUCE == 1

	#define VW_GRASSREDUCE_SUM(x) (x)
	#define convert_grassreduce_uintx(x) convert_uint(x)
	typedef uint grassreduce_uintx;

#elif VW_GRASSREDUCE == 2

	#define VW_GRASSREDUCE_SUM(x) (x.s0 + x.s1)
	#define convert_grassreduce_uintx(x) convert_uint2(x)
	typedef uint2 grassreduce_uintx;

#elif VW_GRASSREDUCE == 4

	#define VW_GRASSREDUCE_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	#define convert_grassreduce_uintx(x) convert_uint4(x)
	typedef uint4 grassreduce_uintx;

#elif VW_GRASSREDUCE == 8

	#define VW_GRASSREDUCE_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	#define convert_grassreduce_uintx(x) convert_uint8(x)
	typedef uint8 grassreduce_uintx;

#elif VW_GRASSREDUCE == 16

	#define VW_GRASSREDUCE_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 \
		+ x.s8 + x.s9 + x.sa + x.sb + x.sc + x.sd + x.se + x.sf)
	#define convert_grassreduce_uintx(x) convert_uint16(x)
	typedef uint16 grassreduce_uintx;

#endif

/* Macros and type definitions for agents, which depend on agent size (number
 * of bits). */
#ifdef PPG_AG_64

	/*
	 * 64-bit agents
	 * -------------
	 *
	 * * ffff 0000 0000 0000 - X (16 bits, 0-65535)
	 * * 0000 ffff 0000 0000 - Y (16 bits, 0-65535)
	 * * 0000 0000 fffe 0000 - Hash (15 bits, 0-32767)
	 * * 0000 0000 0001 0000 - Type (1 bit, 0-65535)
	 * * 0000 0000 0000 ffff - Energy (16 bits, 0-65535)
	 *
	 * */

	#define PPG_AG_ENERGY_GET(agent) ((agent) & 0xffff)

	#define PPG_AG_ENERGY_SET(agent, energy) \
		(agent) = ((agent) & 0xffffffffffff0000) | ((energy) & 0xffff)

	#define PPG_AG_ENERGY_ADD(agent, energy) \
		(agent) = ((agent) & 0xffffffffffff0000) | (((agent) + energy) & 0xffff)

	#define PPG_AG_ENERGY_SUB(agent, energy) \
		(agent) = ((agent) & 0xffffffffffff0000) | (((agent) - energy) & 0xffff)

	#define PPG_AG_TYPE_GET(agent) (((agent) >> 16) & 0x1)

	#define PPG_AG_TYPE_SET(agent, type) \
		(agent) = ((agent) & 0xfffffffffffeffff) | (((type) & 0x1) << 16)

	#define PPG_AG_HASH_SET(agent, hash) \
		(agent) = ((agent) & 0xffffffff0001ffff) | (((hash) & 0x7fff) << 17)

	#define PPG_AG_IS_SHEEP(agent) (PPG_AG_TYPE_GET(agent) == SHEEP_ID)

	#define PPG_AG_IS_WOLF(agent) (PPG_AG_TYPE_GET(agent) == WOLF_ID)

	#define PPG_AG_XY_GET(agent) \
		(ushort2) \
			((ushort) ((agent) >> 48), (ushort) (((agent) >> 32) & 0xffff))

	#define PPG_AG_XY_SET(agent, x, y) \
		(agent) = (((ulong) x) << 48) | \
			((((ulong) y) & 0xffff) << 32) | \
			((agent) & 0xffffffff)

	#define PPG_AG_REPRODUCE(agent) \
		((ulong) \
			((agent) & 0xffffffffffff0000) | (PPG_AG_ENERGY_GET(agent) / 2UL))

	#define PPG_AG_DEAD 0xffffffff

	#define PPG_AG_IS_ALIVE(agent) (((agent) >> 32) != PPG_AG_DEAD)

	#define PPG_AG_SET_DEAD(agent) (agent) = 0xffffffffffffffff

	#define PPG_CELL_IDX(agent) \
		((((agent) >> 32) & 0xffff) * GRID_X + ((agent) >> 48))

	#define PPG_AG_STORE_LO(agent, pos, data) \
		data[pos * 2 + PPG_AG_LO_IDX] = (uint) (agent & 0xffffffffU)

	#define PPG_ATOMIC_TRY_KILL(pos, data, data_half) \
		(atomic_or( \
			&(data_half[pos * 2 + PPG_AG_HI_IDX]), PPG_AG_DEAD) \
				!= PPG_AG_DEAD)

	#define convert_uagr(x) convert_ulong(x)
	#define convert_uagr2(x) convert_ulong2(x)
	#define convert_uagr4(x) convert_ulong4(x)
	#define convert_uagr8(x) convert_ulong8(x)
	#define convert_uagr16(x) convert_ulong16(x)

	typedef uint uagr_half;
	typedef ulong uagr;
	typedef ulong2 uagr2;
	typedef ulong4 uagr4;
	typedef ulong8 uagr8;
	typedef ulong16 uagr16;

	typedef long agr;
	typedef long2 agr2;
	typedef long4 agr4;
	typedef long8 agr8;
	typedef long16 agr16;

#elif defined PPG_AG_32

	/*
	 * 64-bit agents
	 * -------------
	 *
	 * * ffc00000 - X (10 bits, 0-1023)
	 * * 003ff000 - Y (10 bits, 0-1023)
	 * * 00000800 - Type (1 bits, 0-1)
	 * * 000007ff - Energy (11 bits, 0-2047)
	 *
	 * */

	#define PPG_AG_ENERGY_GET(agent) ((agent) & 0x7ff)

	#define PPG_AG_ENERGY_SET(agent, energy) \
		(agent) = ((agent) & 0xfffff800) | ((energy) & 0x7ff)

	#define PPG_AG_ENERGY_ADD(agent, energy) \
		(agent) = ((agent) & 0xfffff800) | (((agent) + energy) & 0x7ff)

	#define PPG_AG_ENERGY_SUB(agent, energy) \
		(agent) = ((agent) & 0xfffff800) | (((agent) - energy) & 0x7ff)

	#define PPG_AG_TYPE_GET(agent) (((agent) >> 11) & 0x1)

	#define PPG_AG_HASH_SET(agent, hash) do {} while (0)

	#define PPG_AG_TYPE_SET(agent, type) \
		(agent) = ((agent) & 0xfffff7ff) | (((type) & 0x1) << 11)

	#define PPG_AG_IS_SHEEP(agent) (PPG_AG_TYPE_GET(agent) == SHEEP_ID)

	#define PPG_AG_IS_WOLF(agent) (PPG_AG_TYPE_GET(agent) == WOLF_ID)

	#define PPG_AG_XY_GET(agent) \
		(ushort2) ((ushort) ((agent) >> 22), (ushort) (((agent) >> 12) & 0x3ff))

	#define PPG_AG_XY_SET(agent, x, y) \
		(agent) = (((uint) x) << 22) | \
			((((uint) y) & 0x3ff) << 12) | \
			((agent) & 0xfff)

	#define PPG_AG_REPRODUCE(agent) \
		((uint) ((agent) & 0xfffff800) | (PPG_AG_ENERGY_GET(agent) / 2))

	#define PPG_AG_DEAD 0xffff

	#define PPG_AG_IS_ALIVE(agent) (((agent) >> 16) != PPG_AG_DEAD)

	#define PPG_AG_SET_DEAD(agent) (agent) = 0xffffffff

	#define PPG_CELL_IDX(agent) \
		((((agent) >> 12) & 0x3ff) * GRID_X + ((agent) >> 22))

	#define PPG_AG_STORE_LO(agent, pos, data) \
		data[pos * 2 + PPG_AG_LO_IDX] = (ushort) (agent & 0xffff)

	#define PPG_ATOMIC_TRY_KILL(pos, data, data_half) \
		PPG_AG_IS_ALIVE(atomic_or(&(data[pos]), 0xffffffff))

	#define convert_uagr(x) convert_uint(x)
	#define convert_uagr2(x) convert_uint2(x)
	#define convert_uagr4(x) convert_uint4(x)
	#define convert_uagr8(x) convert_uint8(x)
	#define convert_uagr16(x) convert_uint16(x)

	typedef ushort uagr_half;
	typedef uint uagr;
	typedef uint2 uagr2;
	typedef uint4 uagr4;
	typedef uint8 uagr8;
	typedef uint16 uagr16;

	typedef int agr;
	typedef int2 agr2;
	typedef int4 agr4;
	typedef int8 agr8;
	typedef int16 agr16;

#endif

/* Macros and type definitions for agent reduction kernels which depend on
 * chosen vector width. */
#if VW_AGENTREDUCE == 1

	#define VW_AGENTREDUCE_SUM(x) (x)
	#define convert_agentreduce_uagr(x) convert_uagr(x)
	typedef uagr agentreduce_uagr;
	typedef agr agentreduce_agr;

#elif VW_AGENTREDUCE == 2

	#define VW_AGENTREDUCE_SUM(x) (x.s0 + x.s1)
	#define convert_agentreduce_uagr(x) convert_uagr2(x)
	typedef uagr2 agentreduce_uagr;
	typedef agr2 agentreduce_agr;

#elif VW_AGENTREDUCE == 4

	#define VW_AGENTREDUCE_SUM(x) (x.s0 + x.s1 + x.s2 + x.s3)
	#define convert_agentreduce_uagr(x) convert_uagr4(x)
	typedef uagr4 agentreduce_uagr;
	typedef agr4 agentreduce_agr;

#elif VW_AGENTREDUCE == 8

	#define VW_AGENTREDUCE_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7)
	#define convert_agentreduce_uagr(x) convert_uagr8(x)
	typedef uagr8 agentreduce_uagr;
	typedef agr8 agentreduce_agr;

#elif VW_AGENTREDUCE == 16

	#define VW_AGENTREDUCE_SUM(x) \
		(x.s0 + x.s1 + x.s2 + x.s3 + x.s4 + x.s5 + x.s6 + x.s7 \
		+ x.s8 + x.s9 + x.sa + x.sb + x.sc + x.sd + x.se + x.sf)
	#define convert_agentreduce_uagr(x) convert_uagr16(x)
	typedef uagr16 agentreduce_uagr;
	typedef agr16 agentreduce_agr;

#endif

#define CLO_SORT_ELEM_TYPE uagr

/**
 * Initialize grid cells.
 *
 * @param grass Grass counters (0 means grass is alive).
 * @param seeds RNG seeds.
 * */
__kernel void init_cell(
			__global uint *grass,
			__global clo_statetype *seeds) {

	/* Grid position for this work-item */
	uint gid = get_global_id(0);

	/* Counter variable, by default it's the maximum possible value. */
	uint counter = UINT_MAX;

	/* Check if this workitem will initialize a cell.*/
	if (gid < CELL_NUM) {

		/* Cells within bounds may be dead or alive with 50% chance. */
		uint is_alive =
			select((uint) 0, (uint) clo_rng_next_int(seeds, 2), gid < CELL_NUM);

		/* If cell is alive, value will be zero. Otherwise, randomly
		 * determine a counter value. */
		counter = select(
			(uint) (clo_rng_next_int(seeds, GRASS_RESTART) + 1),
			(uint) 0,
			is_alive);
	}

	/* Initialize cell counter. Padding cells (gid >= CELL_NUM) will
	 * have their counter initialized to UINT_MAX, thus this value will
	 * limit the maximum number of iterations (otherwise padding cells
	 * will become alive, see grass kernel below). */
	grass[gid] = counter;
}

/**
 * Initialize agents.
 *
 * @param data The agent data array.
 * @param seeds RNG seeds.
 * */
__kernel void init_agent(
			__global uagr *data,
			__global clo_statetype *seeds
) {

	/* Agent to be handled by this workitem. */
	size_t gid = get_global_id(0);
	uagr new_agent;
	PPG_AG_SET_DEAD(new_agent);

	/* Determine what this workitem will do. */
	if (gid < (INIT_SHEEP + INIT_WOLVES)) {
		/* This workitem will initialize an alive agent. */
		PPG_AG_XY_SET(new_agent,
			clo_rng_next_int(seeds, GRID_X),
			clo_rng_next_int(seeds, GRID_Y));
		PPG_AG_HASH_SET(new_agent, clo_rng_next_int(seeds, 0x7fff));
		/* The remaining parameters depend on the type of agent. */
		if (gid < INIT_SHEEP) {
			/* A sheep agent. */
			PPG_AG_TYPE_SET(new_agent, SHEEP_ID);
			PPG_AG_ENERGY_SET(new_agent,
				clo_rng_next_int(seeds, SHEEP_GAIN_FROM_FOOD * 2) + 1);
		} else {
			/* A wolf agent. */
			PPG_AG_TYPE_SET(new_agent, WOLF_ID);
			PPG_AG_ENERGY_SET(new_agent,
				clo_rng_next_int(seeds, WOLVES_GAIN_FROM_FOOD * 2) + 1);
		}
	}
	/* Store new agent in global memory. */
	data[gid] = new_agent;

}


/**
 * Grass kernel. Grows grass and resets agent indexes in cell.
 *
 * @param grass Grass counters (0 means grass is alive).
 * @param agents_index Agent start and end indexes in cell.
 * */
__kernel void grass(
			__global grass_uintx * grass,
			__global grass_ulongx * agents_index)
{
	/* Grid position for this workitem */
	size_t gid = get_global_id(0);

	/* Check if this workitem will do anything */
	uint half_index = PP_DIV_CEIL(CELL_NUM, VW_GRASS);
	if (gid < half_index) {

		/* Get grass counter from global memory. */
		grass_uintx grass_l = grass[gid];

		/* Decrement counter if grass is dead. This might also decrement
		 * counters of padding cells (which are initialized to UINT_MAX)
		 * if vw_int > 1. */
		grass[gid] = select((grass_uintx) 0, grass_l - 1, grass_l > 0);

		/* Reset cell start and finish. */
		agents_index[gid] = (grass_ulongx)
			upsample((grass_uintx) MAX_AGENTS, (grass_uintx) MAX_AGENTS);
		/// @todo The above is a long vectorization, and kernels should have a
		/// specific length vectorization before we had two int memory writes,
		/// which were in accordance to int vectorization. Although like this is
		/// faster for small vector sizes, maybe we should separate the "reset
		/// cell start and finish" in another kernel?
	}
}

/**
 * Grass reduction kernel, part 1.
 *
 * @param grass Grass counters (0 means grass is alive).
 * @param partial_sums Workgroup level (shared memory) grass counts.
 * @param reduce_grass_global Global level grass counts.
 * */
__kernel void reduce_grass1(
			__global grassreduce_uintx * grass,
			__local grassreduce_uintx * partial_sums,
			__global grassreduce_uintx * reduce_grass_global) {

	/* Global and local work-item IDs */
	size_t gid = get_global_id(0);
	size_t lid = get_local_id(0);
	size_t group_size = get_local_size(0);
	size_t global_size = get_global_size(0);
	size_t group_id = get_group_id(0);

	/* Serial sum */
	grassreduce_uintx sum_qty = 0;
	grassreduce_uintx sum_en = 0;

	/* Serial count */
	uint cellVectorCount = PP_DIV_CEIL(CELL_NUM, VW_GRASSREDUCE);
	uint serialCount = PP_DIV_CEIL(cellVectorCount, global_size);
	for (uint i = 0; i < serialCount; i++) {
		uint index = i * global_size + gid;
		if (index < cellVectorCount) {
			sum_qty += 0x1 & convert_grassreduce_uintx(!grass[index]);
			sum_en += convert_grassreduce_uintx(grass[index]);
		}
	}

	/* Put serial sums in local memory */
	partial_sums[lid] = sum_qty;
	partial_sums[group_size + lid] = sum_en;

	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
			partial_sums[group_size + lid] +=
				partial_sums[group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		reduce_grass_global[group_id] = partial_sums[0];
		reduce_grass_global[REDUCE_GRASS_NUM_WORKGROUPS + group_id] =
			partial_sums[group_size];
	}

}

/**
 * Grass reduction kernel, part 2.
 *
 * @param reduce_grass_global Global level grass counts.
 * @param partial_sums Workgroup level (shared memory) grass counts.
 * @param stats Final grass count.
 * */
 __kernel void reduce_grass2(
			__global grassreduce_uintx * reduce_grass_global,
			__local grassreduce_uintx * partial_sums,
			__global PPStatisticsOcl * stats) {

	/* Global and local work-item IDs */
	size_t lid = get_local_id(0);
	size_t group_size = get_local_size(0);

	/* Load partial sum in local memory */
	if (lid < REDUCE_GRASS_NUM_WORKGROUPS) {
		partial_sums[lid] = reduce_grass_global[lid];
		partial_sums[group_size + lid] =
			reduce_grass_global[REDUCE_GRASS_NUM_WORKGROUPS + lid];
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
			partial_sums[group_size + lid] +=
				partial_sums[group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		stats[0].grass = VW_GRASSREDUCE_SUM(partial_sums[0]);
		stats[0].grass_en = VW_GRASSREDUCE_SUM(partial_sums[group_size]);
	}

}

/**
 * Agent reduction kernel, part 1.
 *
 * @param data The agent data array.
 * @param partial_sums Workgroup level (shared memory) agent counts.
 * @param reduce_agent_global Global level agent counts.
 * @param max_agents Maximum agents for current interation.
 * */
__kernel void reduce_agent1(
			__global agentreduce_uagr *data,
			__local agentreduce_uagr *partial_sums,
			__global agentreduce_uagr *reduce_agent_global,
			uint max_agents) {

	/* Global and local work-item IDs */
	size_t gid = get_global_id(0);
	size_t lid = get_local_id(0);
	size_t group_size = get_local_size(0);
	size_t global_size = get_global_size(0);
	size_t group_id = get_group_id(0);

	/* Serial sum */
	agentreduce_uagr sumSheep_pop = 0;
	agentreduce_uagr sumWolves_pop = 0;
	agentreduce_uagr sumSheep_en = 0;
	agentreduce_uagr sumWolves_en = 0;

	/* Serial count */
	uint agentVectorCount = PP_DIV_CEIL(max_agents, VW_AGENTREDUCE);
	uint serialCount = PP_DIV_CEIL(agentVectorCount, global_size);

	for (uint i = 0; i < serialCount; i++) {
		uint index = i * global_size + gid;
		if (index < agentVectorCount) {
			agentreduce_uagr data_l = data[index];
			agentreduce_uagr is_alive =
				0x1 & convert_agentreduce_uagr(PPG_AG_IS_ALIVE(data_l));
			agentreduce_uagr is_sheep =
				convert_agentreduce_uagr(PPG_AG_IS_SHEEP(data_l));
			agentreduce_uagr is_wolf =
				convert_agentreduce_uagr(PPG_AG_IS_WOLF(data_l));
			sumSheep_pop += is_alive & is_sheep;
			sumWolves_pop += is_alive & is_wolf;
			sumSheep_en += select(
				(agentreduce_uagr) (0),
				(agentreduce_uagr) (PPG_AG_ENERGY_GET(data_l)),
				(agentreduce_agr) (is_alive && is_sheep));
			sumWolves_en += select(
				(agentreduce_uagr) (0),
				(agentreduce_uagr) (PPG_AG_ENERGY_GET(data_l)),
				(agentreduce_agr) (is_alive && is_wolf));
		}
	}

	/* Put serial sum in local memory */
	partial_sums[lid] = sumSheep_pop;
	partial_sums[group_size + lid] = sumWolves_pop;
	partial_sums[2 * group_size + lid] = sumSheep_en;
	partial_sums[3 * group_size + lid] = sumWolves_en;

	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
			partial_sums[group_size + lid] +=
				partial_sums[group_size + lid + i];
			partial_sums[2 * group_size + lid] +=
				partial_sums[2 * group_size + lid + i];
			partial_sums[3 * group_size + lid] +=
				partial_sums[3 * group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		reduce_agent_global[group_id] = partial_sums[0];
		reduce_agent_global[MAX_LWS + group_id] = partial_sums[group_size];
		reduce_agent_global[2 * MAX_LWS + group_id] =
			partial_sums[2 * group_size];
		reduce_agent_global[3 * MAX_LWS + group_id] =
			partial_sums[3 * group_size];
	}

}

/**
 * Agent reduction kernel, part 2.
 *
 * @param reduce_agent_global Global level agent counts.
 * @param partial_sums Workgroup level (shared memory) agent counts.
 * @param stats Final agents count.
 * @param num_slots Number of workgroups in step 1.
 * */
 __kernel void reduce_agent2(
			__global agentreduce_uagr *reduce_agent_global,
			__local agentreduce_uagr *partial_sums,
			__global PPStatisticsOcl *stats,
			uint num_slots) {

	/* Global and local work-item IDs */
	size_t lid = get_local_id(0);
	size_t group_size = get_local_size(0);

	/* Load partial sum in local memory */
	if (lid < num_slots) {
		partial_sums[lid] = reduce_agent_global[lid];
		partial_sums[group_size + lid] = reduce_agent_global[MAX_LWS + lid];
		partial_sums[2 * group_size + lid] =
			reduce_agent_global[2 * MAX_LWS + lid];
		partial_sums[3 * group_size + lid] =
			reduce_agent_global[3 * MAX_LWS + lid];
	} else {
		partial_sums[lid] = 0;
		partial_sums[group_size + lid] = 0;
		partial_sums[2 * group_size + lid] = 0;
		partial_sums[3 * group_size + lid] = 0;
	}

	/* Wait for all work items to perform previous operation */
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Reduce */
	for (int i = group_size / 2; i > 0; i >>= 1) {
		if (lid < i) {
			partial_sums[lid] += partial_sums[lid + i];
			partial_sums[group_size + lid] +=
				partial_sums[group_size + lid + i];
			partial_sums[2 * group_size + lid] +=
				partial_sums[2 * group_size + lid + i];
			partial_sums[3 * group_size + lid] +=
				partial_sums[3 * group_size + lid + i];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	/* Put in global memory */
	if (lid == 0) {
		stats[0].sheep =
			(uint) VW_AGENTREDUCE_SUM(partial_sums[0]);
		stats[0].wolves =
			(uint) VW_AGENTREDUCE_SUM(partial_sums[group_size]);
		stats[0].sheep_en =
			(uint) VW_AGENTREDUCE_SUM(partial_sums[2 * group_size]);
		stats[0].wolves_en =
			(uint) VW_AGENTREDUCE_SUM(partial_sums[3 * group_size]);
	}

}

/**
 * Agent movement kernel.
 *
 * @param data The agent data array.
 * @param seeds RNG seeds.
 */
__kernel void move_agent(
			__global uagr *data,
			__global clo_statetype *seeds)
{

	/* Global id for this work-item */
	size_t gid = get_global_id(0);

	/* Load agent state locally. */
	uagr data_l = data[gid];

	/* Only perform if agent is alive. */
	if (PPG_AG_IS_ALIVE(data_l)) {

		ushort2 xy_l = PPG_AG_XY_GET(data_l);

		uint direction = clo_rng_next_int(seeds, 5);

		/* Perform the actual walk */

		/* @ALTERNATIVE (instead of the if's below):
		 *
		 * ushort2 xy_op[5] = {
		 *	(ushort2) (0, 0),
		 *	(ushort2) (1, 0),
		 *	(ushort2) (-1, 0),
		 *	(ushort2) (0, 1),
		 *	(ushort2) (0, -1)
		 * };
		 * xy_l = xy_l + xy_op[direction];
		 * xy_l = select(xy_l, (short2) (0, 0), xy_l == ((short2) (GRID_X, GRID_Y)));
		 * xy_l = select(xy_l, (short2) (GRID_X-1, GRID_Y-1), xy_l == ((short2) (-1, -1)));
		 *
		 * It's slower. Requires xy to be short2 instead of ushort2.
		 * */

		if (direction == 1)
		{
			xy_l.x++;
			if (xy_l.x >= GRID_X) xy_l.x = 0;
		}
		else if (direction == 2)
		{
			if (xy_l.x == 0)
				xy_l.x = GRID_X - 1;
			else
				xy_l.x--;
		}
		else if (direction == 3)
		{
			xy_l.y++;
			if (xy_l.y >= GRID_Y) xy_l.y = 0;
		}
		else if (direction == 4)
		{
			if (xy_l.y == 0)
				xy_l.y = GRID_Y - 1;
			else
				xy_l.y--;
		}

		/* Lose energy */
		PPG_AG_ENERGY_SUB(data_l, 1);
		/* Check if energy is zero... */
		if (PPG_AG_ENERGY_GET(data_l) == 0) {
			/* If so, "kill" agent... */
			PPG_AG_SET_DEAD(data_l);
		} else {
			/* Otherwise update agent location. */
			PPG_AG_XY_SET(data_l, xy_l.x, xy_l.y);
			PPG_AG_HASH_SET(data_l, clo_rng_next_int(seeds, 0x7fff));
		}
		/* Update global mem */
		data[gid] = data_l;

	}

}

/**
 * Find cell start and finish.
 *
 * The cell_agents_idx array is used as uint instead of uint2 because
 * it makes accessing global memory easier, as most likely this kernel
 * will only have to write to an 32-bit address instead of the full
 * 64-bit space occupied by a start and end index.
 *
 * @param data The agent data array.
 * @param cell_agents_idx The agents index in cell array.
 * */
__kernel void find_cell_idx(
			__global uagr *data,
			__global uint *cell_agents_idx)
{

	/* Agent to be handled by this workitem. */
	size_t gid = get_global_id(0);

	uagr data_l = data[gid];

	/* Only perform this if agent is alive. */
	if (PPG_AG_IS_ALIVE(data_l)) {

		/* Find cell where this agent is located... */
		uint cell_idx = 2 * PPG_CELL_IDX(data_l);

		/* Check if this agent is the start of a cell index. */
		ushort2 xy_current = PPG_AG_XY_GET(data_l);
		ushort2 xy_prev = PPG_AG_XY_GET(data[max((int) (gid - 1), (int) 0)]);
		ushort2 xy_next = PPG_AG_XY_GET(data[gid + 1]);

		ushort2 diff_prev = xy_current - xy_prev;
		ushort2 diff_next = xy_current - xy_next;

		if ((gid == 0) || any(diff_prev != ((ushort2) (0, 0)))) {
			cell_agents_idx[cell_idx] = gid;
		}
		/* Check if this agent is the end of a cell index. */
		if (any(diff_next != ((ushort2) (0, 0)))) {
			cell_agents_idx[cell_idx + 1] = gid;
		}
	}

}

/**
 * Agents action kernel.
 *
 * Wolves try to eat sheep.
 * Sheep try to eat grass.
 * Both types of agent try to reproduce.
 *
 * @param grass Grass counters (0 means grass is alive).
 * @param cell_agents_idx Agent start and end indexes in cell.
 * @param data The agent data array.
 * @param data_half The agent data array, allows direct access to upper or lower
 * half of the agent data.
 * @param seeds RNG seeds.
 */
__kernel void action_agent(
			__global uint *grass,
			__global uint2 *cell_agents_idx,
			__global uagr *data,
			__global uagr_half *data_half,
			__global clo_statetype *seeds)
{

	/* Global id for this workitem */
	size_t gid = get_global_id(0);

	/* Get agent for this workitem */
	uagr data_l = data[gid];

	/* Get cell index where agent is */
	uint cell_idx = PPG_CELL_IDX(data_l);

	/* Reproduction threshold and probability (used further ahead) */
	uchar reproduce_threshold, reproduce_prob;

	/* Perform specific agent actions */

	if (PPG_AG_IS_ALIVE(data_l)) {

		if (PPG_AG_IS_SHEEP(data_l)) {
			/* Agent is sheep, perform sheep actions. */

			/* Set reproduction threshold and probability */
			reproduce_threshold = SHEEP_REPRODUCE_THRESHOLD;
			reproduce_prob = SHEEP_REPRODUCE_PROB;

			/* If there is grass, eat it (and I can be the only one to do so) */
			if (atomic_cmpxchg(&grass[cell_idx], (uint) 0, GRASS_RESTART) == 0)
			{ /// @todo Maybe a atomic_or or something would be faster
				/* If grass is alive, sheep eats it and gains energy */
				PPG_AG_ENERGY_ADD(data_l, SHEEP_GAIN_FROM_FOOD);
			}

		} else if (PPG_AG_IS_WOLF(data_l)) {
			/* Agent is wolf, perform wolf actions. */
			/// @todo Maybe remove if is_wolf, it's always wolf in this case

			/* Set reproduction threshold and probability */
			reproduce_threshold = WOLVES_REPRODUCE_THRESHOLD;
			reproduce_prob = WOLVES_REPRODUCE_PROB;

			/* Cycle through agents in this cell */
			uint2 cai = cell_agents_idx[cell_idx];
			if (cai.s0 < MAX_AGENTS) {
				for (uint i = cai.s0; i <= cai.s1; i++) {
					uagr possibleSheep = data[i];
					if (PPG_AG_IS_SHEEP(possibleSheep)) {
						/* If it is a sheep, try to eat it! */
						if (PPG_ATOMIC_TRY_KILL(i, data, data_half)) {
							/* If wolf catches sheep he's satisfied for now, so
							 *  let's get out of this loop. */
							PPG_AG_ENERGY_ADD(data_l, WOLVES_GAIN_FROM_FOOD);
							break;
						}
					}
				}
			}
		}

		/* Try reproducing this agent if energy > reproduce_threshold */
		if (PPG_AG_ENERGY_GET(data_l) > reproduce_threshold) {

			/* Throw dice to see if agent reproduces */
			if (clo_rng_next_int(seeds, 100) < reproduce_prob) {

				/* Agent will reproduce! */
				size_t pos_new = get_global_size(0) + gid;
				uagr data_new = PPG_AG_REPRODUCE(data_l);
				data[pos_new] = data_new;

				/* Current agent's energy will be halved also */
				PPG_AG_ENERGY_SUB(data_l, PPG_AG_ENERGY_GET(data_new));

			}
		}

		/* @ALTERNATIVE for agent reproduction:
		 * 1 - Create new agents in workgroup local memory using a local
		 * atomic counter to determine new agent index.
		 * 2 - In the end of the workitem put a workgroup local mem
		 * barrier, then push new agents in a coalesced fashion to
		 * global memory using a global atomic counter to determine
		 * the index of the first agent in the group.
		 * - Problems: the additional complexity and the use of atomics
		 * will probably not allow for any performance improvements. */

		/* My actions only affect my data (energy), so I will only put back data
		 * (energy)... */
		PPG_AG_STORE_LO(data_l, gid, data_half);

	}
}
