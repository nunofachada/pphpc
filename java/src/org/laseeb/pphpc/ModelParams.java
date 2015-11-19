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

import java.io.FileReader;
import java.io.IOException;
import java.util.Properties;

/**
 * PPHPC model parameters.
 * 
 * @author Nuno Fachada
 */
public class ModelParams {
	/*
	 * Model parameters.
	 */
	private int initSheep;
	private int sheepGainFromFood;
	private int sheepReproduceThreshold;
	private int sheepReproduceProb;
	private int initWolves;
	private int wolvesGainFromFood;
	private int wolvesReproduceThreshold;
	private int wolvesReproduceProb;
	private int grassRestart;
	private int gridX;
	private int gridY;
	private int iters;
	
	/**
	 * Create a simulation parameters object by loading parameters
	 * from a parameters file.
	 * 
	 * @param paramsFile Parameters file.
	 * @throws IOException If it wasn't possible to open the given parameters file.
	 */
	public ModelParams(String paramsFile) throws IOException {
		
		Properties properties = new Properties();
		FileReader in = null;
		in = new FileReader(paramsFile);
		properties.load(in);
		in.close();

		this.initSheep = Integer.parseInt(properties.getProperty("INIT_SHEEP"));
		this.sheepGainFromFood = Integer.parseInt(properties.getProperty("SHEEP_GAIN_FROM_FOOD"));
		this.sheepReproduceThreshold = Integer.parseInt(properties.getProperty("SHEEP_REPRODUCE_THRESHOLD"));
		this.sheepReproduceProb = Integer.parseInt(properties.getProperty("SHEEP_REPRODUCE_PROB"));
		this.initWolves = Integer.parseInt(properties.getProperty("INIT_WOLVES"));
		this.wolvesGainFromFood = Integer.parseInt(properties.getProperty("WOLVES_GAIN_FROM_FOOD"));
		this.wolvesReproduceThreshold = Integer.parseInt(properties.getProperty("WOLVES_REPRODUCE_THRESHOLD"));
		this.wolvesReproduceProb = Integer.parseInt(properties.getProperty("WOLVES_REPRODUCE_PROB"));
		this.grassRestart = Integer.parseInt(properties.getProperty("GRASS_RESTART"));
		this.gridX = Integer.parseInt(properties.getProperty("GRID_X"));
		this.gridY = Integer.parseInt(properties.getProperty("GRID_Y"));
		this.iters = Integer.parseInt(properties.getProperty("ITERS"));
	}

	/**
	 * Get initial number of sheep.
	 * 
	 * @return Initial number of sheep.
	 */
	public int getInitSheep() {
		return initSheep;
	}

	/**
	 * Get sheep gain from food.
	 * 
	 * @return Sheep gain from food.
	 */
	public int getSheepGainFromFood() {
		return sheepGainFromFood;
	}

	/**
	 * Get sheep reproduction threshold.
	 * 
	 * @return Sheep reproduction threshold.
	 */
	public int getSheepReproduceThreshold() {
		return sheepReproduceThreshold;
	}

	/**
	 * Get sheep reproduction probability.
	 * 
	 * @return Sheep reproduction probability.
	 */
	public int getSheepReproduceProb() {
		return sheepReproduceProb;
	}

	/**
	 * Get initial number of wolves.
	 * 
	 * @return Initial number of wolves.
	 */
	public int getInitWolves() {
		return initWolves;
	}

	/**
	 * Get wolves gain from food.
	 * 
	 * @return Wolves gain from food.
	 */
	public int getWolvesGainFromFood() {
		return wolvesGainFromFood;
	}

	/**
	 * Get wolves reproduction threshold.
	 * 
	 * @return Wolves reproduction threshold.
	 */
	public int getWolvesReproduceThreshold() {
		return wolvesReproduceThreshold;
	}

	/**
	 * Get wolves reproduction probability.
	 * 
	 * @return Wolves reproduction probability.
	 */
	public int getWolvesReproduceProb() {
		return wolvesReproduceProb;
	}

	/**
	 * Get grass restart time.
	 * 
	 * @return Grass restart time.
	 */
	public int getGrassRestart() {
		return grassRestart;
	}

	/**
	 * Get horizontal grid size (number of columns).
	 * 
	 * @return Horizontal grid size (number of columns).
	 */
	public int getGridX() {
		return gridX;
	}

	/**
	 * Get vertical grid size (number of rows).
	 * 
	 * @return Vertical grid size (number of rows).
	 */
	public int getGridY() {
		return gridY;
	}

	/**
	 * Get number of iterations.
	 * 
	 * @return Number of iterations.
	 */
	public int getIters() {
		return iters;
	}
	
	
}
