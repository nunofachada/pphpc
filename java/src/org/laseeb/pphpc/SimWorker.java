package org.laseeb.pphpc;

/* A simulation thread. */
public class SimWorker implements Runnable {
	
	/* Grid offset for each thread. */
	private int swId; 
	
	private ISimWorkProvider workProvider;
	
	private SimParams params;
	
	private PredPrey pp;
	
	/* Constructor only sets the grid offset each thread. */
	public SimWorker(ISimWorkProvider workProvider, SimParams params, PredPrey pp) {
		this.workProvider = workProvider;
		this.params = params;
		this.pp = pp;
	}
	
	/* Simulate! */
	public void run() {
		
		/* Current cell being processed. */
		ICell cell;

		/* Partial statistics */
		PPStats stats = new PPStats();
		
		/* Register worker with work provider. */
		ISimWorkerState tState = workProvider.registerWorker();

		/* Initialize simulation grid cells. */
		workProvider.initCells(tState);
		
		/* Populate simulation grid with agents. */
		workProvider.initAgents(tState, params);
		
//		System.out.println("Thread " + Thread.currentThread().getName() + " is going to get the stats");
		
		/* Get initial statistics. */
		stats.reset();
		while ((cell = workProvider.getNextCell(tState)) != null) {
			cell.getStats(stats);
		}
		
		/* Update global statistics. */
		pp.updateStats(0, stats);

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
			stats.reset();
			
			/* Cycle through cells in order to perform step 3 and 4 of simulation. */
			while ((cell = workProvider.getNextCell(tState)) != null) {
				
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
			pp.updateStats(iter, stats);
			
			/* Sync. with barrier. */
			workProvider.syncAfterEndIteration(tState);
			
		}
		
		workProvider.syncAfterSimFinish(tState);
		
	}
}

