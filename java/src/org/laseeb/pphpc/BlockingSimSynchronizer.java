package org.laseeb.pphpc;

import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;

public class BlockingSimSynchronizer extends SimSynchronizer {

//	private int numThreads;
	private CyclicBarrier barrier;
	
	public BlockingSimSynchronizer(final SimEvent event, int numThreads) {
		super(event);
//		this.numThreads = numThreads;
		this.barrier = new CyclicBarrier(numThreads, new Runnable() {

			@Override
			public void run() {
//				System.out.println("All threads synched!");
				notifyObservers();
			}
		});
	}

	@Override
	public void syncNotify() {
//		System.out.println(Thread.currentThread().getName() + " synching with " + event);
		try {
			this.barrier.await();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (BrokenBarrierException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}


}
