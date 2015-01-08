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
public class OnDemandWorkFactory extends AbstractWorkFactory {

	final private String commandName = "on_demand";

	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors", validateWith = PositiveInteger.class)
	private int numThreads = Runtime.getRuntime().availableProcessors();

	/* Block size for ON_DEMAND work type. */
	@Parameter(names = "-b", description = "Block size", validateWith = PositiveInteger.class)
	private Integer blockSize = 100;

	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, IController controller) {
		final OnDemandWorkProvider workProvider = new OnDemandWorkProvider(this.blockSize, workSize);
		IObserver resetCellCounter = new IObserver() {

			@Override
			public void update(SimEvent event, IModel model) {
				workProvider.resetWorkCounter();
			}
		};
		
		controller.registerSimEventObserver(SimEvent.AFTER_INIT_CELLS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_INIT_AGENTS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_FIRST_STATS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_HALF_ITERATION, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_END_ITERATION, resetCellCounter);
		
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
		return new Controller(model,
				new BlockingSynchronizer(SimEvent.AFTER_INIT_CELLS, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new BlockingSynchronizer(SimEvent.AFTER_INIT_AGENTS, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_FIRST_STATS, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_HALF_ITERATION, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_END_ITERATION, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_END_SIMULATION));
	}

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	@Override
	public int getNumWorkers() {
		return this.numThreads;
	}

	@Override
	public String getCommandName() {
		return this.commandName;
	}

}
