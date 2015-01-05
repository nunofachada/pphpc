package org.laseeb.pphpc;

public class NonBlockingSimSynchronizer extends SimSynchronizer {

	public NonBlockingSimSynchronizer(SimEvent event) {
		super(event);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void syncNotify() {
//		System.out.println("UnSYNCHED!" + event);
		this.notifyObservers();
	}


}
