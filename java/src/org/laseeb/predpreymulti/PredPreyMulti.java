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
 * The PPHPC multithreaded package.
 */
package org.laseeb.predpreymulti;

import java.awt.Point;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Properties;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicIntegerArray;

import org.laseeb.predpreysimple.PredPreySimple;

import cern.jet.random.Uniform;
import cern.jet.random.engine.MersenneTwister;

/**
 * Main class for multi-threaded predator-prey model.
 * @author Nuno Fachada
 */
public class PredPreyMulti {

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
	static int NUM_THREADS;

	/* Simulation grid. */
	private Cell[][] grid;
	
	/* Random number generator. */
	private static ThreadLocal<Uniform> rand;

	/* Statistics gathering. */
	private AtomicIntegerArray sheepStats;
	private AtomicIntegerArray wolfStats;
	private AtomicIntegerArray grassStats;
	
	/* Thread barrier. */
	private CyclicBarrier mainBarrier;
	private CyclicBarrier partialBarrier;
	/* Semaphore for waiting until the end of the simulation. */
	private Semaphore semaphore;
	
	/** 
	 * Constructor, no arguments required.
	 */
	public PredPreyMulti() {}
	
	/**
	 * Returns random integer between 0 and i.
	 * @param i Maximum integer to obtain.
	 * @return A random integer between 0 and i.
	 */
	public static int nextInt(int i) {
		return rand.get().nextIntFromTo(0, i);
	}

