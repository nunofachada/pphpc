package org.laseeb.pphpc;

public class SimWorkProviders {

	public enum SimWorkType { EQUAL, EQUAL_REPEAT, ON_DEMAND }

	public static ISimWorkProvider createWorkProvider(SimWorkType workType, SimParams params, int numWorkers) {
		
		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter();
		
		/* Simulation space. */
		ISimSpace space = new Square2DTorusSimSpace(params.getGridX(), params.getGridY());
		
		/* Put agent strategy. */
		CellPutAgentStrategy putAgentStrategy;
		
		/* Synchronizers.*/
		ISimSynchronizer afterInitSync, afterHalfIterSync, afterEndIterSync, afterEndSimSync;
		
		/* The work provider. */
		ISimWorkProvider workProvider = null;
		
		if (numWorkers == 1) {
			
			/* Setup strategies and synchronizers for single threaded execution. */
			afterInitSync =  new NonBlockingSimSynchronizer(SimEvent.AFTER_INIT);
			afterHalfIterSync = new NonBlockingSimSynchronizer(SimEvent.AFTER_HALF_ITERATION);
			afterEndIterSync = new NonBlockingSimSynchronizer(SimEvent.AFTER_END_ITERATION);
			afterEndSimSync = new NonBlockingSimSynchronizer(SimEvent.AFTER_END_SIMULATION);
			putAgentStrategy = new CellPutAgentAsync();
			
		} else {
			
			/* Setup strategies and synchronizers for multi threaded execution. */
			switch (workType) {
				case EQUAL:
					afterInitSync =  new NonBlockingSimSynchronizer(SimEvent.AFTER_INIT);
					afterHalfIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_HALF_ITERATION, numWorkers);
					afterEndIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_ITERATION, numWorkers);
					afterEndSimSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_SIMULATION, numWorkers);
					putAgentStrategy = new CellPutAgentSync();
					break;
				case EQUAL_REPEAT:
					afterInitSync =  new NonBlockingSimSynchronizer(SimEvent.AFTER_INIT);
					afterHalfIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_HALF_ITERATION, numWorkers);
					afterEndIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_ITERATION, numWorkers);
					afterEndSimSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_SIMULATION, numWorkers);
					putAgentStrategy = new CellPutAgentSyncSort();
					break;
				case ON_DEMAND:
					afterInitSync =  new BlockingSimSynchronizer(SimEvent.AFTER_INIT, numWorkers);
					afterHalfIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_HALF_ITERATION, numWorkers);
					afterEndIterSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_ITERATION, numWorkers);
					afterEndSimSync = new BlockingSimSynchronizer(SimEvent.AFTER_END_SIMULATION, numWorkers);
					putAgentStrategy = new CellPutAgentSync();
					break;
				default:
					throw new RuntimeException("Unknown error.");
			}
		}
			
		switch (workType) {
			case EQUAL:
				workProvider = new EqualSimWorkProvider(space, params.getGrassRestart(), grassInitStrategy, 
						putAgentStrategy, numWorkers, afterInitSync, afterHalfIterSync, afterEndIterSync, 
						afterEndSimSync);
				break;
			case EQUAL_REPEAT:
				workProvider = new EqualSimWorkProvider(space, params.getGrassRestart(), grassInitStrategy, 
						putAgentStrategy, numWorkers, afterInitSync, afterHalfIterSync, afterEndIterSync, 
						afterEndSimSync);
				break;
			case ON_DEMAND:
				throw new RuntimeException("Not implemented");
					
			default:
				throw new RuntimeException("Unknown error.");
		}
			
		return workProvider;

	}
}