/*
 * Copyright (c) 2014, Nuno Fachada
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

import java.util.concurrent.atomic.AtomicInteger;

public abstract class AbstractSimWorkProvider implements ISimWorkProvider {
	
	protected ISimSpace space;
	protected CellGrassInitStrategy grassInitStrategy;
	protected int grassRestart;

	private AtomicInteger swIdGenerator;
	
	protected ISimSynchronizer afterInitSync;
	protected ISimSynchronizer afterHalfIterSync;
	protected ISimSynchronizer afterEndIterSync;
	protected ISimSynchronizer afterEndSimSync;
	
	public AbstractSimWorkProvider(ISimSpace space, int grassRestart, CellGrassInitStrategy grassInitStrategy, 
			ISimSynchronizer afterInitSync, ISimSynchronizer afterHalfIterSync, 
			ISimSynchronizer afterEndIterSync, ISimSynchronizer afterEndSimSync) {
		
		this.space = space;
		this.grassRestart = grassRestart;
		this.grassInitStrategy = grassInitStrategy;
	
		this.swIdGenerator = new AtomicInteger(0);
		
		this.afterInitSync = afterInitSync;
		this.afterHalfIterSync = afterHalfIterSync;
		this.afterEndIterSync = afterEndIterSync;
		this.afterEndSimSync = afterEndSimSync;
		
	}

	protected abstract ISimWorkerState doRegisterWorker(int swId);
	
	protected abstract void doSyncAfterInit(ISimWorkerState swState);

	protected abstract void doSyncAfterHalfIteration(ISimWorkerState swState);

	protected abstract void doSyncAfterEndIteration(ISimWorkerState swState);

	protected abstract void doSyncAfterSimFinish(ISimWorkerState swState);
	
	@Override
	public ISimWorkerState registerWorker() {
		return doRegisterWorker(this.swIdGenerator.getAndIncrement());
	}
	
	@Override
	public void syncAfterInit(ISimWorkerState swState) {
		this.doSyncAfterInit(swState);
		this.afterInitSync.syncNotify();
	}
	
	@Override
	public void syncAfterHalfIteration(ISimWorkerState swState) {
		this.doSyncAfterHalfIteration(swState);
		this.afterHalfIterSync.syncNotify();
	}
	
	@Override
	public void syncAfterEndIteration(ISimWorkerState swState) {
		this.doSyncAfterEndIteration(swState);
		this.afterEndIterSync.syncNotify();
	}
	
	@Override
	public void syncAfterSimFinish(ISimWorkerState swState) {
		this.doSyncAfterSimFinish(swState);
		this.afterEndSimSync.syncNotify();
	}
	
	@Override
	public void registerObserver(SimEvent event, Observer observer) {
		switch (event) {
		
			case AFTER_INIT:
				this.afterInitSync.registerObserver(observer);
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

}
