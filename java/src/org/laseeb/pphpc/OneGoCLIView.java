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

/**
 * An exclusive command-line view which performs a simulation from start to
 * finish.
 * 
 * @author Nuno Fachada
 */
public class OneGoCLIView extends AbstractModelEventObserver implements IView {
	
	/* The model object. */
	private IModelQuerier model;
	
	/* The controller object. */
	private IController controller;
	
	/* The main object, which contains all the specified command-line
	 * options. */
	private PredPrey pp;
	
	/* How long did the simulation take to complete? */
	private long timing;
	
	/**
	 * Create a new view which performs a complete simulation in one go.
	 */
	public OneGoCLIView() {}
	
	/**
	 * @see IView#init(IModelQuerier, IController, PredPrey)
	 */
	@Override
	public void init(IModelQuerier model, final IController controller, PredPrey pp) {

		this.model = model;
		this.controller = controller;
		this.pp = pp;
		
		/* Register this view as observer of start, stop and exception events. */
		model.registerObserver(ModelEvent.START, this);
		model.registerObserver(ModelEvent.STOP, this);
		model.registerObserver(ModelEvent.EXCEPTION, this);

		
		/* Create a thread for this view, and start it. */
		new Thread(new Runnable() {

			@Override
			public void run() {
				
				/* Start the simulation and let it finish by its own. */
				try {
					controller.start();
				} catch (IllegalSimStateException e) {
					throw new RuntimeException("Impossible situation. It's a bug then.");
				}
			}

    	}).start();
	}

	/**
	 * @see IView#getType()
	 */
	@Override
	public ViewType getType() {
		return ViewType.ACTIVE_EXCLUSIVE;
	}

	/**
	 * @see AbstractModelEventObserver#updateOnStart()
	 */
	@Override
	protected void updateOnStart() {
		System.out.println("Started simulation with " + this.controller.getNumWorkers() + " threads...");
		this.timing = System.currentTimeMillis();
	}

	/**
	 * @see AbstractModelEventObserver#updateOnStop()
	 */
	@Override
	protected void updateOnStop() {
		System.out.println("Total simulation time: " + ((System.currentTimeMillis() - this.timing) / 1000.0f) + "\n");
		this.controller.export(this.pp.getStatsFile());
		System.exit(PredPrey.Errors.NONE.getValue());
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

}
