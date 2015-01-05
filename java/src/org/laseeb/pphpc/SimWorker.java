package org.laseeb.pphpc;

/* A simulation thread. */
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

				cell.agentActions();
				
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
		
	}
}

