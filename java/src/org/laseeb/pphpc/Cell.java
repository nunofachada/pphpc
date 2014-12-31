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

/**
 * Concrete implementation of a PPHPC model cell, part of a larger simulation grid.
 * 
 * @author Nuno Fachada
 *
 */
public class Cell implements ICell {
	
	/* Put agents now behavior. */
	private CellPutAgentBehavior putAgentBehavior;
	
	/* Iterations for cell restart. */
	private int grassRestart;
		
	/* Structure where to keep current agents. */
	private List<IAgent> agents;
	/* Structure where to put agents to be removed. */
	private List<IAgent> existingAgents;	
	/* Structure where to put future agents. */
	private List<IAgent> newAgents;
	
	private List<IAgent> auxAgents;
	
	/* Grass counter. */
	private int grass;

	
	/**
	 * Constructor.
	 */
	public Cell(int grassRestart,
			CellGrassInitStrategy grassInitStrategy,
			CellPutAgentBehavior putAgentsBehavior) {
		
		this.grass = grassInitStrategy.getInitGrass(grassRestart);
		this.grassRestart = grassRestart;
		this.putAgentBehavior = putAgentsBehavior;
		
		/* Initialize agent keeping structures. */
		this.agents = new ArrayList<IAgent>();
		this.newAgents = new ArrayList<IAgent>();
		this.existingAgents = new ArrayList<IAgent>();
		this.auxAgents = new ArrayList<IAgent>();
	}


	@Override
	public boolean isGrassAlive() {
		return this.grass == 0;
	}
	
	@Override
	public void eatGrass() {
		grass = this.getGrassRestart();
	}
	
	@Override
	public void regenerateGrass() {
		grass--;
	}
	
	@Override
	public int getGrassRestart() {
		return grassRestart;
	}

	@Override
	public Iterable<IAgent> getAgents() {
		
		return this.agents;
		
	}

	@Override
	public void putNewAgent(IAgent agent) {
		this.putAgentBehavior.putAgent(this.newAgents, agent);
	}
	
	@Override
	public void putExistingAgent(IAgent agent) {
		this.putAgentBehavior.putAgent(this.existingAgents, agent);
	}
	
	@Override
	public void getStats(PPStats stats) {
		
		/* Grass alive or not? */
		if (this.isGrassAlive())
			stats.incGrass();
		
		this.auxAgents.clear();
		
		/* Previously existing agents. */
		for (int i = 0; i < this.agents.size(); i++) {
			
			IAgent agent = this.agents.get(i);
			
			if (agent.isAlive()) {
				if (agent instanceof Sheep)
					stats.incSheep();
				else if (agent instanceof Wolf)
					stats.incWolves();
				this.auxAgents.add(agent);
			}
		}
		
		/* Newly born agents. */
		for (int i = 0; i < this.newAgents.size(); i++) {

			IAgent agent = this.newAgents.get(i);

			if (agent instanceof Sheep)
				stats.incSheep();
			else if (agent instanceof Wolf)
				stats.incWolves();
			this.auxAgents.add(agent);

		}
		this.newAgents.clear();
		
		/* Swap agents lists. */
		List<IAgent> aux = this.agents;
		this.agents = this.auxAgents;
		this.auxAgents = aux;
		
	}
	
	@Override
	public void agentActions() {
		
		List<IAgent> aux;
		aux = this.agents;
		this.agents = this.existingAgents;
		this.existingAgents = aux;
		this.existingAgents.clear();
		
//		for (IAgent agent : this.agents) {
		for (int i = 0; i < this.agents.size(); i++) {
			
			IAgent agent = this.agents.get(i);
			if (agent.isAlive())
				agent.doPlay(this);
		}
		
	}
	
}