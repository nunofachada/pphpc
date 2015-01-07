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

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * Concrete implementation of a PPHPC model cell, part of a larger simulation grid.
 * 
 * @author Nuno Fachada
 *
 */
public class Cell implements ICell {
	
	/* Put agents now behavior. */
	private ICellPutAgentStrategy putAgentBehavior;
	
	private ICellGrassInitStrategy grassInitStrategy;
	
	/* Iterations for cell restart. */
	private int grassRestart;
		
	/* List where to keep agents currently being simulated. */
	private List<IAgent> agents;
	
	/* List where to put agents which already exist in simulation
	 * (e.g. moving from another cell). */
	private List<IAgent> existingAgents;	
	
	/* List where to put new agents. */
	private List<IAgent> newAgents;
	
	/* Auxiliary list for gathering agent statistics and incorporating new 
	 * agents in simulation. */
	private List<IAgent> auxAgents;
	
	/* This cell's neighborhood. */
	private List<ICell> neighborhood = null;
	
	/* Grass counter. */
	private int grass;
	
	/**
	 * Constructor.
	 * 
	 * @param grassRestart
	 * @param grassInitStrategy
	 * @param putAgentsBehavior
	 */
	public Cell(int grassRestart, Random rng,
			ICellGrassInitStrategy grassInitStrategy,
			ICellPutAgentStrategy putAgentsBehavior) {
		
		this.grassInitStrategy = grassInitStrategy;
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
		this.grass = this.getGrassRestart();
	}
	
	@Override
	public void regenerateGrass() {
		if (this.grass > 0) {
			this.grass--;
		}
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
	public void getStats(IterationStats stats) {
		
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
	public void agentActions(Random rng) {
		
		List<IAgent> aux;
		aux = this.agents;
		this.agents = this.existingAgents;
		this.existingAgents = aux;
		this.existingAgents.clear();
		
		for (int i = 0; i < this.agents.size(); i++) {
			
			IAgent agent = this.agents.get(i);
			if (agent.isAlive())
				agent.act(this, rng);
		}
		
	}

	@Override
	public void setNeighborhood(List<ICell> neighborhood) {
		if (this.neighborhood == null)
			this.neighborhood = neighborhood;
		else
			throw new IllegalStateException("Cell neighborhood already set!");
		
	}

	@Override
	public void agentsMove(Random rng) {
			

		for (int i = 0; i < this.agents.size(); i++) {
			
			IAgent agent = this.agents.get(i);

			/* Decrement agent energy. */
			agent.decEnergy();

			if (agent.isAlive()) {
				/* Choose direction. */
				int direction = rng.nextInt(this.neighborhood.size());
				
				/* Move agent. */
				this.neighborhood.get(direction).putExistingAgent(agent);
			}
		}
		
		
	}

	@Override
	public void initGrass(Random rng) {
		this.grass = this.grassInitStrategy.getInitGrass(this, rng);
	}

}