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

/**
 * The PPHPC multithreaded package.
 */
package org.laseeb.pphpc;

import java.security.GeneralSecurityException;
import java.util.Arrays;
import java.util.Random;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicIntegerArray;

import org.uncommons.maths.random.SeedException;

import com.beust.jcommander.Parameter;

/**
 * Main class for multi-threaded predator-prey model.
 * @author Nuno Fachada
 */
public class PredPreyMulti extends PredPrey {
	
	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors")
	private int numThreads = Runtime.getRuntime().availableProcessors();
	
	@Parameter(names = "-x", description = "Make the simulation repeatable? (slower)")
	private boolean repeatable = false;

	/* Statistics gathering. */
	private AtomicIntegerArray sheepStats;
	private AtomicIntegerArray wolfStats;
	private AtomicIntegerArray grassStats;
	
	/* Thread barrier. */
	private CyclicBarrier mainBarrier;
	private CyclicBarrier partialBarrier;
	
	/* Latch on which the main thread will wait until the simulation threads
	 * terminate. */
	private CountDownLatch latch;
	
	/* Cell behaviors. */
	CellFutureIsNowPostBehavior futureIsNowPost;
	CellPutAgentBehavior putAgentNow;
	CellPutAgentBehavior putAgentFuture = new CellPutAgentSync();
	
	
	/** 
	 * Constructor, no arguments required.
	 */
	public PredPreyMulti() {}
	
	/* A simulation thread. */
	private class SimThread implements Runnable {
		
		/* Grid offset for each thread. */
		private int offset; 
		
		/* Random number generator for current thread. */
		Random rng;
		
		/* Constructor only sets the grid offset each thread. */
		public SimThread(int offset) {
			this.offset = offset;
		}
		
