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
import java.util.concurrent.atomic.AtomicIntegerArray;
import java.util.concurrent.atomic.AtomicLongArray;

/**
 * Thread-safe management of global simulation statistics.
 * 
 * @author Nuno Fachada
 */
public class ThreadSafeGlobalStats implements IGlobalStats {

	/* Sheep count. */
	private AtomicIntegerArray sheepCount;

	/* Wolves count. */
	private AtomicIntegerArray wolvesCount;

	/* Grass alive. */
	private AtomicIntegerArray grassAlive;

	/* Total sheep energy. */
	private AtomicLongArray sheepEnergy;

	/* Total wolf energy. */
	private AtomicLongArray wolvesEnergy;

	/* Total grass countdown. */
	private AtomicLongArray grassCountdown;
	
	/* Number of iterations. */
	private int iters;
	
	/**
	 * Create a new thread-safe global statistics object.
	 * 
	 * @param iters Number of iterations.
	 */
	public ThreadSafeGlobalStats(int iters) {
		
		this.iters = iters;
		this.reset();
	}
	
	/**
	 * @see IGlobalStats#getStats(StatType, int)
	 */
	@Override
	public Number getStats(StatType st, int iter) {
		
		switch (st) {
			case SHEEP_COUNT:
				return sheepCount.get(iter);
			case WOLVES_COUNT:
				return this.wolvesCount.get(iter);
			case GRASS_ALIVE:
				return this.grassAlive.get(iter);
			case SHEEP_ENERGY:
				return this.sheepEnergy.get(iter);
			case WOLVES_ENERGY:
				return this.wolvesEnergy.get(iter);
			case GRASS_COUNTDOWN:
				return this.grassCountdown.get(iter);
		}
		return 0;
	}

	/**
	 * @see IGlobalStats#updateStats(int, IterationStats)
	 */
	@Override
	public void updateStats(int iter, IterationStats stats) {
		this.sheepCount.addAndGet(iter, stats.getSheepCount());
		this.wolvesCount.addAndGet(iter, stats.getWolvesCount());
		this.grassAlive.addAndGet(iter, stats.getGrassAlive());
		this.sheepEnergy.addAndGet(iter, stats.getSheepEnergy());
		this.wolvesEnergy.addAndGet(iter, stats.getWolvesEnergy());
		this.grassCountdown.addAndGet(iter, stats.getGrassCountdown());
	}

	/**
	 * @see IGlobalStats#getStats(int)
	 */
	@Override
	public IterationStats getStats(int iter) {
		return new IterationStats(
				this.sheepCount.get(iter), this.wolvesCount.get(iter), this.sheepCount.get(iter),
				this.sheepEnergy.get(iter), this.wolvesEnergy.get(iter), this.grassCountdown.get(iter));
	}

	/**
	 * @see IGlobalStats#reset()
	 */
	@Override
	public void reset() {
		int[] resetArrayInt = new int[this.iters + 1];
		long[] resetArrayLong = new long[this.iters + 1];
		Arrays.fill(resetArrayInt, 0);
		Arrays.fill(resetArrayLong, 0);
		this.sheepCount = new AtomicIntegerArray(resetArrayInt);
		this.wolvesCount = new AtomicIntegerArray(resetArrayInt);
		this.grassAlive = new AtomicIntegerArray(resetArrayInt);
		this.sheepEnergy = new AtomicLongArray(resetArrayLong);
		this.wolvesEnergy = new AtomicLongArray(resetArrayLong);
		this.grassCountdown = new AtomicLongArray(resetArrayLong);
	}


}
