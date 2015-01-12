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

public interface IController {

	public void setWorkerSynchronizers(ISyncPoint beforeInitCellsSync,
			ISyncPoint afterInitCellsSync,
			ISyncPoint afterAddCellsNeighsSync, ISyncPoint afterAddAgentsSync,
			ISyncPoint afterFirstStatsSync, ISyncPoint afterHalfIterSync,
			ISyncPoint afterEndIterSync, ISyncPoint afterEndSimSync);

	public void registerControlEventObserver(ControlEvent event, IControlEventObserver observer);

	public void workerNotifyBeforeInitCells() throws InterruptedWorkException;
	
	public void workerNotifyInitCells() throws InterruptedWorkException;

	public void workerNotifyCellsAddNeighbors() throws InterruptedWorkException;

	public void workerNotifyInitAgents() throws InterruptedWorkException;

	public void workerNotifyFirstStats() throws InterruptedWorkException;

	public void workerNotifyHalfIteration() throws InterruptedWorkException;

	public void workerNotifyEndIteration() throws InterruptedWorkException;

	public void workerNotifySimFinish() throws InterruptedWorkException;

	public void stopNow();
	
	public void stop();
	
	public void start();
	
	public void pause();

	public void unpause();

	public void export(String filename);

	public int getNumWorkers();

}
