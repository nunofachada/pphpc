package org.laseeb.pphpc;

public class SimpleGlobalStats implements IGlobalStats {

	private int[] sheepStats;
	private int[] wolfStats;
	private int[] grassStats;
	
	public SimpleGlobalStats(int iters) {
		this.sheepStats = new int[iters + 1];
		this.wolfStats = new int[iters + 1];
		this.grassStats = new int[iters + 1];		
	}
	
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

	@Override
	public void updateStats(int iter, IterationStats stats) {
		sheepStats[iter] = stats.getSheep();
		wolfStats[iter] = stats.getWolves();
		grassStats[iter] = stats.getGrass();
	}


}
