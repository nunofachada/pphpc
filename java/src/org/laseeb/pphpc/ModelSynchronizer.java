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

public class ModelSynchronizer implements IModelSynchronizer {
	
	private IModelState model;

	private ISyncPoint afterInitCellsSync;
	private ISyncPoint afterAddCellsNeighsSync;
	private ISyncPoint afterAddAgentsSync;
	private ISyncPoint afterFirstStatsSync;
	private ISyncPoint afterHalfIterSync;
	private ISyncPoint afterEndIterSync;
	private ISyncPoint afterEndSimSync;
	
	public ModelSynchronizer(IModelState model,
			ISyncPoint afterInitCellsSync,
			ISyncPoint afterAddCellsNeighsSync,
			ISyncPoint afterAddAgentsSync,
			ISyncPoint afterFirstStatsSync,
			ISyncPoint afterHalfIterSync,
			ISyncPoint afterEndIterSync,
			ISyncPoint afterEndSimSync) {
		
		this.afterInitCellsSync = afterInitCellsSync; 
		this.afterAddCellsNeighsSync = afterAddCellsNeighsSync;
		this.afterAddAgentsSync = afterAddAgentsSync;
		this.afterFirstStatsSync = afterFirstStatsSync;
		this.afterHalfIterSync = afterHalfIterSync;
		this.afterEndIterSync = afterEndIterSync;
		this.afterEndSimSync = afterEndSimSync;
		
		this.model.setStatus(ModelStatus.STOPPED);
	}

	@Override
	public void registerSimEventObserver(ModelEvent event, IObserver observer) {
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
	public void syncAfterInitCells() throws WorkException {
		this.afterInitCellsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterCellsAddNeighbors() throws WorkException {
		this.afterAddCellsNeighsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterInitAgents() throws WorkException {
		this.afterAddAgentsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterFirstStats() throws WorkException {
		this.afterFirstStatsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterHalfIteration() throws WorkException {
		this.afterHalfIterSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterEndIteration() throws WorkException {
		this.afterEndIterSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterSimFinish() throws WorkException {
		this.afterEndSimSync.syncNotify(this.model);
	}

	@Override
	public void stopNow() {
		this.afterInitCellsSync.notifyTermination(); 
		this.afterAddCellsNeighsSync.notifyTermination();
		this.afterAddAgentsSync.notifyTermination();
		this.afterFirstStatsSync.notifyTermination();
		this.afterHalfIterSync.notifyTermination();
		this.afterEndIterSync.notifyTermination();
		this.afterEndSimSync.notifyTermination();
	}

	@Override
	public void start() {
		if (this.model.getStatus() == ModelStatus.PAUSED) {
			// TODO Unpause model
			// TODO Notify observers of unpause/continue
		} else if (this.model.getStatus() == ModelStatus.STOPPED) {
			// TODO Launch model
			// TODO Notify observers of start
		}

	}

	@Override
	public void pause() {
		// TODO Pause model
		// TODO Notify observers of pause
	}

	@Override
	public void stop() {
		// TODO Stop model
		// TODO Notify observers of stop
	}


}
