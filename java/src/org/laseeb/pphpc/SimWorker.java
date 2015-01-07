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
import java.util.concurrent.CyclicBarrier;

/**
 *  A simulation worker. 
 *  */
public class SimWorker implements Runnable {
	
	private int swId;
	private IWorkProvider workProvider;
	private SimParams params;
	private IGlobalStats globalStats;
	private IModel model;
	private IController controller;
	
	private static CyclicBarrier barrier = new CyclicBarrier(8);
	
	public SimWorker(int swId, IWorkProvider workProvider, IModel model, IController controller) {
		this.swId = swId;
		this.workProvider = workProvider;
		this.params = model.getParams();
		this.globalStats = model.getGlobalStats();
		this.model = model;
		this.controller = controller;
	}
	
	/* Simulate! */
	public void run() {
		
		/* Random number generator for current worker. */
		Random rng; // TODO Maybe pass rng from outside? Or be part of the model (but then it must be thread local..)? think...

		/* Partial statistics */
		IterationStats iterStats = new IterationStats();
		
		/* A work token. */
		int token;
		
		/* Get cells work with work provider. */
		IWork cellsWork = workProvider.newWork(this.swId, model.getSize());
		
		/* Get initial agent creation work. */
		IWork sheepWork = workProvider.newWork(this.swId, params.getInitSheep());
		IWork wolvesWork = workProvider.newWork(this.swId, params.getInitWolves());
	
		try {

			/* Create random number generator for current worker. */
			rng = PredPrey.getInstance().createRNG(swId);
			
			/* Initialize simulation grid cells. */
			while ((token = workProvider.getNextToken(cellsWork)) >= 0) {
				model.setCellAt(token, rng);
			}
			
			controller.syncAfterInitCells();
			
			workProvider.resetWork(cellsWork);
			while ((token = workProvider.getNextToken(cellsWork)) >= 0) {
				model.setCellNeighbors(token);
			}
			
			controller.syncAfterCellsAddNeighbors();
			
			int agentCount = 0;
			
			/* Populate simulation grid with agents. */
			while ((token = workProvider.getNextToken(sheepWork)) >= 0) {
				agentCount++;
				int idx = rng.nextInt(model.getSize());
				IAgent sheep = new Sheep(1 + rng.nextInt(2 * params.getSheepGainFromFood()), params);
				model.getCell(idx).putNewAgent(sheep);
//				System.err.println(Thread.currentThread().getName() + " put " + token + " sheep");
			}
			System.out.println(Thread.currentThread().getName() + " put " + agentCount + " sheep");
			agentCount = 0;
			while ((token = workProvider.getNextToken(wolvesWork)) >= 0) {
				agentCount++;
				int idx = rng.nextInt(model.getSize());
				IAgent wolf = new Wolf(1 + rng.nextInt(2 * params.getWolvesGainFromFood()), params);
				model.getCell(idx).putNewAgent(wolf);
			}
			System.out.println(Thread.currentThread().getName() + " put " + agentCount + " wolves");
			
			controller.syncAfterInitAgents();
			
			/* Get initial statistics. */
			iterStats.reset();
			workProvider.resetWork(cellsWork);
			while ((token = workProvider.getNextToken(cellsWork)) >= 0) {
				model.getCell(token).getStats(iterStats);
			}
			
			System.out.println(Thread.currentThread().getName() + " counts " + iterStats.getSheep() + " sheep, " + iterStats.getWolves()+ " wolves and " +iterStats.getGrass() + " grass");
			
//			barrier.await();
//			IterationStats tmpStats = new IterationStats();
//			if (swId == 0) {
//				for (int i = 0; i < model.getSize(); i++)
//					model.getCell(i).getStats(tmpStats);
//				System.out.println("\n --- TOTAL COUNT: " + tmpStats.getSheep() + " sheep, " + tmpStats.getWolves()+ " wolves and " + tmpStats.getGrass() + " grass");
//			}
//			
//			barrier.await();
			
			/* Update global statistics. */
			globalStats.updateStats(0, iterStats);
//			System.out.println(Thread.currentThread().getName() + " will enter!");
			/* Sync. with barrier. */
			controller.syncAfterFirstStats();
			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {
//				if (this.swId == 0) System.out.println("==== " + iter);
//				barrier.await();
				workProvider.resetWork(cellsWork);
//				int count = 0;
				while ((token = workProvider.getNextToken(cellsWork)) >= 0) {
//					count++;
					/* Current cell being processed. */
					ICell cell = model.getCell(token);

					/* ************************* */
					/* ** 1 - Agent movement. ** */
					/* ************************* */
	
					cell.agentsMove(rng);
						
					/* ************************* */
					/* *** 2 - Grass growth. *** */
					/* ************************* */
					
					/* Regenerate grass if required. */
					cell.regenerateGrass();
	
				}
//				System.out.println(Thread.currentThread().getName() + " processed " + count + " cells (1)!");
				workProvider.resetWork(cellsWork);
				
				/* Sync. with barrier. */
				controller.syncAfterHalfIteration();
				
				/* Reset statistics for current iteration. */
				iterStats.reset();
//				if (this.swId == 0) System.out.println("---");
//				barrier.await();
//				count=0;
				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				while ((token = workProvider.getNextToken(cellsWork)) >= 0) {
//					count++;
					/* Current cell being processed. */
					ICell cell = model.getCell(token);

					/* ************************** */
					/* *** 3 - Agent actions. *** */
					/* ************************** */
	
					cell.agentActions(rng);
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */
	
					cell.getStats(iterStats);
					
				}
//				System.out.println(Thread.currentThread().getName() + " processed " + count + " cells (2)!");
				
				/* Update global statistics. */
				globalStats.updateStats(iter, iterStats);
				
				/* Sync. with barrier. */
				controller.syncAfterEndIteration();
				
			}
			
			controller.syncAfterSimFinish();
			
		} catch (Exception we) {
			
			this.controller.notifyTermination();
			
			/* Throw runtime exception to be handled by uncaught exception handler. */
			throw new RuntimeException(we);
			
		}
	}
}

