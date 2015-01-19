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

import java.util.Scanner;

/**
 * An interactive command-line view which allows to control the simulation.
 * 
 * @author Nuno Fachada
 */
public class InteractiveCLIView extends AbstractModelEventObserver implements IView {

	/* The model object. */
	private IModelQuerier model;
	
	/* The controller object. */
	private IController controller;
	
	/* The main object, which contains all the specified command-line
	 * options. */
	private PredPrey pp;
	
	/* Total simulation time. */
	private long totalTime;
	
	/* Last time a simulation was started/unpaused. */
	private long lastStart;
	
	/* Show simulation information at next available opportunity? */
	private boolean showInfo;
	
	/* Prompt string. */
	private final String commandQuery = "\n(s)Start (p)Pause/Continue (o)Stop (i)Info (e)Export (q)Quit\n> ";
	
	/* Was simulation requested to stop by the user? */
	private boolean userStopRequest;
	
	/**
	 * Create an interactive command-line view object.
	 */
	public InteractiveCLIView() {}

	/**
	 * @see IView#init(IModelQuerier, IController, PredPrey)
	 */
	@Override
	public void init(IModelQuerier model, final IController controller, final PredPrey pp) {

		this.model = model;
		this.controller = controller;
		this.pp = pp;

		/* Register this view as observer of all model events. */
		model.registerObserver(null, this);

		/* Create a thread for this view, and start it. */
		new Thread(new Runnable() {

			@Override
			public void run() {
				
				/* Use a scanner object to get user input. */
				Scanner s = new Scanner(System.in);
				
				/* User command. */
				char command;
				
				/* Loop until user quits. */
				while (true) {
					
					/* Print command prompt. */
					System.out.print(commandQuery);
					
					/* Get next command. */
					String cmdStr = s.nextLine();
					
					/* If user just pressed ENTER, ignore it and loop again. */
					if (cmdStr.length() == 0) continue;
					
					/* Parse command. */
					command = cmdStr.toLowerCase().charAt(0);
					
					/* Determine command and act upon it. */
					switch (command) {
					
						/* Start command. */
						case 's':
							try {
								userStopRequest = false;
								controller.start();
							} catch (IllegalSimStateException e) {
								System.out.println(e.getMessage());
							}
							break;

						/* Pause/continue command. */
						case 'p':
							try {
								controller.pauseContinue();
							} catch (IllegalSimStateException e) {
								System.out.println(e.getMessage());
							}
							break;

						/* Stop command. */
						case 'o':
							try {
								userStopRequest = true;
								controller.stop();
							} catch (IllegalSimStateException e) {
								System.out.println(e.getMessage());
							}
							break;

						/* Info command. */
						case 'i':
							if (controller.isPaused()) {
								showInfo();
							} else {
								showInfo = true;
							}
							break;

						/* Export command. */
						case 'e':
							controller.export(pp.getStatsFile());
							break;

						/* Quit command. */
						case 'q':
							s.close();
							System.out.println("Bye!");
							System.exit(0);
							break;
							

						/* Invalid command. */
						default:
							System.out.println("Invalid command!");
							break;
					}
				}

			}

		}).start();
	}

	/**
	 * @see IView#getType()
	 */
	@Override
	public ViewType getType() {
		
		/* This is an active, but not active-exclusive view. */
		return ViewType.ACTIVE;
		
	}

	/**
	 * @see AbstractModelEventObserver#updateOnStart()
	 */
	@Override
	protected void updateOnStart() {
		
		System.out.println("Started simulation with " + this.controller.getNumWorkers() + " threads...");
		this.totalTime = 0;
		this.lastStart = System.currentTimeMillis();
		
	}

	/**
	 * @see AbstractModelEventObserver#updateOnStop()
	 */
	@Override
	protected void updateOnStop() {
		
		this.totalTime += System.currentTimeMillis() - this.lastStart;
		System.out.println("\nTotal simulation time: " + (this.totalTime / 1000.0f) + " s");
		if (!this.userStopRequest)
			System.out.print(commandQuery);
		
	}

	/**
	 * @see AbstractModelEventObserver#updateOnException()
	 */
	@Override
	protected void updateOnException() {
		
		String msg = this.pp.errMessage(this.model.getLastThrowable());
		System.err.println(msg);
		System.exit(PredPrey.Errors.SIM.getValue());
		
	}
	
	/**
	 * @see AbstractModelEventObserver#updateOnPause()
	 */
	@Override
	protected void updateOnUnpause() {
		
		this.lastStart = System.currentTimeMillis();
		
	}
	
	/**
	 * @see AbstractModelEventObserver#updateOnNewIteration()
	 */
	@Override
	protected void updateOnNewIteration() {
		
		if (this.showInfo) {
			this.showInfo = false;
			this.showInfo();
			System.out.print(commandQuery);
		}
		
	}
	
	/**
	 * @see AbstractModelEventObserver#updateOnPause()
	 */
	@Override
	protected void updateOnPause() {
		
		this.totalTime += System.currentTimeMillis() - this.lastStart;
		
	}
	
	/**
	 * Show information about current state of simulation.
	 */
	private void showInfo() {
		
		IterationStats stats = this.model.getLatestStats();
		String info = "\nCurrent iteration: " + this.model.getCurrentIteration() + 
				"\nNumber of sheep: " + stats.getSheep() +
				"\nNumber of wolves: " + stats.getWolves() +
				"\nQuantity of grass: " + stats.getGrass();
		System.out.println(info);
		
	}

}
