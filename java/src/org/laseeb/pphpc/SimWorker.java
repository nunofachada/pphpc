package org.laseeb.pphpc;

/* A simulation thread. */
public class SimWorker implements Runnable {
	
	/* Grid offset for each thread. */
	private int swId; 
	
	private ISimWorkProvider workProvider;
	
	private SimParams params;
	
	private PredPrey pp;
	
	/* Constructor only sets the grid offset each thread. */
	public SimWorker(int swId, ISimWorkProvider workProvider, SimParams params, PredPrey pp) {
		this.swId = swId;
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
		
		/* Initialize simulation grid cells. */
		ISimWorkerState tState = workProvider.initCells(swId);
		
		/* Sync. with barrier. */
		workProvider.afterInitCells();
		
		/* Populate simulation grid with agents. */
		workProvider.initAgents(tState, params);
		
		/* Sync. with barrier. */
		workProvider.afterPopulateSim();
		
		/* Get initial statistics. */
		stats.reset();
		workProvider.resetNextCell(tState);
		while ((cell = workProvider.getNextCell(tState)) != null) {
			cell.getStats(stats);
		}
		
		/* Update global statistics. */
		pp.updateStats(0, stats);

		/* Sync. with barrier. */
		workProvider.afterFirstGetStats();

		/* Perform simulation steps. */
		for (int iter = 1; iter <= params.getIters(); iter++) {
			
			workProvider.resetNextCell(tState);
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
			workProvider.afterHalfIteration();
			
			/* Reset statistics for current iteration. */
			stats.reset();
			
			/* Cycle through cells in order to perform step 3 and 4 of simulation. */
			workProvider.resetNextCell(tState);
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
			workProvider.afterEndIteration();
			
		}
		
		workProvider.simFinish(tState);
		
	}
}

