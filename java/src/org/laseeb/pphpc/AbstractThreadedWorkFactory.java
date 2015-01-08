package org.laseeb.pphpc;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.validators.PositiveInteger;

public abstract class AbstractThreadedWorkFactory extends AbstractWorkFactory {

	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors", validateWith = PositiveInteger.class)
	protected int numThreads = Runtime.getRuntime().availableProcessors();

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	@Override
	public int getNumWorkers() {
		return this.numThreads;
	}

}
