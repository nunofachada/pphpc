/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Work provider which divides work equally among the available workers. 
 * If used for processing cells, synchronization is performed at cell-level.
 * 
 * @author Nuno Fachada
 */
public class EqualWorkProvider implements IWorkProvider {

	/**
	 * A class which represents the state of equal work. If used for 
	 * processing cells, synchronization is performed at cell-level.
	 */
	private class EqualWork extends AbstractWork {

		/* Fixed start work token. */
		private int startToken;
		
		/* Fixed end work token. */
		private int endToken;
		
		/* Current work token. */
		private int counter;
		
		/**
		 * Create a new equal work state. If this work state is to be used
		 * for processing cells, synchronization is performed at cell-level.
		 * 
		 * @param wId Worker ID.
		 * @param startToken Start work token.
		 * @param endToken End work token.
		 */
		public EqualWork(int wId, int startToken, int endToken) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = endToken;
			this.counter = startToken;
		}
		
	}
	
	/* Number of workers. */
	private int numWorkers;
	
	/* Total work size to be performed by the available workers. */
	private int workSize;
	
	/**
	 * Create a new equal work provider. If used for processing cells, 
	 * synchronization is performed at cell-level.
	 * 
	 * @param numWorkers Number of available workers.
	 * @param workSize Total work size to be performed by the available workers.
	 */
	public EqualWorkProvider(int numWorkers, int workSize) {
		this.numWorkers = numWorkers;
		this.workSize = workSize;
	}
	
	/**
	 * @see IWorkProvider#newWork(int)
	 */
	@Override
	public IWork newWork(int wId) {
		
		/* Determine tokens per worker. The bellow operation is equivalent to ceil(workSize/numWorkers) */
		int tokensPerWorker = (this.workSize + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end tokens for current worker. */
		int startToken = wId * tokensPerWorker; /* Inclusive */
		int endToken = Math.min((wId + 1) * tokensPerWorker, this.workSize); /* Exclusive */

		/* Create a work state adequate for this work provider. */
		EqualWork eWork = new EqualWork(wId, startToken, endToken);
		
		/* Return the work state. */
		return eWork;
	}

	/**
	 * @see IWorkProvider#getNextToken(IWork)
	 */
	@Override
	public int getNextToken(IWork work) {
		
		/* Set the nextToken to -1, which means no more work
		 * is available. */
		int nextToken = -1;

		/* Cast generic work state to equal work state. */
		EqualWork eWork = (EqualWork) work;

		/* Check if there is any work left to do. */
		if (eWork.counter < eWork.endToken) {
			
			/* If so, get next work token... */
			nextToken = eWork.counter;
			
			/* ...and increment work token counter. */
			eWork.counter++;
		}
		
		/* Return the next work token. */
		return nextToken;
	}

	/**
	 * @see IWorkProvider#resetWork(IWork)
	 */
	@Override
	public void resetWork(IWork work) {

		/* Cast generic work state to equal work state. */
		EqualWork eWork = (EqualWork) work;
		
		/* Reset work tokens for current worker.*/
		eWork.counter = eWork.startToken;
		
	}

}
