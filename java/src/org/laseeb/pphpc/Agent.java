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

/**
 * 
 */
package org.laseeb.pphpc;

import java.util.Random;

/**
 * Generic agent class.
 * @author Nuno Fachada
 */
public abstract class Agent implements Cloneable {
	
	/* Agent energy. */
	protected int energy;
	
	/* Simulation parameters. */
	protected SimParams params;
	
	/**
	 * Constructor.
	 * @param energy Initial agents' energy.
	 */
	public Agent(int energy, SimParams params) {
		this.energy = energy;
		this.params = params;
	}
	
	/**
	 * Returns the agents' energy.
	 * @return The agent's energy.
	 */
	public int getEnergy() {
		return energy;
	}

	/**
	 * Sets the agent energy.
	 * @param energy Value to which set the agents' energy-
	 */
	public void setEnergy(int energy) {
		this.energy = energy;
	}

	/**
	 * Decrements the agent's energy.
	 */
	public void decEnergy() {
		this.energy--;
	}
	
	/**
	 * Agent-specific actions.
	 * @param cell Cell where agent is currently in.
	 */
	protected abstract void play(Cell cell);
	
	/**
	 * Generic agent actions, consisting of:
	 * - Specific agent actions.
	 * - Reproduction.
	 * @param cell Cell where agent is currently in.
	 */
	public void doPlay(Cell cell, Random rng) {
		/* Perform specific agent actions. */
		play(cell);
		/* Maybe perform reproduction. */
		reproduce(cell, rng);
	}
	
	/**
	 * Reproduction action.
	 * @param cell Cell where agent is currently in.
	 */
	protected void reproduce(Cell cell, Random rng) {
		/* Energy needs to be above threshold in order for agents to reproduce. */
		if (energy > getReproduceThreshold()) {
			/* Throw dice, see if agent reproduces. */
			if (rng.nextInt(100) < getReproduceProbability()) {
				/* Create new agent with half of the energy of the current agent. */
				Agent agent = null;
				try {
					agent = (Agent) this.clone();
					agent.energy = this.energy / 2;
					this.energy = this.energy - agent.energy;
				} catch (CloneNotSupportedException e) {
					e.printStackTrace();
				}
				/* Put new agent in current cell. */
				cell.putAgentFuture(agent);
			}
		}
	}
	
	/**
	 * Returns the agent-specific reproduction threshold.
	 * @return Agent-specific reproduction threshold.
	 */
	protected abstract int getReproduceThreshold();
	
	/**
	 * Returns the agent-specific reproduction probability.
	 * @return Agent-specific reproduction probability.
	 */
	protected abstract int getReproduceProbability();
}
