package org.laseeb.pphpc;

import java.util.Random;

public class Model implements IModel {
	
	private int size;
	private SimParams params;
	private ICell cells[];
	private int currentIteration;
	private IGlobalStats globalStats;
	private ISpace space;
	private ICellPutAgentStrategy putNewAgentStrategy;
	private ICellPutAgentStrategy putExistingAgentStrategy;
	private ICellGrassInitStrategy grassInitStrategy;
	
	public Model(SimParams params, IWorkFactory wFactory) {
		this.params = params;
		this.space = new Square2DTorusSpace(params.getGridX(), params.getGridY());
		this.globalStats = wFactory.createGlobalStats(params.getIters());
		this.putNewAgentStrategy = wFactory.createPutNewAgentStrategy();
		this.putExistingAgentStrategy = wFactory.createPutExistingAgentStrategy();
		this.grassInitStrategy = new CellGrassInitCoinRandCounter();
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

	@Override
	public void setCellAt(int idx, Random rng) {
		if (this.cells[idx] == null) {
			this.cells[idx] = new Cell(params.getGrassRestart(), 
					this.grassInitStrategy.getInitGrass(params.getGrassRestart(), rng), 
					this.putNewAgentStrategy, this.putExistingAgentStrategy);
		} else {
			throw new IllegalStateException("Cell " + idx + " already set!");
		}
	}

}
