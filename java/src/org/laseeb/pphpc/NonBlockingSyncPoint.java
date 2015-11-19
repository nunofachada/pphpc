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

import java.util.concurrent.atomic.AtomicInteger;

/**
 * A non-blocking thread-safe synchronization point. Simulation workers
 * are not held by this synchronization point. When the last worker
 * passes by this synchronization point, it notifies and processes the 
 * associated control event observers, before continuing.
 * 
 * @author Nuno Fachada
 */
public class NonBlockingSyncPoint extends AbstractSyncPoint {

	/* Workers counter. */
	private AtomicInteger workersCount;
	
	/* Number of workers. */
	private int numWorkers;
	
	/**
	 * Create a new non-blocking synchronization point.
	 * 
	 * @param event Control event to which this synchronization point will be
	 * associated with.
	 * @param numWorkers Number of workers.
	 */
	public NonBlockingSyncPoint(ControlEvent event, int numWorkers) {
		
		/* Call the super constructor. */
		super(event);
		
		this.numWorkers = numWorkers;
		this.workersCount = new AtomicInteger(numWorkers);
	
	}

	/**
	 * @see AbstractSyncPoint#doSyncNotify(IController)
	 */
	@Override
	protected void doSyncNotify(IController controller) throws InterruptedWorkException {

		/* How many workers have yet to pass this synchronization point before notifying 
		 * observers? */
		int count = this.workersCount.decrementAndGet();
		
		/* If all workers have passed this synchronization point... */
		if (count == 0) {
			
			/* ...notify observers... */
			this.notifyObservers(controller);
			
			/* ...and reset workers counter. */
			this.workersCount.set(this.numWorkers);
			
		}
	}

}
