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

@Parameters(commandNames = {"equal"}, commandDescription = "Equal work command")
public class EqualWorkFactory extends AbstractMultiThreadWorkFactory {
	
	final private String commandName = "equal";

	@Parameter(names = "-x", description = "Make the simulation repeatable? (slower)")
	private boolean repeatable = false;

	@Override
	public IWorkProvider doGetWorkProvider(int workSize, IModelSynchronizer controller) {
		return new EqualWorkProvider(this.numThreads, workSize);
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		if (this.repeatable)
			return new CellPutAgentSyncSort();
		else
			return new CellPutAgentSync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		if (this.repeatable)
			return new CellPutAgentSyncSort();
		else
			return new CellPutAgentSync();
	}

	@Override
	public IModelSynchronizer createSimController(IModelState model) {
		return new ModelSynchronizer(model,
				new BlockingSyncPoint(ModelEvent.AFTER_INIT_CELLS, model, this.numThreads), 
				new SingleThreadSyncPoint(ModelEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new BlockingSyncPoint(ModelEvent.AFTER_INIT_AGENTS, model, this.numThreads), 
				new SingleThreadSyncPoint(ModelEvent.AFTER_FIRST_STATS), 
				new BlockingSyncPoint(ModelEvent.AFTER_HALF_ITERATION, model, this.numThreads), 
				new BlockingSyncPoint(ModelEvent.AFTER_END_ITERATION, model, this.numThreads), 
				new SingleThreadSyncPoint(ModelEvent.AFTER_END_SIMULATION));
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
