/*
 * PPHPC-OCL, an OpenCL implementation of the PPHPC agent-based model
 * Copyright (C) 2015 Nuno Fachada
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
 * OpenCL kernels for the legacy predator-prey OpenCL GPU simulation.
 */

//#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_byte_addressable_store: enable

#define CELL_GRASS_OFFSET 0
#define CELL_NUMPPGSAgentS_OFFSET 1
#define CELL_AGINDEX_OFFSET 2

#define SHEEP_ID 0
#define WOLF_ID 1
#define TOTALPPGSAgentS_ID 3

typedef struct pp_gs_sim_params {
	uint size_x;
	uint size_y;
	uint size_xy;
	uint max_agents;
	uint grass_restart;
	uint grid_cell_space;
} PPGSSimParams;

typedef struct pp_gs_agent {
	uint x;
	uint y;
	uint alive;
	ushort energy;
	ushort type;
} PPGSAgent __attribute__ ((aligned (16)));

/*
 * Agent movement kernel.
 * Also increments iteration number.atomic_
 */
__kernel void RandomWalk(__global PPGSAgent * agents,
				__global clo_statetype * seeds,
				const PPGSSimParams sim_params,
				__global uint * iter)
{
	// Global id for this work-item
	uint gid = get_global_id(0);
	// Pseudo-randomly determine direction
	PPGSAgent agent = agents[gid];
	if (agent.alive)
	{
		uint direction = clo_rng_next_int(seeds, 5);
		// Perform the actual walk
		if (direction == 1)
		{
			agent.x++;
			if (agent.x >= sim_params.size_x) agent.x = 0;
		}
		else if (direction == 2)
		{
			if (agent.x == 0)
				agent.x = sim_params.size_x - 1;
			else
				agent.x--;
		}
		else if (direction == 3)
		{
			agent.y++;
			if (agent.y >= sim_params.size_y) agent.y = 0;
		}
		else if (direction == 4)
		{
			if (agent.y == 0)
				agent.y = sim_params.size_y - 1;
			else
				agent.y--;
		}
		// Lose energy
		agent.energy--;
		if (agent.energy < 1)
			agent.alive = 0;
		// Update global mem
		agents[gid] = agent;
	}
	// Increment iteration
	if (gid == 0) iter[0]++;
}

/*
 * Update agents in grid
 */
__kernel void AgentsUpdateGrid(__global PPGSAgent * agents,
			__global uint * matrix,
			const PPGSSimParams sim_params)
{
	// Global id for this work-item
	uint gid = get_global_id(0);
	// Get my agent
	PPGSAgent agent = agents[gid];
	if (agent.alive) {
		// Update grass matrix by...
		uint index = sim_params.grid_cell_space*(agent.x + sim_params.size_x * agent.y);
		// ...increment number of agents...
		atomic_inc(&matrix[index + CELL_NUMPPGSAgentS_OFFSET]);
		// ...and setting lowest agent vector index with agents in this place.
		atomic_min(&matrix[index + CELL_AGINDEX_OFFSET], gid);
	}
}


/*
 * Bitonic sort kernel
 */
__kernel void BitonicSort(__global PPGSAgent * agents,
				const uint stage,
				const uint step)
{
	// Global id for this work-item
	uint gid = get_global_id(0);

	// Determine what to compare and possibly swap
	uint pair_stride = (uint) 1 << (step - 1);
	uint index1 = gid + (gid / pair_stride) * pair_stride;
	uint index2 = index1 + pair_stride;
	// Get values from global memory
	PPGSAgent agent1 = agents[index1];
	PPGSAgent agent2 = agents[index2];
	// Determine if ascending or descending
	bool desc = (bool) (0x1 & (gid >> stage - 1));
	// Determine if agent1 > agent2 according to our criteria
	bool gt = 0;

	if (agent2.alive)
	{
		if (!agent1.alive)
		{
			// If agent is dead, we want to put it in end of list, so we set "greater than" = true
			gt = 1;
		}
		else if (agent1.x > agent2.x)
		{
			// x is the main dimension to check if value1 is "greater than" value2
			gt = 1;
		}
		else if (agent1.x == agent2.x)
		{
			// Break tie by checking y position
			gt = agent1.y > agent2.y;
		}
	}

	/* Perform swap if needed */
	if ( (desc && !gt) || (!desc && gt))
	{
		//swap = 1;
		agents[index1] = agent2;
		agents[index2] = agent1;
	}


}


