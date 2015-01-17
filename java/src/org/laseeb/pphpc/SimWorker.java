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
 *  A simulation worker. Each worker runs in its own thread.
 *  
 *  @author Nuno Fachada
 *  @see java.lang.Runnable
 * */
public class SimWorker implements Runnable {
	
	/* Worker ID. */
	private int wId;
	
	/* Work factory. */
	private IWorkFactory workFactory;
	
	/* Model parameters. */
	private ModelParams params;
	
	/* MVC model. */
	private IModel model;
	
	/* MVC controller. */
	private IController controller;
	
	/**
	 * Create a new simulation worker. Simulation workers are created by MVC controller
	 * instances.
	 *  
	 * @param wId Worker ID.
	 * @param workFactory Work factory.
	 * @param model The MVC model.
	 * @param controller The MVC controller.
	 */
	SimWorker(int wId, IWorkFactory workFactory, IModel model, IController controller) {
		this.wId = wId;
		this.workFactory = workFactory;
		this.params = model.getParams();
		this.model = model;
		this.controller = controller;
	}
	
	/**
	 *  Perform simulation work.
	 *  
	 *  @see java.lang.Runnable#run()
	 * */
	public void run() {
		
		/* Random number generator for current worker. */
		Random rng;

		/* Partial statistics */
		IterationStats iterStats = new IterationStats();
		
		/* A work token. */
		int token;
		
		/* Get cells work provider. */
		IWorkProvider cellsWorkProvider = this.workFactory.getWorkProvider(
				this.model.getSize(), this.model, this.controller);
		
		/* Get initial agent creation work providers. */
		IWorkProvider sheepWorkProvider = this.workFactory.getWorkProvider(
				this.params.getInitSheep(), this.model, this.controller);
		IWorkProvider wolvesWorkProvider = this.workFactory.getWorkProvider(
				this.params.getInitWolves(), this.model, this.controller);
	
		/* Get thread-local work. */
		IWork cellsWork = cellsWorkProvider.newWork(wId);
		IWork sheepWork = sheepWorkProvider.newWork(wId);
		IWork wolvesWork = wolvesWorkProvider.newWork(wId);
		
		try {

			/* Create random number generator for current worker. */
			rng = this.model.createRNG(wId);
			
			/* Notify controller that I'm about to begin working. */
			this.controller.workerNotifyBeforeInitCells();
			
			/* Initialize simulation grid cells. */
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				this.model.initCellAt(token, rng);
			}

			/* Notify controller I have initialized my allocated cells. */
			this.controller.workerNotifyInitCells();

			/* Reset my cells work. */
			cellsWorkProvider.resetWork(cellsWork);
			
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				this.model.setCellNeighbors(token);
			}
			
			/* Notify controller I already set the neighbors for my allocated cells. */
			controller.workerNotifySetCellNeighbors();
			
			/* Populate simulation grid with agents. */
			while ((token = sheepWorkProvider.getNextToken(sheepWork)) >= 0) {
				int idx = rng.nextInt(this.model.getSize());
				IAgent sheep = new Sheep(
						1 + rng.nextInt(2 * this.params.getSheepGainFromFood()), this.params);
				this.model.getCell(idx).putNewAgent(sheep);
			}

			while ((token = wolvesWorkProvider.getNextToken(wolvesWork)) >= 0) {
				int idx = rng.nextInt(this.model.getSize());
				IAgent wolf = new Wolf(
						1 + rng.nextInt(2 * this.params.getWolvesGainFromFood()), this.params);
				this.model.getCell(idx).putNewAgent(wolf);
			}
			
			/* Notify controller I already initialized my allocated agents. */
			this.controller.workerNotifyInitAgents();
			
			/* Get initial statistics. */
			iterStats.reset();
			cellsWorkProvider.resetWork(cellsWork);
			while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {
				this.model.getCell(token).getStats(iterStats);
			}
			
			/* Update global statistics. */
			this.model.updateStats(0, iterStats);

			/* Notify controller I updated statistics for the zero iteration. */
			this.controller.workerNotifyFirstStats();

			/* Perform simulation steps. */
			for (int iter = 1; iter <= this.params.getIters(); iter++) {

				/* Reset my cells work. */
				cellsWorkProvider.resetWork(cellsWork);
				
				/* Cycle through cells in order to perform step 1 and 2 of simulation. */
				while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {

					/* Current cell being processed. */
					ICell cell = this.model.getCell(token);

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

				/* Reset my cells work. */
				cellsWorkProvider.resetWork(cellsWork);
				
				/* Notify controller I'm half-way through an iteration. */
				this.controller.workerNotifyHalfIteration();
				
				/* Reset statistics for current iteration. */
				iterStats.reset();

				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				while ((token = cellsWorkProvider.getNextToken(cellsWork)) >= 0) {

					/* Current cell being processed. */
					ICell cell = this.model.getCell(token);

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
				this.model.updateStats(iter, iterStats);
				
				/* Notify controller I ended an iteration. */
				this.controller.workerNotifyEndIteration();
				
			}
			
			/* Notify controller I'm finished with this simulation. */
			this.controller.workerNotifySimFinish();
			
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

