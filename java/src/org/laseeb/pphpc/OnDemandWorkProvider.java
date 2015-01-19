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

import java.util.concurrent.atomic.AtomicInteger;

/**
 * Work provider which distributes work on-demand among the available workers.
 * 
 * @author Nuno Fachada
 */
public class OnDemandWorkProvider implements IWorkProvider {

	/**
	 * A class which represents the state of on-demand work performed by 
	 * each worker.
	 */
	private class OnDemandWork extends AbstractWork {

		/* Next work token to provide to worker. */
		private int next;
		
		/* Marks the maximum limit of work to provide to worker. In 
		 * other words, work tokens provided to worker must be lower
		 * than this value. */
		private int max;
		
		/**
		 * Create an on-demand work state.
		 * 
		 * @param wId Worker ID.
		 */
		public OnDemandWork(int wId) {
			super(wId);
			this.next = 0;
			this.max = 0;
		}
	}
	
	/* Work counter for all workers. */
	private AtomicInteger counter;
	
	/* Number of work tokens to allocate for each worker at each request. */
	private int blockSize;
	
	/* Total work size. */
	private int workSize;

	/**
	 * Create a new on-demand work provider.
	 * 
	 * @param blockSize Number of work tokens to allocate for each worker at each request.
	 * @param workSize Total work size.
	 */
	public OnDemandWorkProvider(int blockSize, int workSize) {
		this.counter = new AtomicInteger(0);
		this.blockSize = blockSize;
		this.workSize = workSize;
	}

	/**
	 * @see IWorkProvider#newWork(int)
	 */
	@Override
	public IWork newWork(int wId) {
		return new OnDemandWork(wId);
	}

	/**
	 * @see IWorkProvider#getNextToken(IWork)
	 */
	@Override
	public int getNextToken(IWork work) {
		
		/* Cast generic work to on-demand work. */
		OnDemandWork odWork = (OnDemandWork) work;
		
		/* Set nextToken to -1, which means no more work
		 * is available. */
		int nextToken = -1;
		
		/* Check if worker already processed its current block of work.*/
		if (odWork.next >= odWork.max) {

			/* If so, allocate a new block of work for him. */
			odWork.next = this.counter.getAndAdd(this.blockSize);
			odWork.max = Math.min(odWork.next + this.blockSize, this.workSize);
			
		}
		
		/* If worker has still work left to do... */
		if (odWork.next < this.workSize) {

			/* ...give worker a work token... */
			nextToken = odWork.next;
			
			/* ...and increment the next work token for the next request.*/
			odWork.next++;
			
		}

		/* Return the next work token. */
		return nextToken;
	}

	/**
	 * @see IWorkProvider#resetWork(IWork)
	 */
	@Override
	public void resetWork(IWork work) {
		
		/* Cast generic work to on-demand work... */
		OnDemandWork odWork = (OnDemandWork) work;

		/* ...and reset work state. */
		odWork.next = 0;
		odWork.max = 0;

	}

	/**
	 * Reset global work counter. To be called by one thread only when a work session
	 * is finished.
	 */
	void resetWorkCounter() {
		this.counter.set(0);
	}
	
}