/*
 * Grass kernel
 */
__kernel void Grass(__global uint * matrix,
			const PPGSSimParams sim_params)
{
	// Grid position for this work-item
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	// Check if this thread will do anything
	if ((x < sim_params.size_x) && (y < sim_params.size_y)) {
		// Decrement counter if grass is dead
		uint index = sim_params.grid_cell_space*(x + sim_params.size_x * y);
		if (matrix[index] > 0)
			matrix[index]--;
		// Set number of agents in this place to zero
		matrix[index + CELL_NUMPPGSAgentS_OFFSET] = 0;
		matrix[index + CELL_AGINDEX_OFFSET] = sim_params.max_agents;
	}
}


/*
 * Reduction code for grass count kernels. Makes use of the fact that the number of agents is a base of 2.
 */
void reduceAgents(__local uint2 * lcounter, uint lid)
{
	barrier(CLK_LOCAL_MEM_FENCE);
	/* Determine number of stages/loops. */
	uint stages = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	/* Perform loops/stages. */
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) 1 << i; //round(pown(2.0f, i));
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
 * Reduction code for grass count kernels
 */
void reduceGrass(__local uint * lcounter, uint lid)
{
	barrier(CLK_LOCAL_MEM_FENCE);
	/* Determine number of stages/loops. */
//	uint stages = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint lsbits = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint stages = ((1 << lsbits) == get_local_size(0)) ? lsbits : lsbits + 1;
	/* Perform loops/stages. */
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) 1 << i; //round(pown(2.0f, i));
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
 * Count agents part 1.
 */
__kernel void CountAgents1(__global PPGSAgent * agents,
			__global uint2 * gcounter,
			__local uint2 * lcounter)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	if (agents[gid].alive) {
		uint agentType = agents[gid].type;
		if (agentType == 0) {
			lcounter[lid] = (uint2) (1, 0);
		} else {
			lcounter[lid] = (uint2) (0, 1);
		}
	} else {
		lcounter[lid] = (uint2) (0, 0);
	}
	reduceAgents(lcounter, lid);
	if (lid == 0) {
		gcounter[get_group_id(0)] = lcounter[lid];
	}
}
/*
 * Count agents part 2.
 */
__kernel void CountAgents2(__global uint2 * gcounter,
			__local uint2 * lcounter,
			const uint maxgid,
			__global uint * num_agents,
			__global PPStatisticsOcl * stats,
			__global uint * iter)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint wgid = get_group_id(0);

	if (gid < maxgid) {
		lcounter[lid] = gcounter[wgid * get_local_size(0) + lid];
	} else {
		lcounter[lid] = (uint2) (0, 0);
	}
	reduceAgents(lcounter, lid);
	if (lid == 0) {
		gcounter[wgid] = lcounter[lid];
		if ((gid == 0) && (get_num_groups(0) == 1)) {
			stats[iter[0]].sheep = lcounter[lid].x;
			stats[iter[0]].wolves = lcounter[lid].y;
			num_agents[0] = lcounter[lid].x + lcounter[lid].y;
		}
	}
}



/*
 * Count grass part 1.
 */
__kernel void CountGrass1(__global uint * grass,
			__global uint * gcounter,
			__local uint * lcounter,
			const PPGSSimParams sim_params)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	if (gid < sim_params.size_xy) {
		lcounter[lid] = (grass[gid * sim_params.grid_cell_space] == 0 ? 1 : 0);
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
			__global PPStatisticsOcl * stats,
			__global uint * iter)
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
			stats[iter[0]].grass = lcounter[0];
		}
	}


}

