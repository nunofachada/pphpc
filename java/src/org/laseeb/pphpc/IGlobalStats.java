package org.laseeb.pphpc;

public interface IGlobalStats {
	
	
	/**
	 * Get statistics.
	 * 
	 * @param st Type of statistic (number of sheep or wolves, or quantity of grass).
	 * @param iter Iteration.
	 * @return The requested statistic.
	 */
	public int getStats(StatType st, int iter);
	
	public void updateStats(int iter, IterationStats stats);

}
