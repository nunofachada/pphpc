package org.laseeb.pphpc;

import com.beust.jcommander.Parameter;
import com.beust.jcommander.Parameters;
import com.beust.jcommander.validators.PositiveInteger;

@Parameters(commandNames = {"on_demand"}, commandDescription = "On-demand work command")
public class OnDemandWorkFactory extends AbstractWorkFactory {

	final private String commandName = "on_demand";

	/* Number of threads. */
	@Parameter(names = "-n", description = "Number of threads, defaults to the number of processors", validateWith = PositiveInteger.class)
	private int numThreads = Runtime.getRuntime().availableProcessors();

	/* Block size for ON_DEMAND work type. */
	@Parameter(names = "-b", description = "Block size", validateWith = PositiveInteger.class)
	private Integer blockSize = 100;

	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, IController controller) {
		final OnDemandWorkProvider workProvider = new OnDemandWorkProvider(this.blockSize, workSize);
		IObserver resetCellCounter = new IObserver() {

			@Override
			public void update(SimEvent event, IModel model) {
				workProvider.resetWorkCounter();
			}
		};
		
		controller.registerSimEventObserver(SimEvent.AFTER_INIT_CELLS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_INIT_AGENTS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_FIRST_STATS, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_HALF_ITERATION, resetCellCounter);
		controller.registerSimEventObserver(SimEvent.AFTER_END_ITERATION, resetCellCounter);
		
		return workProvider;
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentSync();
	}

	@Override
	public IController createSimController(IModel model) {
		return new Controller(model,
				new BlockingSynchronizer(SimEvent.AFTER_INIT_CELLS, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_CELLS_ADD_NEIGHBORS), 
				new BlockingSynchronizer(SimEvent.AFTER_INIT_AGENTS, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_FIRST_STATS, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_HALF_ITERATION, model, this.numThreads), 
				new BlockingSynchronizer(SimEvent.AFTER_END_ITERATION, model, this.numThreads), 
				new NoSynchronizer(SimEvent.AFTER_END_SIMULATION));
	}

	@Override
	public IGlobalStats createGlobalStats(int iters) {
		return new ThreadSafeGlobalStats(iters);
	}

	@Override
	public int getNumWorkers() {
		return this.numThreads;
	}

	@Override
	public String getCommandName() {
		return this.commandName;
	}

}
