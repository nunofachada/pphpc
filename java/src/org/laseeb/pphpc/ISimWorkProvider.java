package org.laseeb.pphpc;

public interface ISimWorkProvider {
	
	public ISimWorkerState initCells(int swId);
	
	public ICell getNextCell(ISimWorkerState tState);
	
	public void resetNextCell(ISimWorkerState tState);

	public ICell getCell(int idx);

	public void initAgents(ISimWorkerState tState, SimParams params);

}
