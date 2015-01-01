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
	
	private int[] counter;
	private Limits[] limits;
	
	private int x;
	private int y;
	private int size;
	private CellGrassInitStrategy grassInitStrategy;
	private Threading threading;
	private int numThreads;
	private int grassRestart;
	
	private volatile long maxTid;

	private CountDownLatch latch1, latch2;
	
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
		this.latch1 = new CountDownLatch(this.numThreads);
		this.latch2 = new CountDownLatch(this.numThreads);
		this.counter = new int[numThreads];
		this.limits = new Limits[numThreads];
		this.maxTid = 0;
	}

	@Override
	public void initialize() {
		
		synchronized (this) {
			this.maxTid = Math.max(this.maxTid, Thread.currentThread().getId());
		}
		
		/* Signal that this thread is finished. */
		this.latch1.countDown();
		
		/* Wait for remaining threads... */
		try {
			this.latch1.await();
		} catch (InterruptedException e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Determine this thread's offset. */
		int offset = this.getOffset();

		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + numThreads - 1) / numThreads;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = offset * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((offset + 1) * cellsPerThread, this.size); /* Exclusive */
		
		this.limits[offset] = new Limits(startCellIdx, endCellIdx);
	
		/* Initialize simulation grid cells. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			this.cells[currCellIdx] = new Cell(grassRestart, this.grassInitStrategy, this.threading.putAgentBehavior);
			
		}
		
		/* Signal that this thread is finished. */
		this.latch2.countDown();
		
		/* Wait for remaining threads... */
		try {
			this.latch2.await();
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
		
		int offset = this.getOffset();
		ICell nextCell = null;

		int count = this.counter[offset];
		
		if (count < this.limits[offset].last) {
			nextCell = this.cells[count];
			this.counter[offset] = count + 1;
		}
		return nextCell;
	}

	@Override
	public ICell getCell(int idx) {
		return cells[idx];
	}

	@Override
	public void reset() {
		int offset = this.getOffset();
		this.counter[offset] = this.limits[offset].first;
	}

	private int getOffset() {
		return (int) (this.maxTid - Thread.currentThread().getId());
	}
}
