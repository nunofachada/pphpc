package org.laseeb.pphpc;

public interface IController {

	public void registerSimEventObserver(SimEvent event, IObserver observer);

	public void syncAfterInitCells() throws WorkException;

	public void syncAfterCellsAddNeighbors() throws WorkException;

	public void syncAfterInitAgents() throws WorkException;

	public void syncAfterFirstStats() throws WorkException;

	public void syncAfterHalfIteration() throws WorkException;

	public void syncAfterEndIteration() throws WorkException;

	public void syncAfterSimFinish() throws WorkException;

	public void notifyTermination();

}
