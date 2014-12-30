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

/**
 * An abstract PPHPC model cell, part of a larger simulation grid.
 * 
 * @author Nuno Fachada
 *
 */
public interface ICell {

//	/**
//	 * Return grass counter value.
//	 * @return Grass counter value.
//	 */
//	public int getGrass();
	
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
	 * @return the grassRestart
	 */
	public int getGrassRestart();

	/**
	 * Remove agent from this cell.
	 * @param agent Agent to remove from cell.
	 */
	public void removeAgent(IAgent agent);

	/**
	 * Returns an iterator over agents in this cell.
	 * @return Iterator for agents in this cell.
	 */
	public Iterable<IAgent> getAgents();

	/**
	 * Remove agents to be removed.
	 */
	public void removeAgentsToBeRemoved();

	/**
	 * Make future agents the current agents.
	 */
	public void futureIsNow();

	/**
	 * Put new agent in this cell now.
	 * @param agent Agent to put in cell now.
	 */
	public void putAgentNow(IAgent agent);

	/**
	 * In the future, put new agent in this cell.
	 * @param agent Agent to put in cell in the future.
	 */
	public void putAgentFuture(IAgent agent);


}