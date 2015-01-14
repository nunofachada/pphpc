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
