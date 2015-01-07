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

/**
 * The PPHPC package.
 */
package org.laseeb.pphpc;

import java.io.FileWriter;
import java.io.IOException;
import java.math.BigInteger;
import java.util.HashMap;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.CountDownLatch;

import org.uncommons.maths.random.AESCounterRNG;
import org.uncommons.maths.random.CMWC4096RNG;
import org.uncommons.maths.random.CellularAutomatonRNG;
import org.uncommons.maths.random.JavaRNG;
import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedGenerator;
import org.uncommons.maths.random.XORShiftRNG;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.JCommander;
import com.beust.jcommander.ParameterException;
import com.beust.jcommander.validators.PositiveInteger;

/**
 * Abstract PPHPC model.
 * 
 * @author Nuno Fachada
 */
public class PredPrey {
	
	/* There can be only one. */
	private volatile static PredPrey instance = null;	
	
	/* Enumeration containing program errors. */
	private enum Errors {
		NONE(0), ARGS(-1), PARAMS(-2), PRESIM(-3), SIM(-4), EXPORT(-5);
		private int value;
		private Errors(int value) { this.value = value; }
		public int getValue() { return this.value; }
	}
	
	/* Default parameters filename. */
	private final String paramsFileDefault = "config.txt";

	/* Default statistics output filename. */
	private final String statsFileDefault = "stats.txt";
	
	/* Interval to print current iteration. */
	@Parameter(names = "-i", description = "Interval of iterations to print current iteration")
	protected long stepPrint = 0;
	
	/* File containing simulation parameters. */
	@Parameter(names = "-p", description = "File containing simulation parameters")
	private String paramsFile = paramsFileDefault;
	
	/* File where to output simulation statistics. */
	@Parameter(names = "-s", description = "Statistics output file")
	private String statsFile = statsFileDefault;
	
	/* Seed for random number generator. */
	@Parameter(names = "-r", description = "Seed for random number generator (defaults to System.nanoTime())", 
			converter = BigIntegerConverter.class)
	private BigInteger seed = null;
	
	/* Random number generator implementation. */
	@Parameter(names = "-g", description = "Random number generator (AES, CA, CMWC, JAVA, MT or XORSHIFT)", 
			converter =  RNGTypeConverter.class)
	private RNGType rngType = RNGType.MT;
	
	/* Work provider strategy. */
	@Parameter(names = "-w", description = "Work type (EQUAL, EQUAL_REPEAT or ON_DEMAND)", converter = SimWorkTypeConverter.class)
	private SimWorkType workType = SimWorkType.EQUAL;

	/* Block size for ON_DEMAND work type. */
	@Parameter(names = "-b", description = "Block size for ON_DEMAND work type, ignored otherwise", validateWith = PositiveInteger.class)
	private Integer blockSize = 100;

	/* Debug mode. */
	@Parameter(names = "-d", description = "Debug mode (show stack trace on error)", hidden = true)
	private boolean debug = false;
	
	/* Help option. */
	@Parameter(names = {"--help", "-h", "-?"}, description = "Show options", help = true)
	private boolean help;

	/* Simulation parameters. */
	protected SimParams params;
	
	/* Simulation work provider. */
	protected ISimWorkProvider workProvider;
	
	private PredPrey() {}
	
	public static PredPrey getInstance() {
		if (instance == null) {
			synchronized (PredPrey.class) {
				if (instance == null) {
					instance = new PredPrey();
				}
			}
		}
		return instance;
	}
	

