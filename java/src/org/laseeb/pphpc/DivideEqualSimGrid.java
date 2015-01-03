package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;

public class DivideEqualSimGrid extends SimGrid {

	private class DivideEqualSimWorkerState extends AbstractSimWorkerState {

		private int counter;
		private int first;
		private int last;
		
		public DivideEqualSimWorkerState(Random rng, int stId) {
			super(rng, stId);
		}
	}
	
	
	public DivideEqualSimGrid(int x, int y, int grassRestart,
			CellGrassInitStrategy grassInitStrategy, Threading threading,
			int numThreads) {
		super(x, y, grassRestart, grassInitStrategy, threading, numThreads);
		// TODO Auto-generated constructor stub
	}

	@Override
	public ICell getNextCell(ISimWorkerState istState) {
		
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		ICell nextCell = null;

		if (tState.counter < tState.last) {
			nextCell = this.cells[tState.counter];
			tState.counter++;
		}
		return nextCell;
	}

	@Override
	public void reset(ISimWorkerState istState) {
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		tState.counter = tState.first;
	}

	@Override
	protected ISimWorkerState createCells(int stId, Random rng) {
		
		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + numThreads - 1) / numThreads;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = stId * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((stId + 1) * cellsPerThread, this.size); /* Exclusive */
		
		/* Initialize simulation grid cells. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			this.cells[currCellIdx] = new Cell(grassRestart, rng, this.grassInitStrategy, this.threading.getPutAgentBehavior());
			this.cells[currCellIdx].initGrass();
			
		}
		
		/* Thread state object for current thread. */
		DivideEqualSimWorkerState tState = new DivideEqualSimWorkerState(rng, stId);
		
		tState.first = startCellIdx;
		tState.last = endCellIdx;
	
		/* Return this thread's state. */
		return tState;
		
	}
	
	@Override
	protected void setCellNeighbors(ISimWorkerState istState) {
		
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		
		/* Set cell neighbors. */
		for (int currCellIdx = tState.first; currCellIdx < tState.last; currCellIdx++) {
			
			int up = currCellIdx - this.x >= 0 ? currCellIdx - this.x : this.size - x + currCellIdx;
			int down = currCellIdx + this.x < this.size  ? currCellIdx + this.x : currCellIdx + this.x - this.size;
			int right = currCellIdx + 1 < this.size ? currCellIdx + 1 : 0;
			int left = currCellIdx - 1 >= 0 ? currCellIdx - 1 : this.size - 1;
			
			List<ICell> neighborhood = Arrays.asList(
					this.cells[currCellIdx], this.cells[up], this.cells[right], this.cells[down], this.cells[left]); 
		
			this.cells[currCellIdx].setNeighborhood(Collections.unmodifiableList(neighborhood));
			
		}
		
	}

	@Override
	public void initAgents(ISimWorkerState istState, SimParams params) {

		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		
		/* Determine sheep per thread. The bellow operation is equivalent to ceil(numSheep/numThreads) */
		int sheepPerThread = (params.getInitSheep() + numThreads - 1) / numThreads;
		
		/* Determine start and end sheep index for current thread. */
		int startSheepIdx = tState.getSimWorkerId() * sheepPerThread; /* Inclusive */
		int endSheepIdx = Math.min((tState.getSimWorkerId() + 1) * sheepPerThread, params.getInitSheep()); /* Exclusive */
		
		for (int sheepIdx = startSheepIdx; sheepIdx < endSheepIdx; sheepIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent sheep = new Sheep(1 + tState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			this.cells[idx].putNewAgent(sheep);
		}

		/* Determine wolves per thread. The bellow operation is equivalent to ceil(numWolves/numThreads) */
		int wolvesPerThread = (params.getInitWolves() + numThreads - 1) / numThreads;
		
		/* Determine start and end wolves index for current thread. */
		int startWolvesIdx = tState.getSimWorkerId() * wolvesPerThread; /* Inclusive */
		int endWolvesIdx = Math.min((tState.getSimWorkerId() + 1) * wolvesPerThread, params.getInitWolves()); /* Exclusive */
		
		for (int wolvesIdx = startWolvesIdx; wolvesIdx < endWolvesIdx; wolvesIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent wolf = new Wolf(1 + tState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			this.cells[idx].putNewAgent(wolf);
		}
		
	}
	
}
