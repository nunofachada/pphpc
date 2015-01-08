package org.laseeb.pphpc;

public class EqualWorkProvider implements IWorkProvider {

	private class EqualWork extends AbstractWork {

		private int startToken;
		private int endToken;
		private int counter;
		
		public EqualWork(int wId, int startToken, int endToken) { 
			super(wId);
			this.startToken = startToken;
			this.endToken = endToken;
			this.counter = startToken;
		}
		
	}
	
	private int numWorkers;
	private int workSize;
	
	public EqualWorkProvider(int numWorkers, int workSize) {
		this.numWorkers = numWorkers;
		this.workSize = workSize;
	}
	
	@Override
	public IWork newWork(int wId) {
		
		/* Determine tokens per worker. The bellow operation is equivalent to ceil(workSize/numWorkers) */
		int tokensPerWorker = (this.workSize + this.numWorkers - 1) / this.numWorkers;
		
		/* Determine start and end tokens for current worker. */
		int startToken = wId * tokensPerWorker; /* Inclusive */
		int endToken = Math.min((wId + 1) * tokensPerWorker, this.workSize); /* Exclusive */

		/* Create a work state adequate for this work provider. */
		EqualWork eWork = new EqualWork(wId, startToken, endToken);
		if (workSize < 1000)
			System.out.println("[" + workSize + "] " + Thread.currentThread().getName() + "(wId="+wId+") got work from " + startToken + " to " + endToken);
		
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
