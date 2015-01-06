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

public class EqualSimWorkProvider extends AbstractSimWorkProvider {
	
	private class EqualSimWorkerState extends AbstractSimWorkerState {

		private int counter;
		private int first;
		private int last;
		
		public EqualSimWorkerState(int swId, Random rng, int first, int last) {
			super(swId, rng);
			this.first = first;
			this.last = last;
		}
	}
	
	private int size;
	private ICellPutAgentStrategy putAgentStrategy;
	private int numWorkers;

	private ISimSynchronizer afterCreateCellsSync;
	private ISimSynchronizer afterAddCellNeighborsSync;
	private ISimSynchronizer afterInitAgentsSync;
	
	public EqualSimWorkProvider(ISimSpace space, int grassRestart, ICellGrassInitStrategy grassInitStrategy, 
			ICellPutAgentStrategy putAgentStrategy, int numWorkers, 
			ISimSynchronizer afterInitSync, ISimSynchronizer afterHalfIterSync, 
			ISimSynchronizer afterEndIterSync, ISimSynchronizer afterEndSimSync) {
		
		super(space, grassRestart, grassInitStrategy, afterInitSync, afterHalfIterSync, afterEndIterSync, afterEndSimSync);
	
		this.size = space.getSize();
		this.numWorkers = numWorkers;
		this.putAgentStrategy = putAgentStrategy;
		
		if (numWorkers > 1) {
			this.afterCreateCellsSync = new BlockingSimSynchronizer(null, numWorkers);
			this.afterAddCellNeighborsSync = new BlockingSimSynchronizer(null, numWorkers);
			this.afterInitAgentsSync = new BlockingSimSynchronizer(null, numWorkers);
		} else {
			this.afterCreateCellsSync = new BasicSimSynchronizer(null);
			this.afterAddCellNeighborsSync = new BasicSimSynchronizer(null);
			this.afterInitAgentsSync = new BasicSimSynchronizer(null);
		}
		
	}

	@Override
	protected ISimWorkerState doRegisterWorker(int swId, Random rng) {
		
		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = swId * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((swId + 1) * cellsPerThread, this.size); /* Exclusive */

		/* Create a worker state adequate for this work provider. */
		EqualSimWorkerState swState = new EqualSimWorkerState(swId, rng, startCellIdx, endCellIdx);
		
		/* Return this thread's state. */
		return swState;
	}

	@Override
	public void initCells(ISimWorkerState swState) throws SimWorkerException {
		
		
		/* Set cell neighbors. */
		EqualSimWorkerState deswState = (EqualSimWorkerState) swState;
	
		/* Initialize simulation grid cells. */
		for (int currCellIdx = deswState.first; currCellIdx < deswState.last; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			space.setCell(currCellIdx, new Cell(grassRestart, swState.getRng(), this.grassInitStrategy, this.putAgentStrategy));
			space.getCell(currCellIdx).initGrass();
			
		}
		
		this.afterCreateCellsSync.syncNotify();
	
		/* Set cell neighbors. */
		for (int currCellIdx = deswState.first; currCellIdx < deswState.last; currCellIdx++) {
			this.space.setNeighbors(currCellIdx);
		}
		
		this.afterAddCellNeighborsSync.syncNotify();
	
		
	}

	@Override
	public ICell getNextCell(ISimWorkerState swState) {
		
		EqualSimWorkerState tState = (EqualSimWorkerState) swState;
		ICell nextCell = null;

		if (tState.counter < tState.last) {
			nextCell = space.getCell(tState.counter);
			tState.counter++;
		}
		return nextCell;
	}

	private void resetNextCell(ISimWorkerState swState) {
		EqualSimWorkerState tState = (EqualSimWorkerState) swState;
		tState.counter = tState.first;
	}
	
	@Override
	public void initAgents(ISimWorkerState swState, SimParams params) throws SimWorkerException {

		EqualSimWorkerState tState = (EqualSimWorkerState) swState;
		
		/* Determine sheep per thread. The bellow operation is equivalent to ceil(numSheep/numThreads) */
		int sheepPerThread = (params.getInitSheep() + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end sheep index for current thread. */
		int startSheepIdx = tState.getSimWorkerId() * sheepPerThread; /* Inclusive */
		int endSheepIdx = Math.min((tState.getSimWorkerId() + 1) * sheepPerThread, params.getInitSheep()); /* Exclusive */
		
		for (int sheepIdx = startSheepIdx; sheepIdx < endSheepIdx; sheepIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent sheep = new Sheep(1 + tState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			space.getCell(idx).putNewAgent(sheep);
		}

		/* Determine wolves per thread. The bellow operation is equivalent to ceil(numWolves/numThreads) */
		int wolvesPerThread = (params.getInitWolves() + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end wolves index for current thread. */
		int startWolvesIdx = tState.getSimWorkerId() * wolvesPerThread; /* Inclusive */
		int endWolvesIdx = Math.min((tState.getSimWorkerId() + 1) * wolvesPerThread, params.getInitWolves()); /* Exclusive */
		
		for (int wolvesIdx = startWolvesIdx; wolvesIdx < endWolvesIdx; wolvesIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent wolf = new Wolf(1 + tState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			space.getCell(idx).putNewAgent(wolf);
		}
		
		this.resetNextCell(swState);
		this.afterInitAgentsSync.syncNotify();
		
	}


	@Override
	protected void doSyncAfterInit(ISimWorkerState swState) {
		this.resetNextCell(swState);
	}

	@Override
	protected void doSyncAfterHalfIteration(ISimWorkerState swState) {
		this.resetNextCell(swState);
	}

	@Override
	protected void doSyncAfterEndIteration(ISimWorkerState swState) {
		this.resetNextCell(swState);
	}

	@Override
	protected void doSyncAfterSimFinish(ISimWorkerState swState) {}

	
}


