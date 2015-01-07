package org.laseeb.pphpc;

public class Model implements IModel {
	
	private int size;
	private SimParams params;
	private ICell cells[];
	private int currentIteration;
	private IGlobalStats globalStats;
	private ISpace space;
	
	public Model(SimParams params, ISpace space, IGlobalStats globalStats) {
		this.params = params;
		this.space = space;
		this.globalStats = globalStats;
		this.currentIteration = 0;
		this.size = space.getSize();
		this.cells = new ICell[this.size];
	}
	
	@Override
	public int getSize() {
		return this.size;
	}
	
	@Override
	public ICell getCell(int idx) {
		return this.cells[idx];
	}

	@Override
	public void setCell(int idx, Cell cell) {
		this.cells[idx] = cell;
	}

	@Override
	public void setCellNeighbors(int idx) {
		this.space.setNeighbors(cells, idx);
	}

	@Override
	public SimParams getParams() {
		return this.params;
	}

	@Override
	public int getCurrentIteration() {
		return this.currentIteration;
	}

	@Override
	public void incrementIteration() {
		this.currentIteration++;
	}

	@Override
	public IGlobalStats getGlobalStats() {
		return this.globalStats;
	}

}
