package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CountDownLatch;

public class SimGrid implements ISimGrid {

	public static enum Threading {
		
		SINGLE(new CellPutAgentAsync()), 
		MULTI(new CellPutAgentSync()), 
		MULTI_REPEAT(new CellPutAgentSyncSort());
		
		private CellPutAgentBehavior putAgentBehavior;
		
		private Threading(CellPutAgentBehavior putAgentBehavior) {
			this.putAgentBehavior = putAgentBehavior;
		}
	}
	
	private ICell cells[];
	
	private int x;
	private int y;
	private int size;
	private CellGrassInitStrategy grassInitStrategy;
	private Threading threading;
	private int numThreads;
	private int grassRestart;
	
	private CountDownLatch latch;
	
	private class SimThreadState implements ISimThreadState {
		private int counter;
		private int first;
		private int last;
		private Random rng;
		public Random getRng() {
			return this.rng;
		}
	}
	
	public SimGrid(int x, int y, int grassRestart, CellGrassInitStrategy grassInitStrategy, Threading threading, int numThreads) {
		
		this.x = x;
		this.y = y;
		this.size = x * y;
		this.cells = new ICell[this.size];
		this.grassInitStrategy = grassInitStrategy;
		this.threading = threading;
		this.numThreads = numThreads;
		this.grassRestart = grassRestart;
		this.latch = new CountDownLatch(this.numThreads);
	}

	@Override
	public ISimThreadState initialize(int stId) {
		
		/* Create random number generator for current thread. */
		Random rng;
		try {
			rng = PredPrey.getInstance().createRNG(stId);
		} catch (Exception e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + numThreads - 1) / numThreads;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = stId * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((stId + 1) * cellsPerThread, this.size); /* Exclusive */
		
		/* Initialize simulation grid cells. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			this.cells[currCellIdx] = new Cell(grassRestart, rng, this.grassInitStrategy, this.threading.putAgentBehavior);
			this.cells[currCellIdx].initGrass();
			
		}
		
		/* Signal that this thread is finished. */
		this.latch.countDown();
		
		/* Wait for remaining threads... */
		try {
			this.latch.await();
		} catch (InterruptedException e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Set cell neighbors. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
			
			int up = currCellIdx - this.x >= 0 ? currCellIdx - this.x : this.size - x + currCellIdx;
			int down = currCellIdx + this.x < this.size  ? currCellIdx + this.x : currCellIdx + this.x - this.size;
			int right = currCellIdx + 1 < this.size ? currCellIdx + 1 : 0;
			int left = currCellIdx - 1 >= 0 ? currCellIdx - 1 : this.size - 1;
			
			List<ICell> neighborhood = Arrays.asList(
					this.cells[currCellIdx], this.cells[up], this.cells[right], this.cells[down], this.cells[left]); 
		
			this.cells[currCellIdx].setNeighborhood(Collections.unmodifiableList(neighborhood));
			
		}
		
		/* Thread state object for current thread. */
		SimThreadState tState = new SimThreadState();
		
		tState.first = startCellIdx;
		tState.last = endCellIdx;
		tState.rng = rng;
	
		/* Return this thread's state. */
		return tState;
		
	}

	@Override
	public ICell getNextCell(ISimThreadState istState) {
		
		SimThreadState tState = (SimThreadState) istState;
		ICell nextCell = null;

		if (tState.counter < tState.last) {
			nextCell = this.cells[tState.counter];
			tState.counter++;
		}
		return nextCell;
	}

	@Override
	public ICell getCell(int idx) {
		return cells[idx];
	}

	@Override
	public void reset(ISimThreadState istState) {
		SimThreadState tState = (SimThreadState) istState;
		tState.counter = tState.first;
	}

}
