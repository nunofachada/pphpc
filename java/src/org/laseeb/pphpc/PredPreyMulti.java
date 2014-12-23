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
import java.util.Iterator;
import java.util.Random;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;
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

	/* Statistics gathering. */
	private AtomicIntegerArray sheepStats;
	private AtomicIntegerArray wolfStats;
	private AtomicIntegerArray grassStats;
	
	/* Thread barrier. */
	private CyclicBarrier mainBarrier;
	private CyclicBarrier partialBarrier;
	
	/* Semaphore for waiting until the end of the simulation. */
	private Semaphore semaphore;
	
	/* Thread local random number generator. */
	ThreadLocal<Random> rng;
	
	/** 
	 * Constructor, no arguments required.
	 */
	public PredPreyMulti() {}
	
	/* A simulation thread. */
	private class SimThread implements Runnable {
		/* Grid offset for each thread. */
		private int offset; 
		/* Constructor only sets the grid offset each thread. */
		public SimThread(int offset) {
			this.offset = offset;
		}
		/* Simulate! */
		public void run() {
			
			/* Initialize my own random number generator. */
			try {
				rng.set(createRNG(Thread.currentThread().getId()));
			} catch (Exception e) {
				// TODO Auto-generated catch block
				throw new RuntimeException(e);
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
					Iterator<Agent> agentIter = grid[currCellX][currCellY].getAgents();
					while (agentIter.hasNext()) {
						
						/* Get next agent. */
						Agent agent = agentIter.next();

						/* Decrement agent energy. */
						agent.decEnergy();

						/* If agent energy is greater than zero... */
						if (agent.getEnergy() > 0) {
							
							/* ...perform movement. */
							int x = currCellX; int y = currCellY;
							
							/* Choose direction, if any. */
							int direction = rng.get().nextInt(5);
							
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
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (BrokenBarrierException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
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
					Iterator<Agent> agentIter = grid[currCellX][currCellY].getAgents();
					while (agentIter.hasNext()) {
						
						/* Get next agent in cell. */
						Agent agent = agentIter.next();
						
						/* Tell agent to act. */
						agent.doPlay(grid[currCellX][currCellY], rng.get());
						
					}
					
					/* Remove dead agents. */
					grid[currCellX][currCellY].removeAgentsToBeRemoved();
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */

					agentIter = grid[currCellX][currCellY].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
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
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (BrokenBarrierException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}

	}
	
	/**
	 * Perform simulation.
	 * @throws GeneralSecurityException 
	 * @throws SeedException 
	 */
	protected void start() throws SeedException, GeneralSecurityException {
		
		/* Initialize thread-safe random number generator. */
		this.rng = new ThreadLocal<Random>();
		rng.set(createRNG(0));
		
		/* Initialize and acquire one permit semaphore. */
		semaphore = new Semaphore(1);
		try {
			semaphore.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		/* Initialize thread barriers. */
		this.mainBarrier = new CyclicBarrier(numThreads, new Runnable() {
			int iter = 1;
			public void run() {
				/* Print current iteration, if that is the case. */
				if (iter % stepPrint == 0)
					System.out.println("Iter " + iter);
				iter++;
				/* If this is the last iteration, let main thread continue... */
				if (iter > params.getIters()) {
					semaphore.release();
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
		grid = new CellMulti[params.getGridX()][params.getGridY()];
		
		/* Initialize simulation grid cells. */
		for (int i = 0; i < params.getGridX(); i++) {
			for (int j = 0; j < params.getGridY(); j++) {
				/* Add cell to current place in grid. */
				grid[i][j] = new CellMulti(params.getGrassRestart(), this.numThreads);
				/* Grow grass in current cell. */
				if (rng.get().nextBoolean()) {
					/* Grass not alive, initialize grow timer. */
					grid[i][j].setGrass(1 + rng.get().nextInt(params.getGrassRestart()));
				} else {
					/* Grass alive. */
					grid[i][j].setGrass(0);
					/* Update grass statistics. */
					this.grassStats.getAndIncrement(0);
				}
			}
		}
		
		/* Populate simulation grid with agents. */
		for (int i = 0; i < params.getInitSheep(); i++)
			grid[rng.get().nextInt(params.getGridX())][rng.get().nextInt(params.getGridY())].putAgentNow(
						new Sheep(1 + rng.get().nextInt(2 * params.getSheepGainFromFood()), params));
		for (int i = 0; i < params.getInitWolves(); i++)
			grid[rng.get().nextInt(params.getGridX())][rng.get().nextInt(params.getGridY())].putAgentNow(
						new Wolf(1 + rng.get().nextInt(2 * params.getWolvesGainFromFood()), params));
		
		/* Run simulation. */
		long startTime = System.currentTimeMillis();
		for (int i = 0; i < numThreads; i++)
			(new Thread(new SimThread(i))).start();
		try {
			semaphore.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		long endTime = System.currentTimeMillis();
		float timeInSeconds = (float) (endTime - startTime) / 1000.0f;
		System.out.println("Total simulation time: " + timeInSeconds + "\n");
		
	}
	
	/**
	 * Main function.
	 * @param args Optionally indicate period of iterations to print current iteration to screen.
	 * If ignored, all iterations will be printed to screen.
	 */
	public static void main(String[] args) {
		
		new PredPreyMulti().doMain(args);
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
