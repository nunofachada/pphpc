/*
 * Copyright (c) 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Instituto Superior Técnico nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *     
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

import java.util.Random;

/**
 * Grass initialization strategy where there is a 50% possibility of the grass
 * being alive. In case the grass is not alive, its regrowth counter is set to
 * a random value between 1 and the grassRestart state variable.
 * 
 * @author Nuno Fachada
 */
public class CellGrassInitCoinRandCounter implements ICellGrassInitStrategy {

	/**
	 * Constructor for this strategy.
	 */
	public CellGrassInitCoinRandCounter() {}

	/**
	 * @see ICellGrassInitStrategy#getInitGrass(int, Random)
	 */
	@Override
	public int getInitGrass(int grassRestart, Random rng) {

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
