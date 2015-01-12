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
import java.util.concurrent.atomic.AtomicIntegerArray;

public class ThreadSafeGlobalStats implements IGlobalStats {

	private AtomicIntegerArray sheepStats;
	private AtomicIntegerArray wolfStats;
	private AtomicIntegerArray grassStats;
	
	private int iters;
	
	public ThreadSafeGlobalStats(int iters) {
		
		this.iters = iters;
		this.reset();
	}
	
	@Override
	public int getStats(StatType st, int iter) {
		
		switch (st) {
			case SHEEP:
				return sheepStats.get(iter);
			case WOLVES:
				return wolfStats.get(iter);
			case GRASS:
				return grassStats.get(iter);
		}
		return 0;
	}

	@Override
	public void updateStats(int iter, IterationStats stats) {
		this.sheepStats.addAndGet(iter, stats.getSheep());
		this.wolfStats.addAndGet(iter, stats.getWolves());
		this.grassStats.addAndGet(iter, stats.getGrass());		
	}

	@Override
	public IterationStats getStats(int iter) {
		return new IterationStats(
				this.sheepStats.get(iter), this.wolfStats.get(iter), this.sheepStats.get(iter));
	}

	@Override
	public void reset() {
		int[] resetArray = new int[this.iters + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.grassStats = new AtomicIntegerArray(resetArray);		
	}


}
