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

		/* Start timing. */
		long startTime = System.currentTimeMillis();

		/* Initialize random number generator. */
		rng = this.createRNG(0);
		
		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter(rng); 

		/* Cell behaviors are fixed in the single-threaded case. */
		CellFutureIsNowPostBehavior futureIsNowPost = new CellFutureIsNowPostNop();
		CellPutAgentBehavior putAgent = new CellPutAgentAsync();
		
		/* Initialize statistics. */
		this.sheepStats = new int[params.getIters() + 1];
		this.sheepStats[0] = params.getInitSheep();
		this.wolfStats = new int[params.getIters() + 1];
		this.wolfStats[0] = params.getInitWolves();
		this.grassStats = new int[params.getIters() + 1];
		
		/* Initialize simulation grid. */
		grid = new ICell[params.getGridX()][params.getGridY()];
		
		/* Initialize simulation grid cells. */
		for (int i = 0; i < params.getGridX(); i++) {
			for (int j = 0; j < params.getGridY(); j++) {
				
				/* Add cell to current place in grid. */
				grid[i][j] = new Cell(params.getGrassRestart(), grassInitStrategy, putAgent, putAgent, futureIsNowPost);

				/* Update grass statistics. */
				if (grid[i][j].isGrassAlive())
					this.grassStats[0]++;
			}
		}
		
		/* Populate simulation grid with agents. */
		for (int i = 0; i < params.getInitSheep(); i++) {
			int x = rng.nextInt(params.getGridX());
			int y = rng.nextInt(params.getGridY());
			IAgent sheep = new Sheep(1 + rng.nextInt(2 * params.getSheepGainFromFood()), params);
			grid[x][y].putAgentNow(sheep);
		}
		for (int i = 0; i < params.getInitWolves(); i++) {
			int x = rng.nextInt(params.getGridX());
			int y = rng.nextInt(params.getGridY());
			IAgent wolf = new Wolf(1 + rng.nextInt(2 * params.getWolvesGainFromFood()), params);
			grid[x][y].putAgentNow(wolf);
		}
		
		/* Run simulation. */
		for (int iter = 1; iter <= params.getIters(); iter++) {
			
			if ((stepPrint > 0) && (iter % stepPrint == 0))
				System.out.println("Iter " + iter);
			
			/* Cycle through cells in order to perform step 1 and 2 of simulation. */
			for (int i = 0; i < params.getGridX(); i++) {
				for (int j = 0; j < params.getGridY(); j++) {
					
					/* ************************* */
					/* ** 1 - Agent movement. ** */
					/* ************************* */

					/* Cycle through agents in current cell. */
					for (IAgent agent : grid[i][j].getAgents()) {
						
						/* Decrement agent energy. */
						agent.decEnergy();
						
						/* If agent energy is greater than zero... */
						if (agent.getEnergy() > 0) {
							
							/* ...perform movement. */
							int x = i; int y = j;
							/* Choose direction, if any. */
							int direction = rng.nextInt(5);
							/* If agent decides to move, move him. */
							if (direction == 1) {
								/* Move to the right. */
								x++;
								if (x == params.getGridX())
									x = 0;
							} else if (direction == 2) {
								/* Move to the left. */
								x--;
								if (x < 0)
									x = params.getGridX() - 1;
							} else if (direction == 3) {
								/* Move down. */
								y++;
								if (y == params.getGridY())
									y = 0;
							} else if (direction == 4) {
								/* Move up. */
								y--;
								if (y < 0)
									y = params.getGridY() - 1;
							}
							/* Move agent to new cell in the future. */
							grid[x][y].putAgentFuture(agent);

						}
					}
					
					/* ************************* */
					/* *** 2 - Grass growth. *** */
					/* ************************* */
					
					/* If grass is not alive... */
					if (!grid[i][j].isGrassAlive()) {
						/* ...decrement alive counter. */
						grid[i][j].regenerateGrass();
					}
				}
			}
			
			/* Cycle through cells in order to perform step 3 and 4 of simulation. */
			for (int i = 0; i < params.getGridX(); i++) {
				for (int j = 0; j < params.getGridY(); j++) {

					/* ************************** */
					/* *** 3 - Agent actions. *** */
					/* ************************** */

					/* The future is now (future agents are now present agents)... */
					grid[i][j].futureIsNow();
					
					/* Cycle through agents in cell. */
					for (IAgent agent : grid[i][j].getAgents()) {

						/* Tell agent to act. */
						agent.doPlay(grid[i][j]);
						
					}
					
					/* Remove dead agents. */
					grid[i][j].removeAgentsToBeRemoved();
					
					/* ****************************** */
					/* *** 4 - Gather statistics. *** */
					/* ****************************** */

					for (IAgent agent : grid[i][j].getAgents()) {
						
						if (agent instanceof Sheep)
							sheepStats[iter]++;
						else if (agent instanceof Wolf)
							wolfStats[iter]++;
						
					}
					
					if (grid[i][j].isGrassAlive())
						grassStats[iter]++;
					
				}
			}
			
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
