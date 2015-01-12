package org.laseeb.pphpc;

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JProgressBar;

public class InfoWidgetView extends AbstractModelEventObserver implements IView {

	private JLabel label;
	private JProgressBar progressBar;
	private IModelQuerier model;

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

	public InfoWidgetView() {}

	@Override
	public void init(IModelQuerier model, IController controller, PredPrey pp) {

		this.model = model;
		model.registerObserver(ModelEvent.START, this);
		model.registerObserver(ModelEvent.STOP, this);
		model.registerObserver(ModelEvent.NEW_ITERATION, this);
		
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				createAndShowGUI();
			}
		});
		
	}

	@Override
	public void updateOnNewIteration() {
		/* Get current iteration within allowable time. */
		final int iter = model.getCurrentIteration();

		/* Then enqueue widget update. */
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				label.setText("Iter: " + iter);
				progressBar.setValue(iter);
			}
		});
	}


}