/*
 * Agent reproduction function for agents action kernel
 */
PPGSAgent agentReproduction(__global PPGSAgent * agents, PPGSAgent agent, __global PPAgentParamsOcl * params, __global uint * num_agents, __global clo_statetype * seeds)
{
	// Perhaps agent will reproduce if energy > reproduce_threshold
	if (agent.energy > params[agent.type].reproduce_threshold) {
		// Throw some kind of dice to see if agent reproduces
		if (clo_rng_next_int(seeds, 100) < params[agent.type].reproduce_prob ) {
			// Agent will reproduce! Let's see if there is space...
			PPGSAgent newAgent;
			uint position = atomic_inc(num_agents);
			if (position < get_global_size(0) - 2)
			{
				// There is space, lets put new agent!
				newAgent = agent;
				newAgent.energy = newAgent.energy / 2;
				agents[position] = newAgent;
			}
			// Current agent's energy will be halved also
			agent.energy = agent.energy - newAgent.energy;
		}
	}
	return agent;
}

/*
 * Sheep actions
 */
PPGSAgent sheepAction( PPGSAgent sheep,
		__global uint * matrix,
		const PPGSSimParams sim_params,
		__global PPAgentParamsOcl * params)
{
	// If there is grass, eat it (and I can be the only one to do so)!
	uint index = (sheep.x + sheep.y * sim_params.size_x) * sim_params.grid_cell_space;
	uint grassState = atomic_cmpxchg(&matrix[index], (uint) 0, (uint) sim_params.grass_restart);
	if (grassState == 0) {
		// There is grass, sheep eats it and gains energy (if wolf didn't eat her mean while!)
		sheep.energy += params[SHEEP_ID].gain_from_food;
	}
	return sheep;
}

/*
 * Wolf actions
 */
PPGSAgent wolfAction( PPGSAgent wolf,
		__global PPGSAgent * agents,
		__global uint * matrix,
		const PPGSSimParams sim_params,
		__global PPAgentParamsOcl * params)
{
	// Get index of this location
	uint index = (wolf.x + wolf.y * sim_params.size_x) * sim_params.grid_cell_space;
	// Get number of agents here
	uint numAgents = matrix[index + CELL_NUMPPGSAgentS_OFFSET];
	if (numAgents > 0) {
		// Get starting index of agents here from agent array
		uint agentsIndex = matrix[index + CELL_AGINDEX_OFFSET];
		// Cycle through agents
		for (int i = 0; i < numAgents; i++) {
			if (agents[agentsIndex + i].type == SHEEP_ID) {
				// If it is a sheep, try to eat it!
				uint isSheepAlive = atomic_cmpxchg(&(agents[agentsIndex + i].alive), 1, 0);
				if (isSheepAlive) {
					// If I catch sheep, I'm satisfied for now, let's get out of this loop
					wolf.energy += params[WOLF_ID].gain_from_food;
					break;
				}
			}
		}
	}
	// Return wolf after action taken...
	return wolf;
}


/*
 * Agents action kernel
 */
__kernel void AgentAction( __global PPGSAgent * agents,
			__global uint * matrix,
			const PPGSSimParams sim_params,
			__global PPAgentParamsOcl * params,
			__global clo_statetype * seeds,
			__global uint * num_agents)
{
	// Global id for this work-item
	uint gid = get_global_id(0);
	// Get my agent
	PPGSAgent agent = agents[gid];
	// If agent is alive, do stuff
	if (agent.alive) {
		// Perform specific agent actions
		switch (agent.type) {
			case SHEEP_ID : agent = sheepAction(agent, matrix, sim_params, params); break;
			case WOLF_ID : agent = wolfAction(agent, agents, matrix, sim_params, params); break;
			default : break;
		}
		// Try reproducing this agent
		agent = agentReproduction(agents, agent, params, num_agents, seeds);
		// My actions only affect my energy, so I will only put back energy...
		agents[gid].energy = agent.energy;
	}
}
