package org.laseeb.pphpc;

public class StaticCLIView extends AbstractModelEventObserver implements IView {
	
	private IModel model;
	private IController controller;
	private String statsFile;
	private long timming;
	
	public StaticCLIView(IModel model, IController controller, String statsFile) {
		this.model = model;
		this.controller = controller;
		this.statsFile = statsFile;
		
		model.registerObserver(ModelEvent.START, this);
		model.registerObserver(ModelEvent.STOP, this);
		model.registerObserver(ModelEvent.EXCEPTION, this);
		
	}
	
	@Override
	public void init() {
    	new Thread(new Runnable() {

			@Override
			public void run() {
				controller.start();
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
		this.controller.export(this.statsFile);
		System.exit(PredPrey.Errors.NONE.getValue());
	}

	@Override
	protected void updateOnException() {
		System.err.println("And the winner is... " + this.model.getLastThrowable().getMessage());
		System.exit(PredPrey.Errors.SIM.getValue());
	}

}
