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

import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

public class OnDemandSimWorkProvider extends AbstractSimWorkProvider {
	
	private class OnDemandWorkerState extends AbstractSimWorkerState {

		private int current;
		private int last;
		
		public OnDemandWorkerState(int swId, Random rng) {
			super(swId, rng);
			this.current = 0;
			this.last = 0;
		}
	}

	private AtomicInteger initCellCounter;
	private AtomicInteger neighCellCounter;
	private AtomicInteger cellCounter;
	private AtomicInteger sheepCounter;
	private AtomicInteger wolvesCounter;
	
	private int block;
	private ICellPutAgentStrategy putAgentStrategy;
	
	private ISimSynchronizer afterCreateCellsSync;
	private ISimSynchronizer afterAddCellNeighborsSync;
	private ISimSynchronizer afterInitAgentsSync;

	public OnDemandSimWorkProvider(int block, ISimSpace space, int grassRestart, ICellGrassInitStrategy grassInitStrategy, 
			ICellPutAgentStrategy putAgentStrategy, int numWorkers, ISimSynchronizer afterInitSync, 
			ISimSynchronizer afterHalfIterSync, ISimSynchronizer afterEndIterSync, ISimSynchronizer afterEndSimSync) {

		super(space, grassRestart, grassInitStrategy, afterInitSync, afterHalfIterSync, afterEndIterSync, afterEndSimSync);
	
		this.block = block;
		this.putAgentStrategy = putAgentStrategy;
		
		this.cellCounter = new AtomicInteger(0);
		this.sheepCounter = new AtomicInteger(0);
		this.wolvesCounter = new AtomicInteger(0);
		this.initCellCounter = new AtomicInteger(0);
		this.neighCellCounter = new AtomicInteger(0);
		
		if (numWorkers > 1) {

			this.afterCreateCellsSync = new BlockingSimSynchronizer(null, numWorkers);
			this.afterAddCellNeighborsSync = new BlockingSimSynchronizer(null, numWorkers);
			this.afterInitAgentsSync = new BlockingSimSynchronizer(null, numWorkers);

			IObserver resetCellCounter = new IObserver() {
				@Override
				public void update(SimEvent event) { cellCounter.set(0); }
			};
			
			this.afterInitSync.registerObserver(resetCellCounter);
			this.afterEndIterSync.registerObserver(resetCellCounter);
			this.afterHalfIterSync.registerObserver(resetCellCounter);

		
		} else {
			this.afterCreateCellsSync = new BasicSimSynchronizer(null);
			this.afterAddCellNeighborsSync = new BasicSimSynchronizer(null);
			this.afterInitAgentsSync = new BasicSimSynchronizer(null);
		}

	}
	
	@Override
	protected ISimWorkerState doRegisterWorker(int swId, Random rng) {
		
		OnDemandWorkerState odwState = new OnDemandWorkerState(swId, rng);
		return odwState;
	}

	@Override
	public void initCells(ISimWorkerState swState) throws SimWorkerException {
		
		/* Initialize simulation grid cells. */
		int currCellIdx;
		while ((currCellIdx = getNextIndex(swState, this.initCellCounter, this.space.getSize())) >= 0) {
		
			/* Add cell to current place in grid. */
			this.space.setCell(currCellIdx, 
					new Cell(grassRestart, swState.getRng(), this.grassInitStrategy, this.putAgentStrategy));
			this.space.getCell(currCellIdx).initGrass(swState.getRng());
			
		}
		
		this.afterCreateCellsSync.syncNotify();
	
		/* Set cell neighbors. */
		while ((currCellIdx = getNextIndex(swState, this.neighCellCounter, this.space.getSize())) >= 0) {
			this.space.setNeighbors(currCellIdx);
		}
		
		this.afterAddCellNeighborsSync.syncNotify();
	
		
	}

	private int getNextIndex(ISimWorkerState swState, AtomicInteger counter, int size) {
		
		OnDemandWorkerState odtState = (OnDemandWorkerState) swState;
		
		int nextIndex = -1;
		
		if (odtState.current >= odtState.last) {

			odtState.current = counter.getAndAdd(this.block);
			odtState.last = Math.min(odtState.current + this.block, size);
			
		}
		
		if (odtState.current < size) {

			nextIndex = odtState.current;
			odtState.current++;
			
		}

		return nextIndex;
	}
	
	@Override
	public ICell getNextCell(ISimWorkerState swState) {
		int cellIdx = this.getNextIndex(swState, this.cellCounter, this.space.getSize());
		return cellIdx >= 0 ? space.getCell(cellIdx) : null;
	}

	private void resetWorkerNextItem(ISimWorkerState swState) {
		
		OnDemandWorkerState odtState = (OnDemandWorkerState) swState;
		odtState.current = 0;
		odtState.last = 0;
		
	}

	@Override
	public void initAgents(ISimWorkerState swState, SimParams params) throws SimWorkerException {
		
		int numSheep = params.getInitSheep();
		int numWolves = params.getInitWolves();
		
		/* Create sheep. */
		while (getNextIndex(swState, this.sheepCounter, numSheep) >= 0) {
			
			int cellIdx = swState.getRng().nextInt(this.space.getSize());
			IAgent sheep = new Sheep(1 + swState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			space.getCell(cellIdx).putNewAgent(sheep);
			
		}
		
		/* Create wolves. */
		while (getNextIndex(swState, this.wolvesCounter, numWolves) >= 0) {
			
			int cellIdx = swState.getRng().nextInt(this.space.getSize());
			IAgent wolf = new Wolf(1 + swState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			space.getCell(cellIdx).putNewAgent(wolf);
			
		}
		
		this.resetWorkerNextItem(swState);
		this.afterInitAgentsSync.syncNotify();
	}


	@Override
	protected void doSyncAfterInit(ISimWorkerState swState) {
		this.resetWorkerNextItem(swState);
	}

	@Override
	protected void doSyncAfterHalfIteration(ISimWorkerState swState) {
		this.resetWorkerNextItem(swState);
	}

	@Override
	protected void doSyncAfterEndIteration(ISimWorkerState swState) {
		this.resetWorkerNextItem(swState);
	}

	@Override
	protected void doSyncAfterSimFinish(ISimWorkerState swState) {}

}