	/**
	 * Perform simulation.
	 * @throws SimWorkerException 
	 * @throws InterruptedException 
	 * 
	 * @throws Exception Any exception which may occur during the course of
	 * a simulation run.
	 */
	private IGlobalStats start() throws CompositeException, InterruptedException {
		
		/* Start timing. */
		long startTime = System.currentTimeMillis();
		
		/* Initialize latch. */
		final CountDownLatch latch = new CountDownLatch(1);

		/* Initialize statistics arrays. */
		IGlobalStats globalStats;
		if (this.numThreads > 1)
			globalStats = new ThreadSafeGlobalStats(this.params.getIters());
		else
			globalStats = new SimpleGlobalStats(this.params.getIters());
		
		
		workProvider = SimWorkProviderFactory.createWorkProvider(this.workType, this.params, this.numThreads, this.blockSize);

		workProvider.registerSimEventObserver(SimEvent.AFTER_END_SIMULATION, new IObserver() {

			@Override
			public void update(SimEvent event) {
				latch.countDown();
			}
			
		});
		
		final Map<String, Throwable> threadExceptions = new HashMap<String, Throwable>();
		
		/* Launch simulation threads. */
		for (int i = 0; i < this.numThreads; i++) {
			Thread simThread = new Thread(new SimWorker(workProvider, params, globalStats));
			simThread.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
				
				@Override
				public void uncaughtException(Thread thread, Throwable throwable) {
					threadExceptions.put(thread.getName(), throwable);
				}
			});
			simThread.start();
		}
		
		if (threadExceptions.size() > 0) {
			throw new CompositeException(threadExceptions);
		}
		
		/* Wait for simulation threads to finish. */
		latch.await();

		/* Stop timing and show simulation time. */
		long endTime = System.currentTimeMillis();
		float timeInSeconds = (float) (endTime - startTime) / 1000.0f;
		System.out.println("Total simulation time: " + timeInSeconds + "\n");
		
		return globalStats;
		
	}

	/**
	 * Export statistics to file.
	 * @param str Statistics filename.
	 * @throws IOException 
	 */
	private void export(IGlobalStats stats) throws IOException {
		
		FileWriter out = null;

		out = new FileWriter(this.statsFile);
		
		for (int i = 0; i <= params.getIters() ; i++) {
			out.write(stats.getStats(StatType.SHEEP, i) + "\t"
					+ stats.getStats(StatType.WOLVES, i) + "\t"
					+ stats.getStats(StatType.GRASS, i) + "\n");
		}
		
		if (out != null) {
			out.close();
		}
	}
	
	/**
	 * Show error message or stack trace, depending on debug parameter.
	 * 
	 * @param throwable Exception which caused the error.
	 */
	protected void errMessage(String threadName, Throwable throwable) {
		
		if (this.debug) {
			System.err.println("In thread " + threadName);
			throwable.printStackTrace();
		} else {
			System.err.println("An error ocurred in thread " + threadName
					+ throwable.getMessage());
		}
	}
	
	/**
	 * Run program.
	 * 
	 * @param args Command line arguments.
	 * @return Error code.
	 */
	public int doMain(String[] args) {
		
		IGlobalStats stats;
		
		/* Setup command line options parser. */
		JCommander parser = new JCommander(this);
		parser.setProgramName("java -cp bin" + java.io.File.pathSeparator + "lib/* " 
				+ PredPrey.class.getName());
		
		/* Parse command line options. */
		try {
			parser.parse(args);
		} catch (ParameterException pe) {
			/* On parsing error, show usage and return. */
			errMessage(Thread.currentThread().getName(), pe);
			parser.usage();
			return Errors.ARGS.getValue();
		}
		
		/* If help option was passed, show help and quit. */
		if (this.help) {
			parser.usage();
			return Errors.NONE.getValue();
		}
		
		/* Read parameters file. */
		try {
			this.params = new SimParams(this.paramsFile);
		} catch (IOException ioe) {
			errMessage(Thread.currentThread().getName(), ioe);
			return Errors.PARAMS.getValue();
		}
		
		/* Setup seed for random number generator. */
		if (this.seed == null)
			this.seed = BigInteger.valueOf(System.nanoTime());
		
		/* If in debug mode, ask the user to press ENTER to start. */
		if (this.debug) {
			System.out.println("Press ENTER to start...");
			try {
				System.in.read();
			} catch (IOException e) {
				errMessage(Thread.currentThread().getName(), e);
				return Errors.PRESIM.getValue();
			}
		}
		
		/* Perform simulation. */
		try {
			stats = this.start();
		} catch (InterruptedException ie) {
			errMessage(Thread.currentThread().getName(), ie);
			return Errors.SIM.getValue();
		} catch (CompositeException swe) {
			for (Map.Entry<String, Throwable> entry : swe) {
				errMessage(entry.getKey(), entry.getValue());
			}
			return Errors.SIM.getValue();
		}
		
		/* Export simulation results. */
		try {
			this.export(stats);
		} catch (IOException ioe) {
			errMessage(Thread.currentThread().getName(), ioe);
			return Errors.EXPORT.getValue();
		}
		
		/* Terminate with no errors. */
		return Errors.NONE.getValue();
		
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
	protected Random createRNG(long modifier) throws Exception {
		
		SeedGenerator seedGen = new SimSeedGenerator(modifier, this.seed);

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

	/**
	 * Main function.
	 * 
	 * @param args Command line arguments.
	 */
	public static void main(String[] args) {
		
		PredPrey pp = PredPrey.getInstance();
		int status = pp.doMain(args);
		System.exit(status);
		
	}
}
