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

import java.util.ArrayList;
import java.util.List;

/**
 * Abstract simulation synchronizer which implements the IObservable
 * interface.
 * 
 * @author Nuno Fachada
 */
public abstract class AbstractSyncPoint implements ISyncPoint {

	/* List of observers. */
	private List<IControlEventObserver> observers;
	
	/* Simulation event associated with this synchronizer. */
	private ControlEvent event;
	
	/* Was the simulation interrupted? */
	protected volatile boolean interrupted;
	
	/**
	 * Constructor called by concrete implementations.
	 * 
	 * @param event Simulation event to associate with this simulation
	 * synchronizer. 
	 */
	public AbstractSyncPoint(ControlEvent event) {
		this.event = event;
		this.observers = new ArrayList<IControlEventObserver>();
		this.interrupted = false;
	}
	
	/**
	 * @see IControlEventObservable#registerObserver(IControlEventObserver)
	 */
	@Override
	public void registerObserver(IControlEventObserver observer) {
		this.observers.add(observer);
	}
	
	@Override
	public void stopNow() {
		this.interrupted = true;
	}

	/**
	 * 
	 * 
	 * @see ISyncPoint#syncNotify(IModel model)
	 */
	@Override
	public void syncNotify(IController controller) throws InterruptedWorkException {
		
		/* Stop thread if it was interrupted. */
		if (this.interrupted)
			throw new InterruptedWorkException("Interrupted by another thread.");
		
		/* Perform synchronization. */
		this.doSyncNotify(controller);
	}
	
	/**
	 * Perform proper synchronization. Implementations of this method must invoke
	 * {@link #notifyObservers(IModel)}.
	 * 
	 * @param model The simulation model.
	 */
	protected abstract void doSyncNotify(IController controller) throws InterruptedWorkException;

	/**
	 * Helper method which notifies the registered observers.
	 * 
	 * @param model The simulation model.
	 */
	protected void notifyObservers(IController controller) {
		for (IControlEventObserver o : this.observers) {
			o.update(this.event, controller);
		}
	}
}
