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
 * Work factory which creates the required objects to distribute work to a single worker.
 * 
 * @author Nuno Fachada
 */
@Parameters(commandNames = {"st"}, commandDescription = "Single-threaded work command")
public class SingleThreadWorkFactory implements IWorkFactory {

	/* Name of command which invokes this work factory.*/
	private String commandName = "st";

	/**
	 * @see IWorkFactory#getWorkProvider(int, IController)
	 */
	@Override
	public IWorkProvider getWorkProvider(int workSize, IModel model, IController controller) {
		return new SingleThreadWorkProvider(workSize);
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
	 * @see IWorkFactory#createSimController(IModel)
	 */
	@Override
	public IController createSimController(IModel model) {
		
		/* Instantiate the controller... */
		IController controller = new Controller(model, this);
		
		/* ...and set appropriate sync. points for only one worker. */
		controller.setWorkerSynchronizers(
				new SingleThreadSyncPoint(ControlEvent.BEFORE_INIT_CELLS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_INIT_CELLS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_SET_CELL_NEIGHBORS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_INIT_AGENTS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_FIRST_STATS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_HALF_ITERATION), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_END_ITERATION), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_END_SIMULATION));
		
		/* Return the controller, configured for one worker. */
		return controller;
	}

	/**
	 * @see IWorkFactory#createGlobalStats(int)
	 */
	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new SingleThreadGlobalStats(iters);
	}

	/**
	 * Always returns 1.
	 * 
	 * @see IWorkFactory#getNumWorkers()
	 */
	@Override
	public int getNumWorkers() {
		
		/* Always returns 1. */
		return 1;
		
	}

	/**
	 * @see IWorkFactory#getCommandName()
	 */
	@Override
	public String getCommandName() {
		return this.commandName ;
	}

}