	/* A simulation thread. */
	private class SimThread implements Runnable {
		/* Grid offset for each thread. */
		private int offset; 
		/* Constructor only sets the grid offset each thread. */
		public SimThread(int offset) {
			this.offset = offset;
		}
		/* Simulate! */
		public void run() {
			/* Initialize my own random number generator. */
			rand.set(new Uniform(new MersenneTwister((int) System.nanoTime())));
			/* Partial statistics */
			int sheepStatsPartial;
			int wolfStatsPartial;
			int grassStatsPartial;
			/* Aux. var. */
			Point auxPoint = new Point();
			/* Define this thread's cells. */
			long gridsize = GRID_X * GRID_Y;
			ArrayList<Point> myCells = new ArrayList<Point>();
			for (int k = offset; k < gridsize; k += NUM_THREADS) {
				Point point = new Point();
				point.y = k / GRID_X;
				point.x = k - point.y * GRID_X;
				myCells.add(point);
			}
			int numCells = myCells.size();
			/* Perform simulation steps. */
			for (int iter = 1; iter <= ITERS; iter++) {
				/* Agent movement. */
				for (int i = 0; i < numCells; i++) {
					/* Get x and y coordinates for this cell. */
					auxPoint = myCells.get(i);
					/* Cycle through agents in current cell. */
					Iterator<Agent> agentIter = grid[auxPoint.x][auxPoint.y].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						/* Decrement agent energy. */
						agent.decEnergy();
						/* If agent energy is greater than zero... */
						if (agent.getEnergy() > 0) {
							/* ...perform movement. */
							int x = auxPoint.x; int y = auxPoint.y;
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
				/* Grass growth. */
				for (int i = 0; i < numCells; i++) {
					/* Get x and y coordinates for this cell. */
					auxPoint = myCells.get(i);
					/* If grass is not alive... */
					if (grid[auxPoint.x][auxPoint.y].getGrass() > 0) {
						/* ...decrement alive counter. */
						grid[auxPoint.x][auxPoint.y].decGrass();
					}
				}
				/* Sync. with barrier. */
				try {
					partialBarrier.await();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (BrokenBarrierException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				/* Agent actions */
				for (int i = 0; i < numCells; i++) {
					/* Get x and y coordinates for this cell. */
					auxPoint = myCells.get(i);
					/* The future is now (future agents are now present agents)... */
					grid[auxPoint.x][auxPoint.y].futureIsNow();
					/* Cycle through agents in cell. */
					Iterator<Agent> agentIter = grid[auxPoint.x][auxPoint.y].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						/* Tell agent to act. */
						agent.doPlay(grid[auxPoint.x][auxPoint.y]);
					}
					/* Remove dead agents. */
					grid[auxPoint.x][auxPoint.y].removeAgentsToBeRemoved();
				}
				/* Gather statistics. */
				sheepStatsPartial = 0;
				wolfStatsPartial = 0;
				grassStatsPartial = 0;
				for (int i = 0; i < numCells; i++) {
					/* Get x and y coordinates for this cell. */
					auxPoint = myCells.get(i);
					/* Gather statistics... */
					Iterator<Agent> agentIter = grid[auxPoint.x][auxPoint.y].getAgents();
					while (agentIter.hasNext()) {
						Agent agent = agentIter.next();
						if (agent instanceof Sheep)
							sheepStatsPartial++;
						else if (agent instanceof Wolf)
							wolfStatsPartial++;
					}
					if (grid[auxPoint.x][auxPoint.y].getGrass() == 0)
						grassStatsPartial++;
				}
				/* Update global statistics. */
				sheepStats.addAndGet(iter, sheepStatsPartial);
				wolfStats.addAndGet(iter, wolfStatsPartial);
				grassStats.addAndGet(iter, grassStatsPartial);
				/* Sync. with barrier. */
				try {
					mainBarrier.await();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (BrokenBarrierException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}

	}
	/**
	 * Perform simulation.
	 */
	public void start(final int stepPrint) {
		/* Initialize thread-safe random number generator. */
		rand = new ThreadLocal<Uniform>();
		rand.set(new Uniform(new MersenneTwister((int) System.nanoTime())));
		/* Initialize and acquire one permit semaphore. */
		semaphore = new Semaphore(1);
		try {
			semaphore.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		/* Initialize thread barriers. */
		this.mainBarrier = new CyclicBarrier(NUM_THREADS, new Runnable() {
			int iter = 1;
			public void run() {
				/* Print current iteration, if that is the case. */
				if (iter % stepPrint == 0)
					System.out.println("Iter " + iter);
				iter++;
				/* If this is the last iteration, let main thread continue... */
				if (iter > ITERS) {
					semaphore.release();
				}				
			}
		});
		this.partialBarrier = new CyclicBarrier(NUM_THREADS);
		/* Initialize statistics. */
		int[] resetArray = new int[ITERS + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.sheepStats.set(0, INIT_SHEEP);
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.wolfStats.set(0, INIT_WOLVES);
		this.grassStats = new AtomicIntegerArray(resetArray);
		/* Initialize simulation grid. */
		grid = new Cell[GRID_X][GRID_Y];
		/* Initialize simulation grid cells. */
		for (int i = 0; i < GRID_X; i++) {
			for (int j = 0; j < GRID_Y; j++) {
				/* Add cell to current place in grid. */
				grid[i][j] = new Cell();
				/* Grow grass in current cell. */
				if (rand.get().nextBoolean()) {
					/* Grass not alive, initialize grow timer. */
					grid[i][j].setGrass(1 + nextInt(GRASS_RESTART - 1));
				} else {
					/* Grass alive. */
					grid[i][j].setGrass(0);
					/* Update grass statistics. */
					this.grassStats.getAndIncrement(0);
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
		for (int i = 0; i < NUM_THREADS; i++)
			(new Thread(new SimThread(i))).start();
		try {
			semaphore.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
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
            	out.write(sheepStats.get(i) + "\t" + wolfStats.get(i) + 
            			"\t" + grassStats.get(i) + "\n");

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
				+ PredPreySimple.class.getName() + " PARAMS_FILE [NUM_THREADS] [PRINT_STEP]");
			System.exit(-1);
		}
		loadProperties(args[0]);
		int stepPrint = 1;
		if (args.length >= 2)
				NUM_THREADS = Integer.parseInt(args[1]);
		else
				NUM_THREADS = Runtime.getRuntime().availableProcessors();
		if (args.length >= 3)
			stepPrint = Integer.parseInt(args[2]);
		PredPreyMulti pps = new PredPreyMulti();
		System.out.println("stepprint="+stepPrint+" threads="+NUM_THREADS);
		pps.start(stepPrint);
		pps.export("statsjava.txt");
	}

}
