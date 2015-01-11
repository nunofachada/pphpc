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
 * A blocking simulation synchronizer. Waits for all simulation workers to
 * reach this synchronization point before it lets them continue. Registered 
 * observers to be executed serially before simulation workers are released. 
 * 
 * @author Nuno Fachada
 */
public class BlockingSyncPoint extends AbstractSyncPoint {

	/* Used as a blocking synchronizer. */
	private CyclicBarrier barrier;
	
	private int numWorkers;
	
	/**
	 * Create a new blocking simulation synchronizer.
	 * 
	 * @param event Simulation event to associate with this synchronizer.
	 * @param numThreads Number of simulation workers in current simulation.
	 */
	public BlockingSyncPoint(ControlEvent event, final IController controller, int numWorkers) {
		
		/* Call the super constructor. */
		super(event);
		
		this.numWorkers = numWorkers;
	
		this.barrier = new CyclicBarrier(this.numWorkers, new Runnable() {
			@Override public void run() {
				notifyObservers(controller); 
			}
		});
		
	}

	@Override
	protected void doSyncNotify(IController controller) throws InterruptedWorkException {

		/* Perform synchronization. */
		try {
			this.barrier.await();
		} catch (Exception e) {
			throw new InterruptedWorkException(e);
		}
	}

	@Override
	public void stopNow() {
		super.stopNow();
		this.barrier.reset();
	}


}
