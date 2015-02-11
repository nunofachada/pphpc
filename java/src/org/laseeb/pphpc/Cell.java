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
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * Concrete implementation of a PPHPC model cell, part of a larger simulation grid.
 * 
 * @author Nuno Fachada
 *
 */
public class Cell implements ICell {
	
	/* Put agent strategies. */
	private ICellPutAgentStrategy putInitAgentStrategy;
	private ICellPutAgentStrategy putNewAgentStrategy;
	private ICellPutAgentStrategy putExistingAgentStrategy;
	
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
	 * Create a new grid cell.
	 * 
	 * @param grassRestart Grass restart simulation parameter.
	 * @param initialGrass Initial grass value.
	 * @param putNewAgentsStrategy Strategy for putting new agents in this cell.
	 * @param putExistingAgentStrategy Strategy for putting existing agents in
	 * this cell.
	 */
	public Cell(int grassRestart,
			int initialGrass,
			ICellPutAgentStrategy putInitAgentsStrategy,
			ICellPutAgentStrategy putNewAgentsStrategy,
			ICellPutAgentStrategy putExistingAgentStrategy) {
		
		this.grassRestart = grassRestart;
		this.grass = initialGrass;
		this.putInitAgentStrategy = putInitAgentsStrategy;
		this.putNewAgentStrategy = putNewAgentsStrategy;
		this.putExistingAgentStrategy = putExistingAgentStrategy;
		
		/* Initialize agent keeping structures. */
		this.agents = new ArrayList<IAgent>();
		this.newAgents = new ArrayList<IAgent>();
		this.existingAgents = new ArrayList<IAgent>();
		this.auxAgents = new ArrayList<IAgent>();
	}


	/**
	 * @see ICell#isGrassAlive()
	 */
	@Override
	public boolean isGrassAlive() {
		return this.grass == 0;
	}

	/**
	 * @see ICell#eatGrass()
	 */
	@Override
	public void eatGrass() {
		this.grass = this.getGrassRestart();
	}
	
	/**
	 * @see ICell#regenerateGrass()
	 */
	@Override
	public void regenerateGrass() {
		if (this.grass > 0) {
			this.grass--;
		}
	}
	
	/**
	 * @see ICell#getGrassRestart()
	 */
	@Override
	public int getGrassRestart() {
		return grassRestart;
	}

	/**
	 * @see ICell#getAgents()
	 */
	@Override
	public Iterable<IAgent> getAgents() {
		
		return this.agents;
		
	}

	/**
	 * @see ICell#putInitAgent(IAgent)
	 */
	@Override
	public void putInitAgent(IAgent agent) {
		
		/* Put initial agent according to the specified strategy. */
		this.putInitAgentStrategy.putAgent(this.newAgents, agent);

	}
	
	/**
	 * @see ICell#putNewAgent(IAgent)
	 */
	@Override
	public void putNewAgent(IAgent agent) {
		
		/* Put new agent according to the specified strategy. */
		this.putNewAgentStrategy.putAgent(this.newAgents, agent);

	}
	
	/**
	 * @see ICell#putExistingAgent(IAgent)
	 */
	@Override
	public void putExistingAgent(IAgent agent) {

		/* Put existing agent according to the specified strategy. */
		this.putExistingAgentStrategy.putAgent(this.existingAgents, agent);

	}
	
	/**
	 * @see ICell#getStats(IterationStats)
	 */
	@Override
	public void getStats(IterationStats stats) {
		
		/* Grass alive or not? */
		if (this.isGrassAlive())
			stats.incGrassAlive();
		else
			stats.updateGrassCountdown(this.grass);
		
		this.auxAgents.clear();
		
		/* Count previously existing agents, add them to the auxAgents list. */
		for (int i = 0; i < this.agents.size(); i++) {
			
			/* Get current agent. */
			IAgent agent = this.agents.get(i);
			
			/* If he's alive, count him and add him to the auxAgents list. */
			if (agent.isAlive()) {
				if (agent instanceof Sheep) {
					stats.incSheepCount();
					stats.updateSheepEnergy(agent.getEnergy());
				} else if (agent instanceof Wolf) {
					stats.incWolvesCount();
					stats.updateWolvesEnergy(agent.getEnergy());
				}
				this.auxAgents.add(agent);
			}
		}
		
		/* Count newly born agents, add them to the auxAgents list. */
		for (int i = 0; i < this.newAgents.size(); i++) {

			/* Get current agent. */
			IAgent agent = this.newAgents.get(i);

			/* Count him and add him to the auxAgents list. */
			if (agent instanceof Sheep) {
				stats.incSheepCount();
				stats.updateSheepEnergy(agent.getEnergy());
			} else if (agent instanceof Wolf) {
				stats.incWolvesCount();
				stats.updateWolvesEnergy(agent.getEnergy());
			}
			this.auxAgents.add(agent);

		}
		
		/* Clear the newAgents list. */
		this.newAgents.clear();
		
		/* Swap agents lists. The auxAgents list becomes the current agents list, and vice-versa. */
		List<IAgent> aux = this.agents;
		this.agents = this.auxAgents;
		this.auxAgents = aux;
		
	}
	
	/**
	 * @see ICell#agentActions(Random, boolean shuffle)
	 */
	@Override
	public void agentActions(Random rng, boolean shuffle) {
		
		/* Swap current agents list and existingAgents list. */
		List<IAgent> aux;
		aux = this.agents;
		this.agents = this.existingAgents;
		this.existingAgents = aux;
		this.existingAgents.clear();
		
		if (shuffle) Collections.shuffle(this.agents, rng);
		
		/* Cycle through agents in the current agents list. */
		for (int i = 0; i < this.agents.size(); i++) {
			
			/* Get current agent. */
			IAgent agent = this.agents.get(i);
			
			/* If agent is alive, perform its actions. */
			if (agent.isAlive())
				agent.act(this, rng);
		}
		
	}
	
	/**
	 * @see ICell#setNeighborhood(List)
	 */
	@Override
	public void setNeighborhood(List<ICell> neighborhood) {
		
		if (this.neighborhood == null) {

			/* Only set neighborhood if neighborhood is not already set. */
			this.neighborhood = neighborhood;

		} else {
			
			/* If this exception is thrown, there is a bug somewhere. */
			throw new IllegalStateException("Cell neighborhood already set!");
			
		}
		
	}

	/**
	 * @see ICell#agentsMove(Random)
	 */
	@Override
	public void agentsMove(Random rng) {
			
		/*  Cycle through agents in the current agents list. */
		for (int i = 0; i < this.agents.size(); i++) {
			
			/* Get current agent. */
			IAgent agent = this.agents.get(i);

			/* Decrement agent energy. */
			agent.decEnergy();

			/* Move agent if he's still alive. */
			if (agent.isAlive()) {
				
				/* Choose a random direction. */
				int direction = rng.nextInt(this.neighborhood.size());
				
				/* Move agent. */
				this.neighborhood.get(direction).putExistingAgent(agent);
				
			}
		}
		
	}

}