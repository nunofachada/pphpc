package org.laseeb.pphpc;

public class StaticCLIView implements IView {
	
	private IModel model;
	private IController controller;
	private String statsFile;
	
	public StaticCLIView(IModel model, IController controller, String statsFile) {
		this.model = model;
		this.controller = controller;
		this.statsFile = statsFile;
		
		model.registerObserver(null, this);
		
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
	public void update(ModelEvent event) {
		switch (event) {
			case START:
				System.out.println("Started!");
				break;
			case CONTINUE:
				System.out.println("Continue!");
				break;
			case NEW_ITERATION:
//				System.out.println("New iteration!");
				break;
			case PAUSE:
				System.out.println("Pause!");
				break;
			case STOP:
				System.out.println("Stop!");
				this.controller.export(this.statsFile);
				break;
			case EXCEPTION:
				System.err.println(this.model.getLastThrowable().getMessage());
				System.exit(PredPrey.Errors.SIM.getValue());
				break;
		}
	}

}
