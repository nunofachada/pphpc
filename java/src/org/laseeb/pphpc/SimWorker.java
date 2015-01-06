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

/**
 *  A simulation worker. 
 *  */
public class SimWorker implements Runnable {
	
	private ISimWorkProvider workProvider;
	
	private SimParams params;
	
	private IGlobalStats globalStats;
	
	/* Constructor only sets the grid offset each thread. */
	public SimWorker(ISimWorkProvider workProvider, SimParams params, IGlobalStats globalStats) {
		this.workProvider = workProvider;
		this.params = params;
		this.globalStats = globalStats;
	}
	
	/* Simulate! */
	public void run() {
		
		/* Current cell being processed. */
		ICell cell;

		/* Partial statistics */
		IterationStats iterStats = new IterationStats();
		
		/* Register worker with work provider. */
		ISimWorkerState tState = workProvider.registerWorker();
		
		try {

			/* Initialize simulation grid cells. */
			workProvider.initCells(tState);
			
			/* Populate simulation grid with agents. */
			workProvider.initAgents(tState, params);
			
			/* Get initial statistics. */
			iterStats.reset();
			while ((cell = workProvider.getNextCell(tState)) != null) {
				cell.getStats(iterStats);
			}
			
			/* Update global statistics. */
			globalStats.updateStats(0, iterStats);
	
			/* Sync. with barrier. */
			workProvider.syncAfterInit(tState);
	
			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {
	
				while ((cell = workProvider.getNextCell(tState)) != null) {
					
					/* ************************* */
					/* ** 1 - Agent movement. ** */
					/* ************************* */
	
					cell.agentsMove();
						
					/* ************************* */
					/* *** 2 - Grass growth. *** */
					/* ************************* */
					
					/* Regenerate grass if required. */
					cell.regenerateGrass();
	
				}
				
				
				/* Sync. with barrier. */
				workProvider.syncAfterHalfIteration(tState);
				
				/* Reset statistics for current iteration. */
				iterStats.reset();
				
				/* Cycle through cells in order to perform step 3 and 4 of simulation. */
				while ((cell = workProvider.getNextCell(tState)) != null) {
					
					/* ************************** */
					/* *** 3 - Agent actions. *** */
					/* ************************** */
	
					cell.agentActions(tState.getRng());
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */
	
					cell.getStats(iterStats);
					
				}
				
				/* Update global statistics. */
				globalStats.updateStats(iter, iterStats);
				
				/* Sync. with barrier. */
				workProvider.syncAfterEndIteration(tState);
				
			}
			
			workProvider.syncAfterSimFinish(tState);
			
		} catch (SimWorkerException swe) {
			
			/* Throw runtime exception to be handled by uncaught exception handler. */
			throw new RuntimeException(swe);
			
		}
	}
}

