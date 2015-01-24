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

import java.io.IOException;
import java.math.BigInteger;
import java.util.Random;

import org.laseeb.pphpc.PredPrey.Errors;
import org.uncommons.maths.random.AESCounterRNG;
import org.uncommons.maths.random.CMWC4096RNG;
import org.uncommons.maths.random.CellularAutomatonRNG;
import org.uncommons.maths.random.JavaRNG;
import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedGenerator;
import org.uncommons.maths.random.XORShiftRNG;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;

/**
 * Small class to test the available RNGs with Dieharder.
 * 
 * Usage: java -cp bin:lib/* org.laseeb.pphpc.TestRNGs | dieharder -g 200 -a
 * 
 * @author Nuno Fachada
 */
public class TestRNGs {

	/* Seed for random number generator. */
	@Parameter(names = "-r", description = "Seed for random number generator (defaults to System.nanoTime())", 
			converter = BigIntegerConverter.class)
	private BigInteger seed = null;
	
	/* Random number generator implementation. */
	@Parameter(names = "-g", description = "Random number generator (AES, CA, CMWC, JAVA, MT, RANDU, REALLYPOOR or XORSHIFT)", 
			converter =  RNGTypeConverter.class)
	private RNGType rngType = RNGType.MT;
	
	/* Help option. */
	@Parameter(names = {"--help", "-h", "-?"}, description = "Show options", help = true)
	private boolean help;
	
	/**
	 * Main method.
	 * 
	 * @param args Command-line arguments.
	 */
	public static void main(String[] args) {
		new TestRNGs().doMain(args);
	}

	/**
	 * This method will actually do stuff once the TestRNGs object is created.
	 * 
	 * @param args Command-line arguments.
	 */
	private void doMain(String[] args) {

		/* Setup command line options parser. */
		JCommander parser = new JCommander(this);
		parser.setProgramName("java -cp bin" + java.io.File.pathSeparator + "lib" +  
				java.io.File.separator +  "* " + TestRNGs.class.getName());
		
		/* Parse command line options. */
		try {
			parser.parse(args);
		} catch (ParameterException pe) {
			/* On parsing error, show usage and return. */
			System.err.println(pe.getMessage());
			parser.usage();
			System.exit(Errors.ARGS.getValue());
		}
		
		/* If help option was passed, show help and quit. */
		if (this.help) {
			parser.usage();
			System.exit(Errors.NONE.getValue());
		}
		
		/* Setup seed for random number generator. */
		if (this.seed == null)
			this.seed = BigInteger.valueOf(System.nanoTime());

		/* Create random number generator. */
		Random rng = null;
		try {
			rng = this.createRNG(0);
		} catch (Exception e) {
			System.err.println(e.getMessage());
			System.exit(Errors.OTHER.getValue());
		}
		
		/* Generate random bits and send to stdout. */
		byte[] bytes = new byte[4];
		while (true) {
			rng.nextBytes(bytes);
			try {
				System.out.write(bytes);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * Create random number generator.
	 * @param modifier
	 * @return A random number generator.
	 * @throws Exception
	 */
	private Random createRNG(int modifier) throws Exception {
		
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
			case REALLYPOOR:
				return new ReallyPoorRNG(seedGen);
			case XORSHIFT: 
				return new XORShiftRNG(seedGen);
			default:
				throw new RuntimeException("Don't know this random number generator.");
		}
		
	}

}