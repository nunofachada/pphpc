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
 * Work factory which creates the required objects to divide work equally among 
 * the available workers in a thread-safe fashion. If used for processing cells, 
 * synchronization is performed at cell-level.
 * 
 * @author Nuno Fachada
 */
public class EqualWorkFactory extends AbstractMultiThreadWorkFactory {

	/* Is the simulation repeatable? */
	private boolean repeatable = false;
	
	/**
	 * Create a new equal work factory.
	 * 
	 * @param numThreads Number of threads.
	 * @param repeatable Are the simulations to be reproducible (slower)?
	 */
	public EqualWorkFactory(int numThreads, boolean repeatable) {
		super(numThreads);
		this.repeatable = repeatable;
	}
	
	/**
	 * @see IWorkFactory#createPutInitAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutInitAgentStrategy() {

		/* If simulation is to be repeatable... */
		if (this.repeatable) {

			/* ...agents must be sorted after inserted in cell. */
			return new CellPutAgentSyncOrdered();
			
		} else {
			
			/* Otherwise, synchronous agent insertion suffices. */
			return new CellPutAgentSync();
		
		}
	}
	
	/**
	 * @see IWorkFactory#createPutExistingAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {

		/* If simulation is to be repeatable... */
		if (this.repeatable) {

			/* ...agents must be sorted after inserted in cell. */
			return new CellPutAgentSyncOrdered();
			
		} else {
			
			/* Otherwise, synchronous agent insertion suffices. */
			return new CellPutAgentSync();
		
		}
	}

	/**
	 * @see IWorkFactory#createSimController(IModel)
	 */
	@Override
	public IController createSimController(IModel model) {
		
		/* Instantiate the controller... */
		IController controller = new Controller(model, this);
		
		/* ...and set appropriate sync. points for equal work division. */
		controller.setWorkerSynchronizers(
				new NonBlockingSyncPoint(ControlEvent.BEFORE_INIT_CELLS, 
						this.numThreads),
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_CELLS, 
						controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_SET_CELL_NEIGHBORS, 
						this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_AGENTS, 
						controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_FIRST_STATS, 
						this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_HALF_ITERATION, 
						controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_END_ITERATION, 
						controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_SIMULATION, 
						this.numThreads));
		
		/* Return the controller, configured for equal work division. */
		return controller;
	}

	/**
	 * @see AbstractMultiThreadWorkFactory#doGetWorkProvider(int, WorkType, IModel, IController)
	 */
	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, WorkType workType, 
			IModel model, IController controller) {
		
		/* The equal work provider will assure equal work division among 
		 * workers. */
		return new EqualWorkProvider(this.numThreads, workSize);
		
	}
}
