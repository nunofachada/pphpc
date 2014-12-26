/*
 * Copyright (c) 2014, Nuno Fachada
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

import java.util.ArrayList;
import java.util.List;

public class Cell {
	
	/* Put agents now behavior. */
	private CellPutAgentBehavior putAgentNowBehavior;
	
	private CellPutAgentBehavior putAgentFutureBehavior;
	
	private CellFutureIsNowPostBehavior futureIsNowBehavior;
	
	/* Iterations for cell restart. */
	private int grassRestart;
		
	/* Structure where to keep current agents. */
	private List<Agent> agents;
	/* Structure where to put agents to be removed. */
	private List<Agent> agentsToRemove;	
	/* Structure where to put future agents. */
	private List<Agent> futureAgents;
	
	/* Grass counter. */
	private int grass;
	
	/**
	 * Constructor.
	 */
	public Cell(int grassRestart, CellPutAgentBehavior putAgentsNowBehavior, 
			CellPutAgentBehavior putAgentsFutureBehavior,
			CellFutureIsNowPostBehavior futureIsNowBehavior) {
		
		this.grassRestart = grassRestart;
		this.putAgentNowBehavior = putAgentsNowBehavior;
		this.putAgentFutureBehavior = putAgentsFutureBehavior;
		this.futureIsNowBehavior = futureIsNowBehavior;
		
		/* Initialize agent keeping structures. */
		this.agents = new ArrayList<Agent>();
		this.futureAgents = new ArrayList<Agent>();
		this.agentsToRemove = new ArrayList<Agent>();
	}

	/**
	 * Return grass counter value.
	 * @return Grass counter value.
	 */
	public int getGrass() {
		return grass;
	}
	
	/**
	 * Eat grass.
	 */
	public void eatGrass() {
		grass = this.getGrassRestart();
	}
	
	/**
	 * Decrement grass counter.
	 */
	public void decGrass() {
		grass--;
	}
	
	/**
	 * Set grass counter to a specific value.
	 * @param grass Value to set grass counter.
	 */
	public void setGrass(int grass) {
		this.grass = grass;
	}

	/**
	 * @return the grassRestart
	 */
	public int getGrassRestart() {
		return grassRestart;
	}

	
	/**
	 * Remove agent from this cell.
	 * @param agent Agent to remove from cell.
	 */
	public void removeAgent(Agent agent) {
		agentsToRemove.add(agent);
	}

	
	/**
	 * Returns an iterator over agents in this cell.
	 * @return Iterator for agents in this cell.
	 */
	public Iterable<Agent> getAgents() {
		return agents;
	}

	
	/**
	 * Remove agents to be removed.
	 */
	public void removeAgentsToBeRemoved() {
		agents.removeAll(agentsToRemove);
		agentsToRemove.clear();
	}

	/**
	 * Make future agents the current agents.
	 */
	public void futureIsNow() {
		List<Agent> aux = agents;
		agents = futureAgents;
		futureAgents = aux;
		futureAgents.clear();
		futureIsNowBehavior.futureIsNowPost(agents);
	}	
	
	/**
	 * Put new agent in this cell now.
	 * @param agent Agent to put in cell now.
	 */
	public void putAgentNow(Agent agent) {
		if (agent.getEnergy() > 0)
			this.putAgentNowBehavior.putAgent(this.agents, agent);
	}
	
	/**
	 * In the future, put new agent in this cell.
	 * @param agent Agent to put in cell in the future.
	 */
	public void putAgentFuture(Agent agent) {
		if (agent.getEnergy() > 0)
			this.putAgentFutureBehavior.putAgent(this.futureAgents, agent);
	}
	
}