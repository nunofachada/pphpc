/**
 * @file
 * @brief OpenCL CPU kernels and data structures for PredPrey simulation.
 */

#define SHEEP_ID 0
#define WOLF_ID 1

#define PPCCell_GRASS_OFFSET 0
#define PPCCell_AGINDEX_OFFSET 1

typedef struct pp_c_agent_ocl {
	uint energy;
	uint action;
	uint type;
	uint next;
} PPCAgentOcl __attribute__ ((aligned (16)));

typedef struct pp_c_sim_params_ocl {
	uint size_x;
	uint size_y;
	uint size_xy;
	uint max_agents;
	uint null_agent_pointer;
	uint grass_restart;
	uint rows_per_workitem;
	uint bogus;
} PPCSimParamsOcl __attribute__ ((aligned (32)));

typedef struct pp_c_cell_ocl {
	uint grass;
	uint agent_pointer;
} PPCCellOcl;

/*
 * Remove agent from given cell
 */
void removeAgentFromCell(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		uint cellIndex,
		uint agentIndex,
		uint previousAgentIndex,
		PPCSimParamsOcl sim_params) {

	// Determine if agent index is given by a cell or another agent
	if (previousAgentIndex == sim_params.null_agent_pointer) {
		// Agent index given by a cell
		matrix[cellIndex].agent_pointer = agents[agentIndex].next;
	} else {
		// Agent index given by another agent
		agents[previousAgentIndex].next = agents[agentIndex].next;
	}
	// It's the callers responsability of setting agent's energy to zero in case agent is dead
}

/*
 * Add agent to given cell
 */
void addAgentToCell(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		uint agentIndex,
		uint cellIndex) {
	// Put agent in place and update cell
	agents[agentIndex].next = matrix[cellIndex].agent_pointer;
	matrix[cellIndex].agent_pointer = agentIndex;
	// It's the callers responsability that given agent has energy > 0
}

/*
 * Find a place for the new agent to stay
 */
uint allocateAgentIndex(__global PPCAgentOcl * agents,
			__global ulong * seeds,
			PPCSimParamsOcl sim_params) {
	// Find a place for the agent to stay
	uint agentIndex;
	do {
		// Random strategy will work well if max_agents >> actual number of agents
		agentIndex = clo_rng_next_int(seeds, sim_params.max_agents);
	} while (atomic_cmpxchg(&agents[agentIndex].energy, (uint) 0, (uint) 1) != 0);
	/*for ( agentIndex = 0; agentIndex < sim_params.max_agents; agentIndex++) {
		if (atomic_cmpxchg(&agents[agentIndex].energy, (uint) 0, (uint) 1) == 0)
			break;
	}*/
	return agentIndex;
}

/*
 * Get a random neighbor cell, or current cell
 */
uint getRandomWalkCellIndex(__global ulong * seeds,
				uint cellIndex,
				PPCSimParamsOcl sim_params) {
	// Throw a coin
	uint direction = clo_rng_next_int(seeds, 5);
	int toWalkIndex = cellIndex; // Default is don't walk (case 0)
	// Perform the actual walk
	switch (direction) {
		case 1:
			// right
			toWalkIndex++;
			// destination cell must be on the same row
			if (cellIndex / sim_params.size_x != toWalkIndex / sim_params.size_x)
				toWalkIndex -= (int) sim_params.size_x;
			break;
		case 2:
			// left
			toWalkIndex--;
			// destination cell must be on the same row
			if (cellIndex / sim_params.size_x != toWalkIndex / sim_params.size_x)
				toWalkIndex += (int) sim_params.size_x;
			break;
		case 3:
			// down
			toWalkIndex += (int) sim_params.size_x;
			// destination cell must be inside limits
			if (toWalkIndex >= (int) sim_params.size_xy) {
				toWalkIndex -= (int) sim_params.size_xy;
			}
			break;
		case 4:
			// up
			toWalkIndex -= (int) sim_params.size_x;
			// destination cell must be inside limits
			if (toWalkIndex < 0) {
				toWalkIndex += (int) sim_params.size_xy;
			}
			break;
	}
	// Return value
	return (uint) toWalkIndex;
}

/*
 * Kernel for testing random walk
 */
__kernel void testGetRandomWalkCellIndex(__global int * intarray,
					__global ulong * seeds,
					PPCSimParamsOcl sim_params)
{
	for (uint i = 0; i < 20; i++) {
		intarray[20*get_group_id(0) + i] = getRandomWalkCellIndex(seeds, 10, sim_params);
	}
}


/*
 * MoveAgentGrowGrass (step1) kernel
 */
