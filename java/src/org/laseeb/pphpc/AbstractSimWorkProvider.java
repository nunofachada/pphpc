/*
 * Copyright (c) 2014, Nuno Fachada
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

import java.util.Random;
import java.util.concurrent.CountDownLatch;

public abstract class AbstractSimWorkProvider implements ISimWorkProvider {

	public static enum Threading {
		
		SINGLE(new CellPutAgentAsync()), 
		MULTI(new CellPutAgentSync()), 
		MULTI_REPEAT(new CellPutAgentSyncSort());
		
		private CellPutAgentStrategy putAgentBehavior;
		
		private Threading(CellPutAgentStrategy putAgentBehavior) {
			this.putAgentBehavior = putAgentBehavior;
		}
		
		public CellPutAgentStrategy getPutAgentBehavior() {
			return this.putAgentBehavior;
		}
	}
	
	protected ISimSpace space;
	protected CellGrassInitStrategy grassInitStrategy;
	protected Threading threading;
	protected int numThreads;
	protected int grassRestart;
	
	protected CountDownLatch latch;
	
	public AbstractSimWorkProvider(ISimSpace space, int grassRestart, CellGrassInitStrategy grassInitStrategy, Threading threading, int numThreads) {
		
		this.space = space;
		this.grassInitStrategy = grassInitStrategy;
		this.threading = threading;
		this.numThreads = numThreads;
		this.grassRestart = grassRestart;
		this.latch = new CountDownLatch(this.numThreads);
	}
	
	protected abstract ISimWorkerState createCells(int stId, Random rng);
	
	protected abstract void setCellNeighbors(ISimWorkerState istState);

	@Override
	public ISimWorkerState initCells(int stId) {
		
		/* Create random number generator for current thread. */
		Random rng;
		try {
			rng = PredPrey.getInstance().createRNG(stId);
		} catch (Exception e) {
			throw new RuntimeException(e.getMessage());
		}
		
		/* Initialize simulation grid cells. */
		ISimWorkerState istState = this.createCells(stId, rng);
		
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


}
