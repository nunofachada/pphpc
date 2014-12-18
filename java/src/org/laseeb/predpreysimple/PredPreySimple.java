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
package org.laseeb.predpreysimple;

import java.io.FileReader;
import java.io.FileWriter;
import java.util.Iterator;
import java.util.Properties;

import cern.jet.random.Uniform;
import cern.jet.random.engine.MersenneTwister;

/**
 * Main class for simple predator-prey model.
 * @author Nuno Fachada
 */
public class PredPreySimple {

	/*
	 * Model parameters.
	 */
	static int INIT_SHEEP;
	static int SHEEP_GAIN_FROM_FOOD;
	static int SHEEP_REPRODUCE_THRESHOLD;
	static int SHEEP_REPRODUCE_PROB;
	static int INIT_WOLVES;
	static int WOLVES_GAIN_FROM_FOOD;
	static int WOLVES_REPRODUCE_THRESHOLD;
	static int WOLVES_REPRODUCE_PROB;
	static int GRASS_RESTART;
	static int GRID_X;
	static int GRID_Y;
	static int ITERS;

	/* Simulation grid. */
	private Cell[][] grid;
	
	/* Random number generator. */
	private static Uniform rand;

	/* Statistics gathering. */
	private int[] sheepStats;
	private int[] wolfStats;
	private int[] grassStats;
	
	/** 
	 * Constructor, no arguments required.
	 */
	public PredPreySimple() {}
	
	/**
	 * Returns random integer between 0 and i.
	 * @param i Maximum integer to obtain.
	 * @return A random integer between 0 and i.
	 */
	public static int nextInt(int i) {
		return rand.nextIntFromTo(0, i);
	}

