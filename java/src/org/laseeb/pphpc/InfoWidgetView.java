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

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JProgressBar;

/**
 * A simple information widget to track simulation progress.
 * 
 * @author Nuno Fachada
 */
public class InfoWidgetView extends AbstractModelEventObserver implements IView {

	/* Widget components. */
	private JLabel label;
	private JProgressBar progressBar;
	private IModelQuerier model;

	public InfoWidgetView() {}

	/**
	 * @see IView#init(IModelQuerier, IController, PredPrey)
	 */
	@Override
	public void init(final IModelQuerier model, IController controller, PredPrey pp) {

		this.model = model;
		final IView thisView = this;
		
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				/* Show widget. */
				createAndShowGUI();
				/* Register widget for start, stop and new iteration model events. */
				model.registerObserver(ModelEvent.START, thisView);
				model.registerObserver(ModelEvent.STOP, thisView);
				model.registerObserver(ModelEvent.NEW_ITERATION, thisView);
			}
		});
		
	}

	/**
	 * @see AbstractModelEventObserver#updateOnNewIteration()
	 */
	@Override
	public void updateOnNewIteration() {
		
		/* Get current iteration... */
		final int iter = model.getCurrentIteration();

		/* Then enqueue widget update when possible, so that the
		 * simulation itself is delayed by the update. */
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				label.setText("Iter: " + iter);
				progressBar.setValue(iter);
			}
		});
	}

	/**
	 * @see IView#getType()
	 */
	@Override
	public ViewType getType() {
		return ViewType.PASSIVE;
	}

	/**
	 * Create and show widget.
	 */
	private void createAndShowGUI() {
		
		/* Create and set up the window. */
		JFrame frame = new JFrame("Predator-Prey HPC");
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.getContentPane().setLayout(new BorderLayout());

		/* Add the label. */
		this.label = new JLabel("No iterations");
		frame.getContentPane().add(this.label, BorderLayout.NORTH);

		/* Add the progress bar. */
		this.progressBar = new JProgressBar(0, model.getParams().getIters());
		this.progressBar.setValue(0);
		this.progressBar.setStringPainted(true);
		frame.getContentPane().add(this.progressBar, BorderLayout.CENTER);

		/* Display the window. */
		frame.pack();
		frame.setVisible(true);
	}

}
