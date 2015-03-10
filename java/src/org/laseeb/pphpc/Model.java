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

import java.io.FileWriter;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import org.uncommons.maths.random.AESCounterRNG;
import org.uncommons.maths.random.CMWC4096RNG;
import org.uncommons.maths.random.CellularAutomatonRNG;
import org.uncommons.maths.random.JavaRNG;
import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedGenerator;
import org.uncommons.maths.random.XORShiftRNG;

/**
 * A concrete implementation of the simulation model, in the MVC sense.
 * 
 * @author Nuno Fachada
 */
public class Model implements IModel {
	
	/* Size of model (i.e. number of cells). */
	private int size;
	
	/* Model parameters. */
	private ModelParams params;
	
	/* Model cells. */
	private ICell cells[];
	
	/* Current iteration. */
	private int currentIteration;
	
	/* Global statistics. */
	private IGlobalStats globalStats;
	
	/* Simulation space. */
	private ISpace space;
	
	/* Strategy for putting initial agents in cells. */
	private ICellPutAgentStrategy putInitAgentStrategy;
	
	/* Strategy for putting existing agents in cells. */
	private ICellPutAgentStrategy putExistingAgentStrategy;
	
	/* Grass initialization strategy. */
	private ICellGrassInitStrategy grassInitStrategy;
	
	/* Last exception registered with the model. */
	private Throwable lastThrowable;
	
	/* Model event observers. */
	private Map<ModelEvent, List<IModelEventObserver>> observers = null;
	
	/* Shuffle agents before they act? */
	private boolean shuffle;
	
	/* Type of random number generator used in this model. */
	private RNGType rngType;
	
	/* Random number generator seed. */
	private BigInteger seed;
	
	/**
	 * Create a new simulation model.
	 * 
	 * @param params Model parameters.
	 * @param wFactory Work factory used to execute the simulation.
	 * @param shuffle Shuffle agents before they act?
	 * @param rngType Type of random number generator used in this model.
	 * @param seed Random number generator seed. 
	 */
	public Model(ModelParams params, IWorkFactory wFactory, boolean shuffle, 
			RNGType rngType, BigInteger seed) {
		
		this.params = params;
		this.space = new VonNeumann2DTorusSpace(params.getGridX(), params.getGridY());
		this.globalStats = wFactory.createGlobalStats(params.getIters());
		this.putInitAgentStrategy = wFactory.createPutInitAgentStrategy();
		this.putExistingAgentStrategy = wFactory.createPutExistingAgentStrategy();
		this.grassInitStrategy = new CellGrassInitCoinRandCounter();
		this.currentIteration = 0;
		this.size = space.getSize();
		this.cells = new ICell[this.size];
		this.shuffle = shuffle;
		this.rngType = rngType;
		this.seed = seed;

	}
	
	/**
	 * @see IModelQuerier#isShuffle()
	 */
	@Override
	public boolean isShuffle() {
		return shuffle;
	}

	/**
	 * @see IModelQuerier#getSize()
	 */
	@Override
	public int getSize() {
		return this.size;
	}

	/**
	 * @see IModelManipulator#getCell(int)
	 */
	@Override
	public ICell getCell(int idx) {
		return this.cells[idx];
	}

	/**
	 * @see IModelManipulator#setCellNeighbors(int)
	 */
	@Override
	public void setCellNeighbors(int idx) {
		this.space.setNeighbors(cells, idx);
	}

	/**
	 * @see IModelQuerier#getParams()
	 */
	@Override
	public ModelParams getParams() {
		return this.params;
	}

	/**
	 * @see IModelQuerier#getCurrentIteration()
	 */
	@Override
	public int getCurrentIteration() {
		return this.currentIteration;
	}

	/**
	 * @see IModelManipulator#incrementIteration()
	 */
	@Override
	public void incrementIteration() {
		this.currentIteration++;
		this.updateObservers(ModelEvent.NEW_ITERATION);		
	}

	/**
	 * @see IModelManipulator#initCellAt(int, Random)
	 */
	@Override
	public void initCellAt(int idx, Random rng) {
		if (this.cells[idx] == null) {
			this.cells[idx] = new Cell(params.getGrassRestart(), 
					this.grassInitStrategy.getInitGrass(params.getGrassRestart(), rng),
					this.putInitAgentStrategy, this.putExistingAgentStrategy);
		} else {
			throw new IllegalStateException("Cell " + idx + " already set!");
		}
	}
	
	/**
	 * @see IModelManipulator#reset()
	 */
	@Override
	public void reset() {
		Arrays.fill(this.cells, null);
		this.globalStats.reset();
		this.currentIteration = 0;
	}
	
	/**
	 * @see IModelManipulator#start()
	 */
	@Override
	public void start() {
		this.updateObservers(ModelEvent.START);
	}
	
	/**
	 * @see IModelManipulator#stop()
	 */
	@Override
	public void stop() {
		this.updateObservers(ModelEvent.STOP);
	}
	
