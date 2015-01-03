package org.laseeb.pphpc;

import java.util.Random;

public class AbstractSimThreadState implements ISimThreadState {

	private Random rng;
	private int stId;
	
	public AbstractSimThreadState(Random rng, int stId) {
		this.rng = rng;
		this.stId = stId;
	}

	@Override
	public Random getRng() {
		return this.rng;
	}
	
	@Override
	public int getStId() {
		return this.stId;
	}

}
