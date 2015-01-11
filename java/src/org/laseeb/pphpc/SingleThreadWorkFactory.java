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

@Parameters(commandNames = {"st"}, commandDescription = "Single-threaded work command")
public class SingleThreadWorkFactory implements IWorkFactory {

	private String commandName = "st";

	@Override
	public IWorkProvider getWorkProvider(int workSize, IController controller) {
		return new SingleThreadWorkProvider(workSize);
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentAsync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentAsync();
	}

	@Override
	public IController createSimController(IModel model) {
		IController controller = new Controller(model, this);
		controller.setWorkerSynchronizers(new SingleThreadSyncPoint(ControlEvent.AFTER_INIT_CELLS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_INIT_AGENTS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_FIRST_STATS), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_HALF_ITERATION), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_END_ITERATION), 
				new SingleThreadSyncPoint(ControlEvent.AFTER_END_SIMULATION));
		return controller;
	}

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new SingleThreadGlobalStats(iters);
	}

	@Override
	public int getNumWorkers() {
		return 1;
	}

	@Override
	public String getCommandName() {
		return this.commandName ;
	}

}