	/**
	 * @see IModelManipulator#pause()
	 */
	@Override
	public void pause() {
		this.updateObservers(ModelEvent.PAUSE);
	}	

	/**
	 * @see IModelManipulator#unpause()
	 */
	@Override
	public void unpause() {
		this.updateObservers(ModelEvent.UNPAUSE);
	}	
	
	/**
	 * @see IModelManipulator#export(String)
	 */
	@Override
	public void export(String filename) {
		
		FileWriter out = null;
		try {
			out = new FileWriter(filename);
				
			for (int i = 0; i <= params.getIters() ; i++) {
				
				int sheepCount = this.globalStats.getStats(StatType.SHEEP_COUNT, i).intValue();
				int wolvesCount = this.globalStats.getStats(StatType.WOLVES_COUNT, i).intValue();
				int grassAlive = this.globalStats.getStats(StatType.GRASS_ALIVE, i).intValue();
				float avgSheepEnergy = sheepCount > 0 
						? this.globalStats.getStats(StatType.SHEEP_ENERGY, i).longValue() / (float) sheepCount
						: 0;
				float avgWolvesEnergy = wolvesCount > 0 
						? this.globalStats.getStats(StatType.WOLVES_ENERGY, i).longValue() / (float) wolvesCount
						: 0;
				float avgGrassCountdown = this.globalStats.getStats(StatType.GRASS_COUNTDOWN, i).longValue()
						/ (float) this.getSize();
				
				out.write(sheepCount + "\t" + wolvesCount + "\t" + grassAlive + "\t"
						+ avgSheepEnergy + "\t" + avgWolvesEnergy + "\t" + avgGrassCountdown + "\n");
			}
			
			if (out != null) {
				out.close();
			}
		} catch (Exception e) {
			this.registerException(e, "Exporting statistics to file '" + filename + "'");
		}
	}

	/**
	 * @see IModelQuerier#registerException(Throwable, String)
	 */
	@Override
	public void registerException(Throwable t, String s) {

		synchronized (this) {
			this.lastThrowable = new Throwable(t.getMessage() + " (additional info: " + s + ")", t);
		}
		this.updateObservers(ModelEvent.EXCEPTION);
	}

	/**
	 * @see IModelQuerier#getLastThrowable()
	 */
	@Override
	public Throwable getLastThrowable() {
		return this.lastThrowable;
	}

	/**
	 * @see IModelQuerier#createRNG(int)
	 */
	@Override
	public Random createRNG(int modifier) throws Exception {
		
		SeedGenerator seedGen = new ModelSeedGenerator(modifier, this.seed);

		switch (this.rngType) {
			case AES:
				return new AESCounterRNG(seedGen);
			case CA:
				return new CellularAutomatonRNG(seedGen);
			case CMWC:
				return new CMWC4096RNG(seedGen);
			case JAVA:
				return new JavaRNG(seedGen);
			case MT:
				return new MersenneTwisterRNG(seedGen);
			case RANDU:
				return new RanduRNG(seedGen);
			case MODMIDSQUARE:
				return new ModMidSquareRNG(seedGen);
			case XORSHIFT: 
				return new XORShiftRNG(seedGen);
			default:
				throw new RuntimeException("Don't know this random number generator.");
		}
		
	}
	
	/**
	 * @see IModelEventObservable#registerObserver(ModelEvent, IModelEventObserver)
	 */
	@Override
	public void registerObserver(ModelEvent event, IModelEventObserver observer) {
		
		if (this.observers == null) {
			synchronized (this) {
				if (this.observers == null) {
					this.observers = new HashMap<ModelEvent, List<IModelEventObserver>>();
					for (ModelEvent me : ModelEvent.values()) {
						this.observers.put(me, new ArrayList<IModelEventObserver>());
					}
				}
			}
		}
		
		if (event == null) {
			for (List<IModelEventObserver> listOfObservers : this.observers.values()) {
				listOfObservers.add(observer);
			}
		} else {
			this.observers.get(event).add(observer);
		}
	}

	/**
	 * @see IModelQuerier#getStats(int)
	 */
	@Override
	public IterationStats getStats(int iteration) {
		return this.globalStats.getStats(iteration);
	}

	/**
	 * @see IModelQuerier#getLatestStats()
	 */
	@Override
	public IterationStats getLatestStats() {
		return this.globalStats.getStats(this.currentIteration - 1);
	}

	/**
	 * @see IModelManipulator#updateStats(int, IterationStats)
	 */
	@Override
	public void updateStats(int iter, IterationStats iterStats) {
		this.globalStats.updateStats(iter, iterStats);
	}
	
	/**
	 * @see IModelManipulator#getSpace()
	 */
	@Override
	public ISpace getSpace() {
		return this.space;
	}

	/**
	 * Update model event observers registered for the given event.
	 * 
	 * @param event Model event.
	 */
	private void updateObservers(ModelEvent event) {
		List<IModelEventObserver> observerList = this.observers.get(event);
		for (IModelEventObserver observer : observerList) {
			observer.update(event);
		}
	}

}
