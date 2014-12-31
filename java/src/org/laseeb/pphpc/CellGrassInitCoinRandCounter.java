package org.laseeb.pphpc;

import java.util.Random;

public class CellGrassInitCoinRandCounter implements CellGrassInitStrategy {

	public CellGrassInitCoinRandCounter() {}

	@Override
	public int getInitGrass(int grassRestart) {

		int grassState;
		Random rng = PredPrey.getInstance().getRng();
		
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
