package org.laseeb.pphpc;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.Parameters;
import com.beust.jcommander.validators.PositiveInteger;

@Parameters(commandDescription = "Equal work command")
public class EqualWorkFactory implements IWorkFactory {
	
	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors", validateWith = PositiveInteger.class)
	private int numThreads = Runtime.getRuntime().availableProcessors();

	@Override
	public IWorkProvider createWorkProvider(int workSize) {
		return new EqualWorkProvider(workSize, this.numThreads);
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public IController createSimController(IModel model) {
		return new Controller(model,
				new BlockingSynchronizer(SimEvent.AFTER_INIT_CELLS, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new BlockingSynchronizer(SimEvent.AFTER_INIT_AGENTS, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_FIRST_STATS), 
				new BlockingSynchronizer(SimEvent.AFTER_HALF_ITERATION, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_END_ITERATION, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_END_SIMULATION));
	}

	@Override
	public IGlobalStats createGlobalStats() {
		return new ThreadSafeGlobalStats(this.numThreads);
	}

}