	/**
	 * Perform simulation.
	 */
	public void start(int stepPrint) {
		/* Initialize random number generator. */
		rand = new Uniform(new MersenneTwister((int) System.nanoTime()));
		/* Initialize statistics. */
		this.sheepStats = new int[ITERS + 1];
		this.sheepStats[0] = INIT_SHEEP;
		this.wolfStats = new int[ITERS + 1];
		this.wolfStats[0] = INIT_WOLVES;
		this.grassStats = new int[ITERS + 1];
		/* Initialize simulation grid. */
		grid = new Cell[GRID_X][GRID_Y];
		/* Initialize simulation grid cells. */
		for (int i = 0; i < GRID_X; i++) {
			for (int j = 0; j < GRID_Y; j++) {
				/* Add cell to current place in grid. */
				grid[i][j] = new Cell();
				/* Grow grass in current cell. */
				if (rand.nextBoolean()) {
					/* Grass not alive, initialize grow timer. */
					grid[i][j].setGrass(1 + nextInt(GRASS_RESTART - 1));
				} else {
					/* Grass alive. */
					grid[i][j].setGrass(0);
					/* Update grass statistics. */
					this.grassStats[0]++;
				}
			}
		}
		
		/* Populate simulation grid with agents. */
		for (int i = 0; i < INIT_SHEEP; i++)
			grid[nextInt(GRID_X - 1)][nextInt(GRID_Y - 1)].putAgentNow(new Sheep(1 + nextInt(2 * SHEEP_GAIN_FROM_FOOD - 1)));
		for (int i = 0; i < INIT_WOLVES; i++)
			grid[nextInt(GRID_X - 1)][nextInt(GRID_Y - 1)].putAgentNow(new Wolf(1 + nextInt(2 * WOLVES_GAIN_FROM_FOOD - 1)));
		
		/* Run simulation. */
		long startTime = System.currentTimeMillis();
		for (int iter = 1; iter <= ITERS; iter++) {
			if (iter % stepPrint == 0)
				System.out.println("Iter " + iter);
			/* Agent movement. */
			for (int i = 0; i < GRID_X; i++) {
				for (int j = 0; j < GRID_Y; j++) {
					/* Cycle through agents in current cell. */
					Iterator<Agent> agentIter = grid[i][j].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						/* Decrement agent energy. */
						agent.decEnergy();
						/* If agent energy is greater than zero... */
						if (agent.getEnergy() > 0) {
							/* ...perform movement. */
							int x = i; int y = j;
							/* Choose direction, if any. */
							int direction = nextInt(4);
							/* If agent decides to move, move him. */
							if (direction == 1) {
								/* Move to the right. */
								x++;
								if (x == GRID_X)
									x = 0;
							} else if (direction == 2) {
								/* Move to the left. */
								x--;
								if (x < 0)
									x = GRID_X - 1;
							} else if (direction == 3) {
								/* Move down. */
								y++;
								if (y == GRID_Y)
									y = 0;
							} else if (direction == 4) {
								/* Move up. */
								y--;
								if (y < 0)
									y = GRID_Y - 1;
							}
							/* Move agent to new cell in the future. */
							grid[x][y].putAgentFuture(agent);

						}
					}
				}
			}
			/* Grass growth. */
			for (int i = 0; i < GRID_X; i++) {
				for (int j = 0; j < GRID_Y; j++) {
					/* If grass is not alive... */
					if (grid[i][j].getGrass() > 0) {
						/* ...decrement alive counter. */
						grid[i][j].decGrass();
					}
				}
			}
			/* Agent actions */
			for (int i = 0; i < GRID_X; i++) {
				for (int j = 0; j < GRID_Y; j++) {
					/* The future is now (future agents are now present agents)... */
					grid[i][j].futureIsNow();
					/* Cycle through agents in cell. */
					Iterator<Agent> agentIter = grid[i][j].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						/* Tell agent to act. */
						agent.doPlay(grid[i][j]);
					}
					/* Remove dead agents. */
					grid[i][j].removeAgentsToBeRemoved();
				}
			}
			/* Gather statistics. */
			for (int i = 0; i < GRID_X; i++) {
				for (int j = 0; j < GRID_Y; j++) {
					Iterator<Agent> agentIter = grid[i][j].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						if (agent instanceof Sheep)
							sheepStats[iter]++;
						else if (agent instanceof Wolf)
							wolfStats[iter]++;
					}
					if (grid[i][j].getGrass() == 0)
						grassStats[iter]++;
				}
			}
			
		}
		long endTime = System.currentTimeMillis();
		float timeInSeconds = (float) (endTime - startTime) / 1000.0f;
		System.out.println("Total simulation time: " + timeInSeconds + "\n");
		
	}
	/**
	 * Export statistics to file.
	 * @param str Statistics filename.
	 */
	public void export(String str) {
		FileWriter out = null;
		try {
			out = new FileWriter(str);
            for (int i = 0; i <= ITERS ; i++)
            	out.write(sheepStats[i] + "\t" + wolfStats[i] + "\t" + grassStats[i] + "\n");

        } catch (Exception e) {
			e.printStackTrace();
		} finally {
            if (out != null) {
                try {
					out.close();
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
            }
        }
 	}
	
	private static void loadProperties(String params) {
		Properties properties = new Properties();
		FileReader in = null;
		try {
			in = new FileReader(params);
			properties.load(in);
			in.close();
		} catch (Exception ex) {
			ex.printStackTrace();
		}
		/*
		 * Model parameters.
		 */
		INIT_SHEEP = Integer.parseInt(properties.getProperty("INIT_SHEEP"));
		SHEEP_GAIN_FROM_FOOD = Integer.parseInt(properties.getProperty("SHEEP_GAIN_FROM_FOOD"));
		SHEEP_REPRODUCE_THRESHOLD = Integer.parseInt(properties.getProperty("SHEEP_REPRODUCE_THRESHOLD"));
		SHEEP_REPRODUCE_PROB = Integer.parseInt(properties.getProperty("SHEEP_REPRODUCE_PROB"));
		INIT_WOLVES = Integer.parseInt(properties.getProperty("INIT_WOLVES"));
		WOLVES_GAIN_FROM_FOOD = Integer.parseInt(properties.getProperty("WOLVES_GAIN_FROM_FOOD"));
		WOLVES_REPRODUCE_THRESHOLD = Integer.parseInt(properties.getProperty("WOLVES_REPRODUCE_THRESHOLD"));
		WOLVES_REPRODUCE_PROB = Integer.parseInt(properties.getProperty("WOLVES_REPRODUCE_PROB"));
		GRASS_RESTART = Integer.parseInt(properties.getProperty("GRASS_RESTART"));
		GRID_X = Integer.parseInt(properties.getProperty("GRID_X"));
		GRID_Y = Integer.parseInt(properties.getProperty("GRID_Y"));
		ITERS = Integer.parseInt(properties.getProperty("ITERS"));
	}
	
	/**
	 * Main function.
	 * @param args Optionally indicate period of iterations to print current iteration to screen.
	 * If ignored, all iterations will be printed to screen.
	 */
	public static void main(String[] args) {
		if (args.length == 0) {
			System.err.println("Usage: java -cp bin:lib/colt-1.2.0.jar " 
				+ PredPreySimple.class.getName() + " PARAMS_FILE [PRINT_STEP]");
			System.exit(-1);
		}
		loadProperties(args[0]);
		int stepPrint = 1;
		if (args.length >= 2)
			stepPrint = Integer.parseInt(args[1]);
		PredPreySimple pps = new PredPreySimple();
		pps.start(stepPrint);
		pps.export("statsjava.txt");
	}

}
