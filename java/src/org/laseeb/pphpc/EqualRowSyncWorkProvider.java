/*
 * Copyright (c) 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Instituto Superior Técnico nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *     
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

import java.util.concurrent.CyclicBarrier;

/**
 * Work provider which divides work equally among the available workers, with
 * synchronization performed at the row level. It can only be used for cell
 * processing.
 * 
 * @author Nuno Fachada
 */
public class EqualRowSyncWorkProvider implements IWorkProvider {
	
	/* Total number of workers. */
	private int numWorkers;
	
	/* The row size, i.e. the synchronization interval. */
	private int rowSize;
	
	/* Minimum distance, measured in number of rows, which threads must have
	 * of one another. */
	private int minThreadDist;
	
	/* Number of cells to be processed by each worker (except possibly the
	 * last worker). */
	private int cellsPerWorker;
	
	/* Number of rows to be processed by each worker (except possibly the
	 * last worker). */
	private int rowsPerWorker;
	
	/* The MVC model. */
	private IModel model;
	
	/* Row-level thread synchronizer. */
	private CyclicBarrier barrier;
	
	/**
	 * A class which represents the state of equal work with row-level
	 * synchronization performed by each worker.
	 */
	private class EqualRowSyncWork extends AbstractWork {

		/* Fixed start work token. */
		private int startToken;
		
		/* Fixed end work token. */
		private int endToken;
		
		/* Current work token. */
		private int counter;
		
		/* Row-level synchronization points. */
		private int[] syncPoints;
		
		/* Current synchronization point. */
		private int currSyncPoint;
		
		/**
		 * Create a new equal work state with row-level synchronization.
		 * 
		 * @param wId Worker ID.
		 * @param startToken Start work token.
		 * @param last Is this the last worker?
		 */
		public EqualRowSyncWork(int wId, int startToken, boolean last) {
			
			/* Set the worker ID. */
			super(wId);
			
			/* Keep the start token. */
			this.startToken = startToken;
			
			/* If this is the last worker, then the last token will be the model
			 * size; otherwise, the regular equal work distribution is used. */
			this.endToken = last ? model.getSize() : startToken + cellsPerWorker;
			
			/* Initialize counter. */
			this.counter = startToken;
			
			/* Determine number of sync. points. Basically, synchronize at the end
			 * of each row, except the last one. */
			int numSyncPoints = rowsPerWorker - 1;
			
			/* Initialize sync. points. */
			this.syncPoints = new int[numSyncPoints];
			for (int i = 0; i < numSyncPoints; i++) {
				
				/* Set sync. points at the end of each row, except the last one. */
				this.syncPoints[i] = startToken + (i + 1) *  rowSize;
				
			}
			
			/* Current sync. point is zero, naturally. */
			this.currSyncPoint = 0;
			
//			System.out.println("Worker " + wId + " gets work from " + this.startToken + " to " + this.endToken
//					+ " (sync points: " + Arrays.toString(syncPoints) + ")");
			
			
		}
		
	}

