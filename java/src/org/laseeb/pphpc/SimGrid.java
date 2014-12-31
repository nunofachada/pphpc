package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
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
	
	private ThreadLocal<Integer> counter;
	private ThreadLocal<Limits> limits;
	
	private int x;
	private int y;
	private int size;
	private CellGrassInitStrategy grassInitStrategy;
	private Threading threading;
	private int numThreads;
	private int grassRestart;

	private CountDownLatch latch ;
	
	private class Limits { 
		int first, last; 
		Limits(int first, int last) {
			this.first = first;
			this.last = last;
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
		this.counter = new ThreadLocal<Integer>();
		this.limits = new ThreadLocal<Limits>();
	}

	@Override
	public void initialize(int section) {
		
		
		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + numThreads - 1) / numThreads;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = section * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((section + 1) * cellsPerThread, this.size); /* Exclusive */
		
		this.limits.set(new Limits(startCellIdx, endCellIdx));
	
		/* Initialize simulation grid cells. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			this.cells[currCellIdx] = new Cell(grassRestart, this.grassInitStrategy, this.threading.putAgentBehavior);
			
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
		
	}

	@Override
	public ICell getNextCell() {
		ICell nextCell = null;
		int count = this.counter.get();
		
		if (count < this.limits.get().last) {
			nextCell = this.cells[count];
			this.counter.set(count + 1);
		}
		return nextCell;
	}

	@Override
	public ICell getCell(int idx) {
		return cells[idx];
	}

	@Override
	public void reset() {
		this.counter.set(this.limits.get().first);
	}

}
