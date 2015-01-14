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

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

public class Controller implements IController {
	
	private IModel model;
	
	private IWorkFactory workFactory;

	private ISyncPoint beforeInitCellsSync;
	private ISyncPoint afterInitCellsSync;
	private ISyncPoint afterAddCellsNeighsSync;
	private ISyncPoint afterAddAgentsSync;
	private ISyncPoint afterFirstStatsSync;
	private ISyncPoint afterHalfIterSync;
	private ISyncPoint afterEndIterSync;
	private ISyncPoint afterEndSimSync;
	
	private SimStatus simStatus;
	
	private List<Thread> modelWorkers;
	
	private CountDownLatch pauseLatch;
	
	public Controller(IModel model, IWorkFactory workFactory) {
		this.model = model;
		this.workFactory = workFactory;
		this.simStatus = SimStatus.STOPPED;
	}

	@Override
	public void setWorkerSynchronizers(
			ISyncPoint beforeInitCellsSync, 
			ISyncPoint afterInitCellsSync, 
			ISyncPoint afterAddCellsNeighsSync,
			ISyncPoint afterAddAgentsSync, 
			ISyncPoint afterFirstStatsSync, 
			ISyncPoint afterHalfIterSync,
			ISyncPoint afterEndIterSync,
			ISyncPoint afterEndSimSync) {

		this.beforeInitCellsSync = beforeInitCellsSync; 
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
		
		IControlEventObserver stopSim = new IControlEventObserver() {
			@Override
			public void update(ControlEvent event, IController controller) {
				
				/* The controller stop method must be called asynchronously because it waits
				 * for all simulations to finish before marking the model as stopped. Otherwise
				 * there would be a deadlock. */
				new Thread(new Runnable() {
					@Override
					public void run() {
						try {
							stop();
						} catch (IllegalSimStateException isse) {
							throw new RuntimeException(
									"Somebody stopped the simulation at an impossible time. It's a bug then.");
						}
					}
				}).start();
			}
		};
		
		this.afterFirstStatsSync.registerObserver(updateModelIteration);
		this.afterEndIterSync.registerObserver(updateModelIteration);
		this.afterEndSimSync.registerObserver(stopSim);
	}
	
	@Override
	public void registerControlEventObserver(ControlEvent event, IControlEventObserver observer) {
		switch (event) {
			case BEFORE_INIT_CELLS:
				this.beforeInitCellsSync.registerObserver(observer);
				break;
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
	public void workerNotifyBeforeInitCells() throws InterruptedWorkException {
		this.beforeInitCellsSync.syncNotify(this);
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
		
		if (this.simStatus == SimStatus.PAUSED) {
			try {
				this.pauseLatch.await();
			} catch (InterruptedException e) {
				throw new InterruptedWorkException("Paused thread was interrupted.");
			}
		}
	}

	@Override
	public void workerNotifySimFinish() throws InterruptedWorkException {
		this.afterEndSimSync.syncNotify(this);
	}

	@Override
	public synchronized void stopNow() {
		this.beforeInitCellsSync.stopNow(); 
		this.afterInitCellsSync.stopNow(); 
		this.afterAddCellsNeighsSync.stopNow();
		this.afterAddAgentsSync.stopNow();
		this.afterFirstStatsSync.stopNow();
		this.afterHalfIterSync.stopNow();
		this.afterEndIterSync.stopNow();
		this.afterEndSimSync.stopNow();
		
		if (this.isPaused()) {
			this.pauseLatch.countDown();
		}
		
//		this.model.setStatus(ModelStatus.STOPPED);
	}

	@Override
	public synchronized void start() throws IllegalSimStateException {
		
		if (this.simStatus == SimStatus.STOPPED) {
			
			this.beforeInitCellsSync.reset(); 
			this.afterInitCellsSync.reset(); 
			this.afterAddCellsNeighsSync.reset();
			this.afterAddAgentsSync.reset();
			this.afterFirstStatsSync.reset();
			this.afterHalfIterSync.reset();
			this.afterEndIterSync.reset();
			this.afterEndSimSync.reset();

			this.model.reset();
			this.model.start();
			this.modelWorkers = new ArrayList<Thread>();
			
			/* Model was stopped, launch simulation threads. */
			for (int i = 0; i < this.workFactory.getNumWorkers(); i++) {
				
				Thread modelWorker = new Thread(new ModelWorker(i, this.workFactory, this.model, this));
				this.modelWorkers.add(modelWorker);
				modelWorker.start();
			}
			
			this.simStatus = SimStatus.RUNNING;
			
		} else {
			
			throw new IllegalSimStateException("Simulation can only be started if status == " + SimStatus.STOPPED);
			
		}

	}

	@Override
	public synchronized void pauseContinue() throws IllegalSimStateException {

		if (this.simStatus == SimStatus.RUNNING) {
			
			this.model.pause();
			this.simStatus = SimStatus.PAUSED;
			this.pauseLatch = new CountDownLatch(1);
			
		} else if (this.simStatus == SimStatus.PAUSED) {
			
			this.model.unpause();
			this.simStatus = SimStatus.RUNNING;
			this.pauseLatch.countDown();
			
		} else {
			
			throw new IllegalSimStateException("Simulation can only be paused/continued if status != " + SimStatus.STOPPED);
			
		}


	}

	@Override
	public synchronized void stop() throws IllegalSimStateException {
		
		if (this.simStatus == SimStatus.PAUSED) {
			this.pauseContinue();
		}
		
		if (this.simStatus == SimStatus.RUNNING) {
			
			this.afterEndIterSync.stopNow();
			for (Thread modelWorker : this.modelWorkers) {
				try {
					modelWorker.join();
				} catch (InterruptedException e) {}
			}
			this.model.stop();
			this.simStatus = SimStatus.STOPPED;
			
		} else {
			
			throw new IllegalSimStateException("Simulation can't be stopped if status == " + SimStatus.STOPPED);
			
		}
	}

	/**
	 * Exporting can be done at any time because it is synchronized. Worst case scenario, you export
	 * all zeros.
	 */
	@Override
	public synchronized void export(String filename) {
		this.model.export(filename);
	}

	@Override
	public int getNumWorkers() {
		return this.workFactory.getNumWorkers();
	}

	@Override
	public boolean isRunning() {
		return this.simStatus == SimStatus.RUNNING;
	}

	@Override
	public boolean isPaused() {
		return this.simStatus == SimStatus.PAUSED;
	}

	@Override
	public boolean isStopped() {
		return this.simStatus == SimStatus.STOPPED;
	}


}
