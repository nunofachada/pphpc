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

public class OneGoCLIView extends AbstractModelEventObserver implements IView {
	
	private IModelQuerier model;
	private IController controller;
	private PredPrey pp;
	
	private long timming;
	
	public OneGoCLIView() {
		
	}
	
	@Override
	public void init(IModelQuerier model, final IController controller, PredPrey pp) {

		this.model = model;
		this.controller = controller;
		this.pp = pp;
		
		model.registerObserver(ModelEvent.START, this);
		model.registerObserver(ModelEvent.STOP, this);
		model.registerObserver(ModelEvent.EXCEPTION, this);

		
		new Thread(new Runnable() {

			@Override
			public void run() {
				try {
					controller.start();
				} catch (IllegalSimStateException e) {
					throw new RuntimeException("Impossible situation. It's a bug then.");
				}
			}

    	}).start();
	}

	@Override
	protected void updateOnStart() {
		System.out.println("Started simulation with " + this.controller.getNumWorkers() + " threads...");
		this.timming = System.currentTimeMillis();
	}

	@Override
	protected void updateOnStop() {
		System.out.println("Total simulation time: " + ((System.currentTimeMillis() - this.timming) / 1000.0f) + "\n");
		this.controller.export(this.pp.getStatsFile());
		System.exit(PredPrey.Errors.NONE.getValue());
	}

	@Override
	protected void updateOnException() {
		String msg = this.pp.errMessage(this.model.getLastThrowable());
		System.err.println(msg);
		System.exit(PredPrey.Errors.SIM.getValue());
	}

	@Override
	public ViewType getType() {
		return ViewType.ACTIVE_EXCLUSIVE;
	}

}