		/* Simulate! */
		public void run() {
			
			/* Initialize this thread's random number generator. */
			try {
				rng = createRNG(Thread.currentThread().getId());
			} catch (Exception e) {
				errMessage(e);
				return;
			}
			
			/* Partial statistics */
			int sheepStatsPartial;
			int wolfStatsPartial;
			int grassStatsPartial;

			/* Determine the grid size. */
			long gridsize = ((long) params.getGridX()) * params.getGridY();
		
			/* Determine cells per thread. The bellow operation is equivalent to ceil(gridsize/numThreads) */
			long cellsPerThread = (gridsize + numThreads - 1) / numThreads;
			
			/* Determine start and end cell index for current thread. */
			long startCellIdx = offset * cellsPerThread; /* Inclusive */
			long endCellIdx = Math.min((offset + 1) * cellsPerThread, gridsize); /* Exclusive */
			
//			System.out.println("Thread " + offset + " has cells from " + startCellIdx + " to " + (endCellIdx - 1));
			
			/* Initialize simulation grid cells. */
			for (long currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
			
				/* Get x and y coordinates for this cell. */
				int currCellY = (int) (currCellIdx / params.getGridX());
				int currCellX = (int) (currCellIdx % params.getGridX());

				/* Add cell to current place in grid. */
				grid[currCellX][currCellY] = new Cell(
						params.getGrassRestart(), putAgentNow, putAgentFuture, futureIsNowPost);
				
				/* Grow grass in current cell. */
				if (rng.nextBoolean()) {
				
					/* Grass not alive, initialize grow timer. */
					grid[currCellX][currCellY].setGrass(1 + rng.nextInt(params.getGrassRestart()));
					
				} else {
					
					/* Grass alive. */
					grid[currCellX][currCellY].setGrass(0);
					
					/* Update grass statistics. */
					grassStats.getAndIncrement(0);
					
				}

			}
			
			/* Sync. with barrier. */
			try {
				partialBarrier.await();
			} catch (InterruptedException e) {
				errMessage(e);
				return;
			} catch (BrokenBarrierException e) {
				errMessage(e);
				return;
			}

			/* Populate simulation grid with agents. */

			/* Determine sheep per thread. The bellow operation is equivalent to ceil(numSheep/numThreads) */
			int sheepPerThread = (params.getInitSheep() + numThreads - 1) / numThreads;
			
			/* Determine start and end sheep index for current thread. */
			int startSheepIdx = offset * sheepPerThread; /* Inclusive */
			int endSheepIdx = Math.min((offset + 1) * sheepPerThread, params.getInitSheep()); /* Exclusive */
			
			for (int sheepIdx = startSheepIdx; sheepIdx < endSheepIdx; sheepIdx++) {
				int x = rng.nextInt(params.getGridX());
				int y = rng.nextInt(params.getGridY());
				IAgent sheep = new Sheep(1 + rng.nextInt(2 * params.getSheepGainFromFood()), params);
				grid[x][y].putAgentNow(sheep);
			}

			/* Determine wolves per thread. The bellow operation is equivalent to ceil(numWolves/numThreads) */
			int wolvesPerThread = (params.getInitWolves() + numThreads - 1) / numThreads;
			
			/* Determine start and end wolves index for current thread. */
			int startWolvesIdx = offset * wolvesPerThread; /* Inclusive */
			int endWolvesIdx = Math.min((offset + 1) * wolvesPerThread, params.getInitWolves()); /* Exclusive */
			
			for (int wolvesIdx = startWolvesIdx; wolvesIdx < endWolvesIdx; wolvesIdx++) {
				int x = rng.nextInt(params.getGridX());
				int y = rng.nextInt(params.getGridY());
				IAgent wolf = new Wolf(1 + rng.nextInt(2 * params.getWolvesGainFromFood()), params);		
				grid[x][y].putAgentNow(wolf);
			}
			
			/* Sync. with barrier. */
			try {
				partialBarrier.await();
			} catch (InterruptedException e) {
				errMessage(e);
				return;
			} catch (BrokenBarrierException e) {
				errMessage(e);
				return;
			}

			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {
				
				/* Cycle through cells in order to perform step 1 and 2 of simulation. */
				for (long currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
				
					/* Get x and y coordinates for this cell. */
					int currCellY = (int) (currCellIdx / params.getGridX());
					int currCellX = (int) (currCellIdx % params.getGridX());
					
					/* ************************* */
					/* ** 1 - Agent movement. ** */
					/* ************************* */

					/* Cycle through agents in current cell. */
					for (IAgent agent : grid[currCellX][currCellY].getAgents()) {
						
						/* Decrement agent energy. */
						agent.decEnergy();

						/* If agent energy is greater than zero... */
						if (agent.getEnergy() > 0) {
							
							/* ...perform movement. */
							int x = currCellX; int y = currCellY;
							
							/* Choose direction, if any. */
							int direction = rng.nextInt(5);
							
							/* If agent decides to move, move him. */
							if (direction == 1) {
								/* Move to the right. */
								x++;
								if (x == params.getGridX())
									x = 0;
							} else if (direction == 2) {
								/* Move to the left. */
								x--;
								if (x < 0)
									x = params.getGridX() - 1;
							} else if (direction == 3) {
								/* Move down. */
								y++;
								if (y == params.getGridY())
									y = 0;
							} else if (direction == 4) {
								/* Move up. */
								y--;
								if (y < 0)
									y = params.getGridY() - 1;
							}
							/* Move agent to new cell in the future. */
							grid[x][y].putAgentFuture(agent);
						}
					}

					/* ************************* */
					/* *** 2 - Grass growth. *** */
					/* ************************* */
					
					/* If grass is not alive... */
					if (grid[currCellX][currCellY].getGrass() > 0) {
						/* ...decrement alive counter. */
						grid[currCellX][currCellY].decGrass();
					}
					
				}
				
				/* Sync. with barrier. */
				try {
					partialBarrier.await();
				} catch (InterruptedException e) {
					errMessage(e);
					return;
				} catch (BrokenBarrierException e) {
					errMessage(e);
					return;
				}
				
				/* Reset statistics. */
				sheepStatsPartial = 0;
				wolfStatsPartial = 0;
				grassStatsPartial = 0;
				
				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				for (long currCellIdx = startCellIdx; currCellIdx < endCellIdx; currCellIdx++) {
					
					/* Get x and y coordinates for this cell. */
					int currCellY = (int) (currCellIdx / params.getGridX());
					int currCellX = (int) (currCellIdx % params.getGridX());

					/* ************************** */
					/* *** 3 - Agent actions. *** */
					/* ************************** */

					/* The future is now (future agents are now present agents)... */
					grid[currCellX][currCellY].futureIsNow();

					/* Cycle through agents in cell. */
					for (IAgent agent : grid[currCellX][currCellY].getAgents()) {
						
						/* Tell agent to act. */
						agent.doPlay(grid[currCellX][currCellY], rng);
						
					}
					
					/* Remove dead agents. */
					grid[currCellX][currCellY].removeAgentsToBeRemoved();
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */

					for (IAgent agent : grid[currCellX][currCellY].getAgents()) {

						if (agent instanceof Sheep)
							sheepStatsPartial++;
						else if (agent instanceof Wolf)
							wolfStatsPartial++;
					}
					
					if (grid[currCellX][currCellY].getGrass() == 0)
						grassStatsPartial++;
				}
				
				/* Update global statistics. */
				sheepStats.addAndGet(iter, sheepStatsPartial);
				wolfStats.addAndGet(iter, wolfStatsPartial);
				grassStats.addAndGet(iter, grassStatsPartial);
				
				/* Sync. with barrier. */
				try {
					mainBarrier.await();
				} catch (InterruptedException e) {
					errMessage(e);
					return;
				} catch (BrokenBarrierException e) {
					errMessage(e);
					return;
				}
			}
		}

	}
	
