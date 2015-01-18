package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;

public class IntervalWorkProvider implements IWorkProvider {
	
	private int numWorkers;
	
	/* Synchronization interval. */
	private int syncInterval;
	
	private int exclusionConstant;
	
	private int cellsPerWorker;
	
	private int rowsPerWorker;
	
	private IModel model;
	
	private CyclicBarrier barrier;
	
	/**
	 * A class which represents the state of intervaled work performed by 
	 * each worker.
	 */
	private class IntervalWork extends AbstractWork {

		/* Fixed start work token. */
		private int startToken;
		
		/* Fixed end work token. */
		private int endToken;
		
		/* Current work token. */
		private int counter;
		
		private int[] syncPoints;
		
		private int syncPointPointer;
		
		/**
		 * Create a new intervaled work state representation.
		 * 
		 * @param wId Worker ID.
		 * @param startToken Start work token.
		 * @param endToken End work token.
		 */
		public IntervalWork(int wId, int startToken, boolean last) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = last ? model.getSize() : startToken + cellsPerWorker;
			this.counter = startToken;
			int numSyncPoints = rowsPerWorker - 1;
			this.syncPoints = new int[numSyncPoints];
			for (int i = 0; i < numSyncPoints; i++) {
				this.syncPoints[i] = startToken + (i + 1) *  syncInterval;
			}
			this.syncPointPointer = 0;
			
			
			System.out.println("Worker " + wId + " gets work from " + this.startToken + " to " + this.endToken
					+ " (sync points: " + Arrays.toString(syncPoints) + ")");
			
			
		}
		
	}

	public IntervalWorkProvider(int numThreads, IModel model) {
		
		this.model = model;

		ISpace space = model.getSpace();
		
		int[] dims = space.getDims();
		
		int numDims = space.getNumDims();
		
		this.syncInterval = 1;
		
		for (int i = 0; i < numDims - 1; i++)
			this.syncInterval *= dims[i];
		
		this.exclusionConstant = space.getNeighborhoodRadius() * 2 + 1;
		
		int maxThreads = model.getSize() / (this.syncInterval * this.exclusionConstant);
		
		/* Check if number of threads doesn't exceed maximum value. */
		if (numThreads > maxThreads) {
			throw new RuntimeException("Too many threads! Max. threads is " + maxThreads);
		}
		
		this.numWorkers = numThreads;
		this.barrier = new CyclicBarrier(numThreads);

		int rows = model.getSize() / this.syncInterval;
		this.rowsPerWorker  = rows / this.numWorkers;
		
		if (rows % this.numWorkers > 0) {
			if (rows - rowsPerWorker * this.numWorkers >= this.exclusionConstant - 1) {
				this.rowsPerWorker++;
			}
		}
		
		this.cellsPerWorker = this.syncInterval * this.rowsPerWorker;
	}

	@Override
	public IWork newWork(int wId) {
		boolean last = (wId == this.numWorkers - 1);
		return new IntervalWork(wId, wId * this.cellsPerWorker, last);
	}

	@Override
	public int getNextToken(IWork work) {
		
		/* Set the nextToken to -1, which means no more work
		 * is available. */
		int nextToken = -1;

		/* Cast generic work state to interval work state. */
		IntervalWork iWork = (IntervalWork) work;
		
		/* Check if there is any work left to do. */
		if (iWork.counter < iWork.endToken) {
			
			/* If so, get next work token... */
			nextToken = iWork.counter;
		}

		/* Synchronize? */
		if (iWork.syncPointPointer < iWork.syncPoints.length) {
			
			if (nextToken == iWork.syncPoints[iWork.syncPointPointer]) {
				iWork.syncPointPointer++;
				try {
					this.barrier.await();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (BrokenBarrierException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			
			if (iWork.counter >= model.getSize()) {
				while (iWork.syncPointPointer < iWork.syncPoints.length) {
					iWork.syncPointPointer++;
					try {
						this.barrier.await();
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					} catch (BrokenBarrierException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				
			}
		}
		
		/* Increment work token counter. */
		iWork.counter++;

		/* Return the next work token. */
		return nextToken;
	}

	@Override
	public void resetWork(IWork work) {

		IntervalWork iWork = (IntervalWork) work;
		
		iWork.counter = iWork.startToken;
		iWork.syncPointPointer = 0;
	}

}
