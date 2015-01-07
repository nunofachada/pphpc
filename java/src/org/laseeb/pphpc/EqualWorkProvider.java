package org.laseeb.pphpc;

public class EqualWorkProvider extends AbstractWorkProvider {

	private class EqualWork extends AbstractWorkState {

		private int startToken;
		private int endToken;
		private int counter;
		
		public EqualWork(int wId, int startToken, int endToken) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = endToken;
		}
		
	}
	
	private int workSize;
	private int numWorkers;
	
	public EqualWorkProvider(int workSize, int numWorkers) {
		this.workSize = workSize;
		this.numWorkers = numWorkers;
	}
	
	@Override
	protected IWork doRegisterWork(int wId) {
		
		/* Determine tokens per worker. The bellow operation is equivalent to ceil(workSize/numWorkers) */
		int tokensPerWorker = (this.workSize + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end tokens for current worker. */
		int startToken = wId * tokensPerWorker; /* Inclusive */
		int endToken = Math.min((wId + 1) * tokensPerWorker, this.workSize); /* Exclusive */

		/* Create a work state adequate for this work provider. */
		EqualWork eWork = new EqualWork(wId, startToken, endToken);
		
		/* Return the work state. */
		return eWork;
	}


	@Override
	public int getNextToken(IWork work) {
		
		int nextToken = -1;
		EqualWork eWork = (EqualWork) work;

		if (eWork.counter < eWork.endToken) {
			nextToken = eWork.counter;
			eWork.counter++;
		}
		
		return nextToken;
	}

	@Override
	public void resetWork(IWork work) {
		EqualWork eWork = (EqualWork) work;
		eWork.counter = eWork.startToken;
	}

}
