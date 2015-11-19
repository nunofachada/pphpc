/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Interface for simulation controllers.
 * 
 * @author Nuno Fachada
 */
public interface IController {

	/**
	 * Set worker synchronizers. This operation is usually performed by a work factory.
	 * 
	 * @param beforeInitCellsSync Synchronizes before cell initialization.
	 * @param afterInitCellsSync Synchronizes after cell initialization.
	 * @param afterSetCellNeighsSync Synchronizes after setting cell neighbors.
	 * @param afterInitAgentsSync Synchronizes after initializing agents.
	 * @param afterFirstStatsSync Synchronizes after getting first stats.
	 * @param afterHalfIterSync Synchronizes after half iteration.
	 * @param afterEndIterSync Synchronizes after an iteration is finished.
	 * @param afterEndSimSync Synchronizes after the simulation finishes.
	 */
	public void setWorkerSynchronizers(ISyncPoint beforeInitCellsSync,
			ISyncPoint afterInitCellsSync,
			ISyncPoint afterSetCellNeighsSync, ISyncPoint afterInitAgentsSync,
			ISyncPoint afterFirstStatsSync, ISyncPoint afterHalfIterSync,
			ISyncPoint afterEndIterSync, ISyncPoint afterEndSimSync);

	/**
	 * Registers a control event observer.
	 * 
	 * @param event Control event to be observed.
	 * @param observer Observer to be registered.
	 */
	public void registerControlEventObserver(ControlEvent event, IControlEventObserver observer);

	/**
	 * Used by workers to synchronize before cell initialization.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyBeforeInitCells() throws InterruptedWorkException;
	
	/**
	 * Used by workers to synchronize after cell initialization.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyInitCells() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after setting cell neighbors.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifySetCellNeighbors() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after initializing agents.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyInitAgents() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after getting first stats.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyFirstStats() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after half iteration.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyHalfIteration() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after an iteration is finished.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifyEndIteration() throws InterruptedWorkException;

	/**
	 * Used by workers to synchronize after the simulation finishes.
	 * 
	 * @throws InterruptedWorkException If work is interrupted.
	 */
	public void workerNotifySimFinish() throws InterruptedWorkException;

	/**
	 * Stop simulation as soon as possible, i.e. don't wait for the end
	 * of the current iteration.
	 */
	public void stopNow();
	
	/**
	 * Stop simulation after the current iteration.
	 * 
	 * @throws IllegalSimStateException If simulation is in a state where it can't be 
	 * stopped (e.g. it's already stopped).
	 */
	public void stop() throws IllegalSimStateException;
	
	/**
	 * Start simulation.
	 * 
	 * @throws IllegalSimStateException If simulation is in a state where it can't be 
	 * started (e.g. it's already started).
	 */
	public void start() throws IllegalSimStateException;
	
	/**
	 * Pause simulation if it's currently running, or continue simulation if it's currently
	 * paused.
	 * 
	 * @throws IllegalSimStateException If simulation is in a state where it can't be 
	 * paused or continued (e.g. it's stopped).
	 */
	public void pauseContinue() throws IllegalSimStateException;

	/**
	 * Export simulation statistics to a file.
	 * 
	 * @param filename File where to export simulation statistics to.
	 */
	public void export(String filename);

	/**
	 * Return the number of available simulation workers.
	 * 
	 * @return The number of available simulation workers.
	 */
	public int getNumWorkers();

	/**
	 * Is the simulation running?
	 * 
	 * @return True if simulation is running, false otherwise.
	 */
	public boolean isRunning();
	
	/**
	 * Is the simulation paused?
	 * 
	 * @return True if simulation is paused, false otherwise.
	 */
	public boolean isPaused();
	
	/**
	 * Is the simulation stopped?
	 * 
	 * @return True if simulation is stopped, false otherwise.
	 */
	public boolean isStopped();

}
