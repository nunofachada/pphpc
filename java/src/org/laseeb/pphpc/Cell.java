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
	private CellPutAgentBehavior putAgentNowBehavior;
	
	private CellPutAgentBehavior putAgentFutureBehavior;
	
	private CellFutureIsNowPostBehavior futureIsNowBehavior;
	
	/* Iterations for cell restart. */
	private int grassRestart;
		
	/* Structure where to keep current agents. */
	private List<IAgent> agents;
	/* Structure where to put agents to be removed. */
	private List<IAgent> agentsToRemove;	
	/* Structure where to put future agents. */
	private List<IAgent> futureAgents;
	
	/* Grass counter. */
	private int grass;

	
	/**
	 * Constructor.
	 */
	public Cell(int grassRestart,
			CellGrassInitStrategy grassInitStrategy,
			CellPutAgentBehavior putAgentsNowBehavior, 
			CellPutAgentBehavior putAgentsFutureBehavior,
			CellFutureIsNowPostBehavior futureIsNowBehavior) {
		
		this.grass = grassInitStrategy.getInitGrass(grassRestart);
		this.grassRestart = grassRestart;
		this.putAgentNowBehavior = putAgentsNowBehavior;
		this.putAgentFutureBehavior = putAgentsFutureBehavior;
		this.futureIsNowBehavior = futureIsNowBehavior;
		
		/* Initialize agent keeping structures. */
		this.agents = new ArrayList<IAgent>();
		this.futureAgents = new ArrayList<IAgent>();
		this.agentsToRemove = new ArrayList<IAgent>();
	}

//	/* (non-Javadoc)
//	 * @see org.laseeb.pphpc.ICell#getGrass()
//	 */
//	@Override
//	public int getGrass() {
//		return grass;
//	}
	
	public boolean isGrassAlive() {
		return this.grass == 0;
	}
	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#eatGrass()
	 */
	@Override
	public void eatGrass() {
		grass = this.getGrassRestart();
	}
	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#decGrass()
	 */
	@Override
	public void regenerateGrass() {
		grass--;
	}
	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#getGrassRestart()
	 */
	@Override
	public int getGrassRestart() {
		return grassRestart;
	}

	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#removeAgent(org.laseeb.pphpc.IAgent)
	 */
	@Override
	public void removeAgent(IAgent agent) {
		agentsToRemove.add(agent);
	}

	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#getAgents()
	 */
	@Override
	public Iterable<IAgent> getAgents() {
		return agents;
	}

	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#removeAgentsToBeRemoved()
	 */
	@Override
	public void removeAgentsToBeRemoved() {
		agents.removeAll(agentsToRemove);
		agentsToRemove.clear();
	}

	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#futureIsNow()
	 */
	@Override
	public void futureIsNow() {
		List<IAgent> aux = agents;
		agents = futureAgents;
		futureAgents = aux;
		futureAgents.clear();
		futureIsNowBehavior.futureIsNowPost(agents);
	}	
	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#putAgentNow(org.laseeb.pphpc.IAgent)
	 */
	@Override
	public void putAgentNow(IAgent agent) {
		if (agent.getEnergy() > 0)
			this.putAgentNowBehavior.putAgent(this.agents, agent);
	}
	
	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.ICell#putAgentFuture(org.laseeb.pphpc.IAgent)
	 */
	@Override
	public void putAgentFuture(IAgent agent) {
		if (agent.getEnergy() > 0)
			this.putAgentFutureBehavior.putAgent(this.futureAgents, agent);
	}
	
	public void mergeFutureWithPresent() {
		this.agents.addAll(this.futureAgents);
		this.futureAgents.clear();
	}
	
}