__kernel void step1(__global PPCAgentOcl * agents,
			__global PPCCellOcl * matrix,
			__global ulong * seeds,
			__private uint turn,
			__constant PPCSimParamsOcl* sim_params_ptr)
{
	PPCSimParamsOcl sim_params = *sim_params_ptr;
	// Determine line to process
	uint y = turn + get_global_id(0) * sim_params.rows_per_workitem;
	// Check if this thread has to process anything
	if (y < sim_params.size_y) {
		// Determine start and end of line
		uint indexStart = y * sim_params.size_x;
		uint indexStop = indexStart + sim_params.size_x;
		// Cycle through cells in line
		for (uint index = indexStart; index < indexStop; index++) {
			// Grow grass
			if (matrix[index].grass > 0)
				matrix[index].grass--;
			// Move agents in cell
			uint agentIndex = matrix[index].agent_pointer;
			uint previousAgentIndex = sim_params.null_agent_pointer; // This indicates that current index was obtained via cell, and not via agent.next
			while (agentIndex != sim_params.null_agent_pointer) {
				uint nextAgentIndex = agents[agentIndex].next;
				// If agent hasn't moved yet...
				if (!agents[agentIndex].action) {
					// Set action as performed
					agents[agentIndex].action = 1;
					// Agent looses energy by moving
					agents[agentIndex].energy--;
					// Let's see if agent has any energy left...
					if (agents[agentIndex].energy == 0) {
						// Agent has no energy left, so will die
						removeAgentFromCell(agents, matrix, index, agentIndex, previousAgentIndex, sim_params);
					} else {
						// Agent has energy, prompt him to possibly move
						// Get a destination
						uint neighIndex = getRandomWalkCellIndex(seeds, index, sim_params);
						// Let's see if agent wants to move
						if (neighIndex != index) {
							// If agent wants to move, then move him.
							removeAgentFromCell(agents, matrix, index, agentIndex, previousAgentIndex, sim_params);
							addAgentToCell(agents, matrix, agentIndex, neighIndex);
							// Because this agent moved out of here, previous agent remains the same
						} else {
							// Current agent will not move, as such make previous agent equal to current agent
							previousAgentIndex = agentIndex;
						}
					}
				} else {
					// If current agent did not move, previous agent becomes current agent
					previousAgentIndex = agentIndex;
				}
				// Get next agent, if any
				agentIndex = nextAgentIndex;
			}
		}
	}
}

/*
 * AgentActionsGetStats (step2) kernel
 */
__kernel void step2(__global PPCAgentOcl * agents,
			__global PPCCellOcl * matrix,
			__global ulong * seeds,
			__global PPStatisticsOcl * stats,
			__private uint iter,
			__private uint turn,
			__constant PPCSimParamsOcl* sim_params_ptr,
			__constant PPAgentParamsOcl* agent_params)
{
	PPCSimParamsOcl sim_params = *sim_params_ptr;
	// Reset partial statistics
	uint sheepCount = 0;
	uint wolvesCount = 0;
	uint grassCount = 0;
	// Determine line to process
	uint y = turn + get_global_id(0) * sim_params.rows_per_workitem;
	// Check if this thread has to process anything
	if (y < sim_params.size_y) {
		// Determine start and end of line
		uint indexStart = y * sim_params.size_x;
		uint indexStop = indexStart + sim_params.size_x;
		// Cycle through cells in line
		for (uint index = indexStart; index < indexStop; index++) {
			// For each agent in cell:
			uint agentPointer = matrix[index].agent_pointer;
			while (agentPointer != sim_params.null_agent_pointer) {
				/* Get agent from global memory to private memory. */
				PPCAgentOcl agent = agents[agentPointer];
				/* Set agent action as performed. */
				agent.action = 0;
				if (agent.energy > 0) {
					/* Agent actions. */
					if (agent.type == SHEEP_ID) {
						// Count sheep
						sheepCount++;
						// If there is grass...
						if (matrix[index].grass == 0) {
							// ...eat grass...
							matrix[index].grass = sim_params.grass_restart;
							// ...and gain energy!
							agent.energy += agent_params[SHEEP_ID].gain_from_food;
						}
					} else {
						// Count wolves
						wolvesCount++;
						// Look for sheep
						uint localAgentPointer = matrix[index].agent_pointer;
						uint previousAgentPointer = sim_params.null_agent_pointer;
						while (localAgentPointer != sim_params.null_agent_pointer) {
							uint nextAgentPointer = agents[localAgentPointer].next;
							// Is sheep?
							if (agents[localAgentPointer].type == SHEEP_ID) {
								// It is sheep, eat it!
								agents[localAgentPointer].energy = 0;
								if (agents[localAgentPointer].action == 0) // Can be: if (agentPointer > localAgentPointer)
									sheepCount--;
								removeAgentFromCell(agents, matrix, index, localAgentPointer, previousAgentPointer, sim_params);
								agent.energy += agent_params[WOLF_ID].gain_from_food;
								// If previous agent in list is the wolf currently acting, make sure his next pointer is updated in local var
								if (previousAgentPointer == agentPointer)
									agent.next = agents[localAgentPointer].next;
								// One sheep is enough...
								break;
							}
							// Not a sheep, next agent please
							previousAgentPointer = localAgentPointer;
							localAgentPointer = nextAgentPointer;
						}
					}

					/* Try to reproduce agent. */

					// Perhaps agent will reproduce if energy > reproduce_threshold
					if (agent.energy > agent_params[agent.type].reproduce_threshold) {
						// Throw some kind of dice to see if agent reproduces
						if (clo_rng_next_int(seeds, 100) < agent_params[agent.type].reproduce_prob ) {
							// Agent will reproduce! Let's find some space...
							uint newAgentIndex = allocateAgentIndex(agents, seeds, sim_params);
							// Create agent with half the energy of parent and pointing to first agent in this cell
							PPCAgentOcl newAgent;
							newAgent.action = 0;
							newAgent.type = agent.type;
							newAgent.energy = agent.energy / 2;
							newAgent.next = matrix[index].agent_pointer;
							// Put new agent in this cell
							matrix[index].agent_pointer = newAgentIndex;
							// Save new agent in agent array
							agents[newAgentIndex] = newAgent;
							// Parent's energy will be halved also
							agent.energy = agent.energy - newAgent.energy;
							// Increment count
							if (agent.type == SHEEP_ID)
								sheepCount++;
							else
								wolvesCount++;
						}
					}
					/* Save agent back to global memory. */
					agents[agentPointer] = agent;
				}
				/* Get next agent. */
				agentPointer = agent.next;
			}
			// Count grass
			if (matrix[index].grass == 0)
				grassCount++;
		}
	}

	// Update global stats
	atomic_add(&stats[iter].sheep, sheepCount);
	atomic_add(&stats[iter].wolves, wolvesCount);
	atomic_add(&stats[iter].grass, grassCount);
}


