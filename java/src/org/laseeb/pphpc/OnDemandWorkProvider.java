package org.laseeb.pphpc;

import java.util.concurrent.atomic.AtomicInteger;

public class OnDemandWorkProvider implements IWorkProvider {

	private class OnDemandWork extends AbstractWork {

		private int current;
		private int last;
		
		public OnDemandWork(int wId) {
			super(wId);
			this.current = 0;
			this.last = 0;
		}
	}
	
	private AtomicInteger counter;
	private int blockSize;
	private int workSize;

	public OnDemandWorkProvider(int blockSize, int workSize) {
		this.counter = new AtomicInteger(0);
		this.blockSize = blockSize;
		this.workSize = workSize;
	}

	@Override
	public IWork newWork(int wId) {
		return new OnDemandWork(wId);
	}

	@Override
	public int getNextToken(IWork work) {
		
		OnDemandWork odWork = (OnDemandWork) work;
		
		int nextIndex = -1;
		
		if (odWork.current >= odWork.last) {

			odWork.current = this.counter.getAndAdd(this.blockSize);
			odWork.last = Math.min(odWork.current + this.blockSize, this.workSize);
			
//			if (odWork.current < this.workSize)
//				System.out.println(Thread.currentThread().getName() + " got tokens from " + odWork.current + " to " + odWork.last + " (MAX: " + this.workSize + ")");
			
		}
		
		if (odWork.current < this.workSize) {

			nextIndex = odWork.current;
			odWork.current++;
			
		}

		return nextIndex;
	}

	@Override
	public void resetWork(IWork work) {
		
		OnDemandWork odWork = (OnDemandWork) work;
		odWork.current = 0;
		odWork.last = 0;

	}

	void resetWorkCounter() {
//		System.out.println(Thread.currentThread().getName() + " resetting work provider of size " + this.workSize);
	
		this.counter.set(0);
	}
}
