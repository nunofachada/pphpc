package org.laseeb.pphpc;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.validators.PositiveInteger;

public abstract class AbstractEqualWorkFactory extends AbstractWorkFactory {

	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors", validateWith = PositiveInteger.class)
	protected int numThreads = Runtime.getRuntime().availableProcessors();

	@Override
	public IController createSimController(IModel model) {
		return new Controller(model,
				new BlockingSynchronizer(SimEvent.AFTER_INIT_CELLS, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new BlockingSynchronizer(SimEvent.AFTER_INIT_AGENTS, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_FIRST_STATS), 
				new BlockingSynchronizer(SimEvent.AFTER_HALF_ITERATION, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_END_ITERATION, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_END_SIMULATION));
	}

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	@Override
	public int getNumWorkers() {
		return this.numThreads;
	}

}
