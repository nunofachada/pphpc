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

public class EqualWorkProvider implements IWorkProvider {

	private class EqualWork extends AbstractWork {

		private int startToken;
		private int endToken;
		private int counter;
		
		public EqualWork(int wId, int startToken, int endToken) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = endToken;
			this.counter = startToken;
		}
		
	}
	
	private int numWorkers;
	private int workSize;
	
	public EqualWorkProvider(int numWorkers, int workSize) {
		this.numWorkers = numWorkers;
		this.workSize = workSize;
	}
	
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


	@Override
	public int getNextToken(IWork work) {
		
		int nextToken = -1;
		EqualWork eWork = (EqualWork) work;

		if (eWork.counter < eWork.endToken) {
			nextToken = eWork.counter;
			eWork.counter++;
		}
		
		return nextToken;
	}

	@Override
	public void resetWork(IWork work) {
		EqualWork eWork = (EqualWork) work;
		eWork.counter = eWork.startToken;
	}


}
