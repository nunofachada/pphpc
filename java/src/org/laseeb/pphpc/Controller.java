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

/**
 * Implementation of a simulation controller, in the MVC sense.
 * 
 * @author Nuno Fachada
 */
public class Controller implements IController {
	
	/* The simulation model, in the MVC sense. */
	private IModel model;
	
	/* The work factory. */
	private IWorkFactory workFactory;

	/* Synchronization points, one for each control event. */
	private ISyncPoint beforeInitCellsSync;
	private ISyncPoint afterInitCellsSync;
	private ISyncPoint afterAddCellsNeighsSync;
	private ISyncPoint afterAddAgentsSync;
	private ISyncPoint afterFirstStatsSync;
	private ISyncPoint afterHalfIterSync;
	private ISyncPoint afterEndIterSync;
	private ISyncPoint afterEndSimSync;
	
	/* Simulation status. */
	private SimStatus simStatus;
	
	/* List of model workers. */
	private List<Thread> modelWorkers;
	
	/* Latch used for pausing the simulation. */
	private CountDownLatch pauseLatch;
	
	/**
	 * Create a new controller instance.
	 * 
	 * @param model The simulation model, in the MVC sense.
	 * @param workFactory The work factory.
	 */
	public Controller(IModel model, IWorkFactory workFactory) {
		
		this.model = model;
		this.workFactory = workFactory;
		
		/* The simulation is initially stopped. */
		this.simStatus = SimStatus.STOPPED;
		
	}

	/**
	 * @see IController#setWorkerSynchronizers(ISyncPoint, ISyncPoint, ISyncPoint, ISyncPoint, ISyncPoint, ISyncPoint, ISyncPoint, ISyncPoint)
	 */
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

		/* Setup sync. points. */
		this.beforeInitCellsSync = beforeInitCellsSync; 
		this.afterInitCellsSync = afterInitCellsSync; 
		this.afterAddCellsNeighsSync = afterAddCellsNeighsSync;
		this.afterAddAgentsSync = afterAddAgentsSync;
		this.afterFirstStatsSync = afterFirstStatsSync;
		this.afterHalfIterSync = afterHalfIterSync;
		this.afterEndIterSync = afterEndIterSync;
		this.afterEndSimSync = afterEndSimSync;
		
		/* Setup control event observer to update model iterations. */
		IControlEventObserver updateModelIteration = new IControlEventObserver() {
			@Override
			public void update(ControlEvent event, IController controller) {
				model.incrementIteration();
			}
		};
		
		/* Setup control event observer to stop simulation when it finishes. */
		IControlEventObserver stopSim = new IControlEventObserver() {
			@Override
			public void update(ControlEvent event, IController controller) {
				
				/* The controller stop method must be called asynchronously because it waits
				 * for all workers to finish before marking the model as stopped. Otherwise
				 * there would be a deadlock. */
				new Thread(new Runnable() {
					@Override
					public void run() {
						try {
							stop();
						} catch (IllegalSimStateException isse) {
							throw new IllegalStateException(
									"Somebody stopped the simulation at an impossible time. It's a bug then.");
						}
					}
				}).start();
			}
		};
		
		/* Register observer to update model iteration after getting first stats. */
		this.afterFirstStatsSync.registerObserver(updateModelIteration);

		/* Register observer to update model iteration after iteration finishes. */
		this.afterEndIterSync.registerObserver(updateModelIteration);
		
