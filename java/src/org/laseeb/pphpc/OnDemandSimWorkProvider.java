package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
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

	public OnDemandSimWorkProvider(int x, int y, int grassRestart,
			CellGrassInitStrategy grassInitStrategy, Threading threading,
			int numThreads) {
		super(x, y, grassRestart, grassInitStrategy, threading, numThreads);
		

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
			odtState.last = Math.min(odtState.current + this.block, this.size);

		}
		
		if (odtState.current < this.size) {
			cell = this.cells[odtState.current];
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
			int idx = tState.getRng().nextInt(this.size);
			IAgent sheep = new Sheep(1 + tState.getRng().nextInt(2 * params.getSheepGainFromFood()), params);
			this.cells[idx].putNewAgent(sheep);			
		}
		
		/* Create wolves. */
		while (wolvesCounter.getAndIncrement() < numWolves) {
			int idx = tState.getRng().nextInt(this.size);
			IAgent wolf = new Wolf(1 + tState.getRng().nextInt(2 * params.getWolvesGainFromFood()), params);
			this.cells[idx].putNewAgent(wolf);
		}
		
	}

	@Override
	protected ISimWorkerState createCells(int swId, Random rng) {
		
		OnDemandWorkerState odtState = new OnDemandWorkerState(rng, swId);
		
		int currCellIdx;
		while ((currCellIdx = this.initCellCounter.getAndIncrement()) < this.size) {
			this.cells[currCellIdx] = new Cell(this.grassRestart, rng, this.grassInitStrategy, this.threading.getPutAgentBehavior());
			this.cells[currCellIdx].initGrass();
		}
		
		return odtState;
	}

	@Override
	protected void setCellNeighbors(ISimWorkerState istState) {
		
		int currCellIdx;
		while ((currCellIdx = this.neighCellCounter.getAndIncrement()) < this.size) {

			int up = currCellIdx - this.x >= 0 ? currCellIdx - this.x : this.size - x + currCellIdx;
			int down = currCellIdx + this.x < this.size  ? currCellIdx + this.x : currCellIdx + this.x - this.size;
			int right = currCellIdx + 1 < this.size ? currCellIdx + 1 : 0;
			int left = currCellIdx - 1 >= 0 ? currCellIdx - 1 : this.size - 1;
			
			List<ICell> neighborhood = Arrays.asList(
					this.cells[currCellIdx], this.cells[up], this.cells[right], this.cells[down], this.cells[left]); 
		
			this.cells[currCellIdx].setNeighborhood(Collections.unmodifiableList(neighborhood));

		}
	}

}
