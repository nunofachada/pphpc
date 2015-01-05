package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicIntegerArray;

public class ThreadSafeGlobalStats implements IGlobalStats {

	private AtomicIntegerArray sheepStats;
	private AtomicIntegerArray wolfStats;
	private AtomicIntegerArray grassStats;
	
	public ThreadSafeGlobalStats(int iters) {
		int[] resetArray = new int[iters + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.grassStats = new AtomicIntegerArray(resetArray);		
		
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
		sheepStats.addAndGet(iter, stats.getSheep());
		wolfStats.addAndGet(iter, stats.getWolves());
		grassStats.addAndGet(iter, stats.getGrass());		
	}


}
