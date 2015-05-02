/**
 * @file
 * @brief OpenCL CPU kernels and data structures for PredPrey simulation.
 */

#define SHEEP_ID 0
#define WOLF_ID 1

#define PPCCell_GRASS_OFFSET 0
#define PPCCell_AGINDEX_OFFSET 1

#define MAX_AGENT_PTRS 256

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

/**
 * Remove agent from given cell
 */
void rem_ag_from_cell(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		uint cell_idx,
		uint ag_idx,
		uint prev_ag_idx,
		PPCSimParamsOcl sim_params) {

	/* Determine if agent index is given by a cell or another agent. */
	if (prev_ag_idx == sim_params.null_agent_pointer) {

		/* Agent index given by a cell. */
		matrix[cell_idx].agent_pointer = agents[ag_idx].next;

	} else {

		/* Agent index given by another agent. */
		agents[prev_ag_idx].next = agents[ag_idx].next;
	}

	/* It's the callers responsability of setting agent's energy to
	 * zero in case agent is dead. */
}

/**
 * Add agent to given cell
 */
void add_ag_to_cell(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		uint ag_idx,
		uint cell_idx) {

	/* Put agent in place and update cell. */
	agents[ag_idx].next = matrix[cell_idx].agent_pointer;
	matrix[cell_idx].agent_pointer = ag_idx;

	/* It's the callers responsability that agent has energy > 0. */
}

/**
 * Find a place for the new agent to stay.
 */
uint alloc_ag_idx(__global PPCAgentOcl * agents,
		__global clo_statetype* seeds,
		PPCSimParamsOcl sim_params) {

	/* Index of place to put agent. */
	uint ag_idx;

	/* Find a place for the agent to stay. */
	do {

		/* Random strategy will work well if:
		 * max_agents >> actual number of agents. */
		ag_idx = clo_rng_next_int(seeds, sim_params.max_agents);

	} while (atomic_cmpxchg(&agents[ag_idx].energy,
			(uint) 0, (uint) 1) != 0);

	/*for ( ag_idx = 0; ag_idx < sim_params.max_agents; ag_idx++) {
		if (atomic_cmpxchg(&agents[ag_idx].energy, (uint) 0, (uint) 1) == 0)
			break;
	}*/

	/* Return index of place where to put agent. */
	return ag_idx;
}

void shuffle_agents(__global PPCAgentOcl * agents,
		__global clo_statetype* seeds,
		uint * ag_pointers,
		uint idx) {

	/* Shuffle agents in cell using the Durstenfeld version of
	 * the Fisher-Yates shuffle. */
	for (int i = idx - 1; i > 0; --i) {

		uint j = clo_rng_next_int(seeds, i + 1);

		uint energy = agents[ag_pointers[i]].energy;
		uint type = agents[ag_pointers[i]].type;
		//~ uint action = agents[ag_pointers[i]].action;

		agents[ag_pointers[i]].energy =
			agents[ag_pointers[j]].energy;
		agents[ag_pointers[i]].type =
			agents[ag_pointers[j]].type;
		//~ agents[ag_pointers[i]].action =
			//~ agents[ag_pointers[j]].action;

		agents[ag_pointers[j]].energy = energy;
		agents[ag_pointers[j]].type = type;
		//~ agents[ag_pointers[j]].action = action;
	}
}

/**
 * Get a random neighbor cell, or current cell.
 */
uint random_walk(__global clo_statetype* seeds,
		uint cell_idx,
		PPCSimParamsOcl sim_params) {

	/* Chose a random direction. */
	uint direction = clo_rng_next_int(seeds, 5);

	/* Get x and y positions. */
	uint pos_x = cell_idx % sim_params.size_x;
	uint pos_y = cell_idx / sim_params.size_x;

	/* Select the destination cell based on the chosen direction. */
	switch (direction) {

		case 0:
			/* Don't walk, stay in the same cell. */
			break;

		case 1:
			/* Walk right. */
			if (pos_x + 1 < sim_params.size_x)
				pos_x++;
			else
				pos_x = 0;
			break;

		case 2:
			/* Walk left. */
			if (pos_x > 0)
				pos_x--;
			else
				pos_x = sim_params.size_x - 1;
			break;

		case 3:
			/* Walk down. */
			if (pos_y + 1 < sim_params.size_y)
				pos_y++;
			else
				pos_y = 0;
			break;

		case 4:
			/* Walk up. */
			if (pos_y > 0)
				pos_y--;
			else
				pos_y = sim_params.size_y - 1;
			break;
	}

	/* Return random neighbor cell. */
	return pos_y * sim_params.size_x + pos_x;

}

/**
 * MoveAgentGrowGrass (step1) kernel
 */
