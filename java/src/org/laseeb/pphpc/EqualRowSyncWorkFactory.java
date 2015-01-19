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

import com.beust.jcommander.Parameters;

/**
 * Work factory which creates the required objects to divide work equally among the 
 * available workers in a thread-safe fashion. When used for cell processing, thread-safety 
 * is achieved using row-level synchronization. When used for agent processing, it delegates
 * responsibility to a {@link EqualWorkProvider}.
 * 
 * @author Nuno Fachada
 */
@Parameters(commandNames = {"eq_rowsync"}, commandDescription = "Equal row sync. work command")
public class EqualRowSyncWorkFactory extends AbstractMultiThreadWorkFactory {
	
	/* Name of command which invokes this work factory.*/
	final private String commandName = "eq_rowsync";

	/**
	 * @see IWorkFactory#createSimController(IModel)
	 */
	@Override
	public IController createSimController(IModel model) {
		
		/* Instantiate the controller... */
		IController controller = new Controller(model, this);
		
		/* ...and set appropriate sync. points for equal work division. */
		controller.setWorkerSynchronizers(
				new NonBlockingSyncPoint(ControlEvent.BEFORE_INIT_CELLS, this.numThreads),
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_CELLS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_SET_CELL_NEIGHBORS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_AGENTS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_FIRST_STATS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_HALF_ITERATION, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_END_ITERATION, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_SIMULATION, this.numThreads));
		
		/* Return the controller, configured for equal work division. */
		return controller;
	}

	/**
	 * @see IWorkFactory#createPutInitAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutInitAgentStrategy() {
		return new CellPutAgentSyncSort();
	}
	
	/**
	 * @see IWorkFactory#createPutNewAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentAsync();
	}

	/**
	 * @see IWorkFactory#createPutExistingAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentAsync();
	}

	/**
	 * @see IWorkFactory#getCommandName()
	 */
	@Override
	public String getCommandName() {
		return this.commandName;
	}

	/**
	 * @see AbstractMultiThreadWorkFactory#doGetWorkProvider(int, WorkType, IModel, IController)
	 */
	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, WorkType workType, IModel model, IController controller) {
		
		if (workType == WorkType.CELL) {
			
			/* Use the equal row sync. work provider when dealing with cells. */
			return new EqualRowSyncWorkProvider(this.numThreads, model);
	
		} else {
			
			/* Use the equal work provider when initializing agents. */
			return new EqualWorkProvider(this.numThreads, workSize);
			
		}
				
	}

}
