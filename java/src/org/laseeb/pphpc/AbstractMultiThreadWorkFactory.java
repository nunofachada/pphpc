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

import java.util.HashMap;
import java.util.Map;

/**
 * Abstract class for multithreaded work factories. Produces work providers for
 * a given work size and maintains a map of work sizes and work provider 
 * implementations.
 * 
 * @author Nuno Fachada
 */
public abstract class AbstractMultiThreadWorkFactory implements IWorkFactory {

	/* Number of threads. */
	protected int numThreads;

	/* Map of work sizes and work provider implementations. */
	private Map<Integer, IWorkProvider> workProviders;
	
	/**
	 * Create a new abstract work factory.
	 * 
	 * @param numThreads Number of threads.
	 */
	public AbstractMultiThreadWorkFactory(int numThreads) {
		
		/* Set the number of threads. */
		this.numThreads = numThreads;
		
		/* Initialize map of work sizes and work provider implementations. */
		this.workProviders = new HashMap<Integer, IWorkProvider>();
	}
	
	/**
	 * @see IWorkFactory#getWorkProvider(int, WorkType, IModel, IController)
	 */
	@Override
	public IWorkProvider getWorkProvider(int workSize, WorkType workType, 
			IModel model, IController controller) {

		/* Instantiate work provider if required. Use double-checked locking to 
		 * avoid thread synchronization after initialization. */
		if (!this.workProviders.containsKey(workSize)) {
			synchronized (this) {
				if (!this.workProviders.containsKey(workSize)) {
					
					/* If the work provider for the given work size hasn't yet 
					 * been created, delegate creation to the concrete work 
					 * factory. */
					IWorkProvider workProvider = this.doGetWorkProvider(
							workSize, workType, model, controller);
					
					/* Put new work provider in map, associated with the given 
					 * work size. */
					this.workProviders.put(workSize, workProvider);
					
				}
			}
		}
		
		return this.workProviders.get(workSize);
	}

	/**
	 * Create concrete work provider.
	 * 
	 * @param workSize Size of work to be distributed by the work provider.
	 * @param workType Type of work to perform (agent or cell work).
	 * @param model The MVC model.
	 * @param controller The MVC controller.
	 * @return A new work provider.
	 */
	protected abstract IWorkProvider doGetWorkProvider(int workSize, 
			WorkType workType, IModel model, IController controller);
	
	
	/**
	 * Create a thread-safe global statistics object. 
	 * 
	 * @see IWorkFactory#createGlobalStats(int)
	 */
	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	/**
	 * @see IWorkFactory#getNumWorkers()
	 */
	@Override
	public int getNumWorkers() {
		return this.numThreads;
	}

}
