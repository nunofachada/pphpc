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

import java.util.Random;

public class EqualSimWorkProvider extends AbstractSimWorkProvider {

	private class DivideEqualSimWorkerState extends AbstractSimWorkerState {

		private int counter;
		private int first;
		private int last;
		
		public DivideEqualSimWorkerState(Random rng, int stId) {
			super(rng, stId);
		}
	}
	
	private int size;
	
	public EqualSimWorkProvider(ISimSpace space, int grassRestart,
			CellGrassInitStrategy grassInitStrategy, Threading threading,
			int numThreads) {
		
		super(space, grassRestart, grassInitStrategy, threading, numThreads);
	
		this.size = space.getSize();

	}

	@Override
	public ICell getNextCell(ISimWorkerState istState) {
		
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		ICell nextCell = null;

		if (tState.counter < tState.last) {
			nextCell = space.getCell(tState.counter);
			tState.counter++;
		}
		return nextCell;
	}

	@Override
	public void resetNextCell(ISimWorkerState istState) {
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		tState.counter = tState.first;
	}

	@Override
	protected ISimWorkerState createCells(int stId, Random rng) {
		
		/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
		int cellsPerThread = (this.size + numThreads - 1) / numThreads;
		
		/* Determine start and end cell index for current thread. */
		int startCellIdx = stId * cellsPerThread; /* Inclusive */
		int endCellIdx = Math.min((stId + 1) * cellsPerThread, this.size); /* Exclusive */
		
		/* Initialize simulation grid cells. */
		for (int currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
		
			/* Get x and y coordinates for this cell. */
			/* Add cell to current place in grid. */
			space.setCell(currCellIdx, new Cell(grassRestart, rng, this.grassInitStrategy, this.threading.getPutAgentBehavior()));
			space.getCell(currCellIdx).initGrass();
			
		}
		
		/* Thread state object for current thread. */
		DivideEqualSimWorkerState tState = new DivideEqualSimWorkerState(rng, stId);
		
		tState.first = startCellIdx;
		tState.last = endCellIdx;
	
		/* Return this thread's state. */
		return tState;
		
	}
	
	@Override
	protected void setCellNeighbors(ISimWorkerState istState) {
		
		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		
		/* Set cell neighbors. */
		for (int currCellIdx = tState.first; currCellIdx < tState.last; currCellIdx++) {
			this.space.setNeighbors(currCellIdx);
		}
		
	}

	@Override
	public void initAgents(ISimWorkerState istState, SimParams params) {

		DivideEqualSimWorkerState tState = (DivideEqualSimWorkerState) istState;
		
		/* Determine sheep per thread. The bellow operation is equivalent to ceil(numSheep/numThreads) */
		int sheepPerThread = (params.getInitSheep() + numThreads - 1) / numThreads;
		
		/* Determine start and end sheep index for current thread. */
		int startSheepIdx = tState.getSimWorkerId() * sheepPerThread; /* Inclusive */
		int endSheepIdx = Math.min((tState.getSimWorkerId() + 1) * sheepPerThread, params.getInitSheep()); /* Exclusive */
		
		for (int sheepIdx = startSheepIdx; sheepIdx < endSheepIdx; sheepIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent sheep = new Sheep(1 + tState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			space.getCell(idx).putNewAgent(sheep);
		}

		/* Determine wolves per thread. The bellow operation is equivalent to ceil(numWolves/numThreads) */
		int wolvesPerThread = (params.getInitWolves() + numThreads - 1) / numThreads;
		
		/* Determine start and end wolves index for current thread. */
		int startWolvesIdx = tState.getSimWorkerId() * wolvesPerThread; /* Inclusive */
		int endWolvesIdx = Math.min((tState.getSimWorkerId() + 1) * wolvesPerThread, params.getInitWolves()); /* Exclusive */
		
		for (int wolvesIdx = startWolvesIdx; wolvesIdx < endWolvesIdx; wolvesIdx++) {
			int idx = tState.getRng().nextInt(params.getGridX() * params.getGridY());
			IAgent wolf = new Wolf(1 + tState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			space.getCell(idx).putNewAgent(wolf);
		}
		
	}
	
}
