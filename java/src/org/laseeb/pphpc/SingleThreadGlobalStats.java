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

import java.util.Arrays;

/**
 * Single-thread management of global simulation statistics.
 * Will not work in multi-threaded simulations.
 * 
 * @author Nuno Fachada
 */
public class SingleThreadGlobalStats implements IGlobalStats {

	/* Sheep statistics. */
	private int[] sheepStats;

	/* Wolf statistics. */
	private int[] wolfStats;

	/* Grass statistics. */
	private int[] grassStats;

	/**
	 * Create a new single-threaded global statistics object.
	 * 
	 * @param iters Number of iterations.
	 */
	public SingleThreadGlobalStats(int iters) {
		this.sheepStats = new int[iters + 1];
		this.wolfStats = new int[iters + 1];
		this.grassStats = new int[iters + 1];
		this.reset();
	}
	
	/**
	 * @see IGlobalStats#getStats(StatType, int)
	 */
	@Override
	public int getStats(StatType st, int iter) {

		switch (st) {
			case SHEEP:
				return sheepStats[iter];
			case WOLVES:
				return wolfStats[iter];
			case GRASS:
				return grassStats[iter];
			}

		return 0;
	}

	/**
	 * @see IGlobalStats#updateStats(int, IterationStats)
	 */
	@Override
	public void updateStats(int iter, IterationStats stats) {
		this.sheepStats[iter] = stats.getSheep();
		this.wolfStats[iter] = stats.getWolves();
		this.grassStats[iter] = stats.getGrass();
	}

	/**
	 * @see IGlobalStats#getStats(int)
	 */
	@Override
	public IterationStats getStats(int iter) {
		return new IterationStats(this.sheepStats[iter], this.wolfStats[iter], this.grassStats[iter]);
	}

	/**
	 * @see IGlobalStats#reset()
	 */
	@Override
	public void reset() {
		Arrays.fill(this.sheepStats, 0);
		Arrays.fill(this.wolfStats, 0);
		Arrays.fill(this.grassStats, 0);
	}
}
