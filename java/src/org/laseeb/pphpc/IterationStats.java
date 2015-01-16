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

/**
 * Simulation statistics for a single iteration. 
 * 
 * @author Nuno Fachada
 */
public class IterationStats {

	/* Number of sheep. */
	private int sheep;

	/* Number of wolves. */
	private int wolves;

	/* Grass quantity. */
	private int grass;

	/**
	 * Create a new iteration statistics object in which all statistics 
	 * are initialized to zero.
	 */
	public IterationStats() {
		
		this.sheep = 0;
		this.wolves = 0;
		this.grass = 0;
		
	}
	
	/**
	 * Create a new iteration statistics object in which the several
	 * statistics are initialized to specified values.
	 * 
	 * @param sheep Number of sheep.
	 * @param wolves Number of wolves.
	 * @param grass Grass quantity.
	 */
	public IterationStats(int sheep, int wolves, int grass) {
		this.sheep = sheep;
		this.wolves = wolves;
		this.grass = grass;
	}
	
	/**
	 * Reset statistics, i.e. set all statistics to zero.
	 */
	public void reset() {
		this.sheep = 0;
		this.wolves = 0;
		this.grass = 0;
	}
	
	/**
	 * Get number of sheep.
	 * 
	 * @return Number of sheep.
	 */
	public int getSheep() {
		return sheep;
	}

	/**
	 * Increment number of sheep.
	 */
	public void incSheep() {
		this.sheep++;
	}

	/**
	 * Get number of wolves.
	 * 
	 * @return Number of wolves.
	 */
	public int getWolves() {
		return wolves;
	}

	/**
	 * Increment number of wolves.
	 */
	public void incWolves() {
		this.wolves++;
	}

	/**
	 * Get quantity of grass.
	 * 
	 * @return Quantity of grass.
	 */
	public int getGrass() {
		return grass;
	}

	/**
	 * Increment quantity of grass.
	 */
	public void incGrass() {
		this.grass++;
	}

}
