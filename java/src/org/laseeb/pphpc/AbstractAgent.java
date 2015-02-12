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

import java.util.Random;

/**
 * Abstract PPHPC agent class.
 * 
 * @author Nuno Fachada
 */
public abstract class AbstractAgent implements IAgent {
	
	/* Agent energy. */
	private int energy;
	
	/* Simulation parameters. */
	protected ModelParams params;
	
	/**
	 * Constructor.
	 * 
	 * @param energy Initial agent energy.
	 * @param params Simulation parameters.
	 */
	public AbstractAgent(int energy, ModelParams params) {
		this.energy = energy;
		this.params = params;
	}
	
	/** 
	 * @see org.laseeb.pphpc.IAgent#getEnergy()
	 */
	@Override
	public int getEnergy() {
		return energy;
	}

	/**
	 * @see org.laseeb.pphpc.IAgent#setEnergy(int)
	 */
	@Override
	public void setEnergy(int energy) {
		this.energy = energy;
	}

	/**
	 * @see org.laseeb.pphpc.IAgent#decEnergy()
	 */
	@Override
	public void decEnergy() {
		this.energy--;
	}
	
	/**
	 * @see org.laseeb.pphpc.IAgent#isAlive()
	 */
	@Override
	public boolean isAlive() {
		return this.energy > 0;
	}

	/**
	 * @see IAgent#act(ICell, Random)
	 */
	@Override
	public void act(ICell cell, Random rng) {
		
		/* Maybe eat something. */
		tryEat(cell);
		
		/* Maybe perform reproduction. */
		tryReproduce(cell, rng);
	}
	
	/**
	 * Implementation of the {@link Comparable#compareTo(Object)} method for agent
	 * ordering. Agent ordering is used in multithreaded simulations for which simulation
	 * reproducibility is required.
	 * 
	 * @param otherAgent The agent to which this agent will be compared.
	 * @return A negative integer, zero, or a positive integer if this agent is to
	 * be ordered before, equally or after than the specified agent, respectively.
	 */
	@Override
	public int compareTo(IAgent otherAgent) {
		
		/* Get a unique profile for current agent based on its type and 
		 * energy. */
		int p1 = this.getEnergy() << 2 
				| (this instanceof Wolf ? 0x1 : 0x2);
		
		/* Get a unique profile for the other agent based on its type and 
		 * energy. */
		int p2 = otherAgent.getEnergy() << 2 
				| (otherAgent instanceof Wolf ? 0x1 : 0x2);


		return p1 - p2;
	}

	/**
	 * Try to eat available food in current cell.
	 * 
	 * @param cell Cell where agent is currently in.
	 */
	protected abstract void tryEat(ICell cell);
	
	/**
	 * Try to reproduce the agent.
	 * 
	 * @param cell Cell where agent is currently in.
	 * @param rng Random number generator used to try reproduction.
	 */
	private void tryReproduce(ICell cell, Random rng) {
		
		/* Energy needs to be above threshold in order for agents to reproduce. */
		if (energy > this.getReproduceThreshold()) {
			
			/* Throw dice, see if agent reproduces. */
			if (rng.nextInt(100) < this.getReproduceProbability()) {

				/* Create new agent with half of the energy of the current agent. */
				AbstractAgent agent = null;
				
				try {
				
					agent = (AbstractAgent) this.clone();
					agent.energy = this.energy / 2;
					this.energy = this.energy - agent.energy;
					
				} catch (CloneNotSupportedException e) {
					
					/* This should never be reached. */
					e.printStackTrace();
					
				}
				
				/* Put new agent in current cell. */
				cell.putNewAgent(agent);
			}
		}
	}
	
}
