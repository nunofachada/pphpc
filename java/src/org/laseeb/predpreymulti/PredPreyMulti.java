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
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicIntegerArray;

import org.laseeb.predpreysimple.PredPreySimple;

import cern.jet.random.Uniform;
import cern.jet.random.engine.MersenneTwister;

import org.kohsuke.args4j.CmdLineException;
import org.kohsuke.args4j.CmdLineParser;
import org.kohsuke.args4j.Option;

/**
 * Main class for multi-threaded predator-prey model.
 * @author Nuno Fachada
 */
public class PredPreyMulti {
	
	final String paramsFileDefault = "config.txt";
	final String statsFileDefault = "stats.txt";

	
	/* Number of threads. */
	@Option(name="-n", usage="number of threads, defaults to the number of processors")
	int NUM_THREADS = Runtime.getRuntime().availableProcessors();
	
	/* Interval to print current iteration. */
	@Option(name="-i", usage="interval of iterations to print current iteration")
	long stepPrint = Long.MAX_VALUE;
	
	/* File containing simulation parameters. */
	@Option(name="-p", usage="specify parameters file (default is " + paramsFileDefault + ")")
	String paramsFile = paramsFileDefault;
	
	/* File where to output simulation statistics. */
	@Option(name="-s", usage="Specify statistics output file (default is " + statsFileDefault + ")")
	String statsFile = statsFileDefault;

	/* Simulation parameters. */
	private SimParams params;
	
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
			long gridsize = params.getGridX() * params.getGridY();
			ArrayList<Point> myCells = new ArrayList<Point>();
			for (int k = offset; k < gridsize; k += NUM_THREADS) {
				Point point = new Point();
				point.y = k / params.getGridX();
				point.x = k - point.y * params.getGridX();
				myCells.add(point);
			}
			int numCells = myCells.size();
			/* Perform simulation steps. */
			for (int iter = 1; iter <= params.getIters(); iter++) {
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
	public void start() {
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
				if (iter > params.getIters()) {
					semaphore.release();
				}				
			}
		});
		this.partialBarrier = new CyclicBarrier(NUM_THREADS);
		/* Initialize statistics. */
		int[] resetArray = new int[params.getIters() + 1];
		Arrays.fill(resetArray, 0);
		this.sheepStats = new AtomicIntegerArray(resetArray);
		this.sheepStats.set(0, params.getInitSheep());
		this.wolfStats = new AtomicIntegerArray(resetArray);
		this.wolfStats.set(0, params.getInitWolves());
		this.grassStats = new AtomicIntegerArray(resetArray);
		/* Initialize simulation grid. */
		grid = new Cell[params.getGridX()][params.getGridY()];
		/* Initialize simulation grid cells. */
		for (int i = 0; i < params.getGridX(); i++) {
			for (int j = 0; j < params.getGridY(); j++) {
				/* Add cell to current place in grid. */
				grid[i][j] = new Cell(params, this.NUM_THREADS);
				/* Grow grass in current cell. */
				if (rand.get().nextBoolean()) {
					/* Grass not alive, initialize grow timer. */
					grid[i][j].setGrass(1 + nextInt(params.getGrassRestart() - 1));
				} else {
					/* Grass alive. */
					grid[i][j].setGrass(0);
					/* Update grass statistics. */
					this.grassStats.getAndIncrement(0);
				}
			}
		}
		/* Populate simulation grid with agents. */
		for (int i = 0; i < params.getInitSheep(); i++)
			grid[nextInt(params.getGridX() - 1)][nextInt(params.getGridY() - 1)].putAgentNow(
						new Sheep(1 + nextInt(2 * params.getSheepGainFromFood() - 1), params));
		for (int i = 0; i < params.getInitWolves(); i++)
			grid[nextInt(params.getGridX() - 1)][nextInt(params.getGridY() - 1)].putAgentNow(
						new Wolf(1 + nextInt(2 * params.getWolvesGainFromFood() - 1), params));
		
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
	private void export() {
		FileWriter out = null;
		try {
			out = new FileWriter(this.statsFile);
            for (int i = 0; i <= params.getIters() ; i++)
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
	
	public void doMain(String[] args) {
		
		CmdLineParser parser = new CmdLineParser(this);
		
		try {
			parser.parseArgument(args);
		} catch (CmdLineException e) {
			System.err.println(e.getMessage());
			System.err.println("java -cp bin" + java.io.File.pathSeparator + "lib/* " 
				+ PredPreySimple.class.getName() + " [options...]");
			parser.printUsage(System.err);
			System.err.println();
			return;
		}
		
		try {
			this.params = new SimParams(this.paramsFile);
		} catch (IOException e) {
			System.err.println(e.getMessage());
			return;
		}
		
		System.out.println("Starting simulation with " + this.NUM_THREADS + " threads.");
		
		this.start();
		this.export();
		
	}
	
	
	/**
	 * Main function.
	 * @param args Optionally indicate period of iterations to print current iteration to screen.
	 * If ignored, all iterations will be printed to screen.
	 */
	public static void main(String[] args) {
		
		new PredPreyMulti().doMain(args);
	}

}
