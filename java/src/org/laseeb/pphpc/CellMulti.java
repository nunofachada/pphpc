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

import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 
 * @author Nuno Fachada
 * Grid cell, which contains grass and agents.
 */
public class CellMulti extends Cell {
	
	/* Structure where to keep current agents. */
	private Set<Agent> agents;
	/* Structure where to put future agents. */
	private ConcurrentHashMap<Integer, Agent> futureAgents;
	/* Structure where to put agents to be removed. */
	private HashSet<Agent> agentsToRemove;
	
	/**
	 * Constructor.
	 */
	public CellMulti(int grassRestart, int numThreads) {
		/* Initialize agent keeping structures. */
		super(grassRestart);
		this.agents = new HashSet<Agent>();
		this.futureAgents = new ConcurrentHashMap<Integer, Agent>(16, 0.75f, numThreads);
		this.agentsToRemove = new HashSet<Agent>();
	}
	
	/**
	 * In the future, put new agent in this cell.
	 * @param agent Agent to put in cell in the future.
	 */
	@Override
	public void putAgentFuture(Agent agent) {
		if (agent.getEnergy() > 0)
			futureAgents.put(agent.hashCode(), agent);
	}
	
	/**
	 * Put new agent in this cell now. Only used for initialization.
	 * @param agent Agent to put in cell now.
	 */
	@Override
	public void putAgentNow(Agent agent) {
		if (agent.getEnergy() > 0)
			agents.add(agent);
	}

	/**
	 * Make future agents the current agents.
	 */
	@Override
	public void futureIsNow() {
		agents.clear();
		agents.addAll(futureAgents.values());
		futureAgents.clear();
	}
	
	/**
	 * Remove agents to be removed.
	 */
	@Override
	public void removeAgentsToBeRemoved() {
		agents.removeAll(agentsToRemove);
		agentsToRemove.clear();
	}
	
	/**
	 * Returns an iterator over agents in this cell.
	 * @return Iterator for agents in this cell.
	 */
	@Override
	public Iterator<Agent> getAgents() {
		return Collections.synchronizedSet(agents).iterator();
	}
	
	/**
	 * Remove agent from this cell.
	 * @param agent Agent to remove from cell.
	 */
	@Override
	public void removeAgent(Agent agent) {
		agentsToRemove.add(agent);
	}
	
	/**
	 * Return grass counter value.
	 * @return Grass counter value.
	 */
	@Override
	public int getGrass() {
		return grass;
	}
	
	/**
	 * Eat grass.
	 */
	@Override
	public void eatGrass() {
		grass = this.getGrassRestart();
	}
	
	/**
	 * Decrement grass counter.
	 */
	@Override
	public void decGrass() {
		grass--;
	}
	
	/**
	 * Set grass counter to a specific value.
	 * @param grass Value to set grass counter.
	 */
	@Override
	public void setGrass(int grass) {
		this.grass = grass;
	}

}
