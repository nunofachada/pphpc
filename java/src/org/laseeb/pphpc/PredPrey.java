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

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.math.BigInteger;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.JCommander;
import com.beust.jcommander.ParameterException;

/**
 * Abstract PPHPC model.
 * 
 * @author Nuno Fachada
 */
public class PredPrey {
	
	/* Enumeration containing program errors. */
	public enum Errors {
		NONE(0), ARGS(-1), PARAMS(-2), PRESIM(-3), SIM(-4), EXPORT(-5), UNKNOWN(-6);
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
	private long stepPrint = 0;
	
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

	/* Debug mode. */
	@Parameter(names = "-d", description = "Debug mode (show stack trace on error)", hidden = true)
	private boolean debug = false;
	
	/* Help option. */
	@Parameter(names = {"--help", "-h", "-?"}, description = "Show options", help = true)
	private boolean help;

	/* Simulation parameters. */
	private ModelParams params;
	
	/* Work factory. */
	private IWorkFactory workFactory;
	
	/* Known work factories. */
	private String[] knownWorkFactoryNames = {"EqualWorkFactory", "OnDemandWorkFactory", "SingleThreadWorkFactory"};

	private Map<String, IWorkFactory> knownWorkFactories;
	
	private PredPrey() {}
	
	private void initWorkFactories() throws Exception {
		
		this.knownWorkFactories = new HashMap<String, IWorkFactory>();
		
		for (String factoryName : this.knownWorkFactoryNames) {
			
			Class<? extends IWorkFactory> factoryClass = 
					Class.forName("org.laseeb.pphpc." + factoryName).asSubclass(IWorkFactory.class);
			
			IWorkFactory factoryObject = factoryClass.newInstance();
			
			this.knownWorkFactories.put(factoryObject.getCommandName(), factoryObject);
			
		}
	}

	/**
	 * Show error message or stack trace, depending on debug parameter.
	 * 
	 * @param t Exception which caused the error.
	 */
	public String errMessage(Throwable t) {
		
		String errMessage;
		
		if (this.debug) {
			StringWriter sw = new StringWriter();
			t.printStackTrace(new PrintWriter(sw));
			errMessage = sw.toString();
		} else {
			errMessage = t.getMessage();
		}
		
		return errMessage;
	}
	
	/**
	 * Run program.
	 * 
	 * @param args Command line arguments.
	 */
	public void doMain(String[] args) {
		
		/* Initialize know work factories. */
		try {
			this.initWorkFactories();
		} catch (Exception e) {
			System.err.println(errMessage(e));
			System.exit(Errors.UNKNOWN.getValue());
		}
		
		/* Setup command line options parser. */
		JCommander parser = new JCommander(this);
		parser.setProgramName("java -cp bin" + java.io.File.pathSeparator + "lib/* " 
				+ PredPrey.class.getName());
		
		/* Add available work factories to parser. */
		for (Entry<String, IWorkFactory> entry : this.knownWorkFactories.entrySet()) {
			parser.addCommand(entry.getKey(), entry.getValue());
		}
		
		/* Parse command line options. */
		try {
			parser.parse(args);
		} catch (ParameterException pe) {
			/* On parsing error, show usage and return. */
			System.err.println(errMessage(pe));
			parser.usage();
			System.exit(Errors.ARGS.getValue());
		}
		
		/* Get the work factory which corresponds to the command specified
		 * in the command line. */
		this.workFactory = this.knownWorkFactories.get(parser.getParsedCommand());
		
		/* If help option was passed, show help and quit. */
		if (this.help) {
			parser.usage();
			System.exit(Errors.NONE.getValue());
		}
		
		/* Read parameters file. */
		try {
			this.params = new ModelParams(this.paramsFile);
		} catch (IOException ioe) {
			System.err.println(errMessage(ioe));
			System.exit(Errors.PARAMS.getValue());
		}
		
		/* Setup seed for random number generator. */
		if (this.seed == null)
			this.seed = BigInteger.valueOf(System.nanoTime());
		
		/* If in debug mode, ask the user to press ENTER to start. */
		if (this.debug) {
			System.out.println("Press ENTER to start...");
			try {
				System.in.read();
			} catch (IOException ioe) {
				System.err.println(errMessage(ioe));
				System.exit(Errors.PRESIM.getValue());
			}
		}
		
		/* Perform simulation. */
		IModel model = new Model(this.params, this.workFactory, this.rngType, this.seed);
		IController controller = this.workFactory.createSimController(model);
		
		/* Initialize the views. */
		IView viewMaster = new StaticCLIView();
		IView viewWidget = new InfoWidgetView();
		
		viewWidget.init(model, controller, this);
		viewMaster.init(model, controller, this);
		
	}
	

	/**
	 * Main function.
	 * 
	 * @param args Command line arguments.
	 */
	public static void main(String[] args) {
		
		new PredPrey().doMain(args);
		
	}

	public String getStatsFile() {
		return this.statsFile;
	}
}
