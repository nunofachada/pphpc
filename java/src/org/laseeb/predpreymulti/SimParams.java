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

package org.laseeb.predpreymulti;

import java.io.FileReader;
import java.io.IOException;
import java.util.Properties;

public class SimParams {
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
	
	public SimParams(String paramsFile) throws IOException {
		
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

	public int getInitSheep() {
		return initSheep;
	}

	public int getSheepGainFromFood() {
		return sheepGainFromFood;
	}

	public int getSheepReproduceThreshold() {
		return sheepReproduceThreshold;
	}

	public int getSheepReproduceProb() {
		return sheepReproduceProb;
	}

	public int getInitWolves() {
		return initWolves;
	}

	public int getWolvesGainFromFood() {
		return wolvesGainFromFood;
	}

	public int getWolvesReproduceThreshold() {
		return wolvesReproduceThreshold;
	}

	public int getWolvesReproduceProb() {
		return wolvesReproduceProb;
	}

	public int getGrassRestart() {
		return grassRestart;
	}

	public int getGridX() {
		return gridX;
	}

	public int getGridY() {
		return gridY;
	}

	public int getIters() {
		return iters;
	}
	
	
}
