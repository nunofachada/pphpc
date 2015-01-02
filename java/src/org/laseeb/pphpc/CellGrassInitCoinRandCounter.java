package org.laseeb.pphpc;

import java.util.Random;

public class CellGrassInitCoinRandCounter implements CellGrassInitStrategy {

	public CellGrassInitCoinRandCounter() {}

	@Override
	public int getInitGrass(ICell cell) {

		int grassState;
		Random rng = cell.getRng();
		
		/* Grow grass in current cell. */
		if (rng.nextBoolean()) {
		
			/* Grass not alive, initialize grow timer. */
			grassState = 1 + rng.nextInt(cell.getGrassRestart());
			
		} else {
			
			/* Grass alive. */
			grassState = 0;
			
		}
		
		return grassState;
	}

}
