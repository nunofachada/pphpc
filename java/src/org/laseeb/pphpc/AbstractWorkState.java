package org.laseeb.pphpc;

public class AbstractWorkState implements IWork {
	
	private int wId;
	
	public AbstractWorkState(int wId) {
		this.wId = wId;
	}

	@Override
	public int getWorkerId() {
		return this.wId;
	}

}
