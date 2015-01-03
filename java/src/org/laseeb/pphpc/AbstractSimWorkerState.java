package org.laseeb.pphpc;

import java.util.Random;

public class AbstractSimWorkerState implements ISimWorkerState {

	private Random rng;
	private int swId;
	
	public AbstractSimWorkerState(Random rng, int swId) {
		this.rng = rng;
		this.swId = swId;
	}

	@Override
	public Random getRng() {
		return this.rng;
	}
	
	@Override
	public int getSimWorkerId() {
		return this.swId;
	}

}
