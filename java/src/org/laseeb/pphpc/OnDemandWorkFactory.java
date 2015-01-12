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

@Parameters(commandNames = {"on_demand"}, commandDescription = "On-demand work command")
public class OnDemandWorkFactory extends AbstractMultiThreadWorkFactory {

	final private String commandName = "on_demand";

	/* Block size for ON_DEMAND work type. */
	@Parameter(names = "-b", description = "Block size", validateWith = PositiveInteger.class)
	private Integer blockSize = 100;

	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, IController controller) {
		final OnDemandWorkProvider workProvider = new OnDemandWorkProvider(this.blockSize, workSize);
		IControlEventObserver resetCellCounter = new IControlEventObserver() {

			@Override
			public void update(ControlEvent event, IController controller) {
				workProvider.resetWorkCounter();
			}
		};
		
		controller.registerControlEventObserver(ControlEvent.BEFORE_INIT_CELLS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_INIT_CELLS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_INIT_AGENTS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_FIRST_STATS, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_HALF_ITERATION, resetCellCounter);
		controller.registerControlEventObserver(ControlEvent.AFTER_END_ITERATION, resetCellCounter);
		
		return workProvider;
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public IController createSimController(IModel model) {
		IController controller = new Controller(model, this);
		controller.setWorkerSynchronizers(
				new BlockingSyncPoint(ControlEvent.BEFORE_INIT_CELLS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_CELLS, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_CELLS_ADD_NEIGHBORS, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_AGENTS,controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_FIRST_STATS, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_HALF_ITERATION, controller, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_END_ITERATION, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_SIMULATION, this.numThreads));
		return controller;
	}

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	@Override
	public String getCommandName() {
		return this.commandName;
	}

}
