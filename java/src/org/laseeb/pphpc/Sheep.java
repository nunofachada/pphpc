/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Sheep agent.
 * 
 * @author Nuno Fachada
 */
public class Sheep extends AbstractAgent {

	/**
	 * Create a sheep agent.
	 * 
	 * @param energy Initial sheep energy.
	 * @param params Simulation parameters.
	 */
	public Sheep(int energy, ModelParams params) {
		super(energy, params);
	}

	/**
	 * @see IAgent#getReproduceProbability()
	 */
	@Override
	public int getReproduceProbability() {
		return params.getSheepReproduceProb();
	}

	/**
	 * @see IAgent#getReproduceThreshold()
	 */
	@Override
	public int getReproduceThreshold() {
		return params.getSheepReproduceThreshold();
	}

	/**
	 * @see AbstractAgent#tryEat(ICell)
	 */
	@Override
	protected void tryEat(ICell cell) {
		
		/* Only try to eat grass if I'm alive. */
		if (this.isAlive()) {
			
			/* Check if grass is alive. */
			if (cell.isGrassAlive()) {
				
				/* Grass is alive, eat it... */
				cell.eatGrass();
				
				/* ...and gain energy from it. */
				this.setEnergy(this.getEnergy() + params.getSheepGainFromFood());
				
			}
			
		}
	}
}
