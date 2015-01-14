package org.laseeb.pphpc;

import java.util.Scanner;

public class InteractiveCLIView extends AbstractModelEventObserver implements
IView {

	private IModelQuerier model;
	private IController controller;
	private PredPrey pp;

	private long timming;
	private boolean showInfo;
	
	private final String commandQuery = "(s)Start (p)Pause/Continue (o)Stop (i)Info (q)Quit\n> ";
	
	public InteractiveCLIView() {}
	
	private void showInfo() {
		IterationStats stats = this.model.getLatestStats();
		String info = "\nCurrent iteration: " + this.model.getCurrentIteration() + 
				"\nNumber of sheep: " + stats.getSheep() +
				"\nNumber of wolves: " + stats.getWolves() +
				"\nQuantity of grass: " + stats.getGrass();
		System.out.println(info);
		System.out.print(commandQuery);
	}

	@Override
	public void init(IModelQuerier model, final IController controller, PredPrey pp) {

		this.model = model;
		this.controller = controller;
		this.pp = pp;

		model.registerObserver(null, this);

		new Thread(new Runnable() {

			@Override
			public void run() {
				Scanner s = new Scanner(System.in);
				char command;
				while (true) {
					System.out.print(commandQuery);
					command = s.nextLine().toLowerCase().charAt(0);
					switch (command) {
					case 's':
						try {
							controller.start();
						} catch (IllegalSimStateException e) {
							System.out.println(e.getMessage());
						}
						break;
					case 'p':
						try {
							controller.pauseContinue();
						} catch (IllegalSimStateException e) {
							System.out.println(e.getMessage());
						}
						break;
					case 'o':
						try {
							controller.stop();
						} catch (IllegalSimStateException e) {
							System.out.println(e.getMessage());
						}
						break;
					case 'i':
						if (controller.isPaused()) {
							showInfo();
						} else {
							showInfo = true;
						}
						break;
					case 'q':
						s.close();
						System.out.println("Bye!");
						System.exit(0);
						break;
					default:
						System.out.println("Invalid command!");
						break;
					}
				}

			}

		}).start();
	}

	@Override
	protected void updateOnStart() {
		System.out.println("Started simulation with " + this.controller.getNumWorkers() + " threads...");
		System.out.print(commandQuery);
		this.timming = System.currentTimeMillis();
	}

	@Override
	protected void updateOnStop() {
		System.out.println("Total simulation time: " + ((System.currentTimeMillis() - this.timming) / 1000.0f) + "\n");
//		this.controller.stop();
//		this.controller.export(this.pp.getStatsFile());
//		System.out.print(commandQuery);
	}

	@Override
	protected void updateOnException() {
		String msg = this.pp.errMessage(this.model.getLastThrowable());
		System.err.println(msg);
		System.exit(PredPrey.Errors.SIM.getValue());
	}
	
	@Override
	protected void updateOnContinue() {}
	
	@Override
	protected void updateOnNewIteration() {
		if (this.showInfo) {
			this.showInfo = false;
			this.showInfo();
		}
	}
	
	@Override
	protected void updateOnPause() {}

	@Override
	public ViewType getType() {
		return ViewType.ACTIVE;
	}

}
