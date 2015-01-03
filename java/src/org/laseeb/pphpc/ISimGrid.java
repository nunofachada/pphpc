package org.laseeb.pphpc;

public interface ISimGrid {
	
	public ISimThreadState initCells(int stId);
	
	public ICell getNextCell(ISimThreadState tState);
	
	public void reset(ISimThreadState tState);

	public ICell getCell(int idx);

	public void initAgents(ISimThreadState tState, SimParams params);

}
