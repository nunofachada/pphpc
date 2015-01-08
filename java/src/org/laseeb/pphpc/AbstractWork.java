package org.laseeb.pphpc;

public class AbstractWork implements IWork {
	
	private int wId;
	
	public AbstractWork(int wId) {
		this.wId = wId;
	}

	@Override
	public int getWorkerId() {
		return this.wId;
	}

}
