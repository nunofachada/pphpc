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

/**
 *  A simulation worker. 
 *  */
public class ModelWorker implements Runnable {
	
	private int swId;
	private IWorkFactory workFactory;
	private ModelParams params;
	private IModel model;
	private IController controller;
	
	public ModelWorker(int swId, IWorkFactory workFactory, IModel model, IController controller) {
		this.swId = swId;
		this.workFactory = workFactory;
		this.params = model.getParams();
		this.model = model;
		this.controller = controller;
	}
	
	/* Simulate! */
	public void run() {
		
		/* Random number generator for current worker. */
		Random rng;

		/* Partial statistics */
		IterationStats iterStats = new IterationStats();
		
		/* A work token. */
		int token;
		
		/* Get cells work provider. */
		IWorkProvider cellsWorkProvider = workFactory.getWorkProvider(model.getSize(), this.controller);
		
		/* Get initial agent creation work providers. */
		IWorkProvider sheepWorkProvider = workFactory.getWorkProvider(params.getInitSheep(), this.controller);
		IWorkProvider wolvesWorkProvider = workFactory.getWorkProvider(params.getInitWolves(), this.controller);
	
		/* Get thread-local work. */
		IWork cellsWork = cellsWorkProvider.newWork(swId);
		IWork sheepWork = sheepWorkProvider.newWork(swId);
		IWork wolvesWork = wolvesWorkProvider.newWork(swId);
		
		try {

			/* Create random number generator for current worker. */
			rng = model.createRNG(swId);
			
			controller.workerNotifyBeforeInitCells();
			
			/* Initialize simulation grid cells. */
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				model.setCellAt(token, rng);
			}

			controller.workerNotifyInitCells();

			cellsWorkProvider.resetWork(cellsWork);
			
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				model.setCellNeighbors(token);
			}
			
			controller.workerNotifyCellsAddNeighbors();
			
			/* Populate simulation grid with agents. */
			while ((token = sheepWorkProvider.getNextToken(sheepWork)) >= 0) {
				int idx = rng.nextInt(model.getSize());
				IAgent sheep = new Sheep(1 + rng.nextInt(2 * params.getSheepGainFromFood()), params);
				model.getCell(idx).putNewAgent(sheep);
			}

			while ((token = wolvesWorkProvider.getNextToken(wolvesWork)) >= 0) {
				int idx = rng.nextInt(model.getSize());
				IAgent wolf = new Wolf(1 + rng.nextInt(2 * params.getWolvesGainFromFood()), params);
				model.getCell(idx).putNewAgent(wolf);
			}
			
			controller.workerNotifyInitAgents();
			
			/* Get initial statistics. */
			iterStats.reset();
			cellsWorkProvider.resetWork(cellsWork);
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				model.getCell(token).getStats(iterStats);
			}
			
			/* Update global statistics. */
			model.updateStats(0, iterStats);

			/* Sync. with barrier. */
			controller.workerNotifyFirstStats();
			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {

				cellsWorkProvider.resetWork(cellsWork);
				while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {

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

				cellsWorkProvider.resetWork(cellsWork);
				
				/* Sync. with barrier. */
				controller.workerNotifyHalfIteration();
				
				/* Reset statistics for current iteration. */
				iterStats.reset();

				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {

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
				
				/* Update global statistics. */
				model.updateStats(iter, iterStats);
				
				/* Sync. with barrier. */
				controller.workerNotifyEndIteration();
				
			}
			
			controller.workerNotifySimFinish();
			
		} catch (InterruptedWorkException iwe) {

			/* Thread interrupted by request of another thread. Do nothing, just
			 * let thread finish by its own. */
		} catch (Exception e) {
			
			/* Notify model of exception. */
			this.model.registerException(e);
			
			/* Some other unexpected exception. Notify controller to stop all other
			 * threads immediately. */
			this.controller.stopNow();
			
		}
	}
}

