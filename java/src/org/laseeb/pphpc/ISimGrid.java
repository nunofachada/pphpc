package org.laseeb.pphpc;

public interface ISimGrid {
	
	public ISimWorkerState initCells(int swId);
	
	public ICell getNextCell(ISimWorkerState tState);
	
	public void reset(ISimWorkerState tState);

	public ICell getCell(int idx);

	public void initAgents(ISimWorkerState tState, SimParams params);

}
