package org.laseeb.pphpc;

public class Controller implements IController {
	
	private IModel model;

	private ISynchronizer afterInitCellsSync;
	private ISynchronizer afterAddCellsNeighsSync;
	private ISynchronizer afterAddAgentsSync;
	private ISynchronizer afterFirstStatsSync;
	private ISynchronizer afterHalfIterSync;
	private ISynchronizer afterEndIterSync;
	private ISynchronizer afterEndSimSync;
	
	public Controller(IModel model,
			ISynchronizer afterInitCellsSync,
			ISynchronizer afterAddCellsNeighsSync,
			ISynchronizer afterAddAgentsSync,
			ISynchronizer afterFirstStatsSync,
			ISynchronizer afterHalfIterSync,
			ISynchronizer afterEndIterSync,
			ISynchronizer afterEndSimSync) {
		
		this.afterInitCellsSync = afterInitCellsSync; 
		this.afterAddCellsNeighsSync = afterAddCellsNeighsSync;
		this.afterAddAgentsSync = afterAddAgentsSync;
		this.afterFirstStatsSync = afterFirstStatsSync;
		this.afterHalfIterSync = afterHalfIterSync;
		this.afterEndIterSync = afterEndIterSync;
		this.afterEndSimSync = afterEndSimSync;
	}

	@Override
	public void registerSimEventObserver(SimEvent event, IObserver observer) {
		switch (event) {
			case AFTER_INIT_CELLS:
				this.afterInitCellsSync.registerObserver(observer);
				break;
			case AFTER_CELLS_ADD_NEIGHBORS:
				this.afterAddCellsNeighsSync.registerObserver(observer);
				break;
			case AFTER_INIT_AGENTS:
				this.afterAddAgentsSync.registerObserver(observer);
				break;
			case AFTER_FIRST_STATS:
				this.afterFirstStatsSync.registerObserver(observer);
				break;
			case AFTER_HALF_ITERATION:
				this.afterHalfIterSync.registerObserver(observer);
				break;
			case AFTER_END_ITERATION:
				this.afterEndIterSync.registerObserver(observer);
				break;
			case AFTER_END_SIMULATION:
				this.afterEndSimSync.registerObserver(observer);
				break;
		}
	}

	@Override
	public void syncAfterInitCells() throws WorkException {
		this.afterInitCellsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterCellsAddNeighbors() throws WorkException {
		this.afterAddCellsNeighsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterInitAgents() throws WorkException {
		this.afterAddAgentsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterFirstStats() throws WorkException {
		this.afterFirstStatsSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterHalfIteration() throws WorkException {
		this.afterHalfIterSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterEndIteration() throws WorkException {
		this.afterEndIterSync.syncNotify(this.model);
	}

	@Override
	public void syncAfterSimFinish() throws WorkException {
		this.afterEndSimSync.syncNotify(this.model);
	}

	@Override
	public void notifyTermination() {
		this.afterInitCellsSync.notifyTermination(); 
		this.afterAddCellsNeighsSync.notifyTermination();
		this.afterAddAgentsSync.notifyTermination();
		this.afterFirstStatsSync.notifyTermination();
		this.afterHalfIterSync.notifyTermination();
		this.afterEndIterSync.notifyTermination();
		this.afterEndSimSync.notifyTermination();
	}


}
