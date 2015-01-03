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
import java.util.concurrent.atomic.AtomicInteger;

public class OnDemandSimWorkProvider extends AbstractSimWorkProvider {
	
	private class OnDemandWorkerState extends AbstractSimWorkerState {

		private int current;
		private int last;
		
		public OnDemandWorkerState(Random rng, int swId) {
			super(rng, swId);
		}
	}

	private AtomicInteger initCellCounter;
	private AtomicInteger neighCellCounter;
	private AtomicInteger cellCounter;
	private AtomicInteger sheepCounter;
	private AtomicInteger wolvesCounter;
	
	final int block = 20;

	public OnDemandSimWorkProvider(ISimSpace space, int grassRestart,
			CellGrassInitStrategy grassInitStrategy, Threading threading,
			int numThreads) {
		super(space, grassRestart, grassInitStrategy, threading, numThreads);
		

		this.cellCounter = new AtomicInteger(0);
		this.sheepCounter = new AtomicInteger(0);
		this.wolvesCounter = new AtomicInteger(0);
		this.initCellCounter = new AtomicInteger(0);
		this.neighCellCounter = new AtomicInteger(0);
	}

	@Override
	public ICell getNextCell(ISimWorkerState tState) {
		
		OnDemandWorkerState odtState = (OnDemandWorkerState) tState;
		
		ICell cell = null;
		
		if (odtState.current >= odtState.last) {

			odtState.current = this.cellCounter.getAndAdd(this.block);
			odtState.last = Math.min(odtState.current + this.block, this.space.getSize());

		}
		
		if (odtState.current < this.space.getSize()) {
			cell = space.getCell(odtState.current);
			odtState.current++;
		}

		return cell;
	}

	@Override
	public void resetNextCell(ISimWorkerState tState) {
		
		OnDemandWorkerState odtState = (OnDemandWorkerState) tState;
		if (odtState.getSimWorkerId() == 0) {
			this.cellCounter.set(0);
		}
		
	}

	@Override
	public void initAgents(ISimWorkerState tState, SimParams params) {
		
		int numSheep = params.getInitSheep();
		int numWolves = params.getInitWolves();
		
		/* Create sheep. */
		while (sheepCounter.getAndIncrement() < numSheep) {
			int idx = tState.getRng().nextInt(this.space.getSize());
			IAgent sheep = new Sheep(1 + tState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			space.getCell(idx).putNewAgent(sheep);			
		}
		
		/* Create wolves. */
		while (wolvesCounter.getAndIncrement() < numWolves) {
			int idx = tState.getRng().nextInt(this.space.getSize());
			IAgent wolf = new Wolf(1 + tState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			space.getCell(idx).putNewAgent(wolf);
		}
		
	}

	@Override
	protected ISimWorkerState createCells(int swId, Random rng) {
		
		OnDemandWorkerState odtState = new OnDemandWorkerState(rng, swId);
		
		int currCellIdx;
		while ((currCellIdx = this.initCellCounter.getAndIncrement()) < this.space.getSize()) {
			this.space.setCell(currCellIdx, new Cell(this.grassRestart, rng, this.grassInitStrategy, this.threading.getPutAgentBehavior()));
			space.getCell(currCellIdx).initGrass();
		}
		
		return odtState;
	}

	@Override
	protected void setCellNeighbors(ISimWorkerState istState) {
		
		int currCellIdx;
		while ((currCellIdx = this.neighCellCounter.getAndIncrement()) < this.space.getSize()) {
			this.space.setNeighbors(currCellIdx);
		}
	}

}
