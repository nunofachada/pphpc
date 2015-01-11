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
import java.io.IOException;
import java.math.BigInteger;
import java.util.ArrayList;
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

public class Model implements IModel {
	
	private int size;
	private ModelParams params;
	private ICell cells[];
	private int currentIteration;
	private IGlobalStats globalStats;
	private ISpace space;
	private ICellPutAgentStrategy putNewAgentStrategy;
	private ICellPutAgentStrategy putExistingAgentStrategy;
	private ICellGrassInitStrategy grassInitStrategy;
	private ModelStatus status; 
	private Throwable lastThrowable;
	private Map<ModelEvent, List<IModelEventObserver>> observers = null;
	private RNGType rngType;
	private BigInteger seed;
	
	public Model(ModelParams params, IWorkFactory wFactory, RNGType rngType, BigInteger seed) {
		this.params = params;
		this.space = new Square2DTorusSpace(params.getGridX(), params.getGridY());
		this.globalStats = wFactory.createGlobalStats(params.getIters());
		this.putNewAgentStrategy = wFactory.createPutNewAgentStrategy();
		this.putExistingAgentStrategy = wFactory.createPutExistingAgentStrategy();
		this.grassInitStrategy = new CellGrassInitCoinRandCounter();
		this.currentIteration = 0;
		this.size = space.getSize();
		this.cells = new ICell[this.size];
		this.rngType = rngType;
		this.seed = seed;
		this.status = ModelStatus.STOPPED;
	}
	
	@Override
	public int getSize() {
		return this.size;
	}
	
	@Override
	public ICell getCell(int idx) {
		return this.cells[idx];
	}

	@Override
	public void setCell(int idx, Cell cell) {
		this.cells[idx] = cell;
	}

	@Override
	public void setCellNeighbors(int idx) {
		this.space.setNeighbors(cells, idx);
	}

	@Override
	public ModelParams getParams() {
		return this.params;
	}

	@Override
	public int getCurrentIteration() {
		return this.currentIteration;
	}

	@Override
	public void incrementIteration() {
		this.currentIteration++;
		this.updateObservers(ModelEvent.NEW_ITERATION);		
	}

	@Override
	public IGlobalStats getGlobalStats() {
		return this.globalStats;
	}

	@Override
	public void setCellAt(int idx, Random rng) {
		if (this.cells[idx] == null) {
			this.cells[idx] = new Cell(params.getGrassRestart(), 
					this.grassInitStrategy.getInitGrass(params.getGrassRestart(), rng), 
					this.putNewAgentStrategy, this.putExistingAgentStrategy);
		} else {
			throw new IllegalStateException("Cell " + idx + " already set!");
		}
	}

	/**
	 * @return the status
	 */
	@Override
	public ModelStatus getStatus() {
		return status;
	}

	/**
	 * @param status the status to set
	 */
	@Override
	public void setStatus(ModelStatus status) {
			
		ModelEvent event;
		
		if ((this.status == ModelStatus.RUNNING) && (status == ModelStatus.PAUSED)) {
			event = ModelEvent.PAUSE;
		} else if ((this.status == ModelStatus.RUNNING) && (status == ModelStatus.STOPPED)) {
			event = ModelEvent.STOP;
		} else if ((this.status == ModelStatus.PAUSED) && (status == ModelStatus.RUNNING)) {
			event = ModelEvent.CONTINUE;
		} else if ((this.status == ModelStatus.PAUSED) && (status == ModelStatus.STOPPED)) {
			event = ModelEvent.STOP;
		} else if ((this.status == ModelStatus.STOPPED) && (status == ModelStatus.RUNNING)) {
			event = ModelEvent.START;
		} else {
			return;
		}
			
		this.status = status;
		
		this.updateObservers(event);
	}

	@Override
	public boolean isRunning() {
		return this.status == ModelStatus.RUNNING;
	}

	@Override
	public boolean isStopped() {
		return this.status == ModelStatus.STOPPED;
	}

	@Override
	public boolean isPaused() {
		return this.status == ModelStatus.PAUSED;
	}
	
	@Override
	public void export(String filename) {
		
		FileWriter out = null;
		try {
			out = new FileWriter(filename);
				
			for (int i = 0; i <= params.getIters() ; i++) {
				out.write(this.globalStats.getStats(StatType.SHEEP, i) + "\t"
						+ this.globalStats.getStats(StatType.WOLVES, i) + "\t"
						+ this.globalStats.getStats(StatType.GRASS, i) + "\n");
			}
			
			if (out != null) {
				out.close();
			}
		} catch (Exception e) {
			this.registerException(e);
		}
	}

	@Override
	public synchronized void registerException(Throwable t) {
		this.lastThrowable = t;
		this.updateObservers(ModelEvent.EXCEPTION);
	}

	@Override
	public Throwable getLastThrowable() {
		return this.lastThrowable;
	}

	/**
	 * Create a random number generator of the type and with the seed specified as 
	 * the command line arguments.
	 * 
	 * @param modifier Seed modifier, such as a thread ID, so that each thread
	 * can instantiate an independent random number generator (not really
	 * independent, but good enough for the purpose).
	 * 
	 * @return A new random number generator.
	 * @throws Exception If for some reason, with wasn't possible to create the RNG.
	 */
	@Override
	public Random createRNG(long modifier) throws Exception {
		
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
			case XORSHIFT: 
				return new XORShiftRNG(seedGen);
			default:
				throw new RuntimeException("Don't know this random number generator.");
		}
		
	}
	
	
	private void updateObservers(ModelEvent event) {
		List<IModelEventObserver> observerList = this.observers.get(event);
		for (IModelEventObserver observer : observerList) {
			observer.update(event);
		}
	}

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

}
