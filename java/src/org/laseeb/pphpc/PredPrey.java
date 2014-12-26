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
 * The PPHPC package.
 */
package org.laseeb.pphpc;

import java.io.FileWriter;
import java.io.IOException;
import java.math.BigInteger;
import java.security.GeneralSecurityException;
import java.util.Random;

import org.uncommons.maths.random.AESCounterRNG;
import org.uncommons.maths.random.CMWC4096RNG;
import org.uncommons.maths.random.CellularAutomatonRNG;
import org.uncommons.maths.random.JavaRNG;
import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedException;
import org.uncommons.maths.random.SeedGenerator;
import org.uncommons.maths.random.XORShiftRNG;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.JCommander;
import com.beust.jcommander.ParameterException;

/**
 * Main class for multi-threaded predator-prey model.
 * @author Nuno Fachada
 */
public abstract class PredPrey {
	

	public enum StatType { SHEEP, WOLVES, GRASS }
	
	final String paramsFileDefault = "config.txt";
	final String statsFileDefault = "stats.txt";
	
	/* Interval to print current iteration. */
	@Parameter(names = "-i", description = "Interval of iterations to print current iteration")
	protected long stepPrint = Long.MAX_VALUE;
	
	/* File containing simulation parameters. */
	@Parameter(names = "-p", description = "File containing simulation parameters")
	private String paramsFile = paramsFileDefault;
	
	/* File where to output simulation statistics. */
	@Parameter(names = "-s", description = "Statistics output file")
	private String statsFile = statsFileDefault;
	
	@Parameter(names = "-r", description = "Seed for random number generator (defaults to System.nanoTime())", converter = BigIntegerConverter.class)
	private BigInteger seed = null;
	
	@Parameter(names = "-g", description = "Random number generator (aes, ca, cmwc, java, mt or xorshift)", converter =  RNGTypeConverter.class)
	private RNGType rngType = RNGType.MT;
	
	/* Help option. */
	@Parameter(names = {"--help", "-h", "-?"}, description = "Show options", help = true)
	private boolean help;

	/* Simulation parameters. */
	protected SimParams params;
	
	/* Simulation grid. */
	protected Cell[][] grid;

	/**
	 * Perform simulation.
	 * @throws GeneralSecurityException 
	 * @throws SeedException 
	 */
	protected abstract void start() throws SeedException, GeneralSecurityException;
	
	protected abstract int getStats(StatType st, int iter);
	
	/**
	 * Export statistics to file.
	 * @param str Statistics filename.
	 */
	private void export() {
		
		FileWriter out = null;
		try {
			out = new FileWriter(this.statsFile);
		
			for (int i = 0; i <= params.getIters() ; i++) {
				out.write(this.getStats(StatType.SHEEP, i) + "\t"
						+ this.getStats(StatType.WOLVES, i) + "\t"
						+ this.getStats(StatType.GRASS, i) + "\n");
			}
		
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
		
		JCommander parser = new JCommander(this);
		parser.setProgramName("java -cp bin" + java.io.File.pathSeparator + "lib/* " 
				+ PredPreyMulti.class.getName());
		
		try {
			parser.parse(args);
		} catch (ParameterException pe) {
			System.err.println(pe.getMessage());
			parser.usage();
			return;
		}
		
		if (this.help) {
			parser.usage();
			return;
		}
		
		try {
			this.params = new SimParams(this.paramsFile);
		} catch (IOException e) {
			System.err.println(e.getMessage());
			return;
		}
		
		if (this.seed == null)
			this.seed = BigInteger.valueOf(System.nanoTime());
//		System.out.println("Starting simulation with " + this.NUM_THREADS + " threads.");
		
		try {
			this.start();
		} catch (Exception e) {
			System.err.println(e.getMessage());
			e.printStackTrace();
			return;
		}
		
		this.export();
		
	}
	
	protected Random createRNG(long modifier) throws SeedException, GeneralSecurityException {
		
		SeedGenerator seedGen = new PPSeedGenerator(modifier, this.seed);

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
	
}
