package org.laseeb.pphpc;

import java.util.Random;

public class CellGrassInitCoinRandCounter implements CellGrassInitStrategy {

	Random rng;
	
	public CellGrassInitCoinRandCounter(Random rng) {
		this.rng = rng;
	}

	@Override
	public int getInitGrass(int grassRestart) {

		int grassState;
		
		/* Grow grass in current cell. */
		if (rng.nextBoolean()) {
		
			/* Grass not alive, initialize grow timer. */
			grassState = 1 + rng.nextInt(grassRestart);
			
		} else {
			
			/* Grass alive. */
			grassState = 0;
			
		}
		
		return grassState;
	}

}
