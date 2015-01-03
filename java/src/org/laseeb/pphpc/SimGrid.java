package org.laseeb.pphpc;

import java.util.Random;
import java.util.concurrent.CountDownLatch;

public abstract class SimGrid implements ISimGrid {

	public static enum Threading {
		
		SINGLE(new CellPutAgentAsync()), 
		MULTI(new CellPutAgentSync()), 
		MULTI_REPEAT(new CellPutAgentSyncSort());
		
		private CellPutAgentBehavior putAgentBehavior;
		
		private Threading(CellPutAgentBehavior putAgentBehavior) {
			this.putAgentBehavior = putAgentBehavior;
		}
		
		public CellPutAgentBehavior getPutAgentBehavior() {
			return this.putAgentBehavior;
		}
	}
	
	protected ICell cells[];
	
	protected int x;
	protected int y;
	protected int size;
	protected CellGrassInitStrategy grassInitStrategy;
	protected Threading threading;
	protected int numThreads;
	protected int grassRestart;
	
	protected CountDownLatch latch;
	
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
	
	protected abstract ISimThreadState createCells(int stId, Random rng);
	
	protected abstract void setCellNeighbors(ISimThreadState istState);

	@Override
	public ISimThreadState initCells(int stId) {
		
		/* Create random number generator for current thread. */
		Random rng;
		try {
			rng = PredPrey.getInstance().createRNG(stId);
		} catch (Exception e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Initialize simulation grid cells. */
		ISimThreadState istState = this.createCells(stId, rng);
		
		/* Signal that this thread is finished. */
		this.latch.countDown();
		
		/* Wait for remaining threads... */
		try {
			this.latch.await();
		} catch (InterruptedException e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Set cell neighbors. */
		this.setCellNeighbors(istState);
	
		/* Return this thread's state. */
		return istState;
		
	}


	@Override
	public ICell getCell(int idx) {
		return cells[idx];
	}

}