	/**
	 * Perform simulation.
	 * @throws GeneralSecurityException 
	 * @throws SeedException 
	 */
	protected void start() throws Exception {
		
		if (repeatable) {
			this.futureIsNowPost = new CellFutureIsNowPostSort();
			this.putAgentNow = new CellPutAgentSyncSort();
		} else {
			this.futureIsNowPost = new CellFutureIsNowPostNop();
			this.putAgentNow = new CellPutAgentSync();
		}
		
		/* Start timing. */
		long startTime = System.currentTimeMillis();
		
		/* Initialize latch. */
		latch = new CountDownLatch(1);

		/* Initialize thread barriers. */
		this.mainBarrier = new CyclicBarrier(numThreads, new Runnable() {
			int iter = 1;
			public void run() {
				/* Print current iteration, if that is the case. */
				if ((stepPrint > 0) && (iter % stepPrint == 0))
					System.out.println("Iter " + iter);
				iter++;
				/* If this is the last iteration, let main thread continue... */
				if (iter > params.getIters()) {
					latch.countDown();
				}				
			}
		});
		this.partialBarrier = new CyclicBarrier(numThreads);
		
		/* Initialize statistics. */
		int[] resetArray = new int[params.getIters() + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.sheepStats.set(0, params.getInitSheep());
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.wolfStats.set(0, params.getInitWolves());
		this.grassStats = new AtomicIntegerArray(resetArray);
		
		/* Initialize simulation grid. */
		grid = new Cell[params.getGridX()][params.getGridY()];
		
		/* Launch simulation threads. */
		for (int i = 0; i < numThreads; i++)
			(new Thread(new SimThread(i))).start();

		/* Wait for simulation threads to finish. */
		latch.await();

		/* Stop timing and show simulation time. */
		long endTime = System.currentTimeMillis();
		float timeInSeconds = (float) (endTime - startTime) / 1000.0f;
		System.out.println("Total simulation time: " + timeInSeconds + "\n");
		
	}
	
	/**
	 * Main function.
	 * 
	 * @param args Command line arguments.
	 */
	public static void main(String[] args) {
		
		int status = (new PredPreyMulti()).doMain(args);
		System.exit(status);
		
	}

	@Override
	protected int getStats(StatType st, int iter) {
		
		switch (st) {
			case SHEEP:
				return sheepStats.get(iter);
			case WOLVES:
				return wolfStats.get(iter);
			case GRASS:
				return grassStats.get(iter);
		}
		return 0;
	}

}