		/* Register observer to stop simulation when it finishes. */
		this.afterEndSimSync.registerObserver(stopSim);
		
	}
	
	/**
	 * @see IController#registerControlEventObserver(ControlEvent, IControlEventObserver)
	 */
	@Override
	public void registerControlEventObserver(ControlEvent event, IControlEventObserver observer) {
		
		switch (event) {
			case BEFORE_INIT_CELLS:
				this.beforeInitCellsSync.registerObserver(observer);
				break;
			case AFTER_INIT_CELLS:
				this.afterInitCellsSync.registerObserver(observer);
				break;
			case AFTER_SET_CELL_NEIGHBORS:
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

	/**
	 * @see IController#workerNotifyBeforeInitCells()
	 */
	@Override
	public void workerNotifyBeforeInitCells() throws InterruptedWorkException {
		
		this.beforeInitCellsSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifyInitCells()
	 */
	@Override
	public void workerNotifyInitCells() throws InterruptedWorkException {
		
		this.afterInitCellsSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifySetCellNeighbors()
	 */
	@Override
	public void workerNotifySetCellNeighbors() throws InterruptedWorkException {
		
		this.afterAddCellsNeighsSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifyInitAgents()
	 */
	@Override
	public void workerNotifyInitAgents() throws InterruptedWorkException {
		
		this.afterAddAgentsSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifyFirstStats()
	 */
	@Override
	public void workerNotifyFirstStats() throws InterruptedWorkException {
		
		this.afterFirstStatsSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifyHalfIteration()
	 */
	@Override
	public void workerNotifyHalfIteration() throws InterruptedWorkException {
		
		this.afterHalfIterSync.syncNotify(this);
		
	}

	/**
	 * @see IController#workerNotifyEndIteration()
	 */
	@Override
	public void workerNotifyEndIteration() throws InterruptedWorkException {
		
		this.afterEndIterSync.syncNotify(this);
		
		/* If simulation status is paused... */
		if (this.simStatus == SimStatus.PAUSED) {
			
			try {
				
				/* ...wait until it's unpaused... */
				this.pauseLatch.await();
				
			} catch (InterruptedException e) {
				
				/* ...or until it's interrupted by another reason. */
				throw new InterruptedWorkException("Paused thread was interrupted.");
				
			}
		}
	}

	/**
	 * @see IController#workerNotifySimFinish()
	 */
	@Override
	public void workerNotifySimFinish() throws InterruptedWorkException {
		
		this.afterEndSimSync.syncNotify(this);
		
	}

	/**
	 * @see IController#stopNow()
	 */
	@Override
	public synchronized void stopNow() {
		
		/* Send stop now control signal to all synchronizers. */
		this.beforeInitCellsSync.stopNow(); 
		this.afterInitCellsSync.stopNow(); 
		this.afterAddCellsNeighsSync.stopNow();
		this.afterAddAgentsSync.stopNow();
		this.afterFirstStatsSync.stopNow();
		this.afterHalfIterSync.stopNow();
		this.afterEndIterSync.stopNow();
		this.afterEndSimSync.stopNow();
		
		/* If simulation is paused, release latch and let workers
		 * terminate. */
		if (this.isPaused()) {
			this.pauseLatch.countDown();
		}
		
	}

	/**
	 * @see IController#start()
	 */
	@Override
	public synchronized void start() throws IllegalSimStateException {
		
		/* Simulation can only start if it's stopped. */
		if (this.simStatus == SimStatus.STOPPED) {
			
			/* Reset control synchronizers. */
			this.beforeInitCellsSync.reset(); 
			this.afterInitCellsSync.reset(); 
			this.afterAddCellsNeighsSync.reset();
			this.afterAddAgentsSync.reset();
			this.afterFirstStatsSync.reset();
			this.afterHalfIterSync.reset();
			this.afterEndIterSync.reset();
			this.afterEndSimSync.reset();

			/* Reset and start model. */
			this.model.reset();
			this.model.start();
			
			/* Setup a list of model workers... */
			this.modelWorkers = new ArrayList<Thread>();
			
			/* ...create them, add them to the list and start them. */
			for (int i = 0; i < this.workFactory.getNumWorkers(); i++) {
				
				Thread modelWorker = new Thread(new SimWorker(i, this.workFactory, this.model, this));
				this.modelWorkers.add(modelWorker);
				modelWorker.start();
			}
			
			/* Set simulation status to "running". */
			this.simStatus = SimStatus.RUNNING;
			
		} else {
			
			/* If simulation is not stopped, throw exception. */
			throw new IllegalSimStateException("Simulation can only be started if status == " + SimStatus.STOPPED);
			
		}

	}

	/**
	 * @see IController#pauseContinue()
	 */
	@Override
	public synchronized void pauseContinue() throws IllegalSimStateException {

		if (this.simStatus == SimStatus.RUNNING) {
			
			/* If simulation is running, pause it.*/
			this.pauseLatch = new CountDownLatch(1);
			this.simStatus = SimStatus.PAUSED;
			this.model.pause();
			
		} else if (this.simStatus == SimStatus.PAUSED) {
			
			/* If simulation is paused, unpause it. */
			this.model.unpause();
			this.simStatus = SimStatus.RUNNING;
			this.pauseLatch.countDown();
			
		} else {
			
			/* Simulation can't be stopped for this method to be called. As such, throw exception. */
			throw new IllegalSimStateException("Simulation can only be paused/continued if status != " + SimStatus.STOPPED);
			
		}

	}

	/**
	 * @see IController#stop()
	 */
	@Override
	public synchronized void stop() throws IllegalSimStateException {
		
		/* If simulation is paused, first unpause it. */
		if (this.simStatus == SimStatus.PAUSED) {
			this.pauseContinue();
		}
		
		/* If simulation is running, stop it. */
		if (this.simStatus == SimStatus.RUNNING) {
			
			/* Simulation will stop at the end of the current iteration. */
			this.afterEndIterSync.stopNow();
			
			/* Wait for all workers to finish... */
			for (Thread modelWorker : this.modelWorkers) {
				try {
					modelWorker.join();
				} catch (InterruptedException e) {}
			}
			
			/* ...and stop the model, setting the simulation status to "stopped". */
			this.model.stop();
			this.simStatus = SimStatus.STOPPED;
			
		} else {
			
			/* Simulation can't be stopped if it's already stopped. */
			throw new IllegalSimStateException("Simulation can't be stopped if status == " + SimStatus.STOPPED);
			
		}
	}

	/**
	 * @see IController#export(String)
	 */
	@Override
	public synchronized void export(String filename) {

		/* Exporting can be done at any time because it is synchronized. Worst case scenario, you export
		 * all zeros. */
		this.model.export(filename);
	}

	/**
	 * @see IController#getNumWorkers()
	 */
	@Override
	public int getNumWorkers() {
		
		return this.workFactory.getNumWorkers();
		
	}

	/**
	 * @see IController#isRunning()
	 */
	@Override
	public boolean isRunning() {
		
		return this.simStatus == SimStatus.RUNNING;
		
	}

	/**
	 * @see IController#isPaused()
	 */
	@Override
	public boolean isPaused() {
		
		return this.simStatus == SimStatus.PAUSED;
		
	}

	/**
	 * @see IController#isStopped()
	 */
	@Override
	public boolean isStopped() {
		
		return this.simStatus == SimStatus.STOPPED;
		
	}

}
