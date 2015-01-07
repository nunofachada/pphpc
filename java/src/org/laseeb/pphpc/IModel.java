package org.laseeb.pphpc;

public interface IModel {
	
	public int getSize();

	public ICell getCell(int idx);

	public void setCell(int idx, Cell cell);
	
	public void setCellNeighbors(int idx);
	
	public SimParams getParams();
	
	public int getCurrentIteration();
	
	public void incrementIteration();
	
	public IGlobalStats getGlobalStats();
}
