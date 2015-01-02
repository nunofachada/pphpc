package org.laseeb.pphpc;

public interface ISimGrid {
	
	public ISimThreadState initialize(int stId);
	
	public ICell getNextCell(ISimThreadState tState);
	
	public void reset(ISimThreadState tState);

	public ICell getCell(int idx);

}
