package org.laseeb.pphpc;

import java.util.Iterator;

public abstract class Cell {
	
	private int grassRestart;
	
	/* Grass counter. */
	protected int grass;

	public Cell(int grassRestart) {
		this.grassRestart = grassRestart;
	}

	public abstract void setGrass(int grass);

	public abstract void decGrass();

	public abstract void eatGrass();

	public abstract int getGrass();

	public abstract void removeAgent(Agent agent);

	public abstract Iterator<Agent> getAgents();

	public abstract void removeAgentsToBeRemoved();

	public abstract void futureIsNow();

	public abstract void putAgentNow(Agent agent);

	public abstract void putAgentFuture(Agent agent);

	/**
	 * @return the grassRestart
	 */
	public int getGrassRestart() {
		return grassRestart;
	}



}