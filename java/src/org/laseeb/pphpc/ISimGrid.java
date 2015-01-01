package org.laseeb.pphpc;

public interface ISimGrid {
	
	public void initialize();
	
	public ICell getNextCell();
	
	public void reset();

	public ICell getCell(int idx);

}
