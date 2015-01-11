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

public class Controller implements IController {
	
	private IModel model;
	
	private IWorkFactory workFactory;

	private ISyncPoint afterInitCellsSync;
	private ISyncPoint afterAddCellsNeighsSync;
	private ISyncPoint afterAddAgentsSync;
	private ISyncPoint afterFirstStatsSync;
	private ISyncPoint afterHalfIterSync;
	private ISyncPoint afterEndIterSync;
	private ISyncPoint afterEndSimSync;
	
	public Controller(IModel model, IWorkFactory workFactory) {
		this.model = model;
		this.workFactory = workFactory;
	}

	@Override
	public void setWorkerSynchronizers(ISyncPoint afterInitCellsSync, ISyncPoint afterAddCellsNeighsSync,
			ISyncPoint afterAddAgentsSync, ISyncPoint afterFirstStatsSync, ISyncPoint afterHalfIterSync,
			ISyncPoint afterEndIterSync,ISyncPoint afterEndSimSync) {

		this.afterInitCellsSync = afterInitCellsSync; 
		this.afterAddCellsNeighsSync = afterAddCellsNeighsSync;
		this.afterAddAgentsSync = afterAddAgentsSync;
		this.afterFirstStatsSync = afterFirstStatsSync;
		this.afterHalfIterSync = afterHalfIterSync;
		this.afterEndIterSync = afterEndIterSync;
		this.afterEndSimSync = afterEndSimSync;
		
		IControlEventObserver updateModelIteration = new IControlEventObserver() {
			@Override
			public void update(ControlEvent event, IController controller) {
				model.incrementIteration();
			}
		};
		
		IControlEventObserver stopModel = new IControlEventObserver() {
			@Override
			public void update(ControlEvent event, IController controller) {
				model.setStatus(ModelStatus.STOPPED);
			}
		};
		
		this.afterFirstStatsSync.registerObserver(updateModelIteration);
		this.afterEndIterSync.registerObserver(updateModelIteration);
		this.afterEndSimSync.registerObserver(stopModel);
	}
	
	@Override
	public void registerControlEventObserver(ControlEvent event, IControlEventObserver observer) {
		switch (event) {
			case AFTER_INIT_CELLS:
				this.afterInitCellsSync.registerObserver(observer);
				break;
			case AFTER_CELLS_ADD_NEIGHBORS:
				this.afterAddCellsNeighsSync.registerObserver(observer);
				break;
			case AFTER_INIT_AGENTS:
				this.afterAddAgentsSync.registerObserver(observer);
				break;
			case AFTER_FIRST_STATS:
				this.afterFirstStatsSync.registerObserver(observer);
				break;
			case AFTER_HALF_ITERATION:
				this.afterHalfIterSync.registerObserver(observer);
				break;
			case AFTER_END_ITERATION:
				this.afterEndIterSync.registerObserver(observer);
				break;
			case AFTER_END_SIMULATION:
				this.afterEndSimSync.registerObserver(observer);
				break;
		}
	}

	@Override
	public void workerNotifyInitCells() throws InterruptedWorkException {
		this.afterInitCellsSync.syncNotify(this);
	}

	@Override
	public void workerNotifyCellsAddNeighbors() throws InterruptedWorkException {
		this.afterAddCellsNeighsSync.syncNotify(this);
	}

	@Override
	public void workerNotifyInitAgents() throws InterruptedWorkException {
		this.afterAddAgentsSync.syncNotify(this);
	}

	@Override
	public void workerNotifyFirstStats() throws InterruptedWorkException {
		this.afterFirstStatsSync.syncNotify(this);
	}

	@Override
	public void workerNotifyHalfIteration() throws InterruptedWorkException {
		this.afterHalfIterSync.syncNotify(this);
	}

	@Override
	public void workerNotifyEndIteration() throws InterruptedWorkException {
		this.afterEndIterSync.syncNotify(this);
	}

	@Override
	public void workerNotifySimFinish() throws InterruptedWorkException {
		this.afterEndSimSync.syncNotify(this);
	}

	@Override
	public void stopNow() {
		this.afterInitCellsSync.stopNow(); 
		this.afterAddCellsNeighsSync.stopNow();
		this.afterAddAgentsSync.stopNow();
		this.afterFirstStatsSync.stopNow();
		this.afterHalfIterSync.stopNow();
		this.afterEndIterSync.stopNow();
		this.afterEndSimSync.stopNow();
		
		this.model.setStatus(ModelStatus.STOPPED);
	}

	@Override
	public void start() {
		
		ModelStatus previousStatus = this.model.getStatus();
		
		this.model.setStatus(ModelStatus.RUNNING);
		
		if (previousStatus == ModelStatus.STOPPED) {
			
			/* Model was stopped, launch simulation threads. */
			for (int i = 0; i < this.workFactory.getNumWorkers(); i++) {
				new Thread(new ModelWorker(i, this.workFactory, this.model, this)).start();
			}
			
		}

	}

	@Override
	public void pause() {
		this.model.setStatus(ModelStatus.PAUSED);
	}

	@Override
	public void stop() {
		this.model.setStatus(ModelStatus.STOPPED);
	}

	@Override
	public void export(String filename) {
		this.model.export(filename);
	}

	@Override
	public int getNumWorkers() {
		return this.workFactory.getNumWorkers();
	}


}
