/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Simulation statistics for a single iteration. 
 * 
 * @author Nuno Fachada
 */
public class IterationStats {

	/* Number of sheep. */
	private int sheepCount;

	/* Number of wolves. */
	private int wolvesCount;

	/* Grass quantity. */
	private int grassAlive;
	
	/* Total sheep energy. */
	private long sheepEnergy;

	/* Total wolves energy. */
	private long wolvesEnergy;

	/* Total grass countdown. */
	private long grassCountdown;

	/**
	 * Create a new iteration statistics object in which all statistics 
	 * are initialized to zero.
	 */
	public IterationStats() {
		
		this.reset();
		
	}
	
	/**
	 * Create a new iteration statistics object in which the several
	 * statistics are initialized to specified values.
	 * 
	 * @param sheepCount Number of sheep.
	 * @param wolvesCount Number of wolves.
	 * @param grassAlive Grass quantity.
	 * @param sheepEnergy Total sheep energy.
	 * @param wolvesEnergy Total wolves energy.
	 * @param grassCountdown Total grass countdown.
	 */
	public IterationStats(int sheepCount, int wolvesCount, int grassAlive, 
			long sheepEnergy, long wolvesEnergy, long grassCountdown) {
		this.sheepCount = sheepCount;
		this.wolvesCount = wolvesCount;
		this.grassAlive = grassAlive;
		this.sheepEnergy = sheepEnergy;
		this.wolvesEnergy = wolvesEnergy;
		this.grassCountdown = grassCountdown;
	}
	
	/**
	 * Reset statistics, i.e. set all statistics to zero.
	 */
	public void reset() {
		this.sheepCount = 0;
		this.wolvesCount = 0;
		this.grassAlive = 0;
		this.sheepEnergy = 0;
		this.wolvesEnergy = 0;
		this.grassCountdown = 0;
	}
	
	/**
	 * Get number of sheep.
	 * 
	 * @return Number of sheep.
	 */
	public int getSheepCount() {
		return this.sheepCount;
	}

	/**
	 * Increment number of sheep.
	 */
	public void incSheepCount() {
		this.sheepCount++;
	}

	/**
	 * Get number of wolves.
	 * 
	 * @return Number of wolves.
	 */
	public int getWolvesCount() {
		return this.wolvesCount;
	}

	/**
	 * Increment number of wolves.
	 */
	public void incWolvesCount() {
		this.wolvesCount++;
	}

	/**
	 * Get quantity of grass.
	 * 
	 * @return Quantity of grass.
	 */
	public int getGrassAlive() {
		return this.grassAlive;
	}

	/**
	 * Increment quantity of alive grass.
	 */
	public void incGrassAlive() {
		this.grassAlive++;
	}
	
	/**
	 * Get total sheep energy.
	 * 
	 * @return Total sheep energy.
	 */
	public long getSheepEnergy() {
		return this.sheepEnergy;
	}
	
	/**
	 * Update sheep energy.
	 * 
	 * @param energy Partial sheep energy.
	 */
	public void updateSheepEnergy(int energy) {
		this.sheepEnergy += energy;
	}

	/**
	 * Get total wolves energy.
	 * 
	 * @return Total wolves energy.
	 */
	public long getWolvesEnergy() {
		return this.wolvesEnergy;
	}

	/**
	 * Update wolves energy.
	 * 
	 * @param energy Partial wolves energy.
	 */
	public void updateWolvesEnergy(int energy) {
		this.wolvesEnergy += energy;
	}

	/**
	 * Get total grass countdown.
	 * 
	 * @return Total grass countdown.
	 */
	public long getGrassCountdown() {
		return this.grassCountdown;
	}

	/**
	 * Update grass countdown.
	 * 
	 * @param countdown Partial grass countdown.
	 */
	public void updateGrassCountdown(int countdown) {
		this.grassCountdown += countdown;
	}

}
