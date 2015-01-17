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

import com.beust.jcommander.Parameter;
import com.beust.jcommander.Parameters;
import com.beust.jcommander.validators.PositiveInteger;

/**
 * Work factory which creates the required objects to distribute work on demand among the 
 * available workers in a thread-safe fashion.
 *  
 * @author Nuno Fachada
 */
@Parameters(commandNames = {"on_demand"}, commandDescription = "On-demand work command")
public class OnDemandWorkFactory extends AbstractMultiThreadWorkFactory {

	/* Name of command which invokes this work factory.*/
	final private String commandName = "on_demand";

	/* Block size for ON_DEMAND work type. */
	@Parameter(names = "-b", description = "Block size", validateWith = PositiveInteger.class)
	private Integer blockSize = 100;

	/**
	 * @see IWorkFactory#createPutNewAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentSync();
	}

	/**
	 * @see IWorkFactory#createPutExistingAgentStrategy()
	 */
	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentSync();
	}

	/**
	 * @see IWorkFactory#createSimController(IModel)
	 */
	@Override
	public IController createSimController(IModel model) {
		
		/* Instantiate the controller... */
		IController controller = new Controller(model, this);
		
		/* ...and set appropriate sync. points for on-demand work division. */
		controller.setWorkerSynchronizers(
				new BlockingSyncPoint(ControlEvent.BEFORE_INIT_CELLS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_CELLS, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_SET_CELL_NEIGHBORS, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_AGENTS,controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_FIRST_STATS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_HALF_ITERATION, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_END_ITERATION, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_SIMULATION, this.numThreads));
		
		/* Return the controller, configured for on-demand work division. */
		return controller;
	}

	/**
	 * @see IWorkFactory#getCommandName()
	 */
	@Override
	public String getCommandName() {
		return this.commandName;
	}

	/**
	 * @see AbstractMultiThreadWorkFactory#doGetWorkProvider(int, IController)
	 */
	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, IModel model, IController controller) {
		
		/* Instantiate the on-demand work provider. */
		final OnDemandWorkProvider workProvider = new OnDemandWorkProvider(this.blockSize, workSize);
		
		/* Create a control event observer to reset work at certain control points. */
		IControlEventObserver resetCellCounter = new IControlEventObserver() {

			@Override
			public void update(ControlEvent event, IController controller) {
				workProvider.resetWorkCounter();
			}
		};
		
		/* Add control observer to reset work at the following control points: */
		controller.registerControlEventObserver(ControlEvent.BEFORE_INIT_CELLS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_INIT_CELLS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_INIT_AGENTS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_FIRST_STATS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_HALF_ITERATION, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_END_ITERATION, resetCellCounter);
		
		/* Return the instantiated work provider. */
		return workProvider;
	}

}
