package org.laseeb.pphpc;

import java.io.IOException;
import java.util.Random;

public interface IModelManipulator {
	
	public void reset();
	
	public void incrementIteration();
	
	public void start();
	public void stop();
	public void pause();
	public void unpause();

	/**
	 * Export statistics to file.
	 * @param str Statistics filename.
	 * @throws Exception 
	 * @throws IOException 
	 */
	public void export(String filename);
	

	public void setCell(int idx, Cell cell);

	public ICell getCell(int idx);
	
	public void setCellNeighbors(int idx);
	public void setCellAt(int idx, Random rng);
	public void updateStats(int iter, IterationStats iterStats);
}
