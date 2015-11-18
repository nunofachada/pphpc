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

import java.util.Arrays;

/**
 * Single-thread management of global simulation statistics.
 * Will not work in multi-threaded simulations.
 * 
 * @author Nuno Fachada
 */
public class SingleThreadGlobalStats implements IGlobalStats {

	/* Sheep count. */
	private int[] sheepCount;

	/* Wolves count. */
	private int[] wolvesCount;

	/* Grass alive. */
	private int[] grassAlive;

	/* Total sheep energy. */
	private long[] sheepEnergy;

	/* Total wolf energy. */
	private long[] wolvesEnergy;

	/* Total grass countdown. */
	private long[] grassCountdown;
	
	/**
	 * Create a new single-threaded global statistics object.
	 * 
	 * @param iters Number of iterations.
	 */
	public SingleThreadGlobalStats(int iters) {
		this.sheepCount = new int[iters + 1];
		this.wolvesCount = new int[iters + 1];
		this.grassAlive = new int[iters + 1];
		this.sheepEnergy = new long[iters + 1];
		this.wolvesEnergy = new long[iters + 1];
		this.grassCountdown = new long[iters + 1];
		this.reset();
	}
	
	/**
	 * @see IGlobalStats#getStats(StatType, int)
	 */
	@Override
	public Number getStats(StatType st, int iter) {

		switch (st) {
			case SHEEP_COUNT:
				return this.sheepCount[iter];
			case WOLVES_COUNT:
				return this.wolvesCount[iter];
			case GRASS_ALIVE:
				return this.grassAlive[iter];
			case SHEEP_ENERGY:
				return this.sheepEnergy[iter];
			case WOLVES_ENERGY:
				return this.wolvesEnergy[iter];
			case GRASS_COUNTDOWN:
				return this.grassCountdown[iter];
		}

		return 0;
	}

	/**
	 * @see IGlobalStats#updateStats(int, IterationStats)
	 */
	@Override
	public void updateStats(int iter, IterationStats stats) {
		this.sheepCount[iter] = stats.getSheepCount();
		this.wolvesCount[iter] = stats.getWolvesCount();
		this.grassAlive[iter] = stats.getGrassAlive();
		this.sheepEnergy[iter] = stats.getSheepEnergy();
		this.wolvesEnergy[iter] = stats.getWolvesEnergy();
		this.grassCountdown[iter] = stats.getGrassCountdown();
	}

	/**
	 * @see IGlobalStats#getStats(int)
	 */
	@Override
	public IterationStats getStats(int iter) {
		return new IterationStats(this.sheepCount[iter], this.wolvesCount[iter], this.grassAlive[iter],
				this.sheepEnergy[iter], this.wolvesEnergy[iter], this.grassCountdown[iter]);
	}

	/**
	 * @see IGlobalStats#reset()
	 */
	@Override
	public void reset() {
		Arrays.fill(this.sheepCount, 0);
		Arrays.fill(this.wolvesCount, 0);
		Arrays.fill(this.grassAlive, 0);
		Arrays.fill(this.sheepEnergy, 0);
		Arrays.fill(this.wolvesEnergy, 0);
		Arrays.fill(this.grassCountdown, 0);
	}
}