	/**
	 * Create a new equal work provider. Can only be used for processing cells.
	 * Synchronization is performed at row-level.
	 * 
	 * @param numThreads Number of available workers.
	 * @param model The MVC model.
	 */
	public EqualRowSyncWorkProvider(int numThreads, IModel model) {
		
		/* Keep the MVC model. */
		this.model = model;

		/* Keep the simulation space. */
		ISpace space = model.getSpace();
		
		/* Get the space dimensions. */
		int[] dims = space.getDims();
		
		/* Get the number of dimensions. */
		int numDims = space.getNumDims();
		
		/* The row size is 1 by default (i.e. if the space is 1D). */
		this.rowSize = 1;
		
		/* If the space is nD, n > 1, update the row size. */
		for (int i = 0; i < numDims - 1; i++) {
			
			/* For 2D, the row size is the number of cells in a row.
			 * For 3D, the row size is the number of cells in a horizontal slice. 
			 * And so on. The row size is the synchronization interval. */
			this.rowSize *= dims[i];
			
		}
		
		/* Determine the minimum thread distance, in number of rows (or cells in 1D, 
		 * slices in 3D, etc). */
		this.minThreadDist = space.getNeighborhoodRadius() * 2 + 1;
		
		/* Determine number of rows. */
		int rows = model.getSize() / this.rowSize;

		/* Determine the maximum number of threads, which is the total number of rows
		 * divided by the minimum thread distance. */
		int maxThreads = rows / this.minThreadDist;
		
		/* Check if number of threads doesn't exceed maximum value. */
		if (numThreads > maxThreads) {
			
			/* If so, throw exception to be caught by the simulation workers. */
			throw new RuntimeException("Too many threads! Max. threads is " + maxThreads);
			
		}
		
		/* Number of threads is OK, keep it. */
		this.numWorkers = numThreads;
		
		/* Initialize row-level synchronizer for the given number of threads. */
		this.barrier = new CyclicBarrier(numThreads);

		/* How many rows will be processed by worker? */
		this.rowsPerWorker  = rows / this.numWorkers;

		/* If the rows cannot be equally divided among the available workers...  */
		if (rows % this.numWorkers > 0) {
			
			/* ...check if we can add another row to be processed by each thread. For this
			 * to happen, the starting row of the last worker in a +1 row per worker scenario
			 * must be at least "minThreadDist" rows lower than the total number of rows. */
			if ((this.rowsPerWorker + 1) * this.numWorkers < rows - this.minThreadDist) {
				
				/* Condition verified, use an additional row per worker. */
				this.rowsPerWorker++;
					
			}
		}

		/* Keep the final number of cells per worker. */
		this.cellsPerWorker = this.rowSize * this.rowsPerWorker;

	}

	/**
	 * @see IWorkProvider#newWork(int)
	 */
	@Override
	public IWork newWork(int wId) {
		
		/* Is this the last worker? */
		boolean last = (wId == this.numWorkers - 1);
		
		/* Return a new work state. */
		return new EqualRowSyncWork(wId, wId * this.cellsPerWorker, last);
		
	}

	/**
	 * @see IWorkProvider#getNextToken(IWork)
	 */
	@Override
	public int getNextToken(IWork work) {
		
		/* Set the nextToken to -1, which means no more work
		 * is available. */
		int nextToken = -1;

		/* Cast generic work state to equal cell-level sync. work state. */
		EqualRowSyncWork iWork = (EqualRowSyncWork) work;
		
		/* Check if there is any work left to do. */
		if (iWork.counter < iWork.endToken) {
			
			/* If so, get next work token... */
			nextToken = iWork.counter;
			
		}

		/* Are there any sync. points left? */
		if (iWork.currSyncPoint < iWork.syncPoints.length) {
			
			/* Is it time to synchronize (i.e. are we at the end of row yet)? */
			if (nextToken == iWork.syncPoints[iWork.currSyncPoint]) {
				
				/* Increment current sync. point... */
				iWork.currSyncPoint++;
				
				/* ...and synchronize with remaining workers. */
				try {
					this.barrier.await();
				} catch (Exception e) {
					throw new RuntimeException(e);
				}
			}
			
			/* This is for the last worker only. Did the last worker reached the
			 * end of its work? If so, he may still have to synchronize with the
			 * other workers. */
			if (iWork.counter >= model.getSize()) {
				
				/* Last worker has to synchronize with remaining workers while he
				 * still has sync. points left. */
				while (iWork.currSyncPoint < iWork.syncPoints.length) {
					
					/* Increment current sync. point... */
					iWork.currSyncPoint++;
					
					/* ...and synchronize with remaining workers. */
					try {
						this.barrier.await();
					} catch (Exception e) {
						throw new RuntimeException(e);
					}
				}
				
			}
		}
		
		/* Increment work token counter. */
		iWork.counter++;

		/* Return the next work token. */
		return nextToken;
	}

	/**
	 * @see IWorkProvider#resetWork(IWork)
	 */
	@Override
	public void resetWork(IWork work) {

		/* Cast generic work state to equal cell-level sync. work state. */
		EqualRowSyncWork iWork = (EqualRowSyncWork) work;
		
		/* Reset counter. */
		iWork.counter = iWork.startToken;

		/* Reset current sync. point. */
		iWork.currSyncPoint = 0;
		
	}

}
