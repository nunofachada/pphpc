/*
 * Copyright (c) 2014, Nuno Fachada
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

/**
 * The PPHPC single-threaded package.
 */
package org.laseeb.pphpc;

import java.security.GeneralSecurityException;
import java.util.Random;

import org.uncommons.maths.random.SeedException;

/**
 * Single-threaded PPHPC model.
 * 
 * @author Nuno Fachada
 */
public class PredPreySingle extends PredPrey {

	/* Statistics gathering. */
	private int[] sheepStats;
	private int[] wolfStats;
	private int[] grassStats;
	
	private Random rng;
	
	/** 
	 * Constructor, no arguments required.
	 */
	private PredPreySingle() {}
	
	/**
	 * Perform simulation.
	 * @throws GeneralSecurityException 
	 * @throws SeedException 
	 */
	public void start() throws Exception {

		/* Current cell being processed. */
		ICell cell;
		
		/* Start timing. */
		long startTime = System.currentTimeMillis();

		/* Initialize random number generator. */
		rng = this.createRNG(0);
		
		PPStats stats = new PPStats();
		
		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter();
		
		/* Create simulation grid. */
		grid = new SimGrid(params.getGridX(), params.getGridY(), params.getGrassRestart(), grassInitStrategy, SimGrid.Threading.SINGLE, 1); 
		
		/* Initialize statistics. */
		this.sheepStats = new int[params.getIters() + 1];
		this.wolfStats = new int[params.getIters() + 1];
		this.grassStats = new int[params.getIters() + 1];
		
		/* Initialize simulation grid cells. */
		ISimThreadState tState = grid.initialize(0);
		
		/* Populate simulation grid with agents. */
		for (int i = 0; i < params.getInitSheep(); i++) {
			int idx = this.rng.nextInt(params.getGridX() * params.getGridY());
			IAgent sheep = new Sheep(1 + this.rng.nextInt(2 * params.getSheepGainFromFood()), params);
			grid.getCell(idx).putNewAgent(sheep);
		}
		for (int i = 0; i < params.getInitWolves(); i++) {
			int idx = this.rng.nextInt(params.getGridX() * params.getGridY());
			IAgent wolf = new Wolf(1 + this.rng.nextInt(2 * params.getWolvesGainFromFood()), params);		
			grid.getCell(idx).putNewAgent(wolf);
		}
		
		/* Get initial statistics. */
		stats.reset();
		grid.reset(tState);
		while ((cell = grid.getNextCell(tState)) != null) {
			cell.getStats(stats);
		}
		
		sheepStats[0] = stats.getSheep();
		wolfStats[0] = stats.getWolves();
		grassStats[0] = stats.getGrass();

		/* Run simulation. */
		for (int iter = 1; iter <= params.getIters(); iter++) {
			
			if ((stepPrint > 0) && (iter % stepPrint == 0))
				System.out.println("Iter " + iter);
			
			/* Cycle through cells in order to perform step 1 and 2 of simulation. */
			grid.reset(tState);
			while ((cell = grid.getNextCell(tState)) != null) {
				
				/* ************************* */
				/* ** 1 - Agent movement. ** */
				/* ************************* */

				cell.agentsMove();
					
				/* ************************* */
				/* *** 2 - Grass growth. *** */
				/* ************************* */
					
				/* If grass is not alive... */
				if (!cell.isGrassAlive()) {
					/* ...decrement alive counter. */
					cell.regenerateGrass();
				}
			}
			
			/* Reset statistics for current iteration. */
			stats.reset();
			
			/* Cycle through cells in order to perform step 3 and 4 of simulation. */
			grid.reset(tState);
			while ((cell = grid.getNextCell(tState)) != null) {

				/* ************************** */
				/* *** 3 - Agent actions. *** */
				/* ************************** */
					
				cell.agentActions();

				/* ****************************** */
				/* *** 4 - Gather statistics. *** */
				/* ****************************** */
					
				cell.getStats(stats);
					
			}
			
			/* Update global stats. */
			sheepStats[iter] = stats.getSheep();
			wolfStats[iter] = stats.getWolves();
			grassStats[iter] = stats.getGrass();
			
		}
		
		/* Stop timing and show simulation time. */
		long endTime = System.currentTimeMillis();
		float timeInSeconds = (float) (endTime - startTime) / 1000.0f;
		System.out.println("Total simulation time: " + timeInSeconds + "\n");
		
	}

	/**
	 * Main function.
	 * 
	 * @param args Command line arguments.
	 */
	public static void main(String[] args) {
		
		PredPrey pp = PredPrey.getInstance(PredPreySingle.class);
		int status = pp.doMain(args);
		System.exit(status);
		
	}

	/* (non-Javadoc)
	 * @see org.laseeb.pphpc.PredPrey#getStats(StatType, int)
	 */
	@Override
	protected int getStats(StatType st, int iter) {

		switch (st) {
			case SHEEP:
				return sheepStats[iter];
			case WOLVES:
				return wolfStats[iter];
			case GRASS:
				return grassStats[iter];
			}

		return 0;
	}

	@Override
	protected Random getRng() {
		// TODO Auto-generated method stub
		return this.rng;
	}

}
