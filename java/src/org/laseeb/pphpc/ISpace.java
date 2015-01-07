package org.laseeb.pphpc;

public interface ISpace {

	public void setNeighbors(ICell[] cells, int idx);
	
	public int getNumDims();
	
	public int[] getDims();
	
	public int getSize();

}