__kernel void step1(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		__global clo_statetype* seeds,
		__private uint turn,
		__constant PPCSimParamsOcl* sim_params_ptr) {

	/* Simulation parameters. */
	PPCSimParamsOcl sim_params = *sim_params_ptr;

	/* Determine row to process */
	uint y = turn + get_global_id(0) * sim_params.rows_per_workitem;

	/* Determine start of row. */
	uint idx_start = y * sim_params.size_x;

	/* Determine end of row: if this is not the last work-item
	 * (condition 1) OR this is not the last row to process
	 * (condition 2), then process current row until the end.
	 * Otherwise, process all remaining cells until the end. */
	uint idx_stop = (get_global_id(0) < get_global_size(0) - 1)
					|| (turn < sim_params.rows_per_workitem - 1)
		? idx_start + sim_params.size_x
		: sim_params.size_xy;

	/* Cycle through cells in line */
	for (uint index = idx_start; index < idx_stop; index++) {

		/* *** Grow grass. *** */
		if (matrix[index].grass > 0)
			matrix[index].grass--;

		/* *** Move agents. *** */

		/* Get first agent in cell. */
		uint ag_idx = matrix[index].agent_pointer;

		/* The following indicates that current index was obtained
		 * via cell, and not via agent.next */
		uint prev_ag_idx = sim_params.null_agent_pointer;

		/* Cycle through agents in cell. */
		while (ag_idx != sim_params.null_agent_pointer) {

			/* Get index of next agent. */
			uint next_ag_idx = agents[ag_idx].next;

			/* If agent hasn't moved yet... */
			if (!agents[ag_idx].action) {

				/* Set action as performed. */
				agents[ag_idx].action = 1;

				/* Agent looses energy by moving. */
				agents[ag_idx].energy--;

				/* Let's see if agent has any energy left... */
				if (agents[ag_idx].energy == 0) {

					/* Agent has no energy left, so will die */
					rem_ag_from_cell(agents, matrix, index,
						ag_idx, prev_ag_idx, sim_params);

				} else {

					/* Agent has energy, prompt him to possibly
					 * move */

					/* Get a destination */
					uint neigh_idx = random_walk(
						seeds, index, sim_params);

					/* Let's see if agent wants to move */
					if (neigh_idx != index) {

						/* If agent wants to move, then move him. */
						rem_ag_from_cell(agents, matrix, index,
							ag_idx, prev_ag_idx, sim_params);
						add_ag_to_cell(
							agents, matrix, ag_idx, neigh_idx);

						/* Because this agent moved out of here,
						 * previous agent remains the same */

					} else {

						/* Current agent will not move, as such make
						 * previous agent equal to current agent */
						prev_ag_idx = ag_idx;

					}
				}

			} else {

				/* If current agent did not move, previous agent
				 * becomes current agent */
				prev_ag_idx = ag_idx;

			}

			/* Get next agent, if any */
			ag_idx = next_ag_idx;
		}
	}
}

/*
 * AgentActionsGetStats (step2) kernel
 */
