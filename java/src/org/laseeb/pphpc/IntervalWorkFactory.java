package org.laseeb.pphpc;

import com.beust.jcommander.Parameters;

/**
 * Work factory which creates the required objects to divide work equally among the 
 * available workers in a thread-safe fashion, such that threads never have to
 * synchronize cell access.
 * 
 * @author Nuno Fachada
 */
@Parameters(commandNames = {"interval"}, commandDescription = "Interval work command")
public class IntervalWorkFactory extends AbstractMultiThreadWorkFactory {
	
	/* Name of command which invokes this work factory.*/
	final private String commandName = "interval";

	@Override
	public IController createSimController(IModel model) {
		
		/* Instantiate the controller... */
		IController controller = new Controller(model, this);
		
		/* ...and set appropriate sync. points for equal work division. */
		controller.setWorkerSynchronizers(
				new NonBlockingSyncPoint(ControlEvent.BEFORE_INIT_CELLS, this.numThreads),
				new NonBlockingSyncPoint(ControlEvent.AFTER_INIT_CELLS, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_SET_CELL_NEIGHBORS, this.numThreads), 
				new BlockingSyncPoint(ControlEvent.AFTER_INIT_AGENTS, controller, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_FIRST_STATS, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_HALF_ITERATION, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_ITERATION, this.numThreads), 
				new NonBlockingSyncPoint(ControlEvent.AFTER_END_SIMULATION, this.numThreads));
		
		/* Return the controller, configured for equal work division. */
		return controller;
	}

	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentAsync();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentAsync();
	}

	@Override
	public String getCommandName() {
		return this.commandName;
	}

	@Override
	protected IWorkProvider doGetWorkProvider(int workSize, IModel model, IController controller) {
		
		if (workSize == model.getSize()) {
			
			/* Use the interval work provider when dealing with cells. */
			return new IntervalWorkProvider(this.numThreads, model);
	
		} else {
			
			/* Use the equal work provider when initializing agents. */
			return new EqualWorkProvider(this.numThreads, workSize);
			
		}
				
	}

}
