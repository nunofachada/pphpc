package org.laseeb.pphpc;

public class IntervalWorkProvider implements IWorkProvider {
	
	private int numWorkers;
	
	/* Synchronization interval. */
	private int syncInterval;
	
	/**
	 * A class which represents the state of intervaled work performed by 
	 * each worker.
	 */
	private class IntervalWork extends AbstractWork {

		/* Fixed start work token. */
		private int startToken;
		
		/* Fixed end work token. */
		private int endToken;
		
		/* Current work token. */
		private int counter;
		
		/**
		 * Create a new intervaled work state representation.
		 * 
		 * @param wId Worker ID.
		 * @param startToken Start work token.
		 * @param endToken End work token.
		 */
		public IntervalWork(int wId, int startToken, int endToken) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = endToken;
			this.counter = startToken;
		}
		
	}

	public IntervalWorkProvider(int numThreads, IModel model) {
		
		ISpace space = model.getSpace();
		
		int[] dims = space.getDims();
		
		int numDims = space.getNumDims();
		
		this.syncInterval = 1;
		
		for (int i = 0; i < numDims - 1; i++)
			this.syncInterval *= dims[i];
		
		int exclusionConstant = space.getNeighborhoodRadius() * 2 + 1;
		
		int maxThreads = model.getSize() / (this.syncInterval * exclusionConstant);
		
		/* Check if number of threads doesn't exceed maximum value. */
		if (numThreads > maxThreads) {
			throw new RuntimeException("Too many threads! Max. threads is " + maxThreads);
		}
		
		this.numWorkers = numThreads;
	}

	@Override
	public IWork newWork(int wId) {
		return new IntervalWork(wId, wId * );
		return null;
	}

	@Override
	public int getNextToken(IWork work) {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void resetWork(IWork work) {
		// TODO Auto-generated method stub

	}

}