__kernel void step2(__global PPCAgentOcl * agents,
		__global PPCCellOcl * matrix,
		__global clo_statetype* seeds,
		__global PPStatisticsOcl * stats,
		__private uint iter,
		__private uint turn,
		__constant PPCSimParamsOcl* sim_params_ptr,
		__constant PPAgentParamsOcl* agent_params) {

	/* Get simulation parameters localy. */
	PPCSimParamsOcl sim_params = *sim_params_ptr;

	/* Reset partial statistics */
	uint sheep_count = 0;
	uint wolves_count = 0;
	uint grass_count = 0;
	uint tot_sheep_en = 0;
	uint tot_wolves_en = 0;
	uint tot_grass_en = 0;

	/* Array with agent pointers, for shuffling purposes.*/
	uint ag_pointers[MAX_AGENT_PTRS];

	/* Determine row to process. */
	uint y = turn + get_global_id(0) * sim_params.rows_per_workitem;

	/* Determine start of row. */
	uint idx_start = y * sim_params.size_x;

	/* Determine end of row: if this is not the last work-item
	 * (condition 1) OR this is not the last row to process
	 * (condition 2), then process current row until the end.
	 * Otherwise, process all remaining cells until the end. */
	uint idx_stop = (get_global_id(0) < get_global_size(0) - 1)
					|| (turn < sim_params.rows_per_workitem - 1)
		? idx_start + sim_params.size_x
		: sim_params.size_xy;

	/* Cycle through cells in line */
	for (uint index = idx_start; index < idx_stop; index++) {

		/* Pointer for current agent. */
		uint ag_ptr;

		/* *** Shuffle agent list. *** */

		/* Get pointer to first agent. */
		ag_ptr = matrix[index].agent_pointer;
		uint idx = 0;

		/* Copy pointers to an array, and shuffle agents using
		 * that array. */
		while (1) {

			/* Copy next pointer to array. */
			ag_pointers[idx] = ag_ptr;

			/* If this is the last agent... */
			if (ag_ptr == sim_params.null_agent_pointer) {

				/* ...shuffle agent list using the array of
				 * pointers... */
				shuffle_agents(agents, seeds, ag_pointers, idx);

				/* ...and get out. */
				break;
			}

			/* Increment array index. */
			idx++;

			/* If we're over the array limit... */
			if (idx >= MAX_AGENT_PTRS) {

				/* ...shuffle agents currently pointed to by
				 * pointers in the array... */
				shuffle_agents(agents, seeds, ag_pointers, idx);

				/* ...and reset the array index in order to start
				 * over. */
				idx = 0;
			}

			/* Update the agent pointer. */
			ag_ptr = agents[ag_ptr].next;

		}

		/* For each agent in cell */
		ag_ptr = matrix[index].agent_pointer;
		while (ag_ptr != sim_params.null_agent_pointer) {

			/* Get agent from global memory to private memory. */
			PPCAgentOcl agent = agents[ag_ptr];

			/* Set agent action as performed. */
			agent.action = 0;

			/* Agent actions. Agent will only perform actions if
			 * its energy is positive. */
			if (agent.energy > 0) {

				/* Is agent a sheep? */
				if (agent.type == SHEEP_ID) {

					/* If there is grass... */
					if (matrix[index].grass == 0) {

						/* ...eat grass... */
						matrix[index].grass =
							sim_params.grass_restart;

						/* ...and gain energy! */
						agent.energy +=
							agent_params[SHEEP_ID].gain_from_food;
					}

					/* Update sheep stats. */
					sheep_count++;
					tot_sheep_en += agent.energy;

				/* Or is agent a wolf? */
				} else {

					/* Look for sheep... */
					uint local_ag_ptr =
						matrix[index].agent_pointer;
					uint prev_ag_ptr =
						sim_params.null_agent_pointer;

					/* ...while there are agents to look for. */
					while (local_ag_ptr !=
							sim_params.null_agent_pointer) {

						/* Get next agent. */
						uint next_ag_ptr =
							agents[local_ag_ptr].next;

						/* Is next agent a sheep? */
						if (agents[local_ag_ptr].type == SHEEP_ID) {

							/* It is sheep, eat it! */

							/* If sheep already acted, update
							 * sheep stats. */
							if (agents[local_ag_ptr].action == 0) {
							/* Can also be: if (ag_ptr > local_ag_ptr) */

								sheep_count--;
								tot_sheep_en -=
									agents[local_ag_ptr].energy;

							}

							/* Set sheep energy to zero. */
							agents[local_ag_ptr].energy = 0;

							/* Remove sheep from cell. */
							rem_ag_from_cell(agents, matrix,
								index, local_ag_ptr,
								prev_ag_ptr, sim_params);

							/* Increment wolf energy. */
							agent.energy +=
								agent_params[WOLF_ID].gain_from_food;

							/* If previous agent in list is the wolf
							 * currently acting, make sure his next
							 * pointer is updated in local var */
							if (prev_ag_ptr == ag_ptr) {

								agent.next =
									agents[local_ag_ptr].next;

							}

							/* One sheep is enough... */
							break;
						}

						/* Not a sheep, next agent please */
						prev_ag_ptr = local_ag_ptr;
						local_ag_ptr = next_ag_ptr;
					}

					/* Update wolves stats. */
					wolves_count++;
					tot_wolves_en += agent.energy;

				}

				/* Try to reproduce agent. */

				/* Perhaps agent will reproduce if
				 * energy > reproduce_threshold ? */
				if (agent.energy >
						agent_params[agent.type].reproduce_threshold) {

					/* Throw dice to see if agent reproduces */
					if (clo_rng_next_int(seeds, 100) <
							agent_params[agent.type].reproduce_prob) {

						/* Agent will reproduce!
						 * Let's find some space for new agent... */
						uint new_ag_idx = alloc_ag_idx(
							agents, seeds, sim_params);

						/* Create agent with half the energy of
						 * parent and pointing to first agent in
						 * this cell */
						PPCAgentOcl new_ag;
						new_ag.action = 0;
						new_ag.type = agent.type;
						new_ag.energy = agent.energy / 2;

						/* Put new agent in this cell */
						new_ag.next = matrix[index].agent_pointer;
						matrix[index].agent_pointer = new_ag_idx;

						/* Save new agent in agent array */
						agents[new_ag_idx] = new_ag;

						/* Parent's energy will be halved also */
						agent.energy =
							agent.energy - new_ag.energy;

						/* Increment agent count */
						if (agent.type == SHEEP_ID)
							sheep_count++;
						else
							wolves_count++;

						/* I don't touch the energy stats because
						 * the total energy will remain the same. */
					}
				}

				/* Save agent back to global memory. */
				agents[ag_ptr] = agent;
			}

			/* Get next agent. */
			ag_ptr = agent.next;
		}

		/* Update grass stats. */
		if (matrix[index].grass == 0)
			grass_count++;
		tot_grass_en += matrix[index].grass;

	}

	/* Update global stats */
	atomic_add(&stats[iter].sheep, sheep_count);
	atomic_add(&stats[iter].wolves, wolves_count);
	atomic_add(&stats[iter].grass, grass_count);

	atomic_add(&stats[iter].sheep_en, tot_sheep_en);
	atomic_add(&stats[iter].wolves_en, tot_wolves_en);
	atomic_add(&stats[iter].grass_en, tot_grass_en);
}


