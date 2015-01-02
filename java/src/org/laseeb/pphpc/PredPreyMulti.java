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
 * Multi-threaded PPHPC model.
 * 
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
	
	/* Thread-local random number generator. */
	private ThreadLocal<Random> rng = new ThreadLocal<Random>();
	
	/** 
	 * Constructor, no arguments required.
	 */
	private PredPreyMulti() {}
	
	/* A simulation thread. */
	private class SimThread implements Runnable {
		
		/* Grid offset for each thread. */
		private int stId; 
		
		/* Random number generator for current thread. */
		private Random localRng;
		
		/* Constructor only sets the grid offset each thread. */
		public SimThread(int stId) {
			this.stId = stId;
		}
		
		/* Simulate! */
		public void run() {
			
			/* Current cell being processed. */
			ICell cell;

			/* Initialize this thread's random number generator. */
			try {
				localRng = createRNG(stId);
			} catch (Exception e) {
				errMessage(e);
				return;
			}
			rng.set(localRng);
			
			/* Partial statistics */
			PPStats stats = new PPStats();
			
			/* Initialize simulation grid cells. */
			ISimThreadState tState = grid.initialize(stId);
			
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
			int startSheepIdx = stId * sheepPerThread; /* Inclusive */
			int endSheepIdx = Math.min((stId + 1) * sheepPerThread, params.getInitSheep()); /* Exclusive */
			
			for (int sheepIdx = startSheepIdx; sheepIdx < endSheepIdx; sheepIdx++) {
				int idx = localRng.nextInt(params.getGridX() * params.getGridY());
				IAgent sheep = new Sheep(1 + localRng.nextInt(2 * params.getSheepGainFromFood()), params);
				grid.getCell(idx).putNewAgent(sheep);
			}

			/* Determine wolves per thread. The bellow operation is equivalent to ceil(numWolves/numThreads) */
			int wolvesPerThread = (params.getInitWolves() + numThreads - 1) / numThreads;
			
			/* Determine start and end wolves index for current thread. */
			int startWolvesIdx = stId * wolvesPerThread; /* Inclusive */
			int endWolvesIdx = Math.min((stId + 1) * wolvesPerThread, params.getInitWolves()); /* Exclusive */
			
			for (int wolvesIdx = startWolvesIdx; wolvesIdx < endWolvesIdx; wolvesIdx++) {
				int idx = localRng.nextInt(params.getGridX() * params.getGridY());
				IAgent wolf = new Wolf(1 + localRng.nextInt(2 * params.getWolvesGainFromFood()), params);		
				grid.getCell(idx).putNewAgent(wolf);
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
			
			/* Get initial statistics. */
			stats.reset();
			grid.reset(tState);
			while ((cell = grid.getNextCell(tState)) != null) {
				cell.getStats(stats);
			}
			
			/* Update global statistics. */
			sheepStats.addAndGet(0, stats.getSheep());
			wolfStats.addAndGet(0, stats.getWolves());
			grassStats.addAndGet(0, stats.getGrass());

			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {
				
				grid.reset(tState);
				while ((cell = grid.getNextCell(tState)) != null) {
					
					/* ************************* */
					/* ** 1 - Agent movement. ** */
					/* ************************* */

					cell.agentsMove();
						
					/* ************************* */
					/* *** 2 - Grass growth. *** */
					/* ************************* */
					
					/* If grass is not alive... */
					if (!cell.isGrassAlive()) {
						/* ...decrement alive counter. */
						cell.regenerateGrass();
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
				
				/* Reset statistics for current iteration. */
				stats.reset();
				
				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				grid.reset(tState);
				while ((cell = grid.getNextCell(tState)) != null) {
					
					/* ************************** */
					/* *** 3 - Agent actions. *** */
					/* ************************** */

					cell.agentActions();
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */

					cell.getStats(stats);
					
				}
				
				/* Update global statistics. */
				sheepStats.addAndGet(iter, stats.getSheep());
				wolfStats.addAndGet(iter, stats.getWolves());
				grassStats.addAndGet(iter, stats.getGrass());
				
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
		
		/* Start timing. */
		long startTime = System.currentTimeMillis();
		
		/* Initialize latch. */
		latch = new CountDownLatch(1);

		/* Initialize thread barriers. */
		this.mainBarrier = new CyclicBarrier(numThreads, new Runnable() {
			
			/* Initial iteration is 1. */
			int iter = 1;
			
			/* Method to run after each iteration. */
			public void run() {
				
				/* Print current iteration, if that is the case. */
				if ((stepPrint > 0) && (iter % stepPrint == 0))
					System.out.println("Iter " + iter);
				
				/* Increment iteration count. */
				iter++;
				
				/* If this is the last iteration, let main thread continue... */
				if (iter > params.getIters()) {
					
					/* ...by opening the latch on which it is waiting one. */
					latch.countDown();
					
				}				
			}
		});
		this.partialBarrier = new CyclicBarrier(numThreads);
		
		/* Initialize statistics arrays. */
		int[] resetArray = new int[params.getIters() + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.grassStats = new AtomicIntegerArray(resetArray);
		
		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter();
		
		/* Create simulation grid. */
		SimGrid.Threading threading = repeatable ? SimGrid.Threading.MULTI_REPEAT : SimGrid.Threading.MULTI;
		grid = new SimGrid(params.getGridX(), params.getGridY(), params.getGrassRestart(), grassInitStrategy, threading, numThreads); 
		
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
		
		PredPrey pp = PredPrey.getInstance(PredPreyMulti.class);
		int status = pp.doMain(args);
		System.exit(status);
		
	}

	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.PredPrey#getStats(StatType, int)
	 */
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

	@Override
	protected Random getRng() {
		return this.rng.get();
	}

}
