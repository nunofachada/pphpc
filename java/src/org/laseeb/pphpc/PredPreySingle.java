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

		/* Grass initialization strategy. */
		CellGrassInitStrategy grassInitStrategy = new CellGrassInitCoinRandCounter();
		
		/* Create simulation grid. */
		ISimSpace space = new Square2DTorusSimSpace(params.getGridX(), params.getGridY());
		workProvider = new EqualSimWorkProvider(space, params.getGrassRestart(), grassInitStrategy, 1, false /* irrelevant*/ ); 
//		workProvider = new OnDemandSimWorkProvider(space, params.getGrassRestart(), grassInitStrategy, AbstractSimWorkProvider.Threading.SINGLE, 1); 
		
		/* Initialize statistics. */
		this.initStats();

		/* Run simulation in main thread. */
		SimWorker st = new PredPrey.SimWorker(0);
		st.run();
		
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
	protected void updateStats(int iter, PPStats stats) {
		sheepStats[iter] = stats.getSheep();
		wolfStats[iter] = stats.getWolves();
		grassStats[iter] = stats.getGrass();
	}

	@Override
	protected void initStats() {
		this.sheepStats = new int[params.getIters() + 1];
		this.wolfStats = new int[params.getIters() + 1];
		this.grassStats = new int[params.getIters() + 1];		
	}


}
