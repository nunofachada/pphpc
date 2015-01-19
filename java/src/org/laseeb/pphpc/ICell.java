/*
 * Copyright (c) 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Instituto Superior Técnico nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *     
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

import java.util.List;
import java.util.Random;

/**
 * A PPHPC model cell, part of a larger simulation grid.
 * 
 * @author Nuno Fachada
 */
public interface ICell {

	/**
	 * Is grass alive?
	 * 
	 * @return True if grass is alive, false otherwise.
	 */
	public boolean isGrassAlive();

	/**
	 * Eat grass.
	 */
	public void eatGrass();

	/**
	 * Decrement grass counter.
	 */
	public void regenerateGrass();

	/**
	 * Return the grass restart simulation parameter.
	 * 
	 * @return The grass restart simulation parameter.
	 */
	public int getGrassRestart();

	/**
	 * This method allows the cell to be used in a "foreach" statement,
	 * returning an agent in each iteration.
	 * 
	 * @return An iterable collection for agents in this cell.
	 */
	public Iterable<IAgent> getAgents();

	/**
	 * Put an initial agent in this cell.
	 * 
	 * @param agent Initial agent to put in cell.
	 */
	public void putInitAgent(IAgent agent);

	/**
	 * Put a new agent in this cell. A new agent is a newly-born agent.
	 * 
	 * @param agent New agent to put in cell.
	 */
	public void putNewAgent(IAgent agent);
	
	/**
	 * Put an existing agent in this cell. An existing agent is one that already
	 * exists in the simulation, e.g. an agent which moved in from another cell.
	 * 
	 * @param agent Existing agent to put in cell.
	 */
	public void putExistingAgent(IAgent agent);	
	
	/**
	 * Get agent and grass statistics for this cell.
	 * 
	 * @param stats Statistics object to be populated.
	 */
	public void getStats(IterationStats stats);

	/**
	 * Perform actions for the agents in this cell.
	 * 
	 * @param rng A random number generator for the agent to perform its actions
	 * stochastically.
	 */
	public void agentActions(Random rng);

	/**
	 * Set the neighborhood for this cell.
	 * 
	 * @param neighborhood A list of cells which correspond to the neighborhood
	 * of the current cell.
	 */
	public void setNeighborhood(List<ICell> neighborhood);

	/**
	 * Perform agent movement for the agents in this cell.
	 * 
	 * @param rng A random number generator so that the agents move randomly.
	 */
	public void agentsMove(Random rng);

}