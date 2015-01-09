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
	protected int energy;
	
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
	 * @see org.laseeb.pphpc.IAgent#act(org.laseeb.pphpc.ICell)
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
	 * reproducibility is required. A value is attributed to an agent using a simple hash
	 * based on energy and type, but in a way that ordering is not apparently affected by 
	 * either. 
	 * 
	 * @param otherAgent The agent to which this agent will be compared.
	 * @return A negative integer, zero, or a positive integer if this agent is to
	 * be ordered before, equally or after than the specified agent, respectively.
	 */
	@Override
	public int compareTo(IAgent otherAgent) {
		
		/* Get a unique profile for current agent based on its type and 
		 * energy. */
		int p1 = (this instanceof Wolf ? 1 << 31 : 1 << 30) | this.getEnergy();
		
		/* Get a unique profile for the other agent based on its type and 
		 * energy. */
		int p2 = (otherAgent instanceof Wolf ? 1 << 31 : 1 << 30) | otherAgent.getEnergy();

		/* Get a hash for current agent. */
		int h1 = p1;
		h1 ^= (h1 >>> 20) ^ (h1 >>> 12);
		h1 = h1 ^ (h1 >>> 7) ^ (h1 >>> 4);
		
		/* Get a hash for the other agent. */
		int h2 = p2;
		h2 ^= (h2 >>> 20) ^ (h2 >>> 12);
		h2 = h2 ^ (h2 >>> 7) ^ (h2 >>> 4);
		
		/* Return comparison depending on hash, if hashes are equal, comparison
		 * depends on agent type and energy (something we wish to minimize in
		 * order to make ordering as "random" as possible). */
		return h1 != h2 ? h1 - h2 : p1 - p2;

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
		if (energy > getReproduceThreshold()) {
			
			/* Throw dice, see if agent reproduces. */
			if (rng.nextInt(100) < getReproduceProbability()) {
				
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
