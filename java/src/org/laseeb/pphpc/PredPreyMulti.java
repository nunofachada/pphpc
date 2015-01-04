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
	
	/* Latch on which the main thread will wait until the simulation threads
	 * terminate. */
	private CountDownLatch latch;
	
	/** 
	 * Constructor, no arguments required.
	 */
	private PredPreyMulti() {}
	
	
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

		/* Initialize statistics arrays. */
		this.initStats();
		
		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter();
		
		ISimSpace space = new Square2DTorusSimSpace(this.params.getGridX(), this.params.getGridY());
		
		workProvider = new EqualSimWorkProvider(space, this.params.getGrassRestart(), grassInitStrategy, this.numThreads, this.repeatable); 
//		workProvider = new OnDemandSimWorkProvider(100, space, params.getGrassRestart(), grassInitStrategy, threading, numThreads); 

		workProvider.registerObserver(SimEvent.AFTER_END_SIMULATION, new Observer() {

			@Override
			public void update(SimEvent event, ISimWorkProvider workProvider) {
				latch.countDown();
			}
			
		});
		
		/* Launch simulation threads. */
		for (int i = 0; i < this.numThreads; i++)
			(new Thread(new SimWorker(i))).start();

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
	protected void updateStats(int iter, PPStats stats) {
		sheepStats.addAndGet(iter, stats.getSheep());
		wolfStats.addAndGet(iter, stats.getWolves());
		grassStats.addAndGet(iter, stats.getGrass());		
	}

	@Override
	protected void initStats() {
		int[] resetArray = new int[this.params.getIters() + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.grassStats = new AtomicIntegerArray(resetArray);		
	}

}